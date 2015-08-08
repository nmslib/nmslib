#!/bin/bash
. ./lib.sh

cat <<EOF
This Wikipedia-based data set was created using the gensim library.
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

Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaLSI128/wikipedia_lsi128.7z.001 wikipedia_lsi128.7z.001
CheckMD5 wikipedia_lsi128.7z.001  bed9892cff3d5aa984ada8527d1a349f
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaLSI128/wikipedia_lsi128.7z.002 wikipedia_lsi128.7z.002
CheckMD5 wikipedia_lsi128.7z.002  78a9add44148d3e2db2efd85adb4507e
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaLSI128/wikipedia_lsi128.7z.003 wikipedia_lsi128.7z.003
CheckMD5 wikipedia_lsi128.7z.003  f3cbd8445e7b5a58712e9cd896d2b577
Download https://s3.amazonaws.com/RemoteDisk/TextCollections/WikipediaLSI128/wikipedia_lsi128.7z.004 wikipedia_lsi128.7z.004
CheckMD5 wikipedia_lsi128.7z.004  251a9dbf2bd6747945d0fd94be6ac045


echo "Download was successful! MD5 sums are correct."
