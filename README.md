Non-Metric Space Library
=================

Authors: Bilegsaikhan Naidan, Leonid Boytsov.
Copyright (c) 2010--2013

Documentation will appear shortly. Some underlying principles and motivation are described in our paper:

@incollection{boytsov_naidan:2013,  
&nbsp;&nbsp;title={Engineering Efficient and Effective Non-metric Space Library},  
&nbsp;&nbsp;author={Boytsov, Leonid and Naidan, Bilegsaikhan},  
&nbsp;&nbsp;booktitle={Similarity Search and Applications},  
&nbsp;&nbsp;pages={280--293},  
&nbsp;&nbsp;year={2013},  
&nbsp;&nbsp;publisher={Springer}  
}  

Some prerequisites:

1. G++ that supports the new C++ standard, e.g., version 4.7
2. Linux
3. cmake
4. An Intel or AMD processor that supports SSE 4.1
5. Boost (dev version)
6. GNU scientific library (dev version)

To compile, go to similarity_search, type:  
cmake .  
Then, type:  
make   
A sample run script similarity_search/test/sample_run.sh

We use the data set created by Lawerence Cayton (lcayton.com). See, the download script in the data directory. If you use this data, please, consider citing:

@inproceedings{cayton:2008,  
&nbsp;&nbsp;title={Fast nearest neighbor retrieval for bregman divergences},  
&nbsp;&nbsp;author={Cayton, Lawrence},  
&nbsp;&nbsp;booktitle={Proceedings of the 25th international conference on Machine learning},  
&nbsp;&nbsp;pages={112--119},   
&nbsp;&nbsp;year={2008},   
&nbsp;&nbsp;organization={ACM}  
}  


Most of this code is released under the
Apache License Version 2.0 http://www.apache.org/licenses/.

The LSHKIT, which is embedded in our library, is distributed under the GNU General Public License, see http://www.gnu.org/licenses/. 

