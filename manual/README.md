# NMSLIB Manual

## Overview

Documentation is split into several parts. 
[Links to these parts are given below](#documentation-links).
This is followed by the list of some limitations and terminological introduction. 
Terminology might be crucial to understand certain parts (but mostly these are well-known terms).
Also note that the [PDF part of the manual](/manual/latex/manual.pdf)
contains a detailed description of implemented search spaces and methods.

## Documentation Links

* Python bindings
  - [Overview and further links](/python_bindings/README.md) 
  - [Sample notebooks](/python_bindings/notebooks/README.md)
  - [Python bindings API](https://nmslib.github.io/nmslib/index.html)
* [A brief list of methods and parameters (including basic tuning guidelines)](/manual/methods.md)
* [A brief list of supported spaces/distances](/manual/spaces.md)
* [Some data used in the past](/manual/datasets.md)
* [Building the main library (Linux/Mac)](/manual/build.md)
* [Building and using the query server (Linux/Mac)](/manual/query_server.md)
* [Benchmarking using NMSLIB utility ``experiment``](/manual/benchmarking.md)
* [Extending NMSLIB (adding spaces, methods, and apps)](/manual/extend.md)
* [A detailed and formal description of spaces and methods with examples of benchmarking the methods (PDF)](/manual/latex/manual.pdf)


## Some Limitations

* Only static data sets are supported (with an exception of SW-graph)
* HNSW currently duplicates memory to create optimized indices
* Range/threshold search is not supported by many methods including SW-graph/HNSW


## Terminology and Problem Formulation

NMSLIB provides a fast similarity search.
The search is carried out in a finite database of objects _{o<sub>i</sub>}_
using a search query _q_ and a dissimilarity measure.
 An object is a synonym for a **data point** or simply a **point**. 
 The dissimilarity measure is typically represented by a **distance** function _d(o<sub>i</sub>, q)_. 
The ultimate goal is to answer a query by retrieving a subset of database objects sufficiently similar to the query _q_.
A combination of data points and the distance function is called a **search space**,
or simply a **space**.


Note that we use the terms distance and the distance function in a broader sense than
most other folks:
We do not assume that the distance is a true metric distance. 
The distance function can disobey the triangle inequality and/or be even non-symmetric.

Two retrieval tasks are typically considered: a **nearest-neighbor** and a range search.
Currently, we mostly support only the nearest-neighbor search,
which  aims to find the object at the smallest distance from the query.
Its direct generalization is the _k_ nearest-neighbor search (the _k_-NN search),
which looks for the _k_  closest objects, which 
have _k_ smallest distance values to the query _q_.
 
In generic spaces, the distance is not necessarily symmetric. 
Thus, two types of queries can be considered. 
In a  **left** query, the object is the left argument of the distance function,
while the query is the right argument.
In a **right** query, the query _q_ is the first argument and the object is the second, i.e., the right, argument.

The queries can be answered either exactly, 
i.e., by returning a complete result set that does not contain erroneous elements, or, 
approximately, e.g., by finding only some neighbors.
Thus, the methods are evaluated in terms of efficiency-effectiveness trade-offs
rather than merely in terms of their efficiency.
One common effectiveness metric is recall, 
which is computed as
an average fraction of true neighbors returned by the method (with ties broken arbitrarily).

