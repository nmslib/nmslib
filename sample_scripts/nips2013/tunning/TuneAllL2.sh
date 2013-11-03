#!/bin/bash

RecallList="0.99 0.9 0.8 0.7 0.6 0.5 0.4 0.3 0.2"
BucketSize="50"

for K in 1 ; do
  ../../TuneOneMeth.sh "$RecallList" ~/DataNIPS/unif64.txt l2  64 9 9 $K ResultsL2/OutFile.unif64.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" ~/DataNIPS/colors112.txt l2  112 9 9 $K ResultsL2/OutFile.colors112.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" ~/DataNIPS/final128.txt l2  128 9 9 $K ResultsL2/OutFile.final128.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" ~/DataNIPS/sig10k.txt l2  1111 9 9 $K ResultsL2/OutFile.sig10k.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi


  ../../TuneOneMeth.sh "$RecallList" ~/DataNIPS/final256.txt l2  256 9 9 $K ResultsL2/OutFile.final256.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ../../TuneOneMeth.sh "$RecallList" ~/DataNIPS/final32.txt l2  32 9 9 $K ResultsL2/OutFile.final32.$K $BucketSize
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi
done
