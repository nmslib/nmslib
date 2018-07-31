#!/usr/bin/env python
#
# Converting from the matrix market (MM) format to dense text format
#

import os, sys
import numpy, scipy, scipy.io
from sparse_text_util import sparse_matr_to_text

inpFile = sys.argv[1]
outFile = sys.argv[2]

data = numpy.array(scipy.io.mmread(inpFile).todense())

numpy.savetxt(outFile, data, '%g')


