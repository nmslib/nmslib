# L2
cd FiguresL2
rm -rf colors112*  ; rm -rf final128*  ; rm -rf unif64*  ; rm -rf FiguresL2* ;

../genplots_nips2013.py -d ../../ResultsL2/colors112 -y 1 -t "Colors (L2)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsL2/final128 -y 0 -t "RCV-128 (L2)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsL2/final256 -y 0 -t "RCV-256 (L2)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsL2/final16 -y 1 -t "RCV-16 (L2)" -p "north east" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsL2/unif64 -y 0 -t "Uniform (L2)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsL2/sig10k -y 0 -t "SIFT signatures (L2)" -p "north east" && ../rem.sh


cd ../FiguresKL
rm -rf final16* ; rm -rf final128* ; rm -rf sig10k* ; rm -rf ResultsKL* ;

../genplots_nips2013.py -d ../../ResultsKL/final8 -y 1 -t "RCV-8 (KL-div)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsKL/final16 -y 1 -t "RCV-16 (KL-div)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsKL/final32 -y 1 -t "RCV-32 (KL-div)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsKL/final128 -y 0 -t "RCV-128 (KL-div)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsKL/final256 -y 0 -t "RCV-256 (KL-div)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsKL/sig10k -y 0 -t "SIFT signatures (KL-div)" && ../rem.sh

cd ../FiguresItakuraSaito
rm -rf final16* ; rm -rf final128* ; rm -rf sig10k* ; rm -rf ResultsItakuraSaito* ;

../genplots_nips2013.py -d ../../ResultsItakuraSaito/final8 -y 1 -t "RCV-8 (Itakura-Saito)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsItakuraSaito/final16 -y 1 -t "RCV-16 (Itakura-Saito)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsItakuraSaito/final32 -y 1 -t "RCV-32 (Itakura-Saito)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsItakuraSaito/final128 -y 0 -t "RCV-128 (Itakura-Saito)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsItakuraSaito/final256 -y 0 -t "RCV-256 (Itakura-Saito)" && ../rem.sh
../genplots_nips2013.py -d ../../ResultsItakuraSaito/sig10k -y 0 -t "SIFT signatures (Itakura-Saito)" && ../rem.sh

