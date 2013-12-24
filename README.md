Non-Metric Space Library
=================

Authors: Bilegsaikhan Naidan, Leonid Boytsov.
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

1. G++ that supports the new C++ standard, e.g., version 4.7 or the Intel compiler version 14.
2. Linux
3. cmake (GNU make is also required)
4. An Intel or AMD processor that supports SSE 4.2
5. Boost (dev version)
6. GNU scientific library (dev version)

To compile, go to similarity_search, type:  
cmake .  
Then, type:  
make   

Examples of using the software can be found in sample_scripts. A good starting point is a script sample_scripts/sample_run.sh. This script uses small data sets stored in this repository. The complete data set can be obtained as follows:

1. Download Cayton data using data/download_cayton.sh
2. The colors data set is stored in this repository (the file data/colors112.txt). 
3. Generate a 64-dimensional data set, where each coordinate is a uniform number sampled from [0,1]:  

data/genunif.py -d 64 -n 500000 -o unif64.txt  

Then, copy all the data to some directory X and set the environment variable:  

export DATA_DIR=[path to the directory with data files]


We use the data set created by Lawerence Cayton (lcayton.com). If you use this data, please, consider citing:

@inproceedings{cayton:2008,  
    title={Fast nearest neighbor retrieval for bregman divergences},  
    author={Cayton, Lawrence},   
    booktitle={Proceedings of the 25th international conference on Machine learning},  
    pages={112--119},   
    year={2008},   
    organization={ACM}  
}  

Additionally, we use the colors data set (see the file colors112.txt in the data directory) from the Metric Spaces Library:

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

