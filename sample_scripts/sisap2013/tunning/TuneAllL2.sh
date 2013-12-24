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
  ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/unif64.txt l2  64 9 9 $K ResultsL2/OutFile.unif64.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/colors112.txt l2  112 9 9 $K ResultsL2/OutFile.colors112.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/final128.txt l2  128 9 9 $K ResultsL2/OutFile.final128.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

done
