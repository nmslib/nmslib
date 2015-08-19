#!/bin/bash
if [ "" = "$BIN_DIR" ] ; then
  echo "Define the environemnt variable BIN_DIR, which contains binary files."
  exit 1
fi

if [ ! -d "$BIN_DIR" ] ; then
  echo "BIN_DIR='$BIN_DIR' is not a directory"
  exit 1
fi

RecallList="$1"

if [ "$RecallList" = "" ] ; then
  echo "1st arg is missing: specify the list of recall values"
  exit 1
fi

DataSet=$2

if [ "$DataSet" = "" ] ; then
  echo "2d arg is missing: specify the data set"
  exit 1
fi

Space=$3

if [ "$Space" = "" ] ; then
  echo "3rd arg is missing: specify the space"
  exit 1
fi

K=$4

if [ "$K" = "" ] ; then
  echo "4th arg is missing: specify K"
  exit 1
fi

OutFile=$5

if [ "$OutFile" = "" ] ; then
  echo "5th arg is missing: specify the output file"
  exit 1
fi

BucketSize=$6

if [ "$BucketSize" = "" ] ; then
  BucketSize=1
  echo "6th arg is missing: using the default $BucketSize for the bucket size"
fi

DataQty=$7

if [ "$DataQty" = "" ] ; then
  DataQty=1200
  echo "7th arg is missing: using the default $DataQty for the # of data points"
fi

QueryQty=$8

if [ "$QueryQty" = "" ] ; then
  QueryQty=100
  echo "8th arg is missing: using the default $QueryQty for the # of queries"
fi

TestSetQty=$9

if [ "$TestSetQty" = "" ] ; then
  TestSetQty=3
  echo "9th arg is missing: using the default $TestSetQty for the # of test sets"
fi

minExp=${10}
maxExp=${11}

if [ "$minExp" = "" ] ;then
  minExp=1
fi
if [ "$maxExp" = "" ] ;then
  maxExp=1
fi

RunCmdPrefix="$BIN_DIR/tune_vptree --dataFile $DataSet --maxNumQuery $QueryQty --distType float --testSetQty $TestSetQty --maxNumData $DataQty  --spaceType $Space --knn $K "


echo "================================================================================"
echo "Parameters:"
echo "DataSet: $DataSet Space: $Space DataQty: $DataQty"
echo "QueryQty: $QueryQty TestSetQty: $TestSetQty K=$K"
echo "Bucket size: $BucketSize"
echo "Recall list: $RecallList"
echo "Cmd prefix: "
echo "$RunCmdPrefix"
echo "================================================================================"

function DoRun {
  recall="$1"
  outf="$2"
  exp1="$3"
  exp2="$4"
  cmd="$RunCmdPrefix --method vptree:desiredRecall=$recall,bucketSize=$BucketSize --minExp $exp1 --maxExp $exp2 -o $outf"
  logf=`mktemp`
  echo "Recall: $recall BucketSize: $BucketSize OutFile: $outf Log file: $logf"
  echo "$cmd"
  $cmd 2>&1|tee $logf|grep Iteration

  if [ "$?" != "0" ] ; then
    echo "Failure!!!!"
    echo "See $logf for details"
    exit 1
  fi
  rm "$logf"
}

outf=`mktemp`

echo -n > $OutFile

for r in $RecallList ; do
  DoRun "$r" "$outf" "$minExp" "$maxExp"
  Results=`cat $outf`
  echo "Results for r=$r:$Results"
  echo "$Results" >> $OutFile
done 

rm "$outf"

