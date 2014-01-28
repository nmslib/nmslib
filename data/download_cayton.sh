#!/bin/bash
. ./lib.sh

cat <<EOF
This data set was created by Lawrence Cayton (lcayton.com)
If you use it, please, consider citing:

@inproceedings{cayton2008fast,
  title={Fast nearest neighbor retrieval for bregman divergences},
  author={Cayton, Lawrence},
  booktitle={Proceedings of the 25th international conference on Machine learning},
  pages={112--119},
  year={2008},
  organization={ACM}
}

Note that the sig10k.txt originally used by Cayton is not normalized. Thus,
we additionally created a normalized version: sig10k_norm.txt. 
EOF

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.corelQ_-txt_-gz.file corelQ.txt.gz
CheckMD5 corelQ.txt.gz 2cfd7ad28f728e44bdaebfcdabd2f01d

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.corelX_-txt_-gz.file corelX.txt.gz
CheckMD5 corelX.txt.gz 3e0ab033ec1c80f02294f10fd4cb98c1

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.final128_-txt_-gz.file final128.txt.gz
CheckMD5 final128.txt.gz bcc4b5acd22bd473620fbafa1b6a18b5

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.final16_-txt_-gz.file final16.txt.gz
CheckMD5 final16.txt.gz bfa26b2a3e5ca862569ccdd8a2e58bea

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.final256_-txt_-gz.file final256.txt.gz
CheckMD5 final256.txt.gz 3f82424a5702d9c453e31799e200b888 

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.final2_-txt_-gz.file final2.txt.gz
CheckMD5 final2.txt.gz 81a71287112699ada4027a21235626e3

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.final32_-txt_-gz.file final32.txt.gz 
CheckMD5 final32.txt.gz 1274ef9836154f56720abf6f7e09ba41

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.final4_-txt_-gz.file final4.txt.gz
CheckMD5 final4.txt.gz a5d54ca801f28612a5599a655cfb0aa2

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.final64_-txt_-gz.file final64.txt.gz
CheckMD5 final64.txt.gz 3187516796c1f87608d034cd281494f2

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.final8_-txt_-gz.file final8.txt.gz
CheckMD5 final8.txt.gz acc39c67a3235f79a7b72840ab88d9b4

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.lda128__10k_-txt_-gz.file lda128_10k.txt.gz
CheckMD5 lda128_10k.txt.gz 3ade036582020ebf574c75d9b2889b1c

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.lda16__10k_-txt_-gz.file lda16_10k.txt.gz
CheckMD5 lda16_10k.txt.gz 536fcc507c083871c4082c9ab9bd1f4e

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.lda256__10k_-txt_-gz.file lda256_10k.txt.gz
CheckMD5 lda256_10k.txt.gz 3e5bd062d465cbf31d9aef2963c0b42a

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.lda2__10k_-txt_-gz.file lda2_10k.txt.gz c6e423094ee73b045b524ab25b3d73a1
CheckMD5 c6e423094ee73b045b524ab25b3d73a1

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.lda32__10k_-txt_-gz.file lda32_10k.txt.gz 
CheckMD5 832f2e7b8a1be3c20543329e3ab55bb1

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.lda4__10k_-txt_-gz.file lda4_10k.txt.gz 
CheckMD5 lda4_10k.txt.gz a3e1060a03d7380311210cf57e4db22c

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.lda64__10k_-txt_-gz.file lda64_10k.txt.gz
CheckMD5 lda64_10k.txt.gz ce492f4f541cfafec6e205eef99da788

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.lda8__10k_-txt_-gz.file lda8_10k.txt.gz 
CheckMD5 lda8_10k.txt.gz  190affb8f1945cc81b7d3ebd2ff40a7e 

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.semSpace4500_-txt_-gz.file semSpace4500.txt.gz 
CheckMD5 semSpace4500.txt.gz f0822d37be250f970d6322cfaee1e893 

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.semSpace500_-txt_-gz.file semSpace500.txt.gz
CheckMD5 semSpace500.txt.gz 043a77afe2247260d3a102c94b1fda78

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.sig10k_-txt_-gz.file sig10k.txt.gz  
CheckMD5 sig10k.txt.gz 6927133979ea745b60467d751856e8fc

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.sig10k__norm_-txt_-gz.file sig10k_norm.txt.gz  
CheckMD5 sig10k_norm.txt.gz b2618d4ef84ebdd54658c50cb7fe442e

Download https://s3.amazonaws.com/7dcbd2dde926fb1e854bade78a448b3-default/3.DataSets.Cayton.sig2k_-txt_-gz.file sig2k.txt.gz 
CheckMD5 sig2k.txt.gz 2b19cee166763b982c65bb3f8ce1cf5d 

echo "Download was successful! MD5 sums are correct."
