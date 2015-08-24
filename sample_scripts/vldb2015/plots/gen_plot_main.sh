#!/bin/bash
mkdir -p plots/plot_main
cd plots/plot_main

K=10

N="4999000"

in_file="proc1.in"
in_file0="../../ResultsL2/cophir/res_K=$K.dat"
../extract_data.py -i "$in_file0" -o "$in_file" -q $N

#../genplot.py -i $in_file -o cophir_L2_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.85,-.2)" -t ""
../genplot.py -i $in_file -o cophir_L2_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~south west" -t ""

../genplot.py -i $in_file -o cophir_L2_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "none" -t ""

rm "$in_file"

in_file="proc2.in"
in_file0="../../ResultsL2/sift/res_K=$K.dat"

../extract_data.py -i "$in_file0" -o "$in_file" -q $N

#../genplot.py -i $in_file -o sift_L2_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.85,-.2)" -t ""
../genplot.py -i $in_file -o sift_L2_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~south west" -t ""

../genplot.py -i $in_file -o sift_L2_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "1~north west" -t ""

N="3999000"

in_file="proc3.in"
in_file0="../../ResultsCosine/wikipedia/res_K=$K.dat"

../extract_data.py -i "$in_file0" -o "$in_file" -q $N

#../genplot.py -i $in_file -o wiki_cosine_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.85,-.2)" -t ""
../genplot.py -i $in_file -o wiki_cosine_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~north east" -t ""

../genplot.py -i $in_file -o wiki_cosine_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "none" -t ""


# Beware, look up this number in a dat, no in the rep file!!!!
N="2134795"
fn=8
in_file="proc4.in"
in_file0="../../ResultsJSDivFast/wikipedia_lda_${fn}/res_K=$K.dat"

../extract_data.py -i "$in_file0" -o "$in_file" -q "$N"

#../genplot.py -i $in_file -o wiki_lda_${fn}_jsdiv_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.95,-.2)" -t ""
../genplot.py -i $in_file -o wiki_lda_${fn}_jsdiv_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~north west" -t ""

../genplot.py -i $in_file -o wiki_lda_${fn}_jsdiv_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "none" -t ""

fn=128
in_file="proc5.in"
in_file0="../../ResultsJSDivFast/wikipedia_lda_${fn}/res_K=$K.dat"
N="2134784"


../extract_data.py -i "$in_file0" -o "$in_file" -q "$N"

#../genplot.py -i $in_file -o wiki_lda_${fn}_jsdiv_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.85,-.2)" -t ""
../genplot.py -i $in_file -o wiki_lda_${fn}_jsdiv_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~north west" -t ""

../genplot.py -i $in_file -o wiki_lda_${fn}_jsdiv_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "none" -t ""


# Beware, look up this number in a dat, no in the rep file!!!!
N="2133995"
fn=8
in_file="proc6.in"
in_file0="../../ResultsKLDivGen/wikipedia_lda${fn}/res_K=$K.dat"

../extract_data.py -i "$in_file0" -o "$in_file" -q "$N"

#../genplot.py -i $in_file -o wiki_lda_${fn}_kldiv_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.95,-.2)" -t ""
../genplot.py -i $in_file -o wiki_lda_${fn}_kldiv_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~north west" -t ""

../genplot.py -i $in_file -o wiki_lda_${fn}_kldiv_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "1~north west" -t ""

fn=128
N="2133984"
in_file="proc7.in"
in_file0="../../ResultsKLDivGen/wikipedia_lda${fn}/res_K=$K.dat"


../extract_data.py -i "$in_file0" -o "$in_file" -q "$N"

#../genplot.py -i $in_file -o wiki_lda_${fn}_kldiv_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.85,-.2)" -t ""
../genplot.py -i $in_file -o wiki_lda_${fn}_kldiv_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~north west" -t ""

../genplot.py -i $in_file -o wiki_lda_${fn}_kldiv_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "1~north west" -t ""

# Beware, look up this number in a dat, no in the rep file!!!!
N="2134795"
fn=8
in_file="proc8.in"
in_file0="../../ResultsKLDivGenRQ/wikipedia_lda${fn}/res_K=$K.dat"

../extract_data.py -i "$in_file0" -o "$in_file" -q "$N"

#../genplot.py -i $in_file -o wiki_lda_${fn}_kldivrq_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.95,-.2)" -t ""
../genplot.py -i $in_file -o wiki_lda_${fn}_kldivrq_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~south west" -t ""

../genplot.py -i $in_file -o wiki_lda_${fn}_kldivrq_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "none" -t ""

fn=128
N="2134784"
in_file="proc9.in"
in_file0="../../ResultsKLDivGenRQ/wikipedia_lda${fn}/res_K=$K.dat"


../extract_data.py -i "$in_file0" -o "$in_file" -q "$N"

#../genplot.py -i $in_file -o wiki_lda_${fn}_kldivrq_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.85,-.2)" -t ""
../genplot.py -i $in_file -o wiki_lda_${fn}_kldivrq_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~south west" -t ""

../genplot.py -i $in_file -o wiki_lda_${fn}_kldivrq_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "1~north east" -t ""

N="999800"
in_file="proc10"
in_file0="../../ResultsNormLeven/dna32_4/res_K=$K.dat"


../extract_data.py -i "$in_file0" -o "$in_file" -q "$N"

#../genplot.py -i $in_file -o dna32_4_normleven_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.85,-.2)" -t ""
../genplot.py -i $in_file -o dna32_4_normleven_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~north west" -t ""

../genplot.py -i $in_file -o dna32_4_normleven_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "none" -t ""

N="999800"
in_file="proc11"
in_file0="../../ResultsSQFD/sqfd20_10k/res_K=$K.dat"


../extract_data.py -i "$in_file0" -o "$in_file" -q "$N"

#../genplot.py -i $in_file -o sqfd20_10k_normleven_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.99,-.2)" -t ""
../genplot.py -i $in_file -o sqfd20_10k_eff -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~south west" -t ""

../genplot.py -i $in_file -o sqfd20_10k_dist -x 1~norm~Recall -y 1~log~ImprDistComp -l "none" -t ""

