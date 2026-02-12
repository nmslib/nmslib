/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/nmslib/nmslib
 *
 * Copyright (c) 2013-2026
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

/**
 * Benchmark for batched SIMD short-circuit evaluation
 * Author: Amrith Kumar
 */

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <queue>
#include <set>
#include <cmath>
#include <limits>
#include <iomanip>

#include "distcomp.h"

using namespace std::chrono;

struct DistanceResult {
    float distance;
    size_t index;
    bool operator<(const DistanceResult& other) const {
        return distance < other.distance;
    }
};

struct SearchStats {
    size_t vectors_evaluated;
    size_t full_computations;
    size_t early_aborts;
    double time_ms;
    std::vector<float> top_k_distances;
    std::vector<size_t> top_k_indices;
};

// Standard: full distance, no threshold
SearchStats search_standard_l2(
    const std::vector<std::vector<float>>& dataset,
    const std::vector<float>& query,
    int k
) {
    SearchStats stats = {dataset.size(), dataset.size(), 0, 0, {}, {}};
    std::priority_queue<DistanceResult> top_k_heap;
    
    auto t0 = high_resolution_clock::now();
    
    for (size_t i = 0; i < dataset.size(); ++i) {
        float dist = similarity::L2SqrSIMD(dataset[i].data(), query.data(), query.size());
        
        if (top_k_heap.size() < (size_t)k) {
            top_k_heap.push({dist, i});
        } else if (dist < top_k_heap.top().distance) {
            top_k_heap.pop();
            top_k_heap.push({dist, i});
        }
    }
    
    auto t1 = high_resolution_clock::now();
    stats.time_ms = duration_cast<duration<double, std::milli>>(t1 - t0).count();
    
    std::vector<DistanceResult> results;
    while (!top_k_heap.empty()) {
        results.push_back(top_k_heap.top());
        top_k_heap.pop();
    }
    std::sort(results.begin(), results.end(), [](const DistanceResult& a, const DistanceResult& b) {
        return a.distance < b.distance;
    });
    for (const auto& r : results) {
        stats.top_k_distances.push_back(r.distance);
        stats.top_k_indices.push_back(r.index);
    }
    
    return stats;
}

// Batched: check threshold every batch_size dimensions
SearchStats search_batched_l2(
    const std::vector<std::vector<float>>& dataset,
    const std::vector<float>& query,
    int k,
    size_t batch_size
) {
    SearchStats stats = {dataset.size(), 0, 0, 0, {}, {}};
    std::priority_queue<DistanceResult> top_k_heap;
    
    auto t0 = high_resolution_clock::now();
    
    for (size_t i = 0; i < dataset.size(); ++i) {
        float threshold_sqr = (top_k_heap.size() < (size_t)k) 
            ? std::numeric_limits<float>::infinity() 
            : top_k_heap.top().distance;
        
        if (threshold_sqr == std::numeric_limits<float>::infinity()) {
            stats.full_computations++;
        }
        
        float dist = similarity::L2SqrSIMDBatched(
            dataset[i].data(), query.data(), query.size(), batch_size, threshold_sqr
        );
        
        // Note: dist may be partial if early abort occurred inside L2SqrSIMDBatched
        // We still insert it to maintain heap invariants
        if (dist > threshold_sqr) {
            stats.early_aborts++;
        }
        
        if (top_k_heap.size() < (size_t)k) {
            top_k_heap.push({dist, i});
        } else if (dist < top_k_heap.top().distance) {
            top_k_heap.pop();
            top_k_heap.push({dist, i});
        }
    }
    
    auto t1 = high_resolution_clock::now();
    stats.time_ms = duration_cast<duration<double, std::milli>>(t1 - t0).count();
    
    std::vector<DistanceResult> results;
    while (!top_k_heap.empty()) {
        results.push_back(top_k_heap.top());
        top_k_heap.pop();
    }
    std::sort(results.begin(), results.end(), [](const DistanceResult& a, const DistanceResult& b) {
        return a.distance < b.distance;
    });
    for (const auto& r : results) {
        stats.top_k_distances.push_back(r.distance);
        stats.top_k_indices.push_back(r.index);
    }
    
    return stats;
}

