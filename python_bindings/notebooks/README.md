We have Python notebooks for the following scenarios: 

1. [The Euclidean space with the optimized index](search_vector_dense_optim.ipynb);
2. [The Euclidean space with the non-optimized index](search_vector_dense_nonoptim.ipynb);
3. [The Euclidean space ofr for 8-bit integer SIFT vectors (the index is not optimized)](search_sift_uint8.ipynb);
4. [KL-divergence (non-optimized index)](search_vector_dense_kldiv.ipynb);
3. [Sparse cosine similarity (non-optimized index)](search_sparse_cosine.ipynb);
4. [Sparse Jaccard similarity (non-optimized index)](search_generic_sparse_jaccard.ipynb).

Note that for for the dense space, we have examples of the so-called optimized and non-optimized indices. Except HNSW, all the methods save meta-indices rather than real ones. Meta indices contain only index structure, but not the data. Hence, before a meta-index can be loaded, we need to re-load data. One example is a memory efficient space to search for SIFT vectors. For ease of reproduction, example use very small corpora. For this reason, our methods do not necessarily outperform brute-force search. Typically, the larger is the corpora the larger is the improvement in efficiency over the brute-force search. 

HNSW, can save real indices, but only for the dense vector spaces: Euclidean and the cosine. When you use these optimized indices, the search does not require reloading all the data. However, reloading the data is **required** if you want to use the function **getDistance**. Furthermore, creation of the optimized index can always be disabled specifying the index-time parameter **skip_optimized_index** (value 1).
This separation into optimized and non-optimized indices is not very convenient. In the future, we will fix this issue.

The sparse Jaccard space example is interesting, because data set objects are represented by strings. In the case of the Jaccard space, a string is simply a list of sorted space-separated IDs. Other non-vector spaces can use different formats. The KL-divergence example is interesting, because it is a non-metric data set with non-symmetric distance function.

