#!/bin/bash

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

Dimension=$4

if [ "$Dimension" = "" ] ; then
  echo "4th arg is missing: specify the dimensionality"
  exit 1
fi

AlphaLeft=$5

if [ "$AlphaLeft" = "" ] ; then
  echo "5th arg is missing: specify the initial AlphaLeft"
  exit 1
fi

AlphaRight=$6

if [ "$AlphaRight" = "" ] ; then
  echo "6th arg is missing: specify the initial AlphaRight"
  exit 1
fi

K=$7

if [ "$K" = "" ] ; then
  echo "7th arg is missing: specify K"
  exit 1
fi

OutFile=$8

if [ "$OutFile" = "" ] ; then
  echo "8th arg is missing: specify the output file"
  exit 1
fi

BucketSize=$9

if [ "$BucketSize" = "" ] ; then
  BucketSize=10
  echo "9th arg is missing: using the default $BucketSize for the bucket size"
fi

DataQty=${10}

if [ "$DataQty" = "" ] ; then
  DataQty=1200
  echo "10th arg is missing: using the default $DataQty for the # of data points"
fi

QueryQty=${11}

if [ "$QueryQty" = "" ] ; then
  QueryQty=200
  echo "11th arg is missing: using the default $QueryQty for the # of queries"
fi

TestSetQty=${12}

if [ "$TestSetQty" = "" ] ; then
  TestSetQty=5
  echo "12th arg is missing: using the default $TestSetQty for the # of test sets"
fi

RunCmdPrefix="../../../similarity_search/release/tune_vptree --dataFile $DataSet --maxNumQuery $QueryQty --distType float --testSetQty $TestSetQty --dimension $Dimension --maxNumData $DataQty  --spaceType $Space --knn $K "
#--alphaLeft 0.15 --alphaRight 0.5 --desiredRecall 0.9 2>&1|tee out"


echo "================================================================================"
echo "Parameters:"
echo "DataSet: $DataSet Space: $Space DataQty: $DataQty"
echo "QueryQty: $QueryQty TestSetQty: $TestSetQty K=$K"
echo "Initial alphas Left: $AlphaLeft Right: $AlphaRight"
echo "Bucket size: $BucketSize"
echo "Recall list: $RecallList"
echo "Cmd prefix: "
echo "$RunCmdPrefix"
echo "================================================================================"

function DoRun {
  al="$1"
  ar="$2"
  r="$3"
  f="$4"
  cmd="$RunCmdPrefix --method vptree:alphaLeft=$al,alphaRight=$ar,desiredRecall=$r,bucketSize=$BucketSize"
  echo "Recall: $r AlphaLeft=$al AlphaRight=$ar OutFile: $f"
  echo "$cmd"
  $cmd 2>&1|tee $f|grep -i iteration

  if [ "$?" != "0" ] ; then
    echo "Failure!!!!"
    echo "See $f for details"
    exit 1
  fi
}

al=$AlphaLeft
ar=$AlphaRight

outf=`mktemp`

echo -n > $OutFile

for r in $RecallList ; do
  DoRun "$al" "$ar" "$r" "$outf"
  grep "Optimization\ results" $outf -A 10|grep alpha
  al=`grep "Optimization\ results" $outf -A 10|grep alpha_left|sed 's/^.*alpha_left: //'`
  ar=`grep "Optimization\ results" $outf -A 10|grep alpha_right|sed 's/^.*alpha_right: //'`
  Results="alphaLeft=$al,alphaRight=$ar"
  echo "Results for r=$r:$Results"
  echo "$Results" >> $OutFile
done 

rm $outf

