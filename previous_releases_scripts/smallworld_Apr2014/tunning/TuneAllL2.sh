#!/bin/bash
if [ "$DATA_DIR" = "" ] ; then
  echo "Define the environment variable DATA_DIR, which contains data files."
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
  ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/sift_texmex_base1m.txt l2  64 9 9 $K ResultsL2/OutFile.sift_texmex_base1m.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/unif64.txt l2  64 9 9 $K ResultsL2/OutFile.unif64.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/final256.txt l2  256 9 9 $K ResultsL2/OutFile.final256.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/cophir.txt l2  32 9 9 $K ResultsL2/OutFile.cophir.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" $DATA_DIR/wikipedia_lsi128.txt  l2 128 9 9 $K ResultsL2/OutFile.wikipedia_lsi128.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi
done
