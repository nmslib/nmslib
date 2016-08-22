/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <memory>
#include <cmath>

#include "bunit.h"
#include "space.h"
#include "space/space_sparse_lp.h"
#include "space/space_sparse_scalar.h"
#include "space/space_sparse_vector_inter.h"
#include "space/space_sparse_scalar_fast.h"
#include "space/space_sparse_vector.h"
#include "space/space_scalar.h"
#include "testdataset.h"
#include "distcomp.h"
#include "genrand_vect.h"
#include "permutation_utils.h"
#include "ztimer.h"
#include "pow.h"

#define RANGE          8.0f
#define RANGE_SMALL    1e-6f

namespace similarity {

using namespace std;


/*

#ifdef  __INTEL_COMPILER

// TODO: @Leo figure out how to use this function with Intel and cmake

#include "mkl.h"

TEST(set_intel) {
    vmlSetMode(VML_HA);
    LOG(LIB_INFO) << "Set high-accuracy mode.";
}

#endif

*/


TEST(Platform64) {
  EXPECT_EQ(8 == sizeof(size_t), true);
}

template <typename dist_t>
bool checkElemVectEq(const vector<SparseVectElem<dist_t>>& source,
                     const vector<SparseVectElem<dist_t>>& target) {
  if (source.size() != target.size()) return false;

  for (size_t i = 0; i < source.size(); ++i)
    if (source[i] != target[i]) return false;

  return true;
}

template <typename dist_t>
void TestSparsePackUnpack() {
  for (size_t maxSize = 1024 ; maxSize < 1024*1024; maxSize += 8192) {
    vector<SparseVectElem<dist_t>> source;
    GenSparseVectZipf(maxSize, source);

    LOG(LIB_INFO) << "testing maxSize: " << maxSize << "\nqty: " <<  source.size()
              << " maxId: " << source.back().id_;

    char*     pBuff = NULL; 
    size_t    dataLen = 0;

    PackSparseElements(source, pBuff, dataLen);
    
    vector<SparseVectElem<dist_t>> target;
    UnpackSparseElements(pBuff, dataLen, target);

    bool eqFlag = checkElemVectEq(source, target);

    if (!eqFlag) {
      LOG(LIB_INFO) << "Different source and target, source.size(): " << source.size()
                << " target.size(): " << target.size();
      // Let's print the first different in the case of equal # of elements
      size_t i = 0;
      for (; i < min(source.size(), target.size()); ++i) {
        if (!(source[i] == target[i])) {
          LOG(LIB_INFO) << "First diff, i = " << i << " " << source[i] << " vs " << target[i];
          break;
        }
      }
    }

    EXPECT_EQ(eqFlag, true);
  }
}

TEST(BlockZeros) {
  for (size_t id = 0 ; id <= 3*65536; id++) {
    size_t id1 = removeBlockZeros(id);
   
    size_t id2 = addBlockZeros(id1); 
    EXPECT_EQ(id, id2);
  }
}

#ifdef DISABLE_LONG_TESTS
TEST(DISABLE_SparsePackUnpack) {
#else
TEST(SparsePackUnpack) {
#endif
  TestSparsePackUnpack<float>();
  TestSparsePackUnpack<double>();
}

TEST(TestEfficientPower) {
  double f = 2.0;

  for (unsigned i = 1; i <= 64; i++) {
    double p1 = std::pow(f, i);
    double p2 = EfficientPow(f, i);

    EXPECT_EQ(p1, p2);
  }
}

TEST(TestEfficientFract) {
  unsigned MaxNumDig = 16;

  for (float a = 1.1f ; a <= 2.0f; a+= 0.1f) {
    for (unsigned NumDig = 1; NumDig < MaxNumDig; ++NumDig) {
      uint64_t MaxFract = uint64_t(1) << NumDig;

      for (uint64_t intFract = 0; intFract < MaxFract; ++intFract) {
        float fract = float(intFract) / float(MaxFract);
        float v1 = pow(a, fract);
        float v2 = EfficientFractPow(a, fract, NumDig);

        EXPECT_EQ_EPS(v1, v2, 1e-5f);
      }
    }
  }  
}

template <class T>
bool TestScalarProductAgree(size_t N, size_t dim, size_t Rep) {
    vector<T> vect1(dim), vect2(dim);
    T* pVect1 = &vect1[0]; 
    T* pVect2 = &vect2[0];

    float maxRelDiff = 1e-6f;
    float maxAbsDiff = 1e-6f;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, T(1), T(2), true /* do normalize */);
            GenRandVect(pVect2, dim, T(1), T(2), true /* do normalize */);

            T val1 = ScalarProduct(pVect1, pVect2, dim);
            T val2 = ScalarProductSIMD(pVect1, pVect2, dim);

            bool bug = false;
            T diff = fabs(val1 - val2);
            T diffRel = diff/max(max(fabs(val1),fabs(val2)),T(1e-18));
            if (diffRel > maxRelDiff && diff > maxAbsDiff) {
                bug = true;
                cerr << "Bug ScalarProduct !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 <<  " diff=" << diff << " diffRel=" << diffRel << endl;
            }

            if (bug) return false;
        }
    }

    return true;
}

