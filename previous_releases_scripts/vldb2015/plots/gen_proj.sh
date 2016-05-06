#!/bin/bash

K=10

tmp_name=`mktemp`

function do_work {
  in_file="$1"
  type="$2"
  out_file="$3"
  loc="$4"
  grep -F "projType=$type," "$in_file" > "$tmp_name"
  n=`wc -l "$tmp_name"|awk '{print $1}'`
  if [ "$n" != "0" ] ; then
    echo "$in_file -> $out_file $type n=$n"
    head -1 $in_file > "$out_file"
    cat "$tmp_name" >> "$out_file"
    cd plots/proj1
    ../genplot_proj1.py  -i "$prefix.dat"  -o "$prefix" -x  "1~norm~Recall" -y "1~log~Frac"  -l "1~$loc" -t "" -k $K
    cd -
  fi
}

for f in "cophir_l2" "sift_l2" "wikipedia_cosinesimil_sparse_fast" "dna32_4_normleven" "sqfd20_10k_sqfd_heuristic_func:alpha=1" "wikipedia_lda128_kldivgenfast" "wikipedia_lda8_kldivgenfast" "wikipedia_lda128_jsdivfast" ; do
  mkdir -p plots/proj1/$f
  in_file="ResultsProjData/$f/res_K=${K}.dat"
  loc="north west"
  for type in "rand" "perm" "randrefpt" "fastmap" ; do
    prefix=`echo "${f}_${type}"|sed 's/[=:]/_/g'`
    if [ "$type" = "perm" -a "$f" = "wikipedia_cosinesimil_sparse_fast" ] ; then
      loc="south east"
    fi
    do_work "$in_file" "$type" "plots/proj1/$prefix.dat" "$loc"
  done
done

rm "$tmp_name"
