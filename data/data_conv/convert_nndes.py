#!/usr/bin/env python
# Converting files from the NNDES repository:
# https://code.google.com/p/nndes/downloads/list
# Should be run on an Intel (little-endian) machine

#
# These files are in binary format.  
# In the beginning of the file are three 32-bit unsigned integers:
#       element size == sizeof(float) == 4,
#       SIZE == number of vectors in the file,
#       DIM == dimension of the vectors.
# These are followed by SIZE * DIM 32-bit floating point numbers, 
# with the dimensions of each vector stored contiguously.
#

import sys
import struct
import array
import os

ifile = sys.argv[1]
maxQty = 0
if len(sys.argv) == 3:
  maxQty = int(sys.argv[2])

fsize = os.path.getsize(ifile)

fin = open(ifile, 'rb')

elemSize = struct.unpack('i', fin.read(4))[0]
if elemSize != 4:
  raise Exception('The first element should be an integer 32, but got {}'.format(elemSize))
vectnum = struct.unpack('i', fin.read(4))[0]
dim     = struct.unpack('i', fin.read(4))[0]

if (fsize - 12) % (4*dim) != 0:
  raise Exception("File: " + ifile + " has the wrong format, expected dim = " + str(dim))

rowQty = (fsize - 12) / (dim) / 4

if rowQty != vectnum:
  raise Exception("Expected {} lines, but the head says {}".format(rowQty,vectnum))

if maxQty != 0:
  rowQty = min(rowQty, maxQty)

for i in range(0,rowQty):
  vec = array.array('f')
  
  vec.read(fin, dim)
  out = ''
  for j in range(0,dim):
    if j > 0: out = out + ' '
    out = out + str(vec[j])
  print out
    
