#!/bin/bash
#./download_cayton.sh
wget http://boytsov.info/datasets/colors112.txt.gz
gunzip colors112.txt.gz
./genunif.py -d 64 -n 500000 -o unif64.txt

