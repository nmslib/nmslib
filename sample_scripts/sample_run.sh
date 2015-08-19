#/bin/bash

SampleData="../sample_data"
BinDir="../similarity_search"
LogFile="log"

# If you have python, latex, and PGF install, 
# you can set the following variable to 1 to generate a performance plot.
GEN_PLOT=$1

if [ "$GEN_PLOT" != "0" -a "$GEN_PLOT" != "1" ] ; then
  echo "Specify exactly one argument: 1 to build a plot, 0 not to build a plot"
  exit
fi

# An example of running the 1-nn search

# Testing space-paritioning methods
# The first time we run the testing utility, we specify:
# --appendToResFile 0
# This will create new output files (instead of appending to existing ones)

$BinDir/release/experiment --distType float --spaceType l2 \
        --testSetQty 5 --maxNumQuery 100 --knn 1  \
        --dataFile $SampleData/final8_10K.txt --outFilePrefix result --appendToResFile 0 -l log \
        --method vptree:alphaLeft=2.0,alphaRight=2.0,maxLeavesToVisit=500,bucketSize=10,chunkBucket=1 \
        --method mvptree:maxPathLen=4,maxLeavesToVisit=500,bucketSize=10,chunkBucket=1 \
        --method ghtree:maxLeavesToVisit=10,bucketSize=10,chunkBucket=1 \
        --method list_clusters:useBucketSize=1,bucketSize=100,maxLeavesToVisit=5,strategy=random \
        --method satree

# Testing the bregman-ball tree
$BinDir/release/experiment --distType float --spaceType kldivgenfast \
        --testSetQty 5 --maxNumQuery 100 --knn 1 \
        --dataFile $SampleData/final8_10K.txt --outFilePrefix result --appendToResFile 1 -l log \
        --method bbtree:maxLeavesToVisit=20,bucketSize=10
               
# Testing permutation-based filtering methods
$BinDir/release/experiment --distType float --spaceType l2 \
        --testSetQty 5 --maxNumQuery 100 --knn 1  \
        --dataFile $SampleData/final8_10K.txt --outFilePrefix result --appendToResFile 1 -l log \
        --method proj_incsort:projType=perm,projDim=4,dbScanFrac=0.2 \
        --method pp-index:numPivot=4,prefixLength=4,minCandidate=100  \
        --method proj_vptree:projType=perm,projDim=4,alphaLeft=2,alphaRight=2,dbScanFrac=0.05  \
        --method mi-file:numPivot=128,numPivotIndex=16,numPivotSearch=4,dbScanFrac=0.01  \
        --method napp:numPivot=32,numPivotIndex=8,numPivotSearch=8,chunkIndexSize=1024   \
        --method perm_incsort_bin:numPivot=32,dbScanFrac=0.05  \
        --method perm_bin_vptree:numPivot=32,alphaLeft=2,alphaRight=2,dbScanFrac=0.05  \


# Testing misc methods
$BinDir/release/experiment --distType float --spaceType l2 \
        --testSetQty 5 --maxNumQuery 100 --knn 1 --appendToResFile 1  -l log \
        --dataFile $SampleData/final8_10K.txt --outFilePrefix result \
        --method small_world_rand:NN=3,initIndexAttempts=5,initSearchAttempts=2,indexThreadQty=4  \
        --method nndes:NN=3,rho=0.5,initSearchAttempts=2  \
        --method seq_search  \
        --method mult_index:methodName=vptree,indexQty=5,maxLeavesToVisit=2 

if [ "$GEN_PLOT" = 1 ] ; then
  ./genplot.py -i result_K\=1.dat -o plot_1nn -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.96,-.2)" -t "ImprEfficiency vs Recall"
fi
