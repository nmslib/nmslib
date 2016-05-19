#/bin/bash


DATA_FILE=$1

if [ "$DATA_FILE" = "" ] ; then
  echo "Specify a test file (1st arg)"
  exit 1
fi

if [ ! -f "$DATA_FILE" ] ; then
  echo "The following data file is missing: $DATA_FILE"
  exit 1
fi

THREAD_QTY=$2

if [ "$THREAD_QTY" = "" ] ; then
  echo "Specify the number of test threads (2d arg)"
  exit 1
fi

# If you have python, latex, and PGF installed, 
# you can set the following variable to 1 to generate a performance plot.
GEN_PLOT=$3
if [ "$GEN_PLOT" = "" ] ; then
  echo "Specify a plot-generation flag: 1 to generate plots (3d arg)"
  exit 1
fi

TEST_SET_QTY=2
QUERY_QTY=500
RESULT_FILE=output_file
K=10
SPACE=l2
LOG_FILE_PREFIX="log_$K"
BIN_DIR="../similarity_search"
GS_CACHE_DIR="gs_cache"
mkdir -p $GS_CACHE_DIR
CACHE_PREFIX_GS=$GS_CACHE_DIR/test_run.tq=$THREAD_QTY

rm -f $RESULT_FILE
rm -f $LOG_FILE_PREFIX.*

ARG_PREFIX="-s $SPACE -i $DATA_FILE -g $CACHE_PREFIX_GS -b $TEST_SET_QTY -Q  $QUERY_QTY -k $K  --outFilePrefix $RESULT_FILE --threadTestQty $THREAD_QTY "

LN=1

function do_run {
  DO_APPEND="$1"
  ADD_ARG="$2"
  ARGS="$ARG_PREFIX $ADD_ARG"
  if [ "$DO_APPEND" = "1" ] ; then
    APPEND_FLAG=" -a "
  fi
  cmd="$BIN_DIR/release/experiment $ARGS $APPEND_FLAG -l ${LOG_FILE_PREFIX}.$LN"
  echo "$cmd"
  bash -c "$cmd"
  if [ "$?" != "0" ] ; then
    echo "Command failed, args : $ARGS"
    echo $CMD
    exit 1 
  fi
  LN=$((LN+1))
}

do_run 0 "-m napp -c numPivot=512,numPivotIndex=64 -t numPivotSearch=40 -t numPivotSearch=42 -t numPivotSearch=44 -t numPivotSearch=46 -t numPivotSearch=48"
do_run 1 "-m sw-graph -c NN=10 -t efSearch=10 -t efSearch=20 -t efSearch=40 -t efSearch=80 -t efSearch=160 -t efSearch=240"
do_run 1 "-m hnsw -c M=10 -t efSearch=10 -t efSearch=20 -t efSearch=40 -t efSearch=80 -t efSearch=160 -t efSearch=240"
do_run 1 "-m vptree -c tuneK=$K,bucketSize=50,desiredRecall=0.99,chunkBucket=1 "
do_run 1 "-m vptree -c tuneK=$K,bucketSize=50,desiredRecall=0.975,chunkBucket=1 "
do_run 1 "-m vptree -c tuneK=$K,bucketSize=50,desiredRecall=0.95,chunkBucket=1 "
do_run 1 "-m vptree -c tuneK=$K,bucketSize=50,desiredRecall=0.925,chunkBucket=1 "
do_run 1 "-m vptree -c tuneK=$K,bucketSize=50,desiredRecall=0.9,chunkBucket=1 "

if [ "$GEN_PLOT" = 1 ] ; then
  ./genplot_configurable.py -i ${RESULT_FILE}_K=${K}.dat -o plot_${K}_NN -x 1~norm~Recall -y 1~log~ImprEfficiency -l "1~south west" -t "Improvement in efficiency vs recall" -n "MethodName" -a axis_desc.txt -m meth_desc.txt
fi
