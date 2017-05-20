from distutils.core import setup, Extension
import sys
import os
import numpy as np

libdir='../similarity_search'
release = '%s/release' % libdir

libraries=[]
extra_objects=['%s/libNonMetricSpaceLib.a' % release]

if sys.platform.startswith('linux'):
    if os.path.isfile('%s/liblshkit.a' % release):
        extra_objects.append('%s/liblshkit.a' % release)
        for lib in ['gsl', 'gslcblas', 'boost_program_options']:
            libraries.append(lib)
    extra_link_args=['-fopenmp', '-shared', '-pthread']
else:
    extra_link_args=[]

nmslib = Extension('nmslib', ['nmslib.cc'],
        include_dirs=['%s/include' % libdir, '%s/release' % libdir, np.get_include()],
        libraries=libraries,
        extra_link_args=extra_link_args,
        extra_objects=extra_objects,
        extra_compile_args=['-std=c++11', '-fno-strict-aliasing', '-Wall', '-Ofast', '-fno-strict-aliasing'])


if __name__ == '__main__':
    setup(
            name='nmslib',
            version='1.6',
            description='Non-Metric Space Library (NMSLIB)',
            author='Leonid Boytsov',
            url='https://github.com/searchivarius/nmslib',
            long_description='Non-Metric Space Library (NMSLIB) is an efficient cross-platform similarity search library and a toolkit for evaluation of similarity search methods. The goal of the project is to create an effective and comprehensive toolkit for searching in generic non-metric spaces. Being comprehensive is important, because no single method is likely to be sufficient in all cases. Also note that exact solutions are hardly efficient in high dimensions and/or non-metric spaces. Hence, the main focus is on approximate methods.',
            ext_modules=[nmslib])
