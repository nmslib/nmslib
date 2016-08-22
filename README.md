=================
Non-Metric Space Library (NMSLIB)
=================
The latest release is [1.5.3](https://github.com/searchivarius/nmslib/releases/tag/v1.5.3)
-----------------
Non-Metric Space Library (NMSLIB) is an **efficient** cross-platform similarity search library and a toolkit for evaluation of similarity search methods. The goal of the project is to create an effective and **comprehensive** toolkit for searching in **generic non-metric** spaces. Being comprehensive is important, because no single method is likely to be sufficient in all cases. Also note that exact solutions are hardly efficient in high dimensions and/or non-metric spaces. Hence, the main focus is on **approximate** methods.

NMSLIB is an **extendible library**, which means that is possible to add new search methods and distance functions. NMSLIB can be used directly in C++ and Python (via Python bindings). In addition, it is also possible to build a query server, which can be used from Java (or other languages supported by Apache Thrift). Java has a native client, i.e., it works on many platforms without requiring a C++ library to be installed.

**Main developers** : Bilegsaikhan Naidan, Leonid Boytsov. With contributions from Yury Malkov, David Novak, Lawrence Cayton, Wei Dong, Avrelin Nikita, Dmitry Yashunin, Daniel Lemire, Alexander Ponomarenko.

Leo(nid) Boytsov is a maintainer. Leo is backed by the [Open Advancement of Question Answering Systems (OAQA) group](https://github.com/oaqa). Bileg was  supported by the [iAd Center](https://web.archive.org/web/20160306011711/http://www.iad-center.com/).

**Should you decide to modify the library (and, perhaps, create a pull request), please, use the [develoment branch](https://github.com/searchivarius/NonMetricSpaceLib/tree/develop)**.

NMSLIB is generic yet fast!
=================
Even though our methods are generic (see e.g., evaluation results in [Naidan and Boytsov 2015](http://boytsov.info/pubs/p2332-naidan-arxiv.pdf)), they often outperform specialized methods for the Euclidean and/or angular distance (i.e., for the cosine similarity).
Below are the results (as of May 2016) of NMSLIB compared to the best implementations participated in [a public evaluation code-named ann-benchmarks](https://github.com/erikbern/ann-benchmarks). Our main competitors are: 

1. A popular library [Annoy](https://github.com/spotify/annoy), which uses a forest of trees (older version used random-projection trees, the new one seems to use a hierarchical 2-means).
2. A new library [FALCONN](https://github.com/FALCONN-LIB/FALCONN), which is a highly-optimized implementation of the multiprobe LSH.  It uses a novel type of random projections based on the fast Hadamard transform.

The benchmarks were run on a c4.2xlarge instance on EC2 using a previously unseen subset of 5K queries. The benchmarks employ the following data sets:

1. [GloVe](http://nlp.stanford.edu/projects/glove/) : 1.2M 100-dimensional word embeddings trained on Tweets 
2. 1M of 128-dimensional [SIFT features](http://corpus-texmex.irisa.fr/)  

As of **May 2016** results are:

<table  border="0" width="100%" style="border:none">
<tr width="100%" border="0" style="border:none">
<td border="0" align="center" style="border:none">
1.19M 100d GloVe, cosine similarity.
<img src="https://raw.githubusercontent.com/searchivarius/nmslib/master/docs/figures/glove.png" width="400">
</td>
<td border="0"  align="center" style="border:none">
1M 128d SIFT features, Euclidean distance:
<img src="https://raw.githubusercontent.com/searchivarius/nmslib/master/docs/figures/sift.png" width="400">
</td>
</tr></table>

What's new in version 1.5
-----------------------

1. A new efficient method: a hierarchical (navigable) small-world graph (HNSW), contributed by Yury Malkov (@yurymalkov). Works with g++, Visual Studio, Intel Compiler, but doesn't work with Clang yet.
2. A query server, which can have clients in C++, Java, Python, and other languages supported by Apache Thrift
3. Python bindings for vector **and non**-vector spaces
4. Improved performance of two core methods SW-graph and NAPP
5. Better handling of the gold standard data in the benchmarking utility *experiment*
6. Updated API that permits search methods to serialize indices
7. Improved documentation (e.g., we added tuning guidelines for best methods)

General information
-----------------------

A detailed description is given [in the manual](docs/manual.pdf). The manual also contains instructions for building under Linux and Windows, extending the library, as well as for debugging the code using Eclipse.

Most of this code is released under the
Apache License Version 2.0 http://www.apache.org/licenses/.

To acknowledge the use of the library, you could provide a link to this repository and/or cite our SISAP paper [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/sisap/BoytsovN13). Some other related papers are listed in the end.

The LSHKIT, which is embedded in our library, is distributed under the GNU General Public License, see http://www.gnu.org/licenses/. The k-NN graph construction algorithm *NN-Descent* due to Dong et al. 2011 (see the links below), which is also embedded in our library, seems to be covered by a free-to-use license, similar to Apache 2.

Prerequisites
-----------------------

1. A modern compiler that supports C++11: G++ 4.7, Intel compiler 14, Clang 3.4, or Visual Studio 14 (version 12 can probably be used as well, but the project fileds need to be downgraded).
2. **64-bit** Linux is recommended, but most of our code builds on **64-bit** Windows as well. 
3. Boost (dev version). For Windows, the core library and the standalone sample application do not require Boost.
4. Only for Linux: CMake (GNU make is also required) 
5. Only for Linux: GNU scientific library (dev version)
6. Only for Linux: Eigen (dev version)
6. An Intel or AMD processor that supports SSE 4.2 is recommended

To install prerequisite packages on Ubuntu 14 Linux, type the following
```
sudo apt-get install libboost1.54-all-dev libgsl0-dev libeigen3-dev
```

Quick start on Linux
-----------------------

To compile, go to the directory **similarity_search** and type:  
```bash
cmake .
make  
```

Note that the directory **similarity_search** contains an Eclipse project that can be imported into [The Eclipse IDE for C/C++ Developers](http://www.eclipse.org/ide/).  A more detailed description is given in [in the manual](docs/manual.pdf), which also contains examples of using the software.  

You can also download almost every data set used in our previous evaluations (see the section **Data sets** below). The downloaded data needs to be decompressed (you may need 7z, gzip, and bzip2). Old experimental scripts can be found in the directory [previous_releases_scripts](previous_releases_scripts). However, they will work only with previous releases.

Note that the benchmarking utility **supports caching of ground truth data**, so that ground truth data is not recomputed every time this utility is re-run on the same data set.

Query server (Linux-only)
-----------------------
The query server requires Apache Thrift. We used Apache Thrift 0.9.2, but, perhaps, newer versions will work as well.  
To install Apache Thrift, you need to [build it from source](https://thrift.apache.org/docs/BuildingFromSource).
This may require additional libraries. On Ubuntu they can be installed as follows:
```
sudo apt-get install libboost-dev libboost-test-dev libboost-program-options-dev libboost-system-dev libboost-filesystem-dev libevent-dev automake libtool flex bison pkg-config g++ libssl-dev libboost-thread-dev make
```

After Apache Thrift is installed, you need to build the library itself. Then, change the directory
to [query_server/cpp_client_server](query_server/cpp_client_server) and type ``make`` (the makefile may need to be modified,
if Apache Thrift is installed to a non-standard location).
The query server has a similar set of parameters to the benchmarking utility ``experiment``.  For example,
you can start the server as follows:
```
 ./query_server -i ../../sample_data/final8_10K.txt -s l2 -m sw-graph -c NN=10,efConstruction=200,initIndexAttempts=1 -p 10000
```
There are also three sample clients implemented in [C++](query_server/cpp_client_server), [Python](query_server/python_client/),
and [Java](query_server/java_client/). 
A client reads a string representation of a query object from the standard stream.
The format is the same as the format of objects in a data file. 
Here is an example of searching for ten vectors closest to the first data set vector (stored in row one) of a provided sample data file:
```
export DATA_FILE=../../sample_data/final8_10K.txt
head -1 $DATA_FILE | ./query_client -p 10000 -a localhost  -k 10
```
It is also possible to generate client classes for other languages supported by Thrift from [the interface definition file](query_server/protocol.thrift), e.g., for C#. To this end, one should invoke the thrift compiler as follows:
```
thrift --gen csharp  protocol.thrift
```
For instructions on using generated code, please consult the [Apache Thrift tutorial](https://thrift.apache.org/tutorial/).

Python bindings (Linux-only)
-----------------------

We provide basic Python bindings (for Linux and Python 2.7). To build bindings for dense vector spaces, build the library first. Then, change the directory to
[python_vect_bindings](python_vect_bindings) and type:
```make
sudo make install
```

For an example of using our library in Python, see the script [test_nmslib_vect.py](python_vect_bindings/test_nmslib_vect.py). Generic bindings can be found in the directory [python_gen_bindings](python_gen_bindings).

Quick start on Windows
-----------------------
Building on Windows is straightforward.
Download [Visual Studio 2015 Express for Desktop](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx). 
Download and install respective [Boost binaries (**64-bit version 59**)](http://sourceforge.net/projects/boost/files/boost-binaries/1.59.0/boost_1_59_0-msvc-14.0-64.exe/download). Please, use the **default** installation directory on disk ``c:`` (otherwise, it will be necessary to update project files).

Afterwards, you can simply use the provided  [Visual Studio solution file](similarity_search/NonMetricSpaceLib.sln).
The solution file references several project (\*.vcxproj) files: 
[NonMetricSpaceLib.vcxproj](similarity_search/src/NonMetricSpaceLib.vcxproj)
is the main project file that is used to build the library itself.
The output is stored in the folder **similarity_search\x64**.

Also note that the core library, the test utilities,
 as well as examples of the standalone applications (projects **sample_standalone_app1**
and **sample_standalone_app2**)
can be built **without installing Boost**. 


Data sets
-----------------------

We use several data sets, which were created either by other folks,
or using 3d party software. If you use these data sets, please, consider
giving proper credit. The download scripts prints respective BibTex entries.
More information can be found [in the manual](docs/manual.pdf).

Here is the list of scripts to download major data sets:
* Data sets for our NIPS'13 and SISAP'13 papers [data/get_data_nips2013.sh](data/get_data_nips2013.sh).  
* Data sets for our VLDB'15 paper [data/get_data_vldb2015.sh](data/get_data_vldb2015.sh).  

The downloaded data needs to be decompressed (you may need 7z, gzip, and bzip2)

Related publications
-----------------------

Most important related papers are listed below in the chronological order: 
* L. Boytsov, D. Novak, Y. Malkov, E. Nyberg  (2016). [Off the Beaten Path: Let’s Replace Term-Based Retrieval
with k-NN Search.](http://boytsov.info/pubs/cikm2016.pdf) To appear in proceedings CIKM'16.
* Malkov, Y.A., Yashunin, D.A.. (2016). [Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs. CoRR](http://arxiv.org/abs/1603.09320), abs/1603.09320. [**[BibTex]**](http://adsabs.harvard.edu/cgi-bin/nph-bib_query?bibcode=2016arXiv160309320M&data_type=BIBTEX&db_key=PRE&nocookieset=1)
* Bilegsaikhan, N., Boytsov, L. 2015 [Permutation Search Methods are Efficient, Yet Faster Search is Possible](http://boytsov.info/pubs/p2332-naidan-arxiv.pdf) PVLDB, 8(12):1618--1629, 2015 [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/journals/corr/NaidanBN15)
* Ponomarenko, A., Averlin, N., Bilegsaikhan, N., Boytsov, L., 2014. [Comparative Analysis of Data Structures for Approximate Nearest Neighbor Search.](http://boytsov.info/pubs/da2014.pdf) [**[BibTex]**](http://scholar.google.com/scholar.bib?q=info:yOjNiT2Ql4AJ:scholar.google.com/&output=citation&hl=en&ct=citation&cd=0)
* Malkov, Y., Ponomarenko, A., Logvinov, A., & Krylov, V., 2014. [Approximate nearest neighbor algorithm based on navigable small world graphs.](http://www.sciencedirect.com/science/article/pii/S0306437913001300) Information Systems, 45, 61-68. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/journals/is/MalkovPLK14)
* Boytsov, L., Bilegsaikhan, N., 2013. [Engineering Efficient and Effective Non-Metric Space Library.](http://boytsov.info/pubs/sisap2013.pdf)   In Proceedings of the 6th International Conference on Similarity Search and Applications (SISAP 2013). [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/sisap/BoytsovN13)  
* Boytsov, L., Bilegsaikhan, N., 2013. [Learning to Prune in Metric and Non-Metric Spaces.](http://boytsov.info/pubs/nips2013.pdf)   In Advances in Neural Information Processing Systems 2013. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/nips/BoytsovN13)
* Tellez, Eric Sadit, Edgar Chávez, and Gonzalo Navarro. [Succinct nearest neighbor search.](http://www.dcc.uchile.cl/~gnavarro/ps/is12.pdf) Information Systems 38.7 (2013): 1019-1030. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/journals/is/TellezCN13)
* A. Ponomarenko, Y. Malkov, A. Logvinov, , and V. Krylov  [Approximate nearest
neighbor search small world approach.](http://www.iiis.org/CDs2011/CD2011IDI/ICTA_2011/Abstract.asp?myurl=CT175ON.pdf) ICTA 2011 
* Dong, Wei, Charikar Moses, and Kai Li. 2011. [Efficient k-nearest neighbor graph construction for generic similarity measures.](http://wwwconference.org/proceedings/www2011/proceedings/p577.pdf) Proceedings of the 20th international conference on World wide web. ACM, 2011.
[**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/mir/DongWCL12)
* L. Cayton, 2008 [Fast nearest neighbor retrieval for bregman divergences.](http://lcayton.com/bbtree.pdf) Twenty-Fifth International Conference on Machine Learning (ICML). [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/icml/Cayton08)
* Amato, Giuseppe, and Pasquale Savino. 2008 Approximate similarity search in metric spaces using inverted files. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/infoscale/AmatoS08)
* Gonzalez, Edgar Chavez, Karina Figueroa, and Gonzalo Navarro. [Effective proximity retrieval by ordering permutations.](http://www.dcc.uchile.cl/~gnavarro/ps/tpami07.pdf) Pattern Analysis and Machine Intelligence, IEEE Transactions on 30.9 (2008): 1647-1658. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/journals/pami/ChavezFN08)

