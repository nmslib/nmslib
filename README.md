Non-Metric Space Library
=================

Authors: Bilegsaikhan Naidan, Leonid Boytsov.
Copyright (c) 2010--2013

Documentation will appear shortly. Some underlying principles and motivation are described in our paper:

@incollection{boytsov_naidan:2013,  
  title={Engineering Efficient and Effective Non-metric Space Library},  
  author={Boytsov, Leonid and Naidan, Bilegsaikhan},  
  booktitle={Similarity Search and Applications},  
  pages={280--293},  
  year={2013},  
  publisher={Springer}  
}  

Some prerequisites:

1. G++ that supports the new C++ standard, e.g., version 4.7
2. Linux
3. An Intel or AMD processor that supports SSE 4.1
4. Boost (dev version)
5. GNU scientific library (dev version)

We use the data set created by Lawerence Cayton (lcayton.com). See, the download scrip in the data directory. If you use this data, please, consider citing:

@inproceedings{cayton:2008,  
  title={Fast nearest neighbor retrieval for bregman divergences},  
  author={Cayton, Lawrence},  
  booktitle={Proceedings of the 25th international conference on Machine learning},  
  pages={112--119},   
  year={2008},   
  organization={ACM}  
}  


Most of this code is released under the
Apache License Version 2.0 http://www.apache.org/licenses/.

The LSHKIT, which is embedded in our library, is distributed under the GNU General Public License, see http://www.gnu.org/licenses/. 

