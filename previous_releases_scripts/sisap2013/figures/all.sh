# L2
cd FiguresL2
rm -rf colors112*  ; rm -rf final128*  ; rm -rf unif64*  ; rm -rf FiguresL2* ;

../genplots_sisap2013.py -d ../../ResultsL2/colors112 -y 1 -t "" -p "north east,north east,south east,south east" && ../rem.sh
../genplots_sisap2013.py -d ../../ResultsL2/final128 -y 0 -t "" -p "north east,north east,south east,south east" && ../rem.sh
../genplots_sisap2013.py -d ../../ResultsL2/unif64 -y 0 -t "" -p "north west,north west,south east,south east" && ../rem.sh


cd ../FiguresKL
rm -rf final16* ; rm -rf final128* ; rm -rf sig10k_norm* ; rm -rf FiguresKL* ;

../genplots_sisap2013.py -d ../../ResultsKL/final16 -y 1 -t "" -p "north west,north west,south east,north east" && ../rem.sh
../genplots_sisap2013.py -d ../../ResultsKL/final128 -y 0 -t "" -p "north west,north west,south east,north east" && ../rem.sh
../genplots_sisap2013.py -d ../../ResultsKL/sig10k_norm -y 0 -t ""  -p "north west,north west,south east,south east" && ../rem.sh


cd ../FiguresItakuraSaito
rm -rf final16* ; rm -rf final128* ; rm -rf sig10k_norm* ; rm -rf FiguresItakuraSaito* ;

../genplots_sisap2013.py -d ../../ResultsItakuraSaito/final16 -y 1 -t "" -p "north east,north east,south east,north east" && ../rem.sh
../genplots_sisap2013.py -d ../../ResultsItakuraSaito/final128 -y 0 -t ""  -p "north west,north west,north west,north west" && ../rem.sh
../genplots_sisap2013.py -d ../../ResultsItakuraSaito/sig10k_norm -y 0 -t ""  -p "north west,north west,south east,south east" && ../rem.sh
