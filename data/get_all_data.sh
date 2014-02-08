#!/bin/bash
./download_wikipedia_sparse.sh
./download_wikipedia_lsi128.sh
./download_cayton.sh
./download_colors.sh
./genunif.py -d 64 -n 500000 -o unif64.txt

