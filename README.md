Non-Metric Space Library
=================

Authors: Bilegsaikhan Naidan, Leonid Boytsov. Leo(nid) Boytsov is a maintainer.

Copyright (c) 2010--2013

Detailed documentation will appear shortly. Some underlying principles and motivation are described in our "engineering" paper:

@incollection{boytsov_naidan:2013,  
    title={Engineering Efficient and Effective Non-metric Space Library},  
    author={Boytsov, Leonid and Naidan, Bilegsaikhan},  
    booktitle={Similarity Search and Applications},  
    pages={280--293},  
    year={2013},  
    publisher={Springer}  
}  

Preprint: http://boytsov.info/pubs/sisap2013.pdf  
Slides: http://boytsov.info/pubs/sisap2013slides.pdf   


The details of the prunning approach are outlined in the NIPS'13 paper:  

@incollection{NIPS2013_5018,  
    title = {Learning to Prune in Metric and Non-Metric Spaces},  
    author = {Leonid Boytsov and Bilegsaikhan Naidan},  
    booktitle = {Advances in Neural Information Processing Systems 26},  
    pages = {1574--1582},  
    year = {2013},  
    url = {http://media.nips.cc/nipsbooks/nipspapers/paper_files/nips26/784.pdf},  
}  


Preprint: http://boytsov.info/pubs/nips2013.pdf  
Poster:  http://boytsov.info/pubs/nips2013poster.pdf   
Supplemental materials: http://boytsov.info/pubs/nips2013_suppl.zip  


Some prerequisites:

1. G++ 4.7 or the Intel compiler version 14.
2. Linux
3. cmake (GNU make is also required)
5. Boost (dev version)
6. GNU scientific library (dev version)
7. An Intel or AMD processor that supports SSE 4.2 is recommended

To compile, go to similarity_search, type:  
cmake .  
Then, type:  
make   

Examples of using the software can be found in sample_scripts. A good starting point is a script sample_scripts/sample_run.sh. This script uses small data sets stored in this repository. The complete data set can be obtained using the script data/get_all_data.sh Beware: it is more than 600 Gbytes compressed!

After downloading and generating data, copy it to some directory X and set the environment variable:  

export DATA_DIR=[path to the directory with data files]

We use the data set created by Lawerence Cayton (lcayton.com). If you use it, please, consider citing:

@inproceedings{cayton:2008,  
    title={Fast nearest neighbor retrieval for bregman divergences},  
    author={Cayton, Lawrence},   
    booktitle={Proceedings of the 25th international conference on Machine learning},  
    pages={112--119},   
    year={2008},   
    organization={ACM}  
}  

Additionally, we employ the Colors data set from the Metric Spaces Library. Our software uses slightly different format (without a header). Hence, we put this file into the data directory (in this repository).

@misc{LibMetricSpace, 
    Author = {K.~Figueroa and G.{}~Navarro and E.~Ch\'avez},  
    Keywords = {Metric Spaces, similarity searching},  
    Note = {Available at {\tt http://www.sisap.org/Metric\_Space\_Library.html}},  
    Title = {Metric Spaces Library},  
    Year = {2007  
} 


Sample scripts to tune the decision function for the VP-tree are in sample_scripts/nips2013/tunning   
In addition, the directory sample_scripts contains the full set of scripts that can be used to re-produce our NIPS'13 and SISAP'13 results.  Note that we provide software to generate plots (which requires Python, Latex, and PGF).   


Most of this code is released under the
Apache License Version 2.0 http://www.apache.org/licenses/.

The LSHKIT, which is embedded in our library, is distributed under the GNU General Public License, see http://www.gnu.org/licenses/. 