template <class T>
bool TestNormScalarProductAgree(size_t N, size_t dim, size_t Rep) {
    vector<T> vect1(dim), vect2(dim);
    T* pVect1 = &vect1[0]; 
    T* pVect2 = &vect2[0];

    float maxRelDiff = 1e-6f;
    float maxAbsDiff = 1e-6f;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, T(1), T(2), true /* do normalize */);
            GenRandVect(pVect2, dim, T(1), T(2), true /* do normalize */);

            T val1 = NormScalarProduct(pVect1, pVect2, dim);
            T val2 = NormScalarProductSIMD(pVect1, pVect2, dim);

            bool bug = false;
            T diff = fabs(val1 - val2);
            T diffRel = diff/max(max(fabs(val1),fabs(val2)),T(1e-18));
            if (diffRel > maxRelDiff && diff > maxAbsDiff) {
                bug = true;
                cerr << "Bug NormScalarProduct !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 <<  " diff=" << diff << " diffRel=" << diffRel << endl;
            }

            if (bug) return false;
        }
    }

    return true;
}

// Agreement test functions
template <class T>
bool TestLInfAgree(size_t N, size_t dim, size_t Rep) {
    vector<T> vect1(dim), vect2(dim);
    T* pVect1 = &vect1[0]; 
    T* pVect2 = &vect2[0];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, -T(RANGE), T(RANGE));
            GenRandVect(pVect2, dim, -T(RANGE), T(RANGE));

            T val1 = LInfNormStandard(pVect1, pVect2, dim);
            T val2 = LInfNorm(pVect1, pVect2, dim);
            T val3 = LInfNormSIMD(pVect1, pVect2, dim);

            bool bug = false;

            if (fabs(val1 - val2)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug LInf !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << endl;
                bug = true;
            }
            if (fabs(val1 - val3)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug LInf !!! Dim = " << dim << " val1 = " << val1 << " val3 = " << val3 << endl;
                bug = true;
            }
            if (bug) return false;
        }
    }


    return true;
}

template <class T>
bool TestL1Agree(size_t N, size_t dim, size_t Rep) {
    vector<T> vect1(dim), vect2(dim);
    T* pVect1 = &vect1[0]; 
    T* pVect2 = &vect2[0];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, -T(RANGE), T(RANGE));
            GenRandVect(pVect2, dim, -T(RANGE), T(RANGE));

            T val1 = L1NormStandard(pVect1, pVect2, dim);
            T val2 = L1Norm(pVect1, pVect2, dim);
            T val3 = L1NormSIMD(pVect1, pVect2, dim);

            bool bug = false;

            if (fabs(val1 - val2)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L1 !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << endl;
                bug = true;
            }
            if (fabs(val1 - val3)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L1 !!! Dim = " << dim << " val1 = " << val1 << " val3 = " << val3 << endl;
                bug = true;
            }
            if (bug) return false;
        }
    }

    return true;
}

template <class T>
bool TestL2Agree(size_t N, size_t dim, size_t Rep) {
    vector<T> vect1(dim), vect2(dim);
    T* pVect1 = &vect1[0]; 
    T* pVect2 = &vect2[0];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, -T(RANGE), T(RANGE));
            GenRandVect(pVect2, dim, -T(RANGE), T(RANGE));

            T val1 = L2NormStandard(pVect1, pVect2, dim);
            T val2 = L2Norm(pVect1, pVect2, dim);
            T val3 = L2NormSIMD(pVect1, pVect2, dim);

            bool bug = false;

            if (fabs(val1 - val2)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L2 !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << endl;
                bug = true;
            }
            if (fabs(val1 - val3)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L2 !!! Dim = " << dim << " val1 = " << val1 << " val3 = " << val3 << endl;
                bug = true;
            }
            if (bug) return false;
        }
    }


    return true;
}

