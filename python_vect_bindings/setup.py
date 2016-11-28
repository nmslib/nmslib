from distutils.core import setup, Extension

libdir='../similarity_search'
release = '%s/release' % libdir

nmslib_vector = Extension('nmslib_vector', ['nmslib_vector.cc'],
        include_dirs=['%s/include' % libdir, '%s/release' % libdir],
        libraries=['gsl', 'gslcblas', 'boost_program_options'],
        extra_objects=['%s/libNonMetricSpaceLib.a' % release, '%s/liblshkit.a' % release],
        extra_compile_args=['-std=c++11', '-fno-strict-aliasing'])


if __name__ == '__main__':
    setup(
            name='nmslib_vector',
            version='1.5',
            description='Non-Metric Space Library (NMSLIB)',
            author='Leonid Boytsov',
            url='https://github.com/searchivarius/nmslib',
            long_description='Non-Metric Space Library (NMSLIB) is an efficient cross-platform similarity search library and a toolkit for evaluation of similarity search methods. The goal of the project is to create an effective and comprehensive toolkit for searching in generic non-metric spaces. Being comprehensive is important, because no single method is likely to be sufficient in all cases. Also note that exact solutions are hardly efficient in high dimensions and/or non-metric spaces. Hence, the main focus is on approximate methods.',
            ext_modules=[nmslib_vector])
