#!/bin/bash
if [ "$DATA_DIR" = "" ] ; then
  echo "Define the environemnt variable DATA_DIR, which contains data files."
  exit 1
fi
if [ ! -d "$DATA_DIR" ] ; then
  echo "DATA_DIR='$DATA_DIR' is not a directory"
  exit 1
fi


RecallList="0.99 0.9 0.8 0.7 0.6 0.5"
BucketSize="50"
DataQty="2000"

for K in 1 10 ; do
  for n in 16 64 256 ; do 
    ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/final$n.txt KLDivGenFast  $n 0.1 0.1 $K ResultsKL/OutFile.final$n.$K $BucketSize $DataQty
    if [ "$?" != "0" ] ; then
      echo "Tunning failed!"
      exit 1
    fi
  done
done
