#!/usr/bin/env python
#
# This script is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
# It generates vectors with coordinates sampled randomly and uniformly from [0,1]
# If one specifies the flag -b or --binary, the script generates only binary data
#

import random
import argparse
import sys
import numpy

parser = argparse.ArgumentParser(description='vector generator (uniform)')

parser.add_argument('-d','--dim',   required=True, type=int, help='dimensionality (# of vector elements)')
parser.add_argument('-n','--ndata', required=True, type=int, help='# of data points')
parser.add_argument('-o','--outf',  required=True, help='output file')
parser.add_argument('--min_val', default=1e-5, help='the minimum possible vector element')

args = vars(parser.parse_args())

nd   = args['ndata']
outf = args['outf']
dim  = args['dim']
minVal=args['min_val']

f=open(outf, 'w')
for i in xrange(nd):
  arr = numpy.array([random.random() for _ in xrange(dim)])
  arr /= sum(arr)
  f.write("\t".join([("%g" % max(v,minVal)) for v in arr]) + "\n")
f.close()

