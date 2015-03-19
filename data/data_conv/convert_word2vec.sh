#!/bin/bash
inp=$1
outp=$2

function Usage {
  echo $1
  echo "Usage <input> <output>"
  exit 1
}

if [ "$inp" = "" ] ; then
  Usage "Missing input file"
fi

if [ ! -f "$inp" ] ; then
  Usage "Not a file '$inp'"
fi

if [ "$outp" = "" ] ; then
  Usage "Missing output file"
fi

# We currently keep only vectors themselves
./convert_word2vec "$inp"|cut --complement -d ' ' -f 1 > "$outp"
if [ "$?" != "0" ] ; then
  echo "Conversion failed!"
  exit 1
fi
echo "Conversion succeeded!"
