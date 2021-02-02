import os
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys
import setuptools
import struct

__version__ = '2.1.1'

if sys.platform.startswith("win") and struct.calcsize("P") * 8 == 32:
    raise RuntimeError("Windows 32-bit is not supported.")

dep_list = ['pybind11<2.6.2', 'psutil']
dep_list.append("numpy>=1.10.0,<1.17 ; python_version=='2.7'")
dep_list.append("numpy>=1.10.0 ; python_version>='3.5'")

py_version = tuple([int(v) for v in sys.version.split('.')[:2]])
if py_version != (2, 7) and py_version < (3, 5):
    raise RuntimeError("Python version 2.7 or >=3.5 required.")

print('Dependence list:', dep_list)

libdir = os.path.join(".", "similarity_search")
if not os.path.isdir(libdir) and sys.platform.startswith("win"):
    # If the nmslib symlink doesn't work (windows symlink support w/ git is
    # a little iffy), fallback to use a relative path
    libdir = os.path.join("..", "similarity_search")

library_file = os.path.join(libdir, "release", "libNonMetricSpaceLib.a")
source_files = ['nmslib.cc', 'tensorflow/cpu_feature_guard.cc', 'tensorflow/cpu_info.cc']

libraries = []
extra_objects = []

if os.path.exists(library_file):
    # if we have a prebuilt nmslib library file, use that.
    extra_objects.append(library_file)

else:
    # Otherwise build all the files here directly (excluding extras which need boost)
    exclude_files = set("""space_sqfd.cc dummy_app.cc main.cc""".split())

    full_file_list = list(os.walk(os.path.join(libdir, "src")))

    for root, subdirs, files in full_file_list:
        source_files.extend(os.path.join(root, f) for f in files
                            if f.endswith(".cc") and f not in exclude_files)


class get_pybind_include(object):
    """Helper class to determine the pybind11 include path
    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked. """

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)

ext_modules = [
    Extension(
        'nmslib',
        source_files,
        include_dirs=[os.path.join(libdir, "include"),
                      "tensorflow", 
                      get_pybind_include(),
                      get_pybind_include(user=True)],
        libraries=libraries,
        language='c++',
        extra_objects=extra_objects,
    ),
]


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        try:
            compiler.compile([f.name], extra_postargs=[flagname])
        except setuptools.distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14] compiler flag.

    The c++14 is prefered over c++11 (when it is available).
    """
    if has_flag(compiler, '-std=c++14'):
        return '-std=c++14'
    elif has_flag(compiler, '-std=c++11'):
        return '-std=c++11'
    else:
        raise RuntimeError('Unsupported compiler -- at least C++11 support '
                           'is needed!')


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
        'msvc': [ '/EHsc', '/openmp', '/O2'],
        'unix': [ '-O3'],
    }
    arch_list = '-march -msse -msse2 -msse3 -mssse3 -msse4 -msse4a -msse4.1 -msse4.2 -mavx -mavx2'.split()
    if 'ARCH' in os.environ:
        # /arch:[IA32|SSE|SSE2|AVX|AVX2|ARMv7VE|VFPv4]
        # See https://docs.microsoft.com/en-us/cpp/build/reference/arch-x86
        c_opts['msvc'].append("/arch:{}".format(os.environ['ARCH']))  # bugfix
    no_arch_flag=True
    if 'CFLAGS' in os.environ: 
      for flag in arch_list: 
        if flag in os.environ["CFLAGS"]:
          no_arch_flag=False
          break
    if no_arch_flag:
        c_opts['unix'].append('-march=native')
    link_opts = {
        'unix': [],
        'msvc': [],
    }

    if sys.platform == 'darwin':
        c_opts['unix'] += ['-stdlib=libc++', '-mmacosx-version-min=10.7']
        link_opts['unix'] += ['-stdlib=libc++', '-mmacosx-version-min=10.7']
    else:
        c_opts['unix'].append("-fopenmp")
        link_opts['unix'].extend(['-fopenmp', '-pthread'])

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' %
                        self.distribution.get_version())
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, '-fvisibility=hidden'):
                opts.append('-fvisibility=hidden')
        elif ct == 'msvc':
            opts.append('/DVERSION_INFO=\\"%s\\"' %
                        self.distribution.get_version())

        print('Extra compilation arguments:', opts)

        # extend include dirs here (don't assume numpy/pybind11 are installed when first run, since
        # pip could have installed them as part of executing this script
        import numpy as np
        for ext in self.extensions:
            ext.extra_compile_args.extend(opts)
            ext.extra_link_args.extend(self.link_opts.get(ct, []))
            ext.include_dirs.extend([
                # Path to pybind11 headers
                get_pybind_include(),
                get_pybind_include(user=True),
                # Path to numpy headers
                np.get_include()
            ])

        build_ext.build_extensions(self)

setup(
    name='nmslib',
    version=__version__,
    description='Non-Metric Space Library (NMSLIB)',
    author='B. Naidan, L. Boytsov, Yu. Malkov, B. Frederickson, D. Novak, et al.',
    url='https://github.com/nmslib/nmslib',
    long_description="""Non-Metric Space Library (NMSLIB) is an efficient cross-platform
 similarity search library and a toolkit for evaluation of similarity search methods. 
 The goal of the project is to create an effective and comprehensive toolkit for searching 
 in generic and non-metric spaces. Even though the library contains a variety of metric-space 
 access methods, our main focus is on generic and approximate search methods, in particular, 
 on methods for non-metric spaces. NMSLIB is possibly the first library with a principled 
 support for non-metric space searching.""",
    ext_modules=ext_modules,
    install_requires=dep_list,
    setup_requires=dep_list,
    cmdclass={'build_ext': BuildExt},
    test_suite="tests",
    zip_safe=False,
)
