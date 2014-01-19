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

parser = argparse.ArgumentParser(description='vector generator (uniform)')

parser.add_argument('-d','--dim',   required=True, type=int, help='dimensionality (# of vector elements)')
parser.add_argument('-n','--ndata', required=True, type=int, help='# of data points')
parser.add_argument('-o','--outf',  required=True, help='output file')
parser.add_argument('-b','--binary', action='store_false', help='generate binary data')

args = vars(parser.parse_args())

nd   = args['ndata']
outf = args['outf']
dim  = args['dim']

f=open(outf, 'w')
if args['binary'] :
  f.write("\n".join(["\t".join([str(random.random()) for _ in xrange(dim)]) for _ in xrange(nd+1)]))
else:
  f.write("\n".join(["\t".join([str(random.randint(0,1)) for _ in xrange(dim)]) for _ in xrange(nd+1)]))
f.close()

