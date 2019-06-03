# Benchmarking 

## General Remarks

There is a **single** benchmarking utility 
`experiment` (`experiment.exe` on Windows) that includes implementation of all methods.
It has multiple options, which specify, among others, 
a space, a data set, a type of search, a method to test, and method parameters.
These options and their use cases are described in the following subsections.
Note that unlike older versions it is not possible to test multiple methods in a single run.
However, it is possible to test the single method with different values of query-time parameters.

## Space and Distance Value Type


A distance function can return an integer (`int`), a single-precision (`float`),
or a double-precision (`double`) real value.
A type of the distance and its value is specified as follows:

```
  -s [ --spaceType ] arg       space type, e.g., l1, l2, lp:p=0.25
  --distType arg (=float)      distance value type: 
                               int, float, double
```


A description of a space may contain parameters (parameters may not contain whitespaces).
In this case, there is colon after the space mnemonic name followed by a
comma-separated (not spaces) list of parameters in the format:
`<parameter name>=<parameter value>`.

For real-valued distance functions, one can use either single- or double-precision
type. Single-precision is a recommended default.
One example of integer-valued distance function the Levenshtein distance.

## Input Data/Test Set

There are two options that define the data to be indexed:

```
  -i [ --dataFile ] arg         input data file
  -D [ --maxNumData ] arg (=0)  if non-zero, only the first 
                                maxNumData elements are used
```

The input file can be indexed either completely, or partially.
In the latter case, the user can create the index using only
the first `--maxNumData` elements.

For testing, the user can use a separate query set.
It is, again, possible to limit the number of queries:

```
  -q [ --queryFile ] arg        query file
  -Q [ --maxNumQuery ] arg (=0) if non-zero, use maxNumQuery query 
                                elements(required in the case 
                                of bootstrapping)
```


If a separate query set is not available, it can be simulated by bootstrapping.
To this, end the `--maxNumData` elements of the original data set
are randomly divided into testing and indexable sets.
The number of queries in this case is defined by the option `--maxNumQuery`.
A number of bootstrap iterations is specified through an option:

```
  -b [ --testSetQty ] arg (=0) # of sets created by bootstrapping; 
```

Benchmarking can be carried out in either a single- or a multi-threaded
mode. The number of test threads are specified as follows:

```
 --threadTestQty arg (=1)   # of threads
```

## Query Type

Our framework supports the _k_-NN and the range search.
The user can request to run both types of queries:

```
  -k [ --knn ] arg         comma-separated values of k 
                           for the k-NN search
  -r [ --range ] arg       comma-separated radii for range search
```

For example, by specifying the options 

```
--knn 1,10 --range 0.01,0.1,1
```

the user requests to run queries of five different types: 1-NN, 10-NN,
as well three range queries with radii 0.01, 0.1, and 1.

## Method Specification

It is possible to test only a single method at a time.
To specify a method's name, use the following option:

```
  -m [ --method ] arg           method/index name
```

A method can have a single set of index-time parameters, which is specified
via:

```
-c [ --createIndex ] arg        index-time method(s) parameters
```

In addition to the set of index-time parameters,
the method can have multiple sets of query-time parameters, which are specified
using the following (possibly repeating) option:

```
-t [ --queryTimeParams ] arg    query-time method(s) parameters
```

For each set of query-time parameters, i.e., for each occurrence of the option `--queryTimeParams`, 
the benchmarking utility `experiment`,
carries out an evaluation using the specified set of queries and a query type (e.g., a 10-NN search with
queries from a specified file). 
If the user does not specify any query-time parameters, there is only one evaluation to be carried out.
This evaluation uses default query-time parameters.
In general, we ensure that **whenever a query-time parameter is missed, the default value is used**.

Similar to parameters of the spaces, 
a set of method's parameters is a comma-separated list (no-spaces)
of parameter-value pairs in the format:
`<parameter name>=<parameter value>`.
For a detailed list of methods and their parameters, please, refer [to the manual PDF](/manual/latex/manual.pdf).

Note that a few methods can save/restore (meta) indices. To save and load indices
one should use the following options:

```
  -L [ --loadIndex ] arg        a location to load the index from 
  -S [ --saveIndex ] arg        a location to save the index to
```

When the user defines the location of the index using the option ```--loadIndex```, 
the index-time parameters may be ignored. 
Specifically, if the specified index does not exist, the index is created from scratch.
Otherwise, the index is loaded from disk.
Also note that the benchmarking utility **does not override an already existing index** (when the option `--saveIndex` is present).

If the tests are run the bootstrapping mode, i.e., when queries are randomly sampled (without replacement) from the
data set, several indices may need to be created. Specifically, for each split we create a separate index file.
The identifier of the split is indicated using a special suffix.
Also note that we need to memorize which data points in the split were used as queries. 
This information is saved in a gold standard cache file.
Thus, saving and loading of indices in the bootstrapping mode is possible only if 
gold standard caching is used.


## Saving and Processing Benchmark Results

The benchmarking utility may produce output of three types:

* Benchmarking results (a human readable report and a tab-separated data file);
* Log output (which can be redirected to a file);
* Progress bars to indicate the progress in index creation for some methods (cannot be currently suppressed);

To save benchmarking results to a file, on needs to specify a parameter:

```
  -o [ --outFilePrefix ] arg  output file prefix
```

As noted above, we create two files: a human-readable report (suffix `.rep`) and 
a tab-separated data file (suffix `.data`).
By default, the benchmarking utility creates files from scratch:
If a previously created report exists, it is erased. 
The following option can be used to append results to the previously created report:

