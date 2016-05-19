#!/bin/bash
if [ "$DATA_DIR" = "" ] ; then
  echo "Define the environemnt variable DATA_DIR, which contains data files."
  exit 1
fi
if [ ! -d "$DATA_DIR" ] ; then
  echo "DATA_DIR='$DATA_DIR' is not a directory"
  exit 1
fi


RecallList="0.99 0.9 0.8 0.7 0.6 0.5 0.4 0.3 0.2"
BucketSize="50"

for K in 1 ; do
  ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/sig10k_norm.txt KLDivGenFast  1111 0.1 0.1 $K ResultsKL/OutFile.sig10k_norm.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  for n in 16 128 ; do 
    ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/final$n.txt KLDivGenFast  $n 0.1 0.1 $K ResultsKL/OutFile.final$n.$K $BucketSize
    if [ "$?" != "0" ] ; then
      echo "Tunning failed!"
      exit 1
    fi
  done
done
