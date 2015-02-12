#!/bin/bash
# See instructions at http://hgdownload.cse.ucsc.edu/goldenPath/hg38/bigZips/
wget http://hgdownload.cse.ucsc.edu/goldenPath/hg38/bigZips/hg38.2bit
./twoBitToFa  hg38.2bit humanGenome.fa
grep -v chr1 humanGenome.fa |grep -v N > humanGenomeCleaned.txt
echo "You may wanna delete 'humanGenome.fa' and 'hg38.2bit', e.g.:"
echo "rm 'humanGenome.fa' 'hg38.2bit'"