template <class T>
bool TestItakuraSaitoAgree(size_t N, size_t dim, size_t Rep) {
    vector<T> vect1(dim), vect2(dim);
    T* pVect1 = &vect1[0]; 
    T* pVect2 = &vect2[0];
    vector<T> precompVect1(dim *2), precompVect2(dim * 2);
    T* pPrecompVect1 = &precompVect1[0];
    T* pPrecompVect2 = &precompVect2[0];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, T(RANGE_SMALL), T(1.0), true);
            GenRandVect(pVect2, dim, T(RANGE_SMALL), T(1.0), true);

            copy(pVect1, pVect1 + dim, pPrecompVect1); 
            copy(pVect2, pVect2 + dim, pPrecompVect2); 
    
            PrecompLogarithms(pPrecompVect1, dim);
            PrecompLogarithms(pPrecompVect2, dim);

            T val0 = ItakuraSaito(pVect1, pVect2, dim);
            T val1 = ItakuraSaitoPrecomp(pPrecompVect1, pPrecompVect2, dim);
            T val2 = ItakuraSaitoPrecompSIMD(pPrecompVect1, pPrecompVect2, dim);

            bool bug = false;

            T AbsDiff1 = fabs(val1 - val0);
            T RelDiff1 = AbsDiff1/max(max(fabs(val1),fabs(val0)),T(1e-18));

            if (RelDiff1 > 1e-5 && AbsDiff1 > 1e-5) {
                cerr << "Bug ItakuraSaito !!! Dim = " << dim << " val1 = " << val1 << " val0 = " << val0 << " Diff: " << (val1 - val0) << " RelDiff1: " << RelDiff1 << " << AbsDiff1: " << AbsDiff1 << endl;
                bug = true;
            }

            T AbsDiff2 = fabs(val1 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val1),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug ItakuraSaito !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << " Diff: " << (val1 - val2) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2 << endl;
                bug = true;
            }

            if (bug) return false;
        }
    }


    return true;
}

template <class T>
bool TestKLAgree(size_t N, size_t dim, size_t Rep) {
    vector<T> vect1(dim), vect2(dim);
    T* pVect1 = &vect1[0]; 
    T* pVect2 = &vect2[0];
    vector<T> precompVect1(dim *2), precompVect2(dim * 2);
    T* pPrecompVect1 = &precompVect1[0];
    T* pPrecompVect2 = &precompVect2[0];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, T(RANGE_SMALL), T(1.0), true);
            GenRandVect(pVect2, dim, T(RANGE_SMALL), T(1.0), true);

            copy(pVect1, pVect1 + dim, pPrecompVect1); 
            copy(pVect2, pVect2 + dim, pPrecompVect2); 
    
            PrecompLogarithms(pPrecompVect1, dim);
            PrecompLogarithms(pPrecompVect2, dim);

            T val0 = KLStandard(pVect1, pVect2, dim);
            T val1 = KLStandardLogDiff(pVect1, pVect2, dim);
            T val2 = KLPrecomp(pPrecompVect1, pPrecompVect2, dim);
            T val3 = KLPrecompSIMD(pPrecompVect1, pPrecompVect2, dim);

            bool bug = false;

            /* 
             * KLStandardLog has a worse accuracy due to computing the log of ratios
             * as opposed to difference of logs, but it is more efficient (log can be
             * expensive to compute)
             */

            T AbsDiff1 = fabs(val1 - val0);
            T RelDiff1 = AbsDiff1/max(max(fabs(val1),fabs(val0)),T(1e-18));
            if (RelDiff1 > 1e-5 && AbsDiff1 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val0 = " << val0 << " val1 = " << val1 << " Diff: " << (val0 - val1) << " RelDiff1: " << RelDiff1 << " AbsDiff1: " << AbsDiff1 << endl;
                bug = true;
            }

            T AbsDiff2 = fabs(val1 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val1),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val2 = " << val2 << " val1 = " << val1 << " Diff: " << (val2 - val1) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2 << endl;
                bug = true;
            }

            T AbsDiff3 = fabs(val1 - val3);
            T RelDiff3 = AbsDiff3/max(max(fabs(val1),fabs(val3)),T(1e-18));
            if (RelDiff3 > 1e-5 && AbsDiff3 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val3 = " << val3 << " val1 = " << val1 << " Diff: " << (val3 - val1) << " RelDiff3: " << RelDiff3 << " AbsDiff3: " << AbsDiff3 << endl;
                bug = true;
            }

            if (bug) return false;
        }
    }


    return true;
}

