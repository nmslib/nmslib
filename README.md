[![Pypi version](https://img.shields.io/pypi/v/nmslib.svg)](http://pypi.python.org/pypi/nmslib)
[![Downloads](https://pepy.tech/badge/nmslib)](https://pepy.tech/project/nmslib)
[![Downloads](https://pepy.tech/badge/nmslib/month)](https://pepy.tech/project/nmslib)
[![Build Status](https://travis-ci.org/nmslib/nmslib.svg?branch=master)](https://travis-ci.org/nmslib/nmslib)
[![Windows Build Status](https://ci.appveyor.com/api/projects/status/wd63b9doe7xco81t/branch/master?svg=true)](https://ci.appveyor.com/project/searchivarius/nmslib)
[![Join the chat at https://gitter.im/nmslib/Lobby](https://badges.gitter.im/nmslib/Lobby.svg)](https://gitter.im/nmslib/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

# Non-Metric Space Library (NMSLIB) 

## Important Notes

* NMSLIB is generic but fast, see the results of [ANN benchmarks](https://github.com/erikbern/ann-benchmarks).
* A standalone implementation of our fastest method HNSW [also exists as a header-only library](https://github.com/nmslib/hnswlib).
* **All the documentation** (including using **Python bindings** and the query server, description of methods and spaces, building the library, etc) can be found [on this page](/manual/README.md).
* For **generic questions/inquiries**, please, use [**the Gitter chat**](https://gitter.im/nmslib/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge): GitHub issues page is for bugs and feature requests.

## Objectives

Non-Metric Space Library (NMSLIB) is an **efficient** cross-platform similarity search library and a toolkit for evaluation of similarity search methods. The core-library does **not** have any third-party dependencies. It has been gaining popularity recently. In particular, it has become a part of [Amazon Elasticsearch Service](https://aws.amazon.com/about-aws/whats-new/2020/03/build-k-nearest-neighbor-similarity-search-engine-with-amazon-elasticsearch-service/).

The goal of the project is to create an effective and **comprehensive** toolkit for searching in **generic and non-metric** spaces.
Even though the library contains a variety of metric-space access methods,
our main focus is on **generic** and **approximate** search methods,
in particular, on methods for non-metric spaces.
NMSLIB is possibly the first library with a principled support for non-metric space searching.

NMSLIB is an **extendible library**, which means that is possible to add new search methods and distance functions. NMSLIB can be used directly in C++ and Python (via Python bindings). In addition, it is also possible to build a query server, which can be used from Java (or other languages supported by Apache Thrift). Java has a native client, i.e., it works on many platforms without requiring a C++ library to be installed.

**Authors**: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, David Novak. **With contributions from** Ben Frederickson, Lawrence Cayton, Wei Dong, Avrelin Nikita, Dmitry Yashunin, Bob Poekert, @orgoro, @gregfriedland, 
Scott Gigante, Maxim Andreev, Daniel Lemire, Nathan Kurz, Alexander Ponomarenko.

## Brief History

NMSLIB started as a personal project of Bilegsaikhan Naidan, who created the initial code base, the Python bindings,
and participated in earlier evaluations. 
The most successful class of methods--neighborhood/proximity graphs--is represented by the Hierarchical Navigable Small World Graph (HNSW) due to Malkov and Yashunin (see the publications below). Other most useful methods, include a modification of the VP-tree due to Boytsov and Naidan (2013), a Neighborhood APProximation index (NAPP) proposed by Tellez et al. (2013) and improved by David Novak, as well as a vanilla uncompressed inverted file.


## Credits and Citing

If you find this library useful, feel free to cite our SISAP paper [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/sisap/BoytsovN13) as well as other papers listed in the end. One **crucial contribution** to cite is the fast Hierarchical Navigable World graph (HNSW) method [**[BibTex]**](https://dblp.uni-trier.de/rec/bibtex/journals/corr/MalkovY16). Please, [also check out the stand-alone HNSW implementation by Yury Malkov](https://github.com/nmslib/hnswlib), which is released as a header-only HNSWLib library.

## License

The code is released under the Apache License Version 2.0 http://www.apache.org/licenses/. 
Older versions of the library include additional components, which have different licenses (but this does not apply to NMLISB 2.x):

Older versions of the library included the following components:
* The LSHKIT, which is embedded in our library, is distributed under the GNU General Public License, see http://www.gnu.org/licenses/. 
* The k-NN graph construction algorithm *NN-Descent* due to Dong et al. 2011 (see the links below), which is also embedded in our library, seems to be covered by a free-to-use license, similar to Apache 2.
* FALCONN library's licence is MIT.

## Funding

Leonid Boytsov was supported by the [Open Advancement of Question Answering Systems (OAQA) group](https://github.com/oaqa) and the following NSF grant #1618159: "[Matching and Ranking via Proximity Graphs: Applications to Question Answering and Beyond](https://www.nsf.gov/awardsearch/showAward?AWD_ID=1618159&HistoricalAwards=false)". Bileg was supported by the [iAd Center](https://web.archive.org/web/20160306011711/http://www.iad-center.com/).

## Related Publications

Most important related papers are listed below in the chronological order: 
* L. Boytsov, D. Novak, Y. Malkov, E. Nyberg  (2016). [Off the Beaten Path: Let’s Replace Term-Based Retrieval
with k-NN Search.](http://boytsov.info/pubs/cikm2016.pdf) In proceedings of CIKM'16. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/cikm/BoytsovNMN16) We use a special branch of this library, plus [the following Java code](https://github.com/oaqa/knn4qa/tree/cikm2016).
* Malkov, Y.A., Yashunin, D.A.. (2016). [Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs. CoRR](http://arxiv.org/abs/1603.09320), abs/1603.09320. [**[BibTex]**](http://adsabs.harvard.edu/cgi-bin/nph-bib_query?bibcode=2016arXiv160309320M&data_type=BIBTEX&db_key=PRE&nocookieset=1)
* Bilegsaikhan, N., Boytsov, L. 2015 [Permutation Search Methods are Efficient, Yet Faster Search is Possible](http://boytsov.info/pubs/p2332-naidan-arxiv.pdf) PVLDB, 8(12):1618--1629, 2015 [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/journals/corr/NaidanBN15)
* Ponomarenko, A., Averlin, N., Bilegsaikhan, N., Boytsov, L., 2014. [Comparative Analysis of Data Structures for Approximate Nearest Neighbor Search.](http://boytsov.info/pubs/da2014.pdf) [**[BibTex]**](http://scholar.google.com/scholar.bib?q=info:yOjNiT2Ql4AJ:scholar.google.com/&output=citation&hl=en&ct=citation&cd=0)
* Malkov, Y., Ponomarenko, A., Logvinov, A., & Krylov, V., 2014. [Approximate nearest neighbor algorithm based on navigable small world graphs.](http://www.sciencedirect.com/science/article/pii/S0306437913001300) Information Systems, 45, 61-68. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/journals/is/MalkovPLK14)
* Boytsov, L., Bilegsaikhan, N., 2013. [Engineering Efficient and Effective Non-Metric Space Library.](http://boytsov.info/pubs/sisap2013.pdf)   In Proceedings of the 6th International Conference on Similarity Search and Applications (SISAP 2013). [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/sisap/BoytsovN13)  
* Boytsov, L., Bilegsaikhan, N., 2013. [Learning to Prune in Metric and Non-Metric Spaces.](http://boytsov.info/pubs/nips2013.pdf)   In Advances in Neural Information Processing Systems 2013. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/nips/BoytsovN13)
* Tellez, Eric Sadit, Edgar Chávez, and Gonzalo Navarro. [Succinct nearest neighbor search.](http://www.dcc.uchile.cl/~gnavarro/ps/is12.pdf) Information Systems 38.7 (2013): 1019-1030. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/journals/is/TellezCN13)
* A. Ponomarenko, Y. Malkov, A. Logvinov, and V. Krylov  [Approximate nearest
neighbor search small world approach.](http://www.iiis.org/CDs2011/CD2011IDI/ICTA_2011/Abstract.asp?myurl=CT175ON.pdf) ICTA 2011 
* Dong, Wei, Charikar Moses, and Kai Li. 2011. [Efficient k-nearest neighbor graph construction for generic similarity measures.](http://wwwconference.org/proceedings/www2011/proceedings/p577.pdf) Proceedings of the 20th international conference on World wide web. ACM, 2011.
[**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/mir/DongWCL12)
* L. Cayton, 2008 [Fast nearest neighbor retrieval for bregman divergences.](http://lcayton.com/bbtree.pdf) Twenty-Fifth International Conference on Machine Learning (ICML). [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/icml/Cayton08)
* Amato, Giuseppe, and Pasquale Savino. 2008 Approximate similarity search in metric spaces using inverted files. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/conf/infoscale/AmatoS08)
* Gonzalez, Edgar Chavez, Karina Figueroa, and Gonzalo Navarro. [Effective proximity retrieval by ordering permutations.](http://www.dcc.uchile.cl/~gnavarro/ps/tpami07.pdf) Pattern Analysis and Machine Intelligence, IEEE Transactions on 30.9 (2008): 1647-1658. [**[BibTex]**](http://dblp.uni-trier.de/rec/bibtex/journals/pami/ChavezFN08)

