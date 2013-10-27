#/bin/bash

SampleData="../sample_data"

function Run {
    dim=$1
    DataFile=$2
    MaxNumData=$3
    TestSetQty=$4
    QueryFile=$5
    MaxNumQuery=$6
    SpaceType=$7
    DistanceType=$8
    Queries=$9

    if [ "$QueryFile" != "" ] ; then
        TestSetOpt="--queryFile $QueryFile"
    else
        TestSetOpt="--testSetQty $TestSetQty"
    fi
    TestSetOpt="$TestSetOpt --maxNumQuery $MaxNumQuery"
    DistanceTypeOpt=""
    SpaceTypeOpt=""
    if [ "$DistanceType" != "" ] ; then
        DistanceTypeOpt="--distType $DistanceType"
    fi
    if [ "$SpaceType" != "" ] ; then
        SpaceTypeOpt="--spaceType $SpaceType"
    fi

    cmd="../similarity_search/release/experiment --dimension $dim --dataFile $DataFile --maxNumData $MaxNumData $Queries "
    cmd="$cmd --appendToResFile 1 --outFilePrefix $OutFilePrefix $TestSetOpt $SpaceTypeOpt $DistanceTypeOpt $MethDesc"

    $cmd

    if [ "$?" != "0" ] ; then
        echo "Command failed:"
        echo "$cmd"
        exit 1
    fi
}

export OutFilePrefix="ResultPrefix"

rm -f ${OutFilePrefix}*

K=1
DesiredRecall=0.8
NumPivot=8
DbScanFrac=0.05
BucketSize=50
PrefixLen=4
MinCand=128
MaxK=1000
IndexQty=5

# Testing different variants of spaces
export MethDesc=" --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac  "

Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "l0" "float" "--knn $K"
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "l1" "float" "--knn $K"
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "l2" "float" "--knn $K"
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "kldivfast" "float" "--knn $K"
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "kldivfastrq" "float" "--knn $K"
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "kldivgenfast" "float" "--knn $K"
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "kldivgenslow" "float" "--knn $K"
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "kldivgenfastrq" "float" "--knn $K"
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "itakurasaitofast" "float" "--knn $K"

# KL-div experiments

MaxLeavesToVisit=100
    
export MethDesc="\
 --method perm_vptree:numPivot=$NumPivot,dbScanFrac=$DbScanFrac,alphaLeft=2,alphaRight=2,bucketSize=$BucketSize
  --method mult_index:methodName=perm_incsort,indexQty=$IndexQty,numPivot=$NumPivot,dbScanFrac=$DbScanFrac 
  --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac 
  --method permutation:dbScanFrac=$DbScanFrac,numPivot=$NumPivot
  --method vptree_sample:bucketSize=$BucketSize,distLearnThresh=0.0,quantileStepPivot=0.05,maxK=$MaxK
  --method vptree:alphaLeft=0.3,alphaRight=7,bucketSize=$BucketSize  
  --method bbtree:maxLeavesToVisit=$MaxLeavesToVisit,bucketSize=$BucketSize 
  --method perm_prefix:numPivot=$NumPivot,prefixLength=$PrefixLen,minCandidate=$MinCand"


# Testing external query file
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "KLDivGenFast" "float" "--knn $K"
# Testing external query file with the limit on the number of queries
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "500" "KLDivGenFast" "float" "--knn $K"
# Testing bootstrapping mode
Run "128" "$SampleData/final128_10K.txt" "0" "5" "" "500" "KLDivGenFast" "float" "--knn $K"

# L2 experiments
export MethDesc="\
  --method mult_index:methodName=perm_incsort,indexQty=$IndexQty,numPivot=$NumPivot,dbScanFrac=$DbScanFrac 
  --method perm_vptree:numPivot=$NumPivot,dbScanFrac=$DbScanFrac,alphaLeft=2,alphaRight=2,bucketSize=$BucketSize
  --method permutation:dbScanFrac=$DbScanFrac,numPivot=$NumPivot
  --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac  
  --method lsh_multiprobe:desiredRecall=$DesiredRecall,H=1017881,T=10,L=50,tuneK=$K  
  --method lsh_gaussian
  --method ghtree:bucketSize=$BucketSize  
  --method satree:bucketSize=$BucketSize  
  --method list_clusters:bucketSize=$BucketSize  
  --method mvptree:bucketSize=$BucketSize  
  --method vptree:alphaLeft=1,alphaRight=1,bucketSize=$BucketSize 
  --method perm_prefix:numPivot=$NumPivot,prefixLength=$PrefixLen,minCandidate=$MinCand 
"


# Testing external query file
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "l2" "float" "--knn $K"
# Testing external query file with the limit on the number of queries
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "500" "l2" "float" "--knn $K"
# Testing bootstrapping mode
Run "128" "$SampleData/final128_10K.txt" "0" "5" "" "500" "l2" "float" "--knn $K"

# L1 experiments
export MethDesc="\
  --method perm_vptree:numPivot=$NumPivot,dbScanFrac=$DbScanFrac,alphaLeft=2,alphaRight=2,bucketSize=$BucketSize
  --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac  
  --method permutation:numPivot=$NumPivot,dbScanFrac=$DbScanFrac  
  --method lsh
  --method lsh_cauchy
  --method ghtree:bucketSize=$BucketSize  
  --method satree:bucketSize=$BucketSize  
  --method list_clusters:bucketSize=$BucketSize  
  --method mvptree:bucketSize=$BucketSize  
  --method vptree:alphaLeft=1,alphaRight=1,bucketSize=$BucketSize 
  --method perm_prefix:numPivot=$NumPivot,prefixLength=$PrefixLen,minCandidate=$MinCand 
"

# Testing external query file
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "0" "l1" "float" "--knn $K"
# Testing external query file with the limit on the number of queries
Run "128" "$SampleData/final128_10K.txt" "0" "0" "$SampleData/final128_query1K.txt" "500" "l1" "float" "--knn $K"
# Testing bootstrapping mode
Run "128" "$SampleData/final128_10K.txt" "0" "5" "" "500" "l1" "float" "--knn $K"

