#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include "kdtree.h"

#define verify(_x) \
    do { \
        if (!(_x)) { \
            fprintf(stderr, "Assertion failure in %s line %d of %s: %s\n", \
                    __FUNCTION__, __LINE__, __FILE__, #_x); \
        } \
    } while (0)

// l2 distance
static inline float sqr (float x) {
    return x * x;
}

static inline float l2sqr (const float *p1, const float *p2, unsigned D) {
    float l = 0;
    unsigned d;
    // we hard coded DIM so this loop can be unrolled better by the compiler
    for (d = 0; d < D; d++) {
        l += sqr(p1[d] - p2[d]);
    }
    return l;
}

// very compact simplementation of KD-tree
// mainly based on D. Mount's ANN library.
// the data structure is statically allocated
// and dynamically constructed.

typedef struct kd_node kd_node_t;
                                // node really means internal nodes
union kd_leaf_or_node {
    unsigned long long leaf;    // leaf, a single index into the means array
    const kd_node_t *node;
};
// because all the nodes are statically allocated, the range of
// the address is known, and the range of leaf is from 0 to K-1.
// usually the first node address should be larger than K,
// so we can tell whether it's a leaf or a node simply by looking at the values.
// we check in the 

static inline int kd_is_leaf (union kd_leaf_or_node u, unsigned K) {
    // we have to test with node because node is 64-bit
    // and leaf is 32-bit
    // the lower 32-bit of any 64-bit value can be smaller than K
    //return u.node < (const struct kd_node *)(unsigned long long)K;
    return u.leaf < K;
}

// KD-tree internal node
struct kd_node {                
    unsigned cut_dim;
    float cut_val;
    float lower, upper;         // range of the dimension of this space partition
    union kd_leaf_or_node left, right;
};

// lower and upper bound of each dimension  with all the cluster centers
typedef struct {
    float *lo;
    float *hi;
} bnds_t;

struct kd_tree {
    unsigned K;
    unsigned dim;
    const float *means;
    kd_node_t *nodes;        // statically allocate all the nodes
    unsigned next_node;          // keep the number of node used
    bnds_t bnds;
};

static inline kd_node_t *kd_alloc_node (kd_tree_t *tree) {
    return &tree->nodes[tree->next_node++];
}


kd_tree_t *kd_tree_alloc (unsigned K, unsigned dim) {
    kd_tree_t *tree = (kd_tree_t *)malloc(sizeof(kd_tree_t));
    verify(tree);
    tree->K = K;
    tree->dim = dim;
    tree->means = NULL;
    tree->nodes = (kd_node_t*)malloc((K-1) * sizeof(kd_node_t));
    verify(tree->nodes);
    tree->bnds.lo = (float *)malloc(dim * sizeof(float));
    verify(tree->bnds.lo);
    tree->bnds.hi = (float *)malloc(dim * sizeof(float));
    verify(tree->bnds.hi);
    return tree;
}

void kd_tree_free (kd_tree_t *tree) {
    free(tree->nodes);
    free(tree->bnds.lo);
    free(tree->bnds.hi);
    free(tree);
}


#define ERR 0.001
static inline void bisec (
        kd_tree_t *tree,
        unsigned *idx, unsigned n,          // item ids to be divided
                                            // will be shuffled
        const bnds_t *bnds,                 // bounds of the box to be divided
        unsigned *cut_dim, float *cut_val,
        unsigned *n_lo                      // # items on the lower side
        ) {

    unsigned dim = tree->dim;
    const float *means = tree->means;
    unsigned cd;
    unsigned d, i, t;
    unsigned br1, br2;
    int l, r;
    float cv, ideal_cv;
    float max_len, len;
    float max_spr, spr;
    float cd_min, cd_max;
    cd_min = cd_max = 0;

    // find the longest side of the bounding box
    max_len = bnds->hi[0] - bnds->lo[0];
    for (d = 1; d < dim; d++) {
        len = bnds->hi[d] - bnds->lo[d];
        if (len > max_len) max_len = len;
    }

    // find the dimension with maximal spread as the cutting dimension
    max_spr = -1;
    cd = 0; // just to avoid warning
    for (d = 0; d < dim; d++) {
        len = bnds->hi[d] - bnds->lo[d];

        // if this side is among the longest
        if (len > (1-ERR) * max_len) {

            // take the spread
            float min, max;
            min = max = (means + dim * idx[0])[d];
            for (i = 1; i < n; ++i) {
                float v = (means + dim * idx[i])[d];
                if (v < min) min = v;
                if (v > max) max = v;
            }
            spr = max - min;
            if (spr > max_spr) {
                max_spr = spr;
                cd_min = min;
                cd_max = max;
                cd = d;
            }
        }
    }

    ideal_cv = cv = (bnds->lo[cd] + bnds->hi[cd]) / 2;
    
    if (cv < cd_min) cv = cd_min;
    else if (cv > cd_max) cv = cd_max;

    l = 0;
    r = n - 1;
    for (;;) {
        while (l < n && (means + dim * idx[l])[cd] < cv) l++;
        while (r >= 0 && (means + dim * idx[r])[cd] >= cv) r--;
        if (l > r) break;
        t = idx[l], idx[l] = idx[r], idx[r] = t;
        l++;
        r--;
    }
    br1 = l;            // means[0..br1-1][d] < cv <= means[br1..n-1][d]
    r = n - 1;
    for (;;) {
        while (l < n && (means + dim * idx[l])[cd] <= cv) l++;
        while (r >= 0 && (means + dim * idx[r])[cd] > cv) r--;
        if (l > r) break;
        t = idx[l], idx[l] = idx[r], idx[r] = t;
        l++;
        r--;
    }
    br2 = l;            // means[br1..br2-1][d] = cv < means[br2..n-1][d]

    *cut_dim = cd;
    *cut_val = cv;

    if (ideal_cv < cd_min) *n_lo = 1;
    else if (ideal_cv > cd_max) *n_lo = n - 1;
    else if (br1 > n/2) *n_lo = br1;
    else if (br2 < n/2) *n_lo = br2;
    else *n_lo = n/2;
}

static const kd_node_t *kd_index_hlp (kd_tree_t *tree, unsigned *idx, unsigned n,
                         bnds_t *bnds       // bounds of the subtree
                         ) {

    kd_node_t *node;
    unsigned n_lo, n_hi, cut_dim;
    float cut_val;
    float lo, hi;               // save value for the cutted dimension
    assert(n > 1);
    node = kd_alloc_node(tree);

    bisec(tree, idx, n, bnds, &cut_dim, &cut_val, &n_lo);

    node->cut_dim = cut_dim;
    node->cut_val = cut_val;

    lo = node->lower = bnds->lo[cut_dim];
    hi = node->upper = bnds->hi[cut_dim];

    // construct the left subtree
    bnds->hi[cut_dim] = cut_val;
    assert(n_lo > 0);
    if (n_lo == 1) {
        node->left.leaf = idx[0];
    }
    else {
        node->left.node = kd_index_hlp(tree, idx, n_lo, bnds);
    }
    bnds->hi[cut_dim] = hi;

    // construct the right subtree
    bnds->lo[cut_dim] = cut_val;
    n_hi = n - n_lo;
    if (n_hi == 1) {
        node->right.leaf = idx[n-1];
    }
    else {
        node->right.node = kd_index_hlp(tree, idx + n_lo, n_hi, bnds);
    }
    bnds->lo[cut_dim] = lo;

    return node;
}

void kd_tree_index (kd_tree_t *tree, const float *means) {
    unsigned *kd_idx;
    unsigned d, i;

    tree->means = means;

    kd_idx = (unsigned *)malloc(tree->K * sizeof(unsigned));
    verify(kd_idx);
    tree->next_node = 0;
    // fprintf(stderr, "XXX: %llu.\n", tree->nodes);
    verify(tree->nodes > (kd_node_t *)(unsigned long long)tree->K);

    // init idx to an arbitrary permutation
    for (i = 0; i < tree->K; i++) {
        kd_idx[i] = i;
    }

    // bounding box
    for (d = 0; d < tree->dim; d++) {
        const float *cm = means + d;
        tree->bnds.lo[d] = tree->bnds.hi[d] = *cm;
        cm += tree->dim;
        for (i = 1; i < tree->K; i++, cm += tree->dim) {
            if (*cm < tree->bnds.lo[d]) {
                tree->bnds.lo[d] = *cm;
            }
            else if (*cm > tree->bnds.hi[d]) {
                tree->bnds.hi[d] = *cm;
            }
        }
    }

    // recursively construct the tree
    // the first node automatically becomes the root
    kd_index_hlp(tree, kd_idx, tree->K, &tree->bnds);

    verify(tree->next_node == tree->K - 1);

    free(kd_idx);
}

struct kd_search_stat {
    const float *means;
    unsigned dim;
    unsigned K;
    const float *pt;    // query
    unsigned cnt;
    unsigned nn;
    float nn_dist;
};

static void kd_search_node (const kd_node_t *node, struct kd_search_stat *stat, float d2b);

static inline void kd_search_leaf_or_node (union kd_leaf_or_node lon,
                            struct kd_search_stat *stat,
                            float d2b) {

    if (kd_is_leaf(lon, stat->K)) {
        float l = l2sqr(stat->pt, stat->means + stat->dim * lon.leaf, stat->dim);
        stat->cnt++;
        if (l < stat->nn_dist) {
            stat->nn = lon.leaf;
            stat->nn_dist = l;
        }
    }
    else { 
        kd_search_node(lon.node, stat, d2b);
    }
}

static void kd_search_node (const kd_node_t *node, struct kd_search_stat *stat, float d2b) {

    unsigned cd = node->cut_dim;
    float cv = node->cut_val;
    const float *pt = stat->pt;
    float cut_diff = pt[cd] - cv;
    float box_diff;

    if (cut_diff < 0) {
        kd_search_leaf_or_node(node->left, stat, d2b);

        box_diff = node->lower - pt[cd];
        if (box_diff < 0) {
            box_diff = 0;
        }
        d2b = d2b + sqr(cut_diff) - sqr(box_diff);

        if (d2b < stat->nn_dist) {
            kd_search_leaf_or_node(node->right, stat, d2b);
        }
    }
    else {
        kd_search_leaf_or_node(node->right, stat, d2b);

        float box_diff = pt[cd] - node->upper;
        if (box_diff < 0) {
            box_diff = 0;
        }

        d2b = d2b + sqr(cut_diff) - sqr(box_diff);

        if (d2b < stat->nn_dist) {
            kd_search_leaf_or_node(node->left, stat, d2b);
        }
    }
}

// lookup is readonly
unsigned kd_tree_search (kd_tree_t *tree, const float *pt, unsigned *cnt) {

    unsigned d;
    float d2b = 0;
    for (d = 0; d < tree->dim; d++) {
        if (pt[d] < tree->bnds.lo[d]) d2b += sqr(tree->bnds.lo[d] - pt[d]);
        else if (pt[d] > tree->bnds.hi[d]) d2b += sqr(pt[d] - tree->bnds.hi[d]);
    }

    struct kd_search_stat stat;
    stat.dim = tree->dim;
    stat.K = tree->K;
    stat.means = tree->means;
    stat.pt = pt;
    stat.cnt = 0;
    stat.nn = 0;
    stat.nn_dist = FLT_MAX;

    kd_search_node(tree->nodes, &stat, d2b);

    *cnt = stat.cnt;

    /*
    {
        // fprintf(stderr, "C");
        unsigned r = kd_tree_ln_search(tree, pt, cnt);
        if (r != stat.nn) {
            float l = l2sqr(pt, tree->means + tree->dim * r, tree->dim);
            fprintf(stderr, "KDTREE: %u %g %u %g\n", stat.nn, stat.nn_dist, r, l);
        }
    }
    */

    return stat.nn;
}

unsigned kd_tree_ln_search (kd_tree_t *tree, const float *pt, unsigned *cnt)
{
    unsigned k, c = 0;
    const float *cm = tree->means;
    float d, min = l2sqr(cm, pt, tree->dim);
    cm += tree->dim;
    for (k = 1; k < tree->K; k++, cm += tree->dim) {
        d = l2sqr(cm, pt, tree->dim);
        if (d < min) {
            c = k;
            min = d;
        }
    }
    *cnt = tree->K;
    return c;
}
