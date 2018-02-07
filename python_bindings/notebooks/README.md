We have three Notebooks: two are for dense spaces and one is for the sparse space. For the dense space, we have examples of the so-called optimized and non-optimized indices. Except HNSW, all the methods save meta-indices rather than real onese. Meta indices contain only index structure, but not the data. Hence, before a meta-index can be loaded, we need to re-load data.

HNSW, can save real indices, but only for the dense spaces: Euclidean and the cosine. When you use these optimized indices, the search does not require reloading all the data. However, reloading the data is **required** if you want to use the function **getDistance**. Furthermore, creation of the optimized index can always be disabled specifying the index-time parameter **skip_optimized_index** (value 1).

This separation into optimized and non-optimized indices is not very convenient. In the future, we will fix this issue.
