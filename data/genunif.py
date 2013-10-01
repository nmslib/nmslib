#!/usr/bin/env python
#
# This script is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
# It generates vectors with coordinates sampled randomly and uniformly from [0,1]
#

import random
import argparse

parser = argparse.ArgumentParser(description='vector generator (uniform)')

parser.add_argument('-d','--dim',   type=int, help='dimensionality (# of vector elements)')
parser.add_argument('-n','--ndata', type=int, help='# of data points')
parser.add_argument('-o','--outf',  help='output file')

args = vars(parser.parse_args())

nd   = args['ndata']
outf = args['outf']
dim  = args['dim']

if nd is None:
    parser.error('# of data points is required')
if dim is None:
    parser.error('dimensionality is required')
if outf is None:
    parser.error('output file is required')

f=open(outf, 'w')
f.write("\n".join(["\t".join([str(random.random()) for _ in xrange(dim)]) for _ in xrange(nd+1)]))
f.close()

