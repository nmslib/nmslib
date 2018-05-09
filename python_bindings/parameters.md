Basic parameter tuning for NMSLIB
=======

#### Graph-based search methods: SW-graph and HNSW

The basic guidelines are similar for both methods. Specifically, increasing the
value of ``efConstruction`` improves the quality of a constructed graph and leads
to higher accuracy of search. However this also leads to longer indexing times.
Similarly, increasing the value of ``efSearch`` improves recall at the expense of
longer retrieval time. The reasonable range of values for these parameters is
100-2000. In HNSW, there is a parameter ``ef``, which is merely an alias for the 
parameter ``efSearch``.

The recall values are also affected by parameters ``NN`` (for SW-graph) and ``M``
(HNSW). Increasing the values of these parameters (to a certain degree) leads to
better recall and shorter retrieval times (at the expense of longer indexing time).
For low and moderate recall values (e.g., 60-80%) increasing these parameters
may lead to longer retrieval times. The reasonable range of values for these
parameters is 5-100.

For HNSW, the parameter ``M`` defines the maximum number of neighbors in the 
zero and above-zero layers. However, the actual default maximum number of neighbors 
for the zero layer is 2*``MM``. This behavior can be overriden by explicitly
setting the maximum number of neighbors for the zero (``maxM0``) and
above-zero layers (``maxM``). Note that the zero layer contains all the data
points, but the number of points included in other layers is defined by the parameter 
``mult`` (see the HNSW paper for details).

Furthermore, in the case of HNSW, there is a trade-off between retrieval perfor-
mance and indexing time related to the choice of the pruning heuristic (controlled
by the parameter ``delaunay_type``). Specifically, by default ``delaunay_type`` is
equal to 1. Using delaunay type=1 improves performance—especially at high
recall values (> 80%) at the expense of longer indexing times. Therefore, for
lower recall values, we recommend using ``delaunay_type=0``.

Finally, HNSW has the parameter ``post``, which defines the amount (and type) of 
postprocessing applied to the constructed graph. The default value is 0, which means 
non postprocessing. Additional options are 1 and 2 (2 means more postprocessing).
