#!/usr/bin/env python
import sys, numpy

inf=sys.argv[1]
outf=sys.argv[2]

X = []
for i, line in enumerate(open(inf, 'r')):
    v = [float(x) for x in line.strip().split()]
    X.append(v)

X = numpy.vstack(X)
X = X.astype(numpy.float32)
X /= numpy.linalg.norm(X, axis=1).reshape(-1,  1)
center = numpy.mean(X, axis=0)
X -= center

numpy.savetxt(outf, X)
