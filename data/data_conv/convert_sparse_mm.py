#!/usr/bin/env python
#
# Converting from the matrix market (MM) format to sparse text format
# Using the conversion utility: 
# https://github.com/searchivarius/sparse_text_util
#

import os, sys
import scipy, scipy.io
from sparse_text_util import sparse_matr_to_text

inpFile = sys.argv[1]
outFile = sys.argv[2]

data = scipy.io.mmread(inpFile)

sparse_matr_to_text(data, outFile, append_flag=False)


