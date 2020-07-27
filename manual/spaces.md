# Spaces and Distances

Below, there is a list of nearly all spaces (a space is a combination of data and the distance). The mnemonic name of a space is passed to python bindings function   as well  as  to  the  benchmarking  utility ``experiment``. 
When initializing the space in Python embeddings, please use the type 
`FLOAT` for all spaces, except the space `leven`: [see the description here.](https://nmslib.github.io/nmslib/api.html#nmslib-init)
A more detailed description is given
in the [manual](/manual/latex/manual.pdf).

## Specifying parameters of the space

In some rare cases, spaces have parameters
Currently, this is done only for 
[L<sub>p</sub>](https://en.wikipedia.org/wiki/Lp_space#Lp_spaces)
and [Renyi divergences](https://en.wikipedia.org/wiki/R%C3%A9nyi_entropy#R%C3%A9nyi_divergence).
Specifying parameters is done differently in Python bindings and the command line utility `experiment`.
In Python bindings, one needs to specify a dictionary of space parameters, see [this notebook](/python_bindings/notebooks/search_vector_dense_lp.ipynb) for an example.
In the command line, parameter specifiers go after the colon sign.
For example, ``lp:p=3`` denotes the L<sub>3</sub> space and
``lp:p=2`` is a synonym for the Euclidean, i.e., L<sub>2</sub> space.
Should we ever have more than one parameter, their specifications are going to be separated by commas.

## Fast, Slow, and Approximate variants

There can be more than one version of a distance function,
which have different space-performance trade-off.
In particular, for distances that require computation of logarithms 
we can achieve an order of magnitude improvement (e.g., for the GNU C++
and Clang) by pre-computing
logarithms at index time. This comes at a price of extra storage. 
In the case of Jensen-Shannon distance functions, we can precompute some 
of the logarithms and accurately approximate those we cannot precompute.
Fast versions of inner-product distances, however, 
do not precompute anything. They employ  fast SIMD instructions
and a special data layout.

Straightforward slow implementations of the distance functions may have the substring ``slow``
in their names, while faster versions contain the substring ``fast``.
Fast functions that involve approximate computations contain additionally the substring ``approx``.
For non-symmetric distance function, a space may have two variants: one variant is for left
queries (the data object is the first, i.e., left, argument of the distance function 
while the query object
is the second argument) 
and another is for right queries (the data object is the second argument and the query object is the first argument).
In the latter case the name of the space ends on ``rq``.


## Input Format

For Python bindings, all dense-vector spaces require float32 numpy-array input (two-dimensional). See an example [here](python_bindings/notebooks/search_vector_dense_optim.ipynb). 
One exception is the squared Euclidean space for SIFT vectors, which requires input as uint8 integer numpy arrays. An example can be found [here](python_bindings/notebooks/search_sift_uint8.ipynb).

For sparse spaces that include the L<sub>p</sub>-spaces, the sparse cosine similarity, and the maximum-inner product space, the input data is a sparse scipy matrix. An example can be found [here](python_bindings/notebooks/search_sparse_cosine.ipynb).

In all other cases, including the sparse Jaccard space,
the bit Hamming and the bit Jaccard space, as well 
as the Levenshtein spaces, the input data comes
as an array of strings (one string per data point). All vector
data is represented by space-separated numbers. For binary
vectors, there will be ones and zeros separated by spaces.
An sparse Jaccard example can be found [here](python_bindings/notebooks/search_generic_sparse_jaccard.ipynb). 
 
For Levenshtein spaces, strings are expected to be ASCII strings (it is
currently a limitation).
You can pass a UTF8-encoded string, but the distance will be sometimes
larger than the actual distance. 

## Storage Format

For dense vector spaces, the data can be either single-precision or double-precision floating-point numbers. 
However, double-precision has not been useful so far and we do not recommend use it.
In the case of SIFT vectors, though, vectors are stored more compactly:
as 8-bit integer numbers (Uint8).

For sparse vector spaces, 
we keep a float/double vector value and a 32-bit dimension number/ID.
However, for fast variants of sparse spaces we use only 32-bit single-precision floats. Fast vector spaces are segmented into chunks each containing 65536 IDs. Thus, if vectors have significant gaps 
between dimensions, such storage can be suboptimal.

Bit variants of the spaces (bit-Hamming and bit-Jaccard) are stored compactly as bit-vectors (each vector is an array of 32-bit integers).  

## L<sub>p</sub> spaces
 
L<sub>p</sub> spaces is [a classic family](https://en.wikipedia.org/wiki/Lp_space#Lp_spaces) of spaces,
which include the Euclidean (L<sub>2</sub>) distance.
They are metrics if _p_ is at least one. 
The value of _p_ can be fractional. 
However, computing fractional powers is slower with most math libraries.
Our implementation is more efficient when fractions are in the form
_n/2<sup>k</sup>_, where _k_ is a small integer. 
 

| Space code   | Description and Notes                           |
|--------------|-------------------------------------------------|
| `lp`         | Generic L<sub>p</sub> space with a parameter p  |
| `l1`         | L<sub>1</sub>                                   |
| `l2`         | Euclidean space                                 |
| `linf`       | L<sub>&infin;</sub>                             |
| `l2sqr_sift` | Euclidean distance for SIFT vectors (Uint8 storage)|
| `lp_sparse`  | **sparse** L<sub>p</sub> space                  |
| `l1_sparse`  | **sparse** L<sub>1</sub>                        |
| `l2_sparse`  | **sparse** Euclidean space                      |
| `linf_sparse`| **sparse** L<sub>&infin;</sub>                  |


## Inner-product Spaces

Like L<sub>p</sub> spaces, inner-product based spaces exist in sparse and dense variants.
Note that the cosine distance is defined as one minus the value of the cosine similarity.
The cosine similarity is a **normalized** inner/scalar product.
In contrast, [the angular distance is a monotonically transformed cosine similarity](https://en.wikipedia.org/wiki/Cosine_similarity#Angular_distance_and_similarity).
The resulting transformation is a true metric distance.
The fast and slow variants of the sparse inner-product based distances differ
in that the faster versions rely on SIMD and a special data layout.

| Space code    | Description and Notes                                               |
|---------------|---------------------------------------------------------------------|
| `cosinesimil` | **dense** cosine distance                                           |
| `negdotprod`  | **dense** negative inner-product (for maximum inner-product search) |
| `angulardist` | **dense** angular distance                                          |
| `cosinesimil_sparse`, `cosinesimil_sparse_fast` | **sparse** cosine distance        |
| `negdotprod_sparse`, `negdotprod_sparse_fast`   | **sparse** negative inner-product |
| `angulardist_sparse`, `angulardist_sparse_fast` | **sparse** angular distance       |


## Divergences 

The divergences that we used are defined only for the 
dense vector spaces. The faster and approximate version
of these distances normally rely on precomputation of logarithms,
which doubles the storage requirements.
We implement distances for regular and generalized
[KL-divergence](https://en.wikipedia.org/wiki/Kullback%E2%80%93Leibler_divergence),
the [Jensen-Shannon (JS) divergence](https://en.wikipedia.org/wiki/Jensen%E2%80%93Shannon_divergence),
for the family of [Renyi divergences](https://en.wikipedia.org/wiki/R%C3%A9nyi_entropy#R%C3%A9nyi_divergence),
and for the [Itakura-Saito distance](https://en.wikipedia.org/wiki/Itakura%E2%80%93Saito_distance).
We also explicitly implement the squared JS-divergence,
which is a true metric distance.

For the meaning of infixes ``fast``, ``slow``, ``approx``, and ``rq`` see the information above.

| Space code(s)                              | Description and Notes                           |
|--------------------------------------------|-------------------------------------------------|
| `kldivfast`, `kldivfastrq`             | Regular KL-divergence                           |
| `kldivgenslow`, `kldivgenfast`, `kldivgenfastrq` | Generalized KL-divergence           | 
| `itakurasaitoslow`, `itakurasaitofast`, `itakurasaitofastrq` |  Itakura-Saito distance |
| `jsdivslow`, `jsdivfast`, `jsdivfastapprox` | JS-divergence                              |
| `jsmetrslow`, `jsmetrfast`, `jsmetrfastapprox`  | JS-metric                                  |
| `renyidiv_slow`, `renyidiv_fast`                | Renyi divergence: parameter name `alpha`   |         


## Miscellaneous Distances 

The [Jaccard distance](https://en.wikipedia.org/wiki/Jaccard_index) and
 the [Levenshtein distance](https://en.wikipedia.org/wiki/Levenshtein_distance), 
 and the [Hamming distances](https://en.wikipedia.org/wiki/Hamming_distance) are all true metrics.
However, the normalized Levenshtein distance is mildly non-metric.

| Space code    | Description and Notes                                               |
|---------------|---------------------------------------------------------------------|
| `leven`       | Levenshtein distance: [requires an integer distance value](https://nmslib.github.io/nmslib/api.html#nmslib-init)            |
| `normleven`   | the Levenshtein distance normalized by string lengths: [requires a float distance value](https://nmslib.github.io/nmslib/api.html#nmslib-init)                   |
| `jaccard_sparse` | **sparse** Jaccard distance                                         |
| `bit_jaccard`    | **bit** Jaccard distance                                            |
| `bit_hamming`    | **bit** Hamming distance                                            |