// Standard LInf: full distance, no threshold
SearchStats search_standard_linf(
    const std::vector<std::vector<float>>& dataset,
    const std::vector<float>& query,
    int k
) {
    SearchStats stats = {dataset.size(), dataset.size(), 0, 0, {}, {}};
    std::priority_queue<DistanceResult> top_k_heap;
    
    auto t0 = high_resolution_clock::now();
    
    for (size_t i = 0; i < dataset.size(); ++i) {
        float dist = similarity::LInfNormSIMD(dataset[i].data(), query.data(), query.size());
        
        if (top_k_heap.size() < (size_t)k) {
            top_k_heap.push({dist, i});
        } else if (dist < top_k_heap.top().distance) {
            top_k_heap.pop();
            top_k_heap.push({dist, i});
        }
    }
    
    auto t1 = high_resolution_clock::now();
    stats.time_ms = duration_cast<duration<double, std::milli>>(t1 - t0).count();
    
    std::vector<DistanceResult> results;
    while (!top_k_heap.empty()) {
        results.push_back(top_k_heap.top());
        top_k_heap.pop();
    }
    std::sort(results.begin(), results.end(), [](const DistanceResult& a, const DistanceResult& b) {
        return a.distance < b.distance;
    });
    for (const auto& r : results) {
        stats.top_k_distances.push_back(r.distance);
        stats.top_k_indices.push_back(r.index);
    }
    
    return stats;
}

// Batched LInf: check threshold every batch_size dimensions
SearchStats search_batched_linf(
    const std::vector<std::vector<float>>& dataset,
    const std::vector<float>& query,
    int k,
    size_t batch_size
) {
    SearchStats stats = {dataset.size(), 0, 0, 0, {}, {}};
    std::priority_queue<DistanceResult> top_k_heap;
    
    auto t0 = high_resolution_clock::now();
    
    for (size_t i = 0; i < dataset.size(); ++i) {
        float threshold = (top_k_heap.size() < (size_t)k) 
            ? std::numeric_limits<float>::infinity() 
            : top_k_heap.top().distance;
        
        if (threshold == std::numeric_limits<float>::infinity()) {
            stats.full_computations++;
        }
        
        float dist = similarity::LInfNormSIMDBatched(
            dataset[i].data(), query.data(), query.size(), batch_size, threshold
        );
        
        if (dist > threshold) {
            stats.early_aborts++;
        }
        
        if (top_k_heap.size() < (size_t)k) {
            top_k_heap.push({dist, i});
        } else if (dist < top_k_heap.top().distance) {
            top_k_heap.pop();
            top_k_heap.push({dist, i});
        }
    }
    
    auto t1 = high_resolution_clock::now();
    stats.time_ms = duration_cast<duration<double, std::milli>>(t1 - t0).count();
    
    std::vector<DistanceResult> results;
    while (!top_k_heap.empty()) {
        results.push_back(top_k_heap.top());
        top_k_heap.pop();
    }
    std::sort(results.begin(), results.end(), [](const DistanceResult& a, const DistanceResult& b) {
        return a.distance < b.distance;
    });
    for (const auto& r : results) {
        stats.top_k_distances.push_back(r.distance);
        stats.top_k_indices.push_back(r.index);
    }
    
    return stats;
}

