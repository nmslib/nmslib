#undef STRIP_EIGEN_AVX
#if defined(__AVX__) && (defined(__CYGWIN__) || defined(__MINGW32__))
#define STRIP_EIGEN_AVX
#endif

#ifdef STRIP_EIGEN_AVX
#undef __AVX__
#warning "Stripping AVX support from Eigen"
#endif

#include <Eigen/Dense>

#ifdef STRIP_EIGEN_AVX
#define __AVX__
#endif
