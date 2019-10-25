# List of Methods and Parameters

## Overview

The list of methods is followed by a brief tuning guideline:
* ``hnsw`` a Hierarchical Navigable Small World Graph.
* ``sw-graph`` a Small World Graph.
* ``vp-tree`` a Vantage-Point tree with a pruning rule adaptable to non-metric distances
* ``napp`` a Neighborhood APProximation index
* ``simple_invindx`` a vanilla, uncompressed, inverted index, which has no parameters
* ``brute_force`` a brute-force search, which has no parameters

The mnemonic name of a method is passed to python bindings function   as well  as  to  the  benchmarking  utility ``experiment``.

## Graph-based search methods: SW-graph and HNSW

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

In what follows, we discuss HNSW-specific parameters. 
First, for HNSW, the parameter ``M`` defines the maximum number of neighbors in the 
zero and above-zero layers. However, the actual default maximum number of neighbors 
for the zero layer is 2*``M``. This behavior can be overriden by explicitly
setting the maximum number of neighbors for the zero (``maxM0``) and
above-zero layers (``maxM``). Note that the zero layer contains all the data
points, but the number of points included in other layers is defined by the parameter 
``mult`` (see the HNSW paper for details).

Second, there is a trade-off between retrieval performance and indexing time 
related to the choice of the pruning heuristic (controlled
by the parameter ``delaunay_type``). 
Specifically, by default ``delaunay_type`` is
equal to 2. This default is generally quite good.
However, it maybe worth trying other viable options values: 0, 1, and 3.


Third, HNSW has the parameter ``post``, which defines the amount (and type) of 
postprocessing applied to the constructed graph. The default value is 0, which means 
no postprocessing. Additional options are 1 and 2 (2 means more postprocessing).

Fourth, there is a pesky design descision that an index does not necessarily
contain the data points, which are loaded separately. HNSW, chooses
to include data points into the index in several important cases, which include
the dense spaces for the Euclidean and the cosine distance. These optimized indices
are created automatically whenever possible. However, this behavior can be
overriden by setting the parameter ``skip_optimized_index`` to 1.

## A Vantage-Point tree (VP-tree)

VP-tree has the autotuning procedure,
which can be optionally initiated during the creation of the index. 
To this end, one needs to specify the parameters ``desiredRecall``
 (the minimum desired recall), ``bucketSize`` (the size 
of the bucket), ``tuneK`` or ``tuneR``, and (optionally) parameters ``tuneQty``, ``minExp`` and ``maxExp``. 
Parameters ``tuneK`` and ``tuneR`` are used to specify the value of _k_ for _k_-NN search,
or the search radius _r_ for the range search.

## Neighborhood APProximation index (NAPP)

Generally, increasing the overall number of pivots (parameter ``numPivot``) helps to improve
performance. However, using a large number of pivots leads to increased indexing
times. A good compromise is to use numPivot somewhat larger than ``sqrt(N)`` , where
``N`` is the overall number of data points. Similarly, the number
 of pivots to be indexed (``numPivotIndex``) could be somewhat larger than ``sqrt(numPivot)``. 
 
As a rough guideline, the number of pivots to be searched (``numPivotSearch``) 
could be in the order of ``sqrt(numPivotIndex)``. 
After selecting the values of ``numPivot`` and ``numPivotIndex``, finding an 
optimal value of ``numPivotSearch`` that provides a necessary recall requires just
a single run of the utility experiment (you need to specify all potentially good
values of numPivotSearch multiple times using the option ``--QueryTimeParams``, which 
is aliased to ``-t``).

Selecting good pivots is extremely important to the method's efficiency. 
For dense spaces, one can get decent performance by randomly sampling
pivots from the data set (this is a default scenario). 
An alternative way is to provide an external set of pivots.

One option for dense pivots, which we have not tested yet,
is to generate pivots using (a hierarchical) k-means clustering. 
However, for sparse spaces, we have found this option to be ineffective. 
In contrast, David Novak discovered (see our CIKM'2016 paper) that good pivots 
can be composed from randomly (and uniformly) selected terms.
[There is a script to do this](/scripts/gen_pivots_sparse.py). 

The script requires you to specify the maximum term/word ID. 
In many cases, the value of 50 or 100 thousand is enough. 
However, we sometimes encounter data sets with extremely long tail
of frequent words.
In these cases, one has to rely on the hashing trick to reduce dimensionality. 
Specifically, if there is a long tail of relatively frequent words,
you may want to set the value ``hashTrickDim`` to something like 50000.
Then, the maximum ID for the pivot generation utility would be 49999.

As we mentioned, by default pivots are sampled from the data set:
To use external pivots you need to provide them in the file specified 
by the parameter ``pivotFile``.

Finally, we note that NAPP divides data set into chunks, which can be
indexed in parallel. A chunk size (in the number of data points)
is defined by the parameter ``chunkIndexSize``.
By default, we will try to use all the threads. However,
the number of threads can be set explicitly using the parameter
``indexThreadQty``.
