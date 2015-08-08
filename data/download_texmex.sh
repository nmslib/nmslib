#!/bin/bash
. ./lib.sh

cat <<EOF
The SIFT1M data set are a part of the TexMex collection http://corpus-texmex.irisa.fr/
If you use it, please, consider citing:

@article{jegou2011product,
  title={Product quantization for nearest neighbor search},
  author={Jegou, Herve and Douze, Matthijs and Schmid, Cordelia},
  journal={Pattern Analysis and Machine Intelligence, IEEE Transactions on},
  volume={33},
  number={1},
  pages={117--128},
  year={2011},
  publisher={IEEE}
}

EOF

Download http://boytsov.info/datasets/sift_texmex_base1m.txt.gz sift_texmex_base1m.txt.gz
CheckMD5 sift_texmex_base1m.txt.gz f71f8230a0d23bbab61149bd88fd584b

Download http://boytsov.info/datasets/sift_texmex_query.txt.gz sift_texmex_query.txt.gz
CheckMD5 sift_texmex_query.txt.gz afedd5deb184589077829ea1d0d55de7  
