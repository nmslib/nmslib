import sys

try:
    import pypandoc
    long_description = pypandoc.convert('README.md', 'rst')
except(IOError, ImportError):
    long_description = open('README.md').read()

try:
    from setuptools import setup, find_packages, Extension
except ImportError:
    sys.stderr.write('Setuptools not found!\n')
    raise

try:
    import numpy as np
except ImportError:
    sys.stderr.write('NumPy not found!\n')
    raise

module = Extension('_ffht',
                   sources=['_ffht.c', 'fht.c'],
                   extra_compile_args=['-march=native', '-O3', '-Wall', '-Wextra', '-pedantic',
                                       '-Wshadow', '-Wpointer-arith', '-Wcast-qual',
                                       '-Wstrict-prototypes', '-Wmissing-prototypes',
                                       '-std=c99'],
                   include_dirs=[np.get_include()])

setup(name='FFHT',
      version='1.0',
      author='Ilya Razenshteyn, Ludwig Schmidt',
      author_email='falconn.lib@gmail.com',
      url='https://github.com/FALCONN-LIB/FFHT',
      description='Fast implementation of the Fast Hadamard Transform (FHT)',
      long_description=long_description,
      license='MIT',
      keywords='fast Fourier Hadamard transform butterfly',
      packages=find_packages(),
      include_package_data=True,
      ext_modules=[module])
