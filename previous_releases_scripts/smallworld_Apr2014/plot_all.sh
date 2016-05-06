#!/bin/sh
rm -rf figures/*
cd figures
../../genplot.py -i ../ResultsL2/cophir/res_K\=10.dat -o cophir_L2_dist -x 0~norm~Recall -y 1~log~ImprDistComp -l "none" -t "CoPhIR"
../../genplot.py -i ../ResultsL2/sift_texmex_base1m/res_K\=10.dat -o sift_L2_dist -x 0~norm~Recall -y 1~log~ImprDistComp -l "none" -t "SIFT"
../../genplot.py -i ../ResultsL2/unif64/res_K\=10.dat -o unif64_L2_dist -x 0~norm~Recall -y 1~log~ImprDistComp -l "none" -t "Unif64"


../../genplot.py -i ../ResultsL2/cophir/res_K\=10.dat -o cophir_L2_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(1,-.2)" -t ""
../../genplot.py -i ../ResultsL2/sift_texmex_base1m/res_K\=10.dat -o sift_L2_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(1,-.2)" -t ""
../../genplot.py -i ../ResultsL2/unif64/res_K\=10.dat -o unif64_L2_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(1,-.2)" -t ""


../../genplot.py -i ../ResultsKL/final16/res_K\=10.dat -o final16_KL_dist -x 0~norm~Recall -y 1~log~ImprDistComp -l "none" -t "Final16"
../../genplot.py -i ../ResultsKL/final64/res_K\=10.dat -o final64_KL_dist -x 0~norm~Recall -y 1~log~ImprDistComp -l "none" -t "Final64"
../../genplot.py -i ../ResultsKL/final256/res_K\=10.dat -o final256_KL_dist -x 0~norm~Recall -y 1~log~ImprDistComp -l "none" -t "Final256"


../../genplot.py -i ../ResultsKL/final16/res_K\=10.dat -o final16_KL_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.96,-.2)" -t ""
../../genplot.py -i ../ResultsKL/final64/res_K\=10.dat -o final64_KL_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.96,-.2)" -t ""
../../genplot.py -i ../ResultsKL/final256/res_K\=10.dat -o final256_KL_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.96,-.2)" -t ""

../../genplot.py -i ../ResultsCosineSimil/wikipedia_3200000/res_K\=10.dat -o wiki_all_eff  -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.95,-.2)" -t ""
../../genplot.py -i ../ResultsCosineSimil/wikipedia_3200000/res_K\=10.dat -o wiki_all_dist -x 0~norm~Recall -y 1~log~ImprDistComp -l "none" -t "Wikipedia"

../../genplot.py -i ../ResultsCosineSimil/sw_fixed_recall.dat -o wiki_sw -y 1~norm~QueryTime -x 0~log~NumData -l "none" -t "Wikipedia"
../../genplot.py -i ../ResultsCosineSimil/all_meth_fixed_recall.dat -o wiki_all -y 1~norm~QueryTime -x 1~norm~NumData -l "2~(0.95,-.2)" -t ""