template <class T>
bool TestKLGeneralAgree(size_t N, size_t dim, size_t Rep) {
    T* pVect1 = new T[dim];
    T* pVect2 = new T[dim];
    T* pPrecompVect1 = new T[dim * 2];
    T* pPrecompVect2 = new T[dim * 2];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, T(RANGE_SMALL), T(1.0), false);
            GenRandVect(pVect2, dim, T(RANGE_SMALL), T(1.0), false);

            copy(pVect1, pVect1 + dim, pPrecompVect1); 
            copy(pVect2, pVect2 + dim, pPrecompVect2); 
    
            PrecompLogarithms(pPrecompVect1, dim);
            PrecompLogarithms(pPrecompVect2, dim);

            T val0 = KLGeneralStandard(pVect1, pVect2, dim);
            T val2 = KLGeneralPrecomp(pPrecompVect1, pPrecompVect2, dim);
            T val3 = KLGeneralPrecompSIMD(pPrecompVect1, pPrecompVect2, dim);

            bool bug = false;

            T AbsDiff1 = fabs(val2 - val0);
            T RelDiff1 = AbsDiff1/max(max(fabs(val2),fabs(val0)),T(1e-18));
            if (RelDiff1 > 1e-5 && AbsDiff1 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val0 = " << val0 << " val2 = " << val2 << " Diff: " << (val0 - val2) << " RelDiff1: " << RelDiff1 << " AbsDiff1: " << AbsDiff1 << endl;
                bug = true;
            }

            T AbsDiff2 = fabs(val3 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val3),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val2 = " << val2 << " val3 = " << val3 << " Diff: " << (val2 - val3) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2 << endl;
                bug = true;
            }

            if (bug) return false;
        }
    }


    return true;
}

template <class T>
bool TestJSAgree(size_t N, size_t dim, size_t Rep, double pZero) {
    vector<T> vect1(dim), vect2(dim);
    T* pVect1 = &vect1[0]; 
    T* pVect2 = &vect2[0];
    vector<T> precompVect1(dim *2), precompVect2(dim * 2);
    T* pPrecompVect1 = &precompVect1[0];
    T* pPrecompVect2 = &precompVect2[0];

    T Dist = 0;
    T Error = 0;
    T TotalQty = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, T(RANGE_SMALL), T(1.0), true);
            SetRandZeros(pVect1, dim, pZero);
            Normalize(pVect1, dim);
            GenRandVect(pVect2, dim, T(RANGE_SMALL), T(1.0), true);
            SetRandZeros(pVect2, dim, pZero);
            Normalize(pVect2, dim);

            copy(pVect1, pVect1 + dim, pPrecompVect1); 
            copy(pVect2, pVect2 + dim, pPrecompVect2); 
    
            PrecompLogarithms(pPrecompVect1, dim);
            PrecompLogarithms(pPrecompVect2, dim);

            T val0 = JSStandard(pVect1, pVect2, dim);
            T val1 = JSPrecomp(pPrecompVect1, pPrecompVect2, dim);

            bool bug = false;

            T AbsDiff1 = fabs(val1 - val0);
            T RelDiff1 = AbsDiff1/max(max(fabs(val1),fabs(val0)),T(1e-18));

            if (RelDiff1 > 1e-5 && AbsDiff1 > 1e-5) {
                cerr << "Bug JS (1) " << typeid(T).name() << " !!! Dim = " << dim << " val0 = " << val0 << " val1 = " << val1 << " Diff: " << (val0 - val1) << " RelDiff1: " << RelDiff1 << " AbsDiff1: " << AbsDiff1 << endl;
                bug = true;
            }

            T val2 = JSPrecompApproxLog(pPrecompVect1, pPrecompVect2, dim);
            T val3 = JSPrecompSIMDApproxLog(pPrecompVect1, pPrecompVect2, dim);

            T AbsDiff2 = fabs(val2 - val3);
            T RelDiff2 = AbsDiff2/max(max(fabs(val2),fabs(val3)),T(1e-18));

            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug JS (2) " << typeid(T).name() << " !!! Dim = " << dim << " val2 = " << val2 << " val3 = " << val3 << " Diff: " << (val2 - val3) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2 << endl;
                bug = true;
            }

            T AbsDiff3 = fabs(val1 - val2);
            T RelDiff3 = AbsDiff3/max(max(fabs(val1),fabs(val2)),T(1e-18));

            Dist += val1;
            Error += AbsDiff3;
            ++TotalQty;

            if (RelDiff3 > 1e-4 && AbsDiff3 > 1e-4) {
                cerr << "Bug JS (3) " << typeid(T).name() << " !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << " Diff: " << (val1 - val2) << " RelDiff3: " << RelDiff3 << " AbsDiff2: " << AbsDiff3 << endl;
                bug = true;
            }

            if (bug) return false;
        }
    }

    LOG(LIB_INFO) << typeid(T).name() << " JS approximation error: average absolute: " << Error / TotalQty << 
                                " avg. dist: " << Dist / TotalQty << " average relative: " << Error/Dist;


    return true;
}

