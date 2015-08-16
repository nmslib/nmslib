#!/bin/bash
if [ "$DATA_DIR" = "" ] ; then
  echo "Define the environemnt variable DATA_DIR, which contains data files."
  exit 1
fi
if [ ! -d "$DATA_DIR" ] ; then
  echo "DATA_DIR='$DATA_DIR' is not a directory"
  exit 1
fi


#
# Don't make the top recall larger than 0.98
# it may not converge for some cases and methods.
#

RecallList="0.96 0.91 0.86 0.81 0.76 0.71"

# L2
for K in 10 ; do
  BucketSize="5"
  DataQty="10000"

  ./TuneOneMeth.sh "$RecallList" $DATA_DIR/cophir.txt l2 $K ResultsL2/OutFile.cophir.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  DataQty="10000"

  ./TuneOneMeth.sh "$RecallList" $DATA_DIR/sift.txt l2 $K ResultsL2/OutFile.sift.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

done

# KL-divergence
for K in 10 ; do
  for n in 8 128 ; do 
    BucketSize="5"
    DataQty="5000"
    MinExp="2"
    MaxExp="2"
    QueryQty="100"
    TestSetQty="3"
    
    ./TuneOneMeth.sh "$RecallList" $DATA_DIR/wikipedia_lda$n.txt KLDivGenFastRQ $K ResultsKLFastRQ/OutFile.wikipedia_lda$n.$K $BucketSize $DataQty $QueryQty $TestSetQty $MinExp $MaxExp
    if [ "$?" != "0" ] ; then
      echo "Tunning failed!"
      exit 1
    fi
  
    ./TuneOneMeth.sh "$RecallList" $DATA_DIR/wikipedia_lda$n.txt KLDivGenFast $K ResultsKLFast/OutFile.wikipedia_lda$n.$K $BucketSize $DataQty $QueryQty $TestSetQty $MinExp $MaxExp
    if [ "$?" != "0" ] ; then
      echo "Tunning failed!"
      exit 1
    fi
  done 
done

# SQFD
for K in 10 ; do
  BucketSize="1"
  DataQty="2000"

  ./TuneOneMeth.sh "$RecallList" $DATA_DIR/in_10_10k.txt "sqfd_heuristic_func:alpha=1" $K ResultsSQFD_10_10k/OutFile.sqfd.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

  ./TuneOneMeth.sh "$RecallList" $DATA_DIR/in_20_10k.txt "sqfd_heuristic_func:alpha=1" $K ResultsSQFD_20_10k/OutFile.sqfd.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi

done

# DNA
for K in 10 ; do
  BucketSize="1"
  DataQty="2000"

  ./TuneOneMeth.sh "$RecallList" $DATA_DIR/dna5M_32_4.txt "normleven" $K ResultsNormLeven/OutFile.normleven.$K $BucketSize $DataQty
  if [ "$?" != "0" ] ; then
    echo "Tunning failed!"
    exit 1
  fi
done

# JS-divergence

for K in 10 ; do
  for n in 8 128 ; do 
    BucketSize="1"
    DataQty="2000"
    MinExp="2"
    MaxExp="2"
    
    ./TuneOneMeth.sh "$RecallList" $DATA_DIR/wikipedia_lda$n.txt jsdivfast $K ResultsJSFast/OutFile.wikipedia_lda$n.$K $BucketSize $DataQty 
    if [ "$?" != "0" ] ; then
      echo "Tunning failed!"
      exit 1
    fi
  done
done

