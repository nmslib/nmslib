#!/bin/bash

inp=$1
outp=$2

if [ "$inp" = "" -o "$outp" = "" ] ; then
  echo "Usage: <one cophir tar.gz file> <out file>"
  exit 1
fi

if [ ! -f "$inp"  ] ; then
  echo "$inp is not a file"
  exit 1
fi

rm -f $outp

tar -zxvf $inp

if [ "$?" != "0" ] ; then
  exit 1
fi

n=0
for x in `find xml -name \*.xml` ; do
  n=$(($n+1))
  ./parse_one_cophir_doc.py $x >>$outp 
  echo "$n: $x"
  if [ "$?" != "0" ] ; then
    echo "Failed to process $x"
    exit 1
  fi
done

rm -r xml
