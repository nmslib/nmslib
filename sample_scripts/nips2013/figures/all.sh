# L2
cd FiguresL2
rm -rf colors112*  ; rm -rf final128*  ; rm -rf unif64*  ; rm -rf FiguresL2* ;

../genplots_nips2013.py -d ../../ResultsL2/colors112 -y 1 -t Colors && ../rem.sh
../genplots_nips2013.py -d ../../ResultsL2/final128 -y 0 -t RCV-128 && ../rem.sh
../genplots_nips2013.py -d ../../ResultsL2/unif64 -y 0 -t Uniform && ../rem.sh


cd ../FiguresKL
rm -rf final16* ; rm -rf final128* ; rm -rf sig10k* ; rm -rf ResultsKL* ;

../genplots_nips2013.py -d ../../ResultsKL/final16 -y 1 -t RCV-16 && ../rem.sh
../genplots_nips2013.py -d ../../ResultsKL/final128 -y 0 -t RCV-128 && ../rem.sh
../genplots_nips2013.py -d ../../ResultsKL/sig10k -y 0 -t "SIFT signatures" && ../rem.sh