bool TestSpearmanFootruleAgree(size_t N, size_t dim, size_t Rep) {
    vector<PivotIdType> vect1(dim), vect2(dim);
    PivotIdType* pVect1 = &vect1[0]; 
    PivotIdType* pVect2 = &vect2[0];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandIntVect(pVect1, dim);
            GenRandIntVect(pVect2, dim);

            int val0 = SpearmanFootrule(pVect1, pVect2, dim);
            int val1 = SpearmanFootruleSIMD(pVect1, pVect2, dim);

            bool bug = false;


            if (val0 != val1) {
                cerr << "Bug SpearmanFootrule  !!! Dim = " << dim << " val0 = " << val0 << " val1 = " << val1  << endl;
                bug = true;
            }

            if (bug) return false;
        }
    }


    return true;
}

bool TestSpearmanRhoAgree(size_t N, size_t dim, size_t Rep) {
    vector<PivotIdType> vect1(dim), vect2(dim);
    PivotIdType* pVect1 = &vect1[0]; 
    PivotIdType* pVect2 = &vect2[0];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandIntVect(pVect1, dim);
            GenRandIntVect(pVect2, dim);

            int val0 = SpearmanRho(pVect1, pVect2, dim);
            int val1 = SpearmanRhoSIMD(pVect1, pVect2, dim);

            bool bug = false;


            if (val0 != val1) {
                cerr << "Bug SpearmanRho !!! Dim = " << dim << " val0 = " << val0 << " val1 = " << val1 << " Diff: " << (val0 - val1) << endl;
                bug = true;
            }

            if (bug) return false;
        }
    }


    return true;
}

template <class T>
bool TestLPGenericAgree(size_t N, size_t dim, size_t Rep, T power) {
    vector<T> vect1(dim), vect2(dim);
    T* pVect1 = &vect1[0]; 
    T* pVect2 = &vect2[0];

    T  TotalQty = 0, Error = 0, Dist = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, -T(RANGE), T(RANGE));
            GenRandVect(pVect2, dim, -T(RANGE), T(RANGE));

            T val0 = LPGenericDistance(pVect1, pVect2, dim, power);
            T val1 = LPGenericDistanceOptim(pVect1, pVect2, dim, power);

            bool bug = false;

            T AbsDiff1 = fabs(val1 - val0);
            T RelDiff1 = AbsDiff1/max(max(fabs(val1),fabs(val0)),T(1e-18));

            T maxRelDiff = 1e-5f;
            T maxAbsDiff = 1e-5f;
            /* 
             * For large powers, the difference can be larger,
             * because our approximations are efficient, but not very
             * precise
             */
            if (power > 8) { maxAbsDiff = maxRelDiff = 1e-3f;}
            if (power > 12) { maxAbsDiff = maxRelDiff = 0.01f;}
            if (power > 22) { maxAbsDiff = maxRelDiff = 0.1f;}

            ++TotalQty;
            Error += RelDiff1;
            Dist += val0;

            if (RelDiff1 > maxRelDiff && AbsDiff1 > maxAbsDiff) {
                cerr << "Bug LP" << power << " !!! Dim = " << dim << 
                " val1 = " << val1 << " val0 = " << val0 << 
                " Diff: " << (val1 - val0) << 
                " RelDiff1: " << RelDiff1 << 
                " (max for this power: " << maxRelDiff << ")  " <<
                " AbsDiff1: " << AbsDiff1 << " (max for this power: " << maxAbsDiff << ")" << endl;
            }

            if (bug) return false;
        }
    }

    if (power < 4) {
      LOG(LIB_INFO) << typeid(T).name() << " LP approximation error: average absolute " << Error / TotalQty << " avg. dist: " << Dist / TotalQty << " average relative: " << Error/Dist;

    }

    return true;
}