```
  -a [ --appendToResFile ]    do not override information in results
```

For information on processing and interpreting results see the following sections.

By default, all log messages are printed to the standard error stream.
However, they can also be redirected to a log-file:

```
  -l [ --logFile ] arg         log file
```

## Efficiency of Testing

Except for  measuring methods' performance, 
the following are the most expensive operations: 

* computing ground truth answers (also
known as \emph{gold standard} data);
* loading the data set; 
* indexing. 

To make testing faster, the following methods can be used: 

* Caching of gold standard data;
* Creating gold standard data using multiple threads; 
* Reusing previously created indices (when loading and saving is supported by a method);
* Carrying out multiple tests using different sets of query-time parameters. 

By default, we recompute gold standard data every time we run benchmarks, which may take long time.
However, it is possible to save gold standard data and re-use it later
by specifying an additional argument:

```
-g [ --cachePrefixGS ] arg  a prefix of gold standard cache files
```

The benchmarks can be run in a multi-threaded mode by specifying a parameter:

```
--threadTestQty arg (=1)    # of threads during querying
```

In this case, the gold standard data is also created in a multi-threaded mode (which can also be much faster).
Note that NMSLIB directly supports only an inter-query parallelism, i.e., multiple queries are executed in parallel,
rather than the intra-query parallelism, where a single query can be processed by multiple CPU cores.

Gold standard data is stored in two files. One is a textual meta file
that memorizes important input parameters such as the name of the data and/or query file,
the number of test queries, etc. 
For each query, the binary cache files contains ids of answers (as well as distances and 
class labels). 
When queries are created by random sampling from the main data set,
we memorize which objects belong to each query set.

When the gold standard data is reused later,
the benchmarking code verifies if input parameters match the content of the cache file. 
Thus, we can prevent an accidental use of gold standard data created for one data set
while testing with a different data set.

Another sanity check involves verifying that data points obtained via an approximate
search are not closer to the query than data points obtained by an exact search.
This check has turned out to be quite useful. 
It has helped detecting a problem in at least the following two cases:

* The user creates a gold standard cache. Then, the user modifies a distance function and runs the tests again;
* Due to a bug, the search method reports distances to the query that are smaller than
the actual distances (computed using a distance function). This may occur, e.g., due to
a memory corruption.

When the benchmarking utility detects a situation when an approximate method
returns points closer than points returned by an exact method, the testing procedure is terminated
and the user sees a diagnostic message like this one:

```
... [INFO] >>>> Computing effectiveness metrics for sw-graph
... [INFO] Ex: -2.097 id = 140154 -> Apr: -2.111 id = 140154 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.974 id = 113850 -> Apr: -2.005 id = 113850 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.883 id = 102001 -> Apr: -1.898 id = 102001 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.6667 id = 58445 -> Apr: -1.6782 id = 58445 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.6547 id = 76888 -> Apr: -1.6688 id = 76888 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.5805 id = 47669 -> Apr: -1.5947 id = 47669 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.5201 id = 65783 -> Apr: -1.4998 id = 14954 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.4688 id = 14954 -> Apr: -1.3946 id = 25564 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.454 id = 90204 -> Apr: -1.3785 id = 120613 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.3804 id = 25564 -> Apr: -1.3190 id = 22051 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.367 id = 120613 -> Apr: -1.205 id = 101722 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.318 id = 71704 -> Apr: -1.1661 id = 136738 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.3103 id = 22051 -> Apr: -1.1039 id = 52950 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.191 id = 101722 -> Apr: -1.0926 id = 16190 1 - ratio: -0.0202 diff: -0.0221
... [INFO] Ex: -1.157 id = 136738 -> Apr: -1.0348 id = 13878 1 - ratio: -0.0202 diff: -0.0221
... [FATAL] bug: the approximate query should not return objects that are closer to the query 
                 than object returned by (exact) sequential searching! 
                 Approx: -1.09269 id = 16190 Exact: -1.09247 id = 52950
```

To compute recall, it is enough to memorize only query answers.
For example, in the case of a 10-NN search, it is enough to memorize only 10 data points closest 
to the query as well as respective distances (to the query).
However, this is not sufficient for computation of  a rank approximation metric (see \S~\ref{SectionEffect}).
To control the accuracy of computation, we permit the user to change the number of entries memorized.
This number is defined by the parameter:

```
--maxCacheGSRelativeQty arg (=10)   a maximum number of gold 
                                    standard entries
```

Note that this parameter is a **coefficient**: the actual number of entries is defined relative to
the result set size. For example, if a range search returns 30 entries and the value of 
`--maxCacheGSRelativeQty` is 10, then 30 * 10=300 entries are saved in the gold standard cache file.


## Interpretation of Results

If the user specifies the option `--outFilePrefix`,
the benchmarking results are stored to the file system.
A prefix of result files is defined by the parameter `--outFilePrefix`
while the suffix is defined by a type of the search procedure (the \knn or the range search)
as well as by search parameters (e.g., the range search radius).
For each type of search, two files are generated:
a report in a human-readable format,
and a tab-separated data file intended for automatic processing.
We compute quite a number of efficiency and effectiveness metrics. However,
the most useful are:


* `Recall` : _k_-NN recall
* `QueryTime` : an average query-time
* `ImprEfficiency` : a speed-up over the brute-force search (that uses the same number of threads)
* `ImprDistComp` : reduction in the number of distance computations (compared to the brute-force search)
* `Mem` : an estimated amount of memory used by the index