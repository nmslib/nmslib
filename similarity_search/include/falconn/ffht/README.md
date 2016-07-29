# Fast Fast Hadamard Transform

FFHT (Fast Fast Hadamard Transform) is a library that provides a heavily
optimized C99 implementation of the Fast Hadamard Transform. FFHT also provides
a thin Python wrapper that allows to perform the Fast Hadamard Transform on
one-dimensional [NumPy](http://www.numpy.org/) arrays.

The Hadamard Transform is a linear orthogonal map defined on real vectors whose
length is a _power of two_. For the precise definition, see the
[Wikipedia entry](https://en.wikipedia.org/wiki/Hadamard_transform). The
Hadamard Transform has been recently used a lot in various machine learning
and numerical algorithms.

FFHT uses [AVX](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions)
to speed up the computation. If AVX is not supported on your machine, a simpler
implementation without (explicit) vectorization is used.

The header file `fht.h` exports two functions: `int FHTFloat(float *buffer, int
len, int chunk)` and `int FHTDouble(double *buffer, int len, int chunk)`. The
only difference between them is the type of vector entries. So, in what follows,
we describe how the version for floats `FHTFloat` works.

The function `FHTFloat` takes three parameters:

* `buffer` is a pointer to the data on which one needs to perform the Fast
Hadamard Transform. If AVX is used, `buffer` must be aligned to 32 bytes (one
may use `posix_memalign` to allocate aligned memory).
* `len` is the length of `buffer`. It must be equal to a power of two.
* `chunk` is a positive integer that controls when FFHT switches from recursive
to iterative algorithm. The overall algorithm is recursive, but as soon as the
vector becomes no longer than `chunk`, the iterative algorithm is invoked. For
technical reasons, `chunk` must be at least 8. To choose `chunk` one should use
a program `best_chunk`, see below.

The return value is -1 if the input is invalid and is zero otherwise.

A header-only version of the library is provided in `fht_header_only.h`.

In addition to the Fast Hadamard Transform, we provide two auxiliary programs:
`test` and `best_chunk`, which are implemented in C++11. The former exhaustively
tests the library. The latter chooses the best value of `chunk` given the length
of a buffer, on which the Fast Hadamard Transform would be performed.

FFHT has been tested on 64-bit versions of Linux, OS X and Windows (the latter
is via Cygwin).

To install the Python package, run `python setup.py install`. The script
`example.py` shows how to use FFHT from Python.

## Benchmarks

Below are the times for the Fast Hadamard Transform for vectors of
various lengths. The benchmarks were run on a machine with Intel
Core&nbsp;i5-2500 and 1333 MHz DDR3 RAM. We compare FFHT,
[FFTW 3.3.4](http://fftw.org/), and
[fht](https://github.com/nbarbey/fht) by
[Nicolas Barbey](https://github.com/nbarbey).

Let us stress that FFTW is a great versatile tool, and the authors of FFTW did
not try to optimize the performace of the Fast Hadamard Transform. On the other
hand, FFHT does one thing (the Fast Hadamard Transform), but does it extremely
well.

Vector size | FFHT (float) | FFHT (double) | FFTW 3.3.4 (float) | FFTW 3.3.4 (double) | fht (float) | fht (double)
:---: | :---: | :---: | :---: | :---: | :---: | :---:
2<sup>10</sup> | 0.76 us | 1.47 us | 12.1 us | 16.87 us | 31.4 us | 33.3 us
2<sup>20</sup> | 2.26 ms | 5.50 ms | 18.4 ms | 27.68 ms | 46.8 ms | 61.3 ms
2<sup>27</sup> | 0.74 s | 1.59 s | 2.93 s | 4.64 s | 10.1 s | 11.5 s

## Troubleshooting

For some versions of OS X the native `clang` compiler (that mimicks `gcc`) may
not recognize the availability of AVX. A solution for this problem is to use a
genuine `gcc` (say from [Homebrew](http://brew.sh/)) or to use `-march=corei7-avx`
instead of `-march=native` for compiler flags.

A symptom of the above happening is the undefined macros `__AVX__`.

## Related Work

FFHT has been created as a part of
[FALCONN](https://github.com/falconn-lib/falconn): a library for similarity
search over high-dimensional data. FALCONN's underlying algorithms are described
and analyzed in the following research paper:

> Alexandr Andoni, Piotr Indyk, Thijs Laarhoven, Ilya Razenshteyn and Ludwig
> Schmidt, "Practical and Optimal LSH for Angular Distance", NIPS 2015, full
> version available at [arXiv:1509.02897](http://arxiv.org/abs/1509.02897)

## Acknowledgments

We thank Ruslan Savchenko for useful discussions.

Thanks to:

* Clement Canonne
* Michal Forisek
* Rati Gelashvili
* Daniel Grier
* Dhiraj Holden
* Justin Holmgren
* Aleksandar Ivanovic
* Vladislav Isenbaev
* Jacob Kogler
* Ilya Kornakov
* Anton Lapshin
* Rio LaVigne
* Oleg Martynov
* Linar Mikeev
* Cameron Musco
* Sam Park
* Sunoo Park
* William Perry
* Andrew Sabisch
* Abhishek Sarkar
* Ruslan Savchenko
* Vadim Semenov
* Arman Yessenamanov

for helping us with testing FFHT.