bool TestBitHammingAgree(size_t N, size_t dim, size_t Rep) {
    size_t WordQty = (dim + 31)/32; 
    vector<uint32_t> arr(N * WordQty);
    uint32_t*        pArr = &arr[0];

    uint32_t *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= WordQty) {
        vector<PivotIdType> perm(dim);
        GenRandIntVect(&perm[0], dim);
        for (unsigned j = 0; j < dim; ++j)
          perm[j] = perm[j] % 2;
        vector<uint32_t> h;
        Binarize(perm, 1, h); 
        CHECK(h.size() == WordQty);
        memcpy(p, &h[0], WordQty * sizeof(h[0]));
    }

    WallClockTimer  t;

    t.reset();

    bool res = true;

    for (size_t j = 1; j < N; ++j) {
        uint32_t* pVect1 = pArr + j*WordQty;
        uint32_t* pVect2 = pArr + (j-1)*WordQty;
        int d1 =  BitHamming(pVect1, pVect2, WordQty);
        int d2 = 0;

        for (unsigned t = 0; t < WordQty; ++t) {
          for (unsigned k = 0; k < 32; ++k) {
            d2 += ((pVect1[t]>>k)&1) != ((pVect2[t]>>k)&1);
          }
        }
        if (d1 != d2) {
          cerr << "Bug bit hamming, WordQty = " << WordQty << " d1 = " << d1 << " d2 = " << d2 << endl;
          res = false;
          break;
        }
    }

    return res;
}


bool TestSparseAngularDistanceAgree(const string& dataFile, size_t N, size_t Rep) {
    typedef float T;

    unique_ptr<SpaceSparseAngularDistanceFast>     spaceFast(new SpaceSparseAngularDistanceFast());
    unique_ptr<SpaceSparseAngularDistance<float>>  spaceReg(new SpaceSparseAngularDistance<T>());

    ObjectVector                                 elemsFast;
    ObjectVector                                 elemsReg;
    vector<string>                               tmp;

    unique_ptr<DataFileInputState> inpStateFast(spaceFast->ReadDataset(elemsFast, tmp, dataFile, N));
    spaceFast->UpdateParamsFromFile(*inpStateFast);
    unique_ptr<DataFileInputState> inpStateReg(spaceReg->ReadDataset(elemsReg, tmp, dataFile, N));
    spaceReg->UpdateParamsFromFile(*inpStateReg);

    CHECK(elemsFast.size() == elemsReg.size());

    N = min(N, elemsReg.size());

    bool bug = false;

    float maxRelDiff = 2e-5f;
    float maxAbsDiff = 1e-6f;

    for (size_t j = Rep; j < N; ++j)
        for (size_t k = j - Rep; k < j; ++k) {
        float val1 = spaceFast->IndexTimeDistance(elemsFast[k], elemsFast[j]);
        float val2 = spaceReg->IndexTimeDistance(elemsReg[k], elemsReg[j]);

        float AbsDiff1 = fabs(val1 - val2);
        float RelDiff1 = AbsDiff1 / max(max(fabs(val1), fabs(val2)), T(1e-18));

        if (RelDiff1 > maxRelDiff && AbsDiff1 > maxAbsDiff) {
            cerr << "Bug fast vs non-fast angular dist " <<
                " val1 = " << val1 << " val2 = " << val2 <<
                " Diff: " << (val1 - val2) <<
                " RelDiff1: " << RelDiff1 <<
                " AbsDiff1: " << AbsDiff1 << endl;
            bug = true;
        }

        if (bug) return false;
        }

    return true;
}



bool TestSparseCosineSimilarityAgree(const string& dataFile, size_t N, size_t Rep) {
    typedef float T;

    unique_ptr<SpaceSparseCosineSimilarityFast>     spaceFast(new SpaceSparseCosineSimilarityFast());
    unique_ptr<SpaceSparseCosineSimilarity<float>>  spaceReg (new SpaceSparseCosineSimilarity<T>());

    ObjectVector                                 elemsFast;
    ObjectVector                                 elemsReg;
    vector<string>                               tmp;

    unique_ptr<DataFileInputState> inpStateFast(spaceFast->ReadDataset(elemsFast, tmp, dataFile,  N)); 
    spaceFast->UpdateParamsFromFile(*inpStateFast);
    unique_ptr<DataFileInputState> inpStateReg(spaceReg->ReadDataset(elemsReg, tmp, dataFile,  N)); 
    spaceReg->UpdateParamsFromFile(*inpStateReg);

    CHECK(elemsFast.size() == elemsReg.size());

    N = min(N, elemsReg.size());

    bool bug = false;

    float maxRelDiff = 1e-6f;
    float maxAbsDiff = 1e-6f;

    for (size_t j = Rep; j < N; ++j) 
    for (size_t k = j - Rep; k < j; ++k) {
        float val1 = spaceFast->IndexTimeDistance(elemsFast[k], elemsFast[j]);
        float val2 = spaceReg->IndexTimeDistance(elemsReg[k], elemsReg[j]);

        float AbsDiff1 = fabs(val1 - val2);
        float RelDiff1 = AbsDiff1/max(max(fabs(val1),fabs(val2)),T(1e-18));

        if (RelDiff1 > maxRelDiff && AbsDiff1 > maxAbsDiff) {
            cerr << "Bug fast vs non-fast cosine " << 
            " val1 = " << val1 << " val2 = " << val2 << 
            " Diff: " << (val1 - val2) << 
            " RelDiff1: " << RelDiff1 << 
            " AbsDiff1: " << AbsDiff1 << endl;
            bug = true;
        }

        if (bug) return false;
    }

    return true;
}

