#/bin/bash


DATA_DIR=$1

if [ "$DATA_DIR" = "" ] ; then
  echo "Specify a directory with data files (1st arg)"
  exit 1
fi

if [ ! -d "$DATA_DIR" ] ; then
  echo "Not a directory: $DATA_DIR"
  exit 1
fi

DATA_FILE="$DATA_DIR/final64.txt"

if [ ! -f "$DATA_FILE" ; then
  echo "The following data file is missing: $DATA_FILE"
  exit 1
fi

THREAD_QTY=$2

if [ "$THREAD_QTY" = "" ] ; then
  echo "Specify the number of test threads (2d arg)"
  exit 1
fi

# If you have python, latex, and PGF install, 
# you can set the following variable to 1 to generate a performance plot.
GEN_PLOT=$3
if [ "$GEN_PLOT" = "" ]
  echo "Specify a plot-generation flag: 1 to generate plots (3d arg)"
  exit 1
fi

TEST_SET_QTY=2
QUERY_QTY=500
RESULT_FILE=output_file
K=10
SPACE=l2
BIN_DIR="../similarity_search"

rm -f $RESULT_FILE
rm -f $LOG_FILE

CMD_PREFIX=$BIN_DIR/release/experiment --distType float --spaceType $SPACE \
        --cachePrefixGS exper1_cache -b $TEST_SET_QTY -Q  $QUERY_QTY -k $K \
        --dataFile "$DATA_FILE" --outFilePrefix $RESULT_FILE --appendToResFile 0 -l "$LOG_FILE" 


        --method vptree:tuneK=$K,bucketSize=50,chunkBucket=1 \

# Testing the bregman-ball tree
$BinDir/release/experiment --distType float --spaceType kldivgenfast \
        --cachePrefixGS exper2_cache --maxCacheGSQty 2000 \
        --testSetQty 5 --maxNumQuery 100 --knn 1 \
        --dataFile $SampleData/final8_10K.txt --outFilePrefix result --appendToResFile 1 -l log \
        --method bbtree:maxLeavesToVisit=20,bucketSize=10
               
# Testing permutation-based filtering methods
$BinDir/release/experiment --distType float --spaceType l2 \
        --cachePrefixGS exper3_cache --maxCacheGSQty 2000 \
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
        --cachePrefixGS exper4_cache --maxCacheGSQty 2000 \
        --testSetQty 5 --maxNumQuery 100 --knn 1 --appendToResFile 1  -l log \
        --dataFile $SampleData/final8_10K.txt --outFilePrefix result \
        --method small_world_rand:NN=3,initIndexAttempts=5,initSearchAttempts=2,indexThreadQty=4  \
        --method nndes:NN=3,rho=0.5,initSearchAttempts=2  \
        --method seq_search  \
        --method mult_index:methodName=vptree,indexQty=5,maxLeavesToVisit=2 

if [ "$GEN_PLOT" = 1 ] ; then
  ./genplot_configurable.py -i result_K\=1.dat -o plot_1nn -x 1~norm~Recall -y 1~log~ImprEfficiency -l "2~(0.96,-.2)" -t "ImprEfficiency vs Recall"
fi