// Standard Cosine: full computation, no threshold
// Note: Cosine similarity is HIGHER is BETTER (similarity, not distance)
SearchStats search_standard_cosine(
    const std::vector<std::vector<float>>& dataset,
    const std::vector<float>& query,
    int k
) {
    SearchStats stats = {dataset.size(), dataset.size(), 0, 0, {}, {}};
    // For similarity (higher is better), use max-heap but negate values
    std::priority_queue<DistanceResult> top_k_heap;
    
    auto t0 = high_resolution_clock::now();
    
    for (size_t i = 0; i < dataset.size(); ++i) {
        float sim = similarity::NormScalarProductSIMD(dataset[i].data(), query.data(), query.size());
        
        if (top_k_heap.size() < (size_t)k) {
            top_k_heap.push({-sim, i}); // negate for min-heap behavior
        } else if (sim > -top_k_heap.top().distance) { // higher similarity is better
            top_k_heap.pop();
            top_k_heap.push({-sim, i});
        }
    }
    
    auto t1 = high_resolution_clock::now();
    stats.time_ms = duration_cast<duration<double, std::milli>>(t1 - t0).count();
    
    std::vector<DistanceResult> results;
    while (!top_k_heap.empty()) {
        auto r = top_k_heap.top();
        r.distance = -r.distance; // restore original similarity
        results.push_back(r);
        top_k_heap.pop();
    }
    std::sort(results.begin(), results.end(), [](const DistanceResult& a, const DistanceResult& b) {
        return a.distance > b.distance; // sort descending for similarity
    });
    for (const auto& r : results) {
        stats.top_k_distances.push_back(r.distance);
        stats.top_k_indices.push_back(r.index);
    }
    
    return stats;
}