bool TestSparseNegativeScalarProductAgree(const string& dataFile, size_t N, size_t Rep) {
    typedef float T;

    unique_ptr<SpaceSparseNegativeScalarProductFast>     spaceFast(new SpaceSparseNegativeScalarProductFast());
    unique_ptr<SpaceSparseNegativeScalarProduct<T>>      spaceReg (new SpaceSparseNegativeScalarProduct<T>());

    ObjectVector                                 elemsFast;
    ObjectVector                                 elemsReg;
    vector<string>                               tmp;

    unique_ptr<DataFileInputState> inpStateFast(spaceFast->ReadDataset(elemsFast, tmp, dataFile,  N)); 
    spaceFast->UpdateParamsFromFile(*inpStateFast);
    unique_ptr<DataFileInputState> inpStateReg(spaceReg->ReadDataset(elemsReg, tmp, dataFile,  N)); 
    spaceReg->UpdateParamsFromFile(*inpStateReg);

    CHECK(elemsFast.size() == elemsReg.size());

    N = min(N, elemsReg.size());

    bool bug = false;

    float maxRelDiff = 1e-6f;
    float maxAbsDiff = 1e-6f;

    for (size_t j = Rep; j < N; ++j) 
    for (size_t k = j - Rep; k < j; ++k) {
        float val1 = spaceFast->IndexTimeDistance(elemsFast[k], elemsFast[j]);
        float val2 = spaceReg->IndexTimeDistance(elemsReg[k], elemsReg[j]);

        float AbsDiff1 = fabs(val1 - val2);
        float RelDiff1 = AbsDiff1/max(max(fabs(val1),fabs(val2)),T(1e-18));

        if (RelDiff1 > maxRelDiff && AbsDiff1 > maxAbsDiff) {
            cerr << "Bug fast vs non-fast negative scalar/dot product " <<
            " val1 = " << val1 << " val2 = " << val2 << 
            " Diff: " << (val1 - val2) << 
            " RelDiff1: " << RelDiff1 << 
            " AbsDiff1: " << AbsDiff1 << endl;
            bug = true;
        }

        if (bug) return false;
    }

    return true;
}

bool TestSparseQueryNormNegativeScalarProductAgree(const string& dataFile, size_t N, size_t Rep) {
    typedef float T;

    unique_ptr<SpaceSparseQueryNormNegativeScalarProductFast>     spaceFast(new SpaceSparseQueryNormNegativeScalarProductFast());
    unique_ptr<SpaceSparseQueryNormNegativeScalarProduct<T>>      spaceReg (new SpaceSparseQueryNormNegativeScalarProduct<T>());

    ObjectVector                                 elemsFast;
    ObjectVector                                 elemsReg;
    vector<string>                               tmp;

    unique_ptr<DataFileInputState> inpStateFast(spaceFast->ReadDataset(elemsFast, tmp, dataFile,  N));
    spaceFast->UpdateParamsFromFile(*inpStateFast);
    unique_ptr<DataFileInputState> inpStateReg(spaceReg->ReadDataset(elemsReg, tmp, dataFile,  N));
    spaceReg->UpdateParamsFromFile(*inpStateReg);

    CHECK(elemsFast.size() == elemsReg.size());

    N = min(N, elemsReg.size());

    bool bug = false;

    float maxRelDiff = 1e-6f;
    float maxAbsDiff = 1e-6f;

    for (size_t j = Rep; j < N; ++j)
        for (size_t k = j - Rep; k < j; ++k) {
            float val1 = spaceFast->IndexTimeDistance(elemsFast[k], elemsFast[j]);
            float val2 = spaceReg->IndexTimeDistance(elemsReg[k], elemsReg[j]);

            float AbsDiff1 = fabs(val1 - val2);
            float RelDiff1 = AbsDiff1/max(max(fabs(val1),fabs(val2)),T(1e-18));

            if (RelDiff1 > maxRelDiff && AbsDiff1 > maxAbsDiff) {
                cerr << "Bug fast vs non-fast QUERY-NORMALIZED negative scalar/dot product " <<
                " val1 = " << val1 << " val2 = " << val2 <<
                " Diff: " << (val1 - val2) <<
                " RelDiff1: " << RelDiff1 <<
                " AbsDiff1: " << AbsDiff1 << endl;
                bug = true;
            }

            if (bug) return false;
        }

    return true;
}


