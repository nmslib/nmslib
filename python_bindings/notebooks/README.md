We have four Notebooks: three are for dense vector spaces, one is for the sparse vector space, and one is for the sparse Jaccard space. For the dense space, we have examples of the so-called optimized and non-optimized indices. Except HNSW, all the methods save meta-indices rather than real ones. Meta indices contain only index structure, but not the data. Hence, before a meta-index can be loaded, we need to re-load data. One example is a memory efficient space to search for SIFT vectors.

HNSW, can save real indices, but only for the dense vector spaces: Euclidean and the cosine. When you use these optimized indices, the search does not require reloading all the data. However, reloading the data is **required** if you want to use the function **getDistance**. Furthermore, creation of the optimized index can always be disabled specifying the index-time parameter **skip_optimized_index** (value 1).
This separation into optimized and non-optimized indices is not very convenient. In the future, we will fix this issue.

The sparse Jaccard space example is particularly interesting, because data set objects are represented by strings. In the case of the Jaccard space, a string is simply a list of sorted space-separated IDs. Other non-vector spaces can use different formats. 

