#!/bin/bash
. ./lib.sh

cat <<EOF
This Wikipedia based data set was created using the gensim library.
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

Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.001 wikipedia.7z.001
CheckMD5 wikipedia.7z.001 1f7c779cb223a556e0e83b7406ffc3a3
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.002 wikipedia.7z.002
CheckMD5 wikipedia.7z.002 1d2e58bb3ae8437f02a2df9ed7ad9af5
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.003 wikipedia.7z.003
CheckMD5 wikipedia.7z.003 e4dff31ee1fe4ab5ea8dd75bb747f37c
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.004 wikipedia.7z.004
CheckMD5 wikipedia.7z.004 f27bfdcc0f12ffda12358668cef0f804
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.005 wikipedia.7z.005
CheckMD5 wikipedia.7z.005 d0ab7e4e2429954598d6974aa9e6d0e9
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.006 wikipedia.7z.006
CheckMD5 wikipedia.7z.006 61bca1ad3cd0caf59b9bec98eedad68d
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.007 wikipedia.7z.007
CheckMD5 wikipedia.7z.007 648a816969997be4c72dd3ede017af2c
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.008 wikipedia.7z.008
CheckMD5 wikipedia.7z.008 08e6c2ddcc974d18aa0a693df5490d82
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.009 wikipedia.7z.009
CheckMD5 wikipedia.7z.009 169ccfb5affecfc9023e8414815fbf53
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaSparse/wikipedia.7z.010 wikipedia.7z.010
CheckMD5 wikipedia.7z.010 fe7ce81d36ad132fc4d982edd767be48


echo "Download of the Wikipedia-sparse was successful! MD5 sums are correct."
