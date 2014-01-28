#!/bin/bash

function CheckMD5() {
    file=$1
    md5=$2
    echo "Computing MD5 for $file"
    val=`md5sum $file|cut -f 1 -d \   `
    echo "Checking MD5 for $file"
    if [ "$?" != "0" ] ; then
        echo "Failed to compute the MD5 sum for $file"
        exit 1
    fi
    if [ "$val" != "$md5" ] ; then
        echo "MD5 check failed, expected value: '$md5', but obtained: '$val'"
        exit 2
    fi
    echo "$md5 -> $val"
}

function Download() {
    url=$1
    file=$2
    echo "Downloading $url"
    rm -f $file
    wget $1 --output-document $file 
    if [ $? != "0" ] ; then
        echo "Failed to download, url: $url"
        exit 1
    fi
}