#ifdef DISABLE_LONG_TESTS
TEST(DISABLE_TestAgree) {
#else
TEST(TestAgree) {
#endif
    int nTest  = 0;
    int nFail = 0;

    nTest++;
    nFail += !TestSparseAngularDistanceAgree(sampleDataPrefix + "sparse_5K.txt", 1000, 200);

    nTest++;
    nFail += !TestSparseAngularDistanceAgree(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 200);

    nTest++;
    nFail += !TestSparseCosineSimilarityAgree(sampleDataPrefix + "sparse_5K.txt", 1000, 200);

    nTest++;
    nFail += !TestSparseCosineSimilarityAgree(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 200);


    nTest++;
    nFail += !TestSparseNegativeScalarProductAgree(sampleDataPrefix + "sparse_5K.txt", 1000, 200);

    nTest++;
    nFail += !TestSparseNegativeScalarProductAgree(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 200);

    nTest++;
    nFail += !TestSparseQueryNormNegativeScalarProductAgree(sampleDataPrefix + "sparse_5K.txt", 1000, 200);

    nTest++;
    nFail += !TestSparseQueryNormNegativeScalarProductAgree(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 200);


  /*
   * 32 should be more than enough for almost all methods,
   * where loop-unrolling  includes at most 16 distance computations.
   *
   * Bit-Hamming is an exception.
   *
   */
    for (unsigned dim = 1; dim <= 1024; dim+=2) {
        LOG(LIB_INFO) << "Dim = " << dim;

        nFail += !TestBitHammingAgree(1000, dim, 1000);
    }

    for (unsigned dim = 1; dim <= 32; ++dim) {
        LOG(LIB_INFO) << "Dim = " << dim;

        /* 
         * This is a costly check, we don't need to do it for large # dimensions.
         * Anyways, the function is not using any loop unrolling, so 8 should be sufficient.
         */
        if (dim <= 8) {

          for (float power = 0.125; power <= 32; power += 0.125) {
            TestLPGenericAgree(1024, dim, 10, power);
          }

          for (double power = 0.125; power <= 32; power += 0.125) {
            TestLPGenericAgree(1024, dim, 10, power);
          }
        }

        nTest++;
        nFail += !TestNormScalarProductAgree<float>(1024, dim, 10);
        nTest++;
        nFail += !TestNormScalarProductAgree<double>(1024, dim, 10);

        nTest++;
        nFail += !TestScalarProductAgree<float>(1024, dim, 10);
        nTest++;
        nFail += !TestScalarProductAgree<double>(1024, dim, 10);

        nTest++;
        nFail += !TestSpearmanFootruleAgree(1024, dim, 10);

        nTest++;
        nFail += !TestSpearmanRhoAgree(1024, dim, 10);

        nTest++;
        nFail += !TestJSAgree<float>(1024, dim, 10, 0.5);
        nTest++;
        nFail += !TestJSAgree<double>(1024, dim, 10, 0.5);

        nTest++;
        nFail += !TestKLGeneralAgree<float>(1024, dim, 10);
        nTest++;
        nFail += !TestKLGeneralAgree<double>(1024, dim, 10);

        nTest++;
        nFail += !TestLInfAgree<float>(1024, dim, 10);
        nTest++;
        nFail += !TestLInfAgree<double>(1024, dim, 10);

        nTest++;
        nFail += !TestL1Agree<float>(1024, dim, 10);
        nTest++;
        nFail += !TestL1Agree<double>(1024, dim, 10);

        nTest++;
        nFail += !TestL2Agree<float>(1024, dim, 10);
        nTest++;
        nFail += !TestL2Agree<double>(1024, dim, 10);

        nTest++;
        nFail += !TestKLAgree<float>(1024, dim, 10);
        nTest++;
        nFail += !TestKLAgree<double>(1024, dim, 10);

        nTest++;
        nFail += !TestItakuraSaitoAgree<float>(1024, dim, 10);
        nTest++;
        nFail += !TestItakuraSaitoAgree<double>(1024, dim, 10);
    }

    LOG(LIB_INFO) << nTest << " (sub) tests performed " << nFail << " failed";

    EXPECT_EQ(0, nFail);
}

}  // namespace similarity

