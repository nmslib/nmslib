#!/bin/bash
. ./lib.sh

function get_item() {
  md5=$1
  fn=$2

  Download "https://s3.amazonaws.com/RemoteDisk/TextCollections/VLDB2015/$fn" "$fn"
  CheckMD5 "$fn" "$md5"
}


# SQFD
#get_item c12453480d0d161563f8f84fbe317834  in_10_10k.txt.bz2
get_item 2ccc604b95dfcaa06b26b6c60ddd7429  in_20_10k.txt.bz2

cat <<EOF
The Signature Quadratic Form Distance (SQFD) distance was proposed by Christian Beecks:

@inproceedings{beecks2010signature,
  title={Signature quadratic form distance},
  author={Beecks, Christian and Uysal, Merih Seran and Seidl, Thomas},
  booktitle={Proceedings of the ACM International Conference on Image and Video Retrieval},
  pages={438--445},
  year={2010},
  organization={ACM}
}

The data set to be used with SQFD was created using ImageNet.

@article{russakovsky2014imagenet,
  title={Imagenet large scale visual recognition challenge},
  author={Russakovsky, Olga and Deng, Jia and Su, Hao and Krause, Jonathan and Satheesh, Sanjeev and Ma, Sean and Huang, Zhiheng and Karpathy, Andrej and Khosla, Aditya and Bernstein, Michael and others},
  journal={International Journal of Computer Vision},
  pages={1--42},
  year={2014},
  publisher={Springer}
}
EOF


# DNA
get_item 47833a114cc6eb67c10c129c65221a5f  dna5M_32_4.txt.bz2

cat <<EOF
DNA sequences were sampled from the human genome, see:
http://hgdownload.cse.ucsc.edu/goldenPath/hg38/bigZips/
EOF

# SIFT
get_item 334e871baf3338b2ae889bc9f8d5f7dc  sift_texmex_learn5m.txt.bz2

cat <<EOF
The SIFT 1B data sets is a part of the TexMex collection http://corpus-texmex.irisa.fr/
If you use them, please, consider citing:

@inproceedings{jegou2011searching,
  title={Searching in one billion vectors: re-rank with source coding},
  author={J{\'e}gou, Herv{\'e} and Tavenard, Romain and Douze, Matthijs and Amsaleg, Laurent},
  booktitle={Acoustics, Speech and Signal Processing (ICASSP), 2011 IEEE International Conference on},
  pages={861--864},
  year={2011},
  organization={IEEE}
}

EOF

get_item c471e23616fab3a4ab7d18b85af9be26  wikipedia_lda128.txt.bz2
get_item 07afa2abe6e4cddd117ae4750dc59c62  wikipedia_lda8.txt.bz2


cat <<EOF
All Wikipedia based data sets used in the VLDB 2015 work were created using the gensim library.
If you use it, please, consider citing:

@inproceedings{rehurek_lrec,
  title = {{Software Framework for Topic Modelling 
            with Large Corpora}},
  author = {Radim {\v R}eh{\r u}{\v r}ek and Petr Sojka},
  booktitle = {{Proceedings of the LREC 2010 Workshop on New
                Challenges for NLP Frameworks}},
  pages = {45--50},
  year = 2010,
  month = May,
  day = 22,
  publisher = {ELRA},
  address = {Valletta, Malta},
  note={\url{http://is.muni.cz/publication/884893/en}},
  language={English}
}

EOF

./download_wikipedia_sparse.sh

if [ "$?" != "0" ] ; then
  echo "Download of Wikipedia-sparse failed"
  exit 1
fi