// Batched Cosine: check threshold every batch_size dimensions
SearchStats search_batched_cosine(
    const std::vector<std::vector<float>>& dataset,
    const std::vector<float>& query,
    int k,
    size_t batch_size
) {
    SearchStats stats = {dataset.size(), 0, 0, 0, {}, {}};
    std::priority_queue<DistanceResult> top_k_heap;
    
    auto t0 = high_resolution_clock::now();
    
    for (size_t i = 0; i < dataset.size(); ++i) {
        float threshold = (top_k_heap.size() < (size_t)k) 
            ? -std::numeric_limits<float>::infinity() 
            : -top_k_heap.top().distance; // negate back to get actual threshold
        
        if (threshold == -std::numeric_limits<float>::infinity()) {
            stats.full_computations++;
        }
        
        float sim = similarity::NormScalarProductSIMDBatched(
            dataset[i].data(), query.data(), query.size(), batch_size, threshold
        );
        
        if (sim < threshold) { // aborted early (similarity too low)
            stats.early_aborts++;
        }
        
        if (top_k_heap.size() < (size_t)k) {
            top_k_heap.push({-sim, i});
        } else if (sim > -top_k_heap.top().distance) {
            top_k_heap.pop();
            top_k_heap.push({-sim, i});
        }
    }
    
    auto t1 = high_resolution_clock::now();
    stats.time_ms = duration_cast<duration<double, std::milli>>(t1 - t0).count();
    
    std::vector<DistanceResult> results;
    while (!top_k_heap.empty()) {
        auto r = top_k_heap.top();
        r.distance = -r.distance;
        results.push_back(r);
        top_k_heap.pop();
    }
    std::sort(results.begin(), results.end(), [](const DistanceResult& a, const DistanceResult& b) {
        return a.distance > b.distance;
    });
    for (const auto& r : results) {
        stats.top_k_distances.push_back(r.distance);
        stats.top_k_indices.push_back(r.index);
    }
    
    return stats;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <num_vectors> <dimensions> <k> <num_queries>\n";
        return 1;
    }
    
    size_t num_vectors = std::atoi(argv[1]);
    size_t dims = std::atoi(argv[2]);
    int k = std::atoi(argv[3]);
    int num_queries = std::atoi(argv[4]);
    
    std::cout << "=== Batched SIMD Distance Computation ===\n";
    std::cout << "Configuration:\n";
    std::cout << "  Dataset size: " << num_vectors << " vectors\n";
    std::cout << "  Dimensions: " << dims << "\n";
    std::cout << "  k (top-k): " << k << "\n";
    std::cout << "  Queries: " << num_queries << "\n\n";
    
    // Generate dataset
    std::cout << "Generating dataset...\n";
    std::mt19937 rng(42);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    std::vector<std::vector<float>> dataset(num_vectors, std::vector<float>(dims));
    for (auto& vec : dataset) {
        for (auto& val : vec) {
            val = dist(rng);
        }
    }
    
    // Generate queries
    std::cout << "Generating queries...\n";
    std::vector<std::vector<float>> queries(num_queries, std::vector<float>(dims));
    for (auto& query : queries) {
        for (auto& val : query) {
            val = dist(rng);
        }
    }
    
    // Normalize all vectors for cosine similarity (L2 norm = 1.0)
    std::cout << "Normalizing vectors for cosine similarity...\n";
    auto normalize = [](std::vector<float>& vec) {
        float norm = 0.0f;
        for (float v : vec) norm += v * v;
        norm = sqrt(norm);
        if (norm > 1e-10f) {
            for (float& v : vec) v /= norm;
        }
    };
    
    for (auto& vec : dataset) normalize(vec);
    for (auto& query : queries) normalize(query);
    
    // Test different batch sizes
    std::vector<size_t> batch_sizes = {16, 32, 64, 128, 256, 512};
    
    std::cout << "\n=== L2 Squared Distance ===\n";
    std::cout << "Query | Standard | Batch16 | Batch32 | Batch64 | Batch128 | Batch256 | Batch512\n";
    std::cout << "------|----------|---------|---------|---------|----------|----------|----------\n";
    
    double total_l2_standard = 0;
    std::vector<double> total_l2_batched(batch_sizes.size(), 0.0);
    bool all_results_match = true;
    
    for (int i = 0; i < num_queries; ++i) {
        auto std_stats = search_standard_l2(dataset, queries[i], k);
        total_l2_standard += std_stats.time_ms;
        
        printf("%5d | %8.2f", i, std_stats.time_ms);
        
        for (size_t b = 0; b < batch_sizes.size(); ++b) {
            auto batch_stats = search_batched_l2(dataset, queries[i], k, batch_sizes[b]);
            total_l2_batched[b] += batch_stats.time_ms;
            printf(" | %7.2f", batch_stats.time_ms);
            
            // Verify top-k indices match (order doesn't matter, just the set)
            if (std_stats.top_k_indices.size() != batch_stats.top_k_indices.size()) {
                all_results_match = false;
            } else {
                std::set<size_t> std_set(std_stats.top_k_indices.begin(), std_stats.top_k_indices.end());
                std::set<size_t> batch_set(batch_stats.top_k_indices.begin(), batch_stats.top_k_indices.end());
                if (std_set != batch_set) {
                    all_results_match = false;
                }
            }
        }
        printf("\n");
    }
    
    
    std::cout << "\n=== Correctness Check ===\n";
    if (all_results_match) {
        std::cout << "✓ All batched results match standard results\n";
    } else {
        std::cout << "✗ WARNING: Some batched results differ from standard!\n";
    }
    
    std::cout << "\n=== L2 Summary ===\n";
    std::cout << "Standard (full SIMD, no threshold): " << total_l2_standard << " ms\n";
    for (size_t b = 0; b < batch_sizes.size(); ++b) {
        double pct = 100.0 * (total_l2_batched[b] - total_l2_standard) / total_l2_standard;
        std::cout << "Batch size " << batch_sizes[b] << ": " << total_l2_batched[b] << " ms ("
                  << (pct >= 0 ? "+" : "") << pct << "%)\n";
    }
    
    // === LInf Distance Benchmark ===
    std::cout << "\n=== LInf Distance ===\n";
    std::cout << "Query | Standard | Batch16 | Batch32 | Batch64 | Batch128 | Batch256 | Batch512\n";
    std::cout << "------|----------|---------|---------|---------|----------|----------|----------\n";
    
    double total_linf_standard = 0;
    std::vector<double> total_linf_batched(batch_sizes.size(), 0.0);
    bool all_linf_results_match = true;
    
    for (int i = 0; i < num_queries; ++i) {
        auto std_stats = search_standard_linf(dataset, queries[i], k);
        total_linf_standard += std_stats.time_ms;
        
        printf("%5d | %8.2f", i, std_stats.time_ms);
        
        for (size_t b = 0; b < batch_sizes.size(); ++b) {
            auto batch_stats = search_batched_linf(dataset, queries[i], k, batch_sizes[b]);
            total_linf_batched[b] += batch_stats.time_ms;
            printf(" | %7.2f", batch_stats.time_ms);

            // Verify top-k indices match (order doesn't matter, just the set)
            if (std_stats.top_k_indices.size() != batch_stats.top_k_indices.size()) {
                all_linf_results_match = false;
            } else {
                std::set<size_t> std_set(std_stats.top_k_indices.begin(), std_stats.top_k_indices.end());
                std::set<size_t> batch_set(batch_stats.top_k_indices.begin(), batch_stats.top_k_indices.end());
                if (std_set != batch_set) {
                    all_linf_results_match = false;
                }
            }
        }
        printf("\n");
    }

    std::cout << "\n=== LInf Correctness Check ===\n";
    if (all_linf_results_match) {
        std::cout << "✓ All batched LInf results match standard results\n";
    } else {
        std::cout << "✗ WARNING: Some batched LInf results differ from standard!\n";
    }
    
    std::cout << "\n=== LInf Summary ===\n";
    std::cout << "Standard (full SIMD, no threshold): " << total_linf_standard << " ms\n";
    for (size_t b = 0; b < batch_sizes.size(); ++b) {
        double pct = 100.0 * (total_linf_batched[b] - total_linf_standard) / total_linf_standard;
        std::cout << "Batch size " << batch_sizes[b] << ": " << total_linf_batched[b] << " ms ("
                  << (pct >= 0 ? "+" : "") << pct << "%)\n";
    }
    
    // === Cosine Similarity Benchmark ===
    std::cout << "\n=== Cosine Similarity ===\n";
    std::cout << "Query | Standard | Batch16 | Batch32 | Batch64 | Batch128 | Batch256 | Batch512\n";
    std::cout << "------|----------|---------|---------|---------|----------|----------|----------\n";
    
    double total_cosine_standard = 0;
    std::vector<double> total_cosine_batched(batch_sizes.size(), 0.0);
    bool all_cosine_results_match = true;

    for (int i = 0; i < num_queries; ++i) {
        auto std_stats = search_standard_cosine(dataset, queries[i], k);
        total_cosine_standard += std_stats.time_ms;
        
        printf("%5d | %8.2f", i, std_stats.time_ms);

        for (size_t b = 0; b < batch_sizes.size(); ++b) {
            auto batch_stats = search_batched_cosine(dataset, queries[i], k, batch_sizes[b]);
            total_cosine_batched[b] += batch_stats.time_ms;
            printf(" | %7.2f", batch_stats.time_ms);

            // Verify top-k indices match (order doesn't matter, just the set)
            if (std_stats.top_k_indices.size() != batch_stats.top_k_indices.size()) {
                all_cosine_results_match = false;
            } else {
                std::set<size_t> std_set(std_stats.top_k_indices.begin(), std_stats.top_k_indices.end());
                std::set<size_t> batch_set(batch_stats.top_k_indices.begin(), batch_stats.top_k_indices.end());
                if (std_set != batch_set) {
                    all_cosine_results_match = false;
                }
            }
        }
        printf("\n");
    }
    
    std::cout << "\n=== Cosine Correctness Check ===\n";
    if (all_cosine_results_match) {
        std::cout << "✓ All batched Cosine results match standard results\n";
    } else {
        std::cout << "✗ WARNING: Some batched Cosine results differ from standard!\n";
    }
    
    std::cout << "\n=== Cosine Summary ===\n";
    std::cout << "Standard (full SIMD, no threshold): " << total_cosine_standard << " ms\n";
    for (size_t b = 0; b < batch_sizes.size(); ++b) {
        double pct = 100.0 * (total_cosine_batched[b] - total_cosine_standard) / total_cosine_standard;
        std::cout << "Batch size " << batch_sizes[b] << ": " << total_cosine_batched[b] << " ms ("
                  << (pct >= 0 ? "+" : "") << pct << "%)\n";
    }
    
    return 0;
}
