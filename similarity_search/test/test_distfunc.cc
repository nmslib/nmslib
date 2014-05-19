/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan, Leonid Boytsov.
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

#include "space.h"
#include "space/space_sparse_lp.h"
#include "space/space_sparse_scalar.h"
#include "space/space_sparse_vector_inter.h"
#include "space/space_sparse_scalar_fast.h"
#include "space/space_sparse_vector.h"
#include "space/space_scalar.h"
#include "common.h"
#include "bunit.h"
#include "distcomp.h"
#include "permutation_utils.h"
#include "ztimer.h"
#include "pow.h"

#define TEST_SPEED_DOUBLE 1

#define TEST_AGREE     1
#define TEST_SPEED_ LP 0
#define RANGE          8.0f
#define RANGE_SMALL    1e-6f

namespace similarity {

using std::unique_ptr;
using std::vector;

const string sampleDataPrefix = string("..") + PATH_SEPARATOR + string("sample_data") + PATH_SEPARATOR;


template <class T> 
inline void Normalize(T* pVect, size_t qty) {
  T sum = 0;
  for (size_t i = 0; i < qty; ++i) {
    sum += pVect[i];
  }
  if (sum != 0) {
    for (size_t i = 0; i < qty; ++i) {
      pVect[i] /= sum;
    }
  }
}


template <class T> 
inline void GenRandVect(T* pVect, size_t qty, T MinElem = T(0), T MaxElem = T(1), bool DoNormalize = false) {
  T sum = 0;
  for (size_t i = 0; i < qty; ++i) {
    pVect[i] = MinElem + (MaxElem - MinElem) * RandomReal<T>();
    sum += fabs(pVect[i]);
  }
  if (DoNormalize && sum != 0) {
    for (size_t i = 0; i < qty; ++i) {
      pVect[i] /= sum;
    }
  }
}

inline void GenRandIntVect(int* pVect, size_t qty) {
  for (size_t i = 0; i < qty; ++i) {
    pVect[i] = RandomInt();
  }
}

template <class T> 
inline void SetRandZeros(T* pVect, size_t qty, double pZero) {
    for (size_t j = 0; j < qty; ++j) if (RandomReal<T>() < pZero) pVect[j] = T(0);
}

using namespace std;


/*

#ifdef  __INTEL_COMPILER

// TODO: @Leo figure out how to use this function with Intel and cmake

#include "mkl.h"

TEST(set_intel) {
    vmlSetMode(VML_HA);
    LOG(INFO) << "Set high-accuracy mode.";
}

#endif

*/


TEST(Platform64) {
  EXPECT_EQ(8 == sizeof(size_t), true);
}

template <typename dist_t>
void GenSparseVectZipf(size_t maxSize, vector<SparseVectElem<dist_t>>& res) {
  maxSize = max(maxSize, (size_t)1);

  for (size_t i = 1; i < maxSize; ++i) {
    float f = RandomReal<float>();
    if (f <= sqrt((float)i)/i) { // This is a bit ad hoc, but is ok for testing purposes
      res.push_back(SparseVectElem<dist_t>(i, RandomReal<float>()));
    }
  }
};

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

    LOG(INFO) << "testing maxSize: " << maxSize << "\nqty: " <<  source.size()
              << " maxId: " << source.back().id_;

    char*     pBuff = NULL; 
    size_t    dataLen = 0;

    PackSparseElements(source, pBuff, dataLen);
    
    vector<SparseVectElem<dist_t>> target;
    UnpackSparseElements(pBuff, dataLen, target);

    bool eqFlag = checkElemVectEq(source, target);

    if (!eqFlag) {
      LOG(INFO) << "Different source and target, source.size(): " << source.size()
                << " target.size(): " << target.size();
      // Let's print the first different in the case of equal # of elements
      size_t i = 0;
      for (; i < min(source.size(), target.size()); ++i) {
        if (!(source[i] == target[i])) {
          LOG(INFO) << "First diff, i = " << i << " " << source[i] << " vs " << target[i];
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

TEST(SparsePackUnpack) {
  TestSparsePackUnpack<float>();
  TestSparsePackUnpack<double>();
}

TEST(TestEfficientPower) {
  float f = 2.0;

  for (unsigned i = 1; i <= 64; i++) {
    float p1 = std::pow(f, i);
    float p2 = EfficientPow(f, i);

    EXPECT_EQ(p1, p2);
  }
}

TEST(TestEfficientFract) {
  unsigned MaxNumDig = 16;

  for (float a = 1.1 ; a <= 2; a+= 0.1) {
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

// Agreement test functions
template <class T>
bool TestLInfAgree(size_t N, size_t dim, size_t Rep) {
    T* pVect1 = new T[dim];
    T* pVect2 = new T[dim];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, -T(RANGE), T(RANGE));
            GenRandVect(pVect2, dim, -T(RANGE), T(RANGE));

            T val1 = LInfNormStandard(pVect1, pVect2, dim);
            T val2 = LInfNorm(pVect1, pVect2, dim);
            T val3 = LInfNormSIMD(pVect1, pVect2, dim);

            bool bug = false;

            if (fabs(val1 - val2)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug LInf !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2;
            }
            if (fabs(val1 - val3)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug LInf !!! Dim = " << dim << " val1 = " << val1 << " val3 = " << val3;
            }
            if (bug) return false;
        }
    }


    delete [] pVect1;
    delete [] pVect2;

    return true;
}

template <class T>
bool TestL1Agree(size_t N, size_t dim, size_t Rep) {
    T* pVect1 = new T[dim];
    T* pVect2 = new T[dim];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, -T(RANGE), T(RANGE));
            GenRandVect(pVect2, dim, -T(RANGE), T(RANGE));

            T val1 = L1NormStandard(pVect1, pVect2, dim);
            T val2 = L1Norm(pVect1, pVect2, dim);
            T val3 = L1NormSIMD(pVect1, pVect2, dim);

            bool bug = false;

            if (fabs(val1 - val2)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L1 !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2;
            }
            if (fabs(val1 - val3)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L1 !!! Dim = " << dim << " val1 = " << val1 << " val3 = " << val3;
            }
            if (bug) return false;
        }
    }


    delete [] pVect1;
    delete [] pVect2;

    return true;
}

template <class T>
bool TestL2Agree(size_t N, size_t dim, size_t Rep) {
    T* pVect1 = new T[dim];
    T* pVect2 = new T[dim];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, -T(RANGE), T(RANGE));
            GenRandVect(pVect2, dim, -T(RANGE), T(RANGE));

            T val1 = L2NormStandard(pVect1, pVect2, dim);
            T val2 = L2Norm(pVect1, pVect2, dim);
            T val3 = L2NormSIMD(pVect1, pVect2, dim);

            bool bug = false;

            if (fabs(val1 - val2)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L2 !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2;
            }
            if (fabs(val1 - val3)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L2 !!! Dim = " << dim << " val1 = " << val1 << " val3 = " << val3;
            }
            if (bug) return false;
        }
    }


    delete [] pVect1;
    delete [] pVect2;

    return true;
}

template <class T>
bool TestItakuraSaitoAgree(size_t N, size_t dim, size_t Rep) {
    T* pVect1 = new T[dim];
    T* pVect2 = new T[dim];
    T* pPrecompVect1 = new T[dim * 2];
    T* pPrecompVect2 = new T[dim * 2];

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
                cerr << "Bug ItakuraSaito !!! Dim = " << dim << " val1 = " << val1 << " val0 = " << val0 << " Diff: " << (val1 - val0) << " RelDiff1: " << RelDiff1 << " << AbsDiff1: " << AbsDiff1;
            }

            T AbsDiff2 = fabs(val1 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val1),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug ItakuraSaito !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << " Diff: " << (val1 - val2) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2;
            }

            if (bug) return false;
        }
    }


    delete [] pVect1;
    delete [] pVect2;
    delete [] pPrecompVect1;
    delete [] pPrecompVect2;

    return true;
}

template <class T>
bool TestKLAgree(size_t N, size_t dim, size_t Rep) {
    T* pVect1 = new T[dim];
    T* pVect2 = new T[dim];
    T* pPrecompVect1 = new T[dim * 2];
    T* pPrecompVect2 = new T[dim * 2];

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
                cerr << "Bug KL !!! Dim = " << dim << " val0 = " << val0 << " val1 = " << val1 << " Diff: " << (val0 - val1) << " RelDiff1: " << RelDiff1 << " AbsDiff1: " << AbsDiff1;
            }

            T AbsDiff2 = fabs(val1 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val1),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val2 = " << val2 << " val1 = " << val1 << " Diff: " << (val2 - val1) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2;
            }

            T AbsDiff3 = fabs(val1 - val3);
            T RelDiff3 = AbsDiff3/max(max(fabs(val1),fabs(val3)),T(1e-18));
            if (RelDiff3 > 1e-5 && AbsDiff3 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val3 = " << val3 << " val1 = " << val1 << " Diff: " << (val3 - val1) << " RelDiff3: " << RelDiff3 << " AbsDiff3: " << AbsDiff3;
            }

            if (bug) return false;
        }
    }


    delete [] pVect1;
    delete [] pVect2;
    delete [] pPrecompVect1;
    delete [] pPrecompVect2;

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
                cerr << "Bug KL !!! Dim = " << dim << " val0 = " << val0 << " val2 = " << val2 << " Diff: " << (val0 - val2) << " RelDiff1: " << RelDiff1 << " AbsDiff1: " << AbsDiff1;
            }

            T AbsDiff2 = fabs(val3 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val3),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val2 = " << val2 << " val3 = " << val3 << " Diff: " << (val2 - val3) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2;
            }

            if (bug) return false;
        }
    }


    delete [] pVect1;
    delete [] pVect2;
    delete [] pPrecompVect1;
    delete [] pPrecompVect2;

    return true;
}

template <class T>
bool TestJSAgree(size_t N, size_t dim, size_t Rep, double pZero) {
    T* pVect1 = new T[dim];
    T* pVect2 = new T[dim];
    T* pPrecompVect1 = new T[dim * 2];
    T* pPrecompVect2 = new T[dim * 2];

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
                cerr << "Bug JS (1) " << typeid(T).name() << " !!! Dim = " << dim << " val0 = " << val0 << " val1 = " << val1 << " Diff: " << (val0 - val1) << " RelDiff1: " << RelDiff1 << " AbsDiff1: " << AbsDiff1;
                bug = true;
            }

            T val2 = JSPrecompApproxLog(pPrecompVect1, pPrecompVect2, dim);
            T val3 = JSPrecompSIMDApproxLog(pPrecompVect1, pPrecompVect2, dim);

            T AbsDiff2 = fabs(val2 - val3);
            T RelDiff2 = AbsDiff2/max(max(fabs(val2),fabs(val3)),T(1e-18));

            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug JS (2) " << typeid(T).name() << " !!! Dim = " << dim << " val2 = " << val2 << " val3 = " << val3 << " Diff: " << (val2 - val3) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2;
                bug = true;
            }

            T AbsDiff3 = fabs(val1 - val2);
            T RelDiff3 = AbsDiff3/max(max(fabs(val1),fabs(val2)),T(1e-18));

            Dist += val1;
            Error += AbsDiff3;
            ++TotalQty;

            if (RelDiff3 > 1e-4 && AbsDiff3 > 1e-4) {
                cerr << "Bug JS (3) " << typeid(T).name() << " !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << " Diff: " << (val1 - val2) << " RelDiff3: " << RelDiff3 << " AbsDiff2: " << AbsDiff3;
                bug = true;
            }

            if (bug) return false;
        }
    }

    LOG(INFO) << typeid(T).name() << " JS approximation error: average absolute: " << Error / TotalQty << 
                                " avg. dist: " << Dist / TotalQty << " average relative: " << Error/Dist;


    delete [] pVect1;
    delete [] pVect2;
    delete [] pPrecompVect1;
    delete [] pPrecompVect2;

    return true;
}

bool TestSpearmanFootruleAgree(size_t N, size_t dim, size_t Rep) {
    int* pVect1 = new int[dim];
    int* pVect2 = new int[dim];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandIntVect(pVect1, dim);
            GenRandIntVect(pVect2, dim);

            int val0 = SpearmanFootrule(pVect1, pVect2, dim);
            int val1 = SpearmanFootruleSIMD(pVect1, pVect2, dim);

            bool bug = false;


            if (val0 != val1) {
                cerr << "Bug SpearmanFootrule  !!! Dim = " << dim << " val0 = " << val0 << " val1 = " << val1 ;
                bug = true;
            }

            if (bug) return false;
        }
    }


    delete [] pVect1;
    delete [] pVect2;

    return true;
}

bool TestSpearmanRhoAgree(size_t N, size_t dim, size_t Rep) {
    int* pVect1 = new int[dim];
    int* pVect2 = new int[dim];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandIntVect(pVect1, dim);
            GenRandIntVect(pVect2, dim);

            int val0 = SpearmanRho(pVect1, pVect2, dim);
            int val1 = SpearmanRhoSIMD(pVect1, pVect2, dim);

            bool bug = false;


            if (val0 != val1) {
                cerr << "Bug SpearmanRho !!! Dim = " << dim << " val0 = " << val0 << " val1 = " << val1 << " Diff: " << (val0 - val1);
                bug = true;
            }

            if (bug) return false;
        }
    }


    delete [] pVect1;
    delete [] pVect2;

    return true;
}

template <class T>
bool TestLPGenericAgree(size_t N, size_t dim, size_t Rep, T power) {
    T* pVect1 = new T[dim];
    T* pVect2 = new T[dim];

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

            T maxRelDiff = 1e-5;
            T maxAbsDiff = 1e-5;
            /* 
             * For large powers, the difference can be larger,
             * because our approximations are efficient, but not very
             * precise
             */
            if (power > 8) { maxAbsDiff = maxRelDiff = 1e-3;}
            if (power > 12) { maxAbsDiff = maxRelDiff = 0.01;}
            if (power > 22) { maxAbsDiff = maxRelDiff = 0.1;}

            ++TotalQty;
            Error += RelDiff1;
            Dist += val0;

            if (RelDiff1 > maxRelDiff && AbsDiff1 > maxAbsDiff) {
                cerr << "Bug LP" << power << " !!! Dim = " << dim << 
                " val1 = " << val1 << " val0 = " << val0 << 
                " Diff: " << (val1 - val0) << 
                " RelDiff1: " << RelDiff1 << 
                " (max for this power: " << maxRelDiff << ")  " <<
                " AbsDiff1: " << AbsDiff1 << " (max for this power: " << maxAbsDiff << ")";
            }

            if (bug) return false;
        }
    }

    if (power < 4) {
      LOG(INFO) << typeid(T).name() << " LP approximation error: average absolute " << Error / TotalQty << " avg. dist: " << Dist / TotalQty << " average relative: " << Error/Dist;

    }

    delete [] pVect1;
    delete [] pVect2;

    return true;
}

bool TestBitHammingAgree(size_t N, size_t dim, size_t Rep) {
    size_t WordQty = (dim + 31)/32; 
    uint32_t* pArr = new uint32_t[N * WordQty];

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
          cerr << "Bug bit hamming, WordQty = " << WordQty << " d1 = " << d1 << " d2 = " << d2;
          res = false;
          break;
        }
    }

    delete [] pArr;

    return res;
}

bool TestSparseCosineSimilarityAgree(const string& dataFile, size_t N, size_t Rep) {
    typedef float T;

    unique_ptr<SpaceSparseCosineSimilarityFast>     spaceFast(new SpaceSparseCosineSimilarityFast());
    unique_ptr<SpaceSparseCosineSimilarity<float>>  spaceReg (new SpaceSparseCosineSimilarity<T>());

    ObjectVector                                 elemsFast;
    ObjectVector                                 elemsReg;

    spaceFast->ReadDataset(elemsFast, NULL, dataFile.c_str(),  N); 
    spaceReg->ReadDataset(elemsReg, NULL, dataFile.c_str(),  N); 

    CHECK(elemsFast.size() == elemsReg.size());

    N = min(N, elemsReg.size());

    bool bug = false;

    float maxRelDiff = 1e-6;
    float maxAbsDiff = 1e-6;

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
            " AbsDiff1: " << AbsDiff1;
            bug = true;
        }

        if (bug) return false;
    }

    return true;
}

#if TEST_AGREE
TEST(TestAgree) {
    int nTest  = 0;
    int nFail = 0;

    nTest++;
    nFail += !TestSparseCosineSimilarityAgree(sampleDataPrefix + "sparse_5K.txt", 1000, 200);

    nTest++;
    nFail += !TestSparseCosineSimilarityAgree(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 200);

    /* 
     * 32 should be more than enough for almost all methods,
     * where loop-unrolling  includes at most 16 distance computations.
     *
     * Bit-Hamming is an exception.
     * 
     */
    for (unsigned dim = 1; dim <= 1024; dim+=2) {
        LOG(INFO) << "Dim = " << dim;

        nFail += !TestBitHammingAgree(1000, dim, 1000);
    }

    for (unsigned dim = 1; dim <= 32; ++dim) {
        LOG(INFO) << "Dim = " << dim;

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

    LOG(INFO) << nTest << " (sub) tests performed " << nFail << " failed";

    EXPECT_EQ(0, nFail);
}
#endif

// Efficiency test functions

template <class T>
bool TestLInfNormStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;
    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * LInfNormStandard(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard LInfs per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestLInfNorm(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;
    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * LInfNorm(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of optim. LInfs per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestLInfNormSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];
    T* p = pArr;

    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;
    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * LInfNormSIMD(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD LInfs per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}


template <class T>
bool TestL1NormStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;
    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * L1NormStandard(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard L1s per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL1Norm(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;
    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * L1Norm(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of optim. L1s per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL1NormSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];
    T* p = pArr;

    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;
    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * L1NormSIMD(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD L1s per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL2NormStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;
    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * L2NormStandard(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard L2s per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL2Norm(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;
    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * L2Norm(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of optim. L2s per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL2NormSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;
 
    T fract = T(1)/N;
    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * L2NormSIMD(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }
 
    uint64_t tDiff = t.split();
 
    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD L2s per second: " << (1e6/tDiff) * N * Rep ;
 
    delete [] pArr;
 
    return true;
}


template <class T>
bool TestLPGeneric(size_t N, size_t dim, size_t Rep, T power) {
    T* pArr = new T[N * dim];
    T* p = pArr;

    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, T(-RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * LPGenericDistance(pArr + j*dim, pArr + (j-1)*dim, dim, power) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of Generic L" << power << " per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestLPGenericOptim(size_t N, size_t dim, size_t Rep, T power) {
    T* pArr = new T[N * dim];
    T* p = pArr;

    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, T(-RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * LPGenericDistanceOptim(pArr + j*dim, pArr + (j-1)*dim, dim, power) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of Optimized generic L" << power << " per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestItakuraSaitoPrecomp(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim, T(RANGE_SMALL), T(1.0));
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * ItakuraSaitoPrecomp(pArr + j*dim*2, pArr + (j-1)*dim*2, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of precomp. ItakuraSaito per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestItakuraSaitoPrecompSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim, T(RANGE_SMALL), T(1.0));
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * ItakuraSaitoPrecompSIMD(pArr + j*dim*2, pArr + (j-1)*dim*2, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD precomp. ItakuraSaito per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestItakuraSaitoStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, T(RANGE_SMALL), T(1.0));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * ItakuraSaito(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of ItakuraSaito per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}


template <class T>
bool TestKLPrecomp(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim, T(RANGE_SMALL), T(1.0), true /* norm. for regular KL */);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * KLPrecomp(pArr + j*dim*2, pArr + (j-1)*dim*2, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of precomp. KLs per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestKLPrecompSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim, T(RANGE_SMALL), T(1.0), true /* norm. for regular KL */);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * KLPrecompSIMD(pArr + j*dim*2, pArr + (j-1)*dim*2, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD precomp. KLs per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestKLStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, T(RANGE_SMALL), T(1.0), true /* norm. for regular KL */);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * KLStandard(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of KLs per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}


template <class T>
bool TestKLGeneralPrecomp(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim, T(RANGE_SMALL), T(1.0));
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * KLGeneralPrecomp(pArr + j*dim*2, pArr + (j-1)*dim*2, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of precomp. general. KLs per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestKLGeneralPrecompSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim, T(RANGE_SMALL), T(1.0));
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * KLGeneralPrecompSIMD(pArr + j*dim*2, pArr + (j-1)*dim*2, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD precomp. general. KLs per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestKLGeneralStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, T(RANGE_SMALL), T(1.0));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * KLGeneralStandard(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of general. KLs per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestJSStandard(size_t N, size_t dim, size_t Rep, float pZero) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, T(0), T(1), true);
        SetRandZeros(p, dim, pZero);
        Normalize(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * JSStandard(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of JSs (sparsity:" << pZero << ") per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestJSPrecomp(size_t N, size_t dim, size_t Rep, float pZero) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim, T(0), T(1), true);
        SetRandZeros(p, dim, pZero);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * JSPrecomp(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of JSs (precomp) (sparsity:" << pZero << ")  per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestJSPrecompApproxLog(size_t N, size_t dim, size_t Rep, float pZero) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim, T(0), T(1), true);
        SetRandZeros(p, dim, pZero);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * JSPrecompApproxLog(pArr + 2*j*dim, pArr + 2*(j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of JSs (precomp, one log approx) (sparsity:" << pZero << ") per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestJSPrecompSIMDApproxLog(size_t N, size_t dim, size_t Rep, float pZero) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim, T(0), T(1), true);
        SetRandZeros(p, dim, pZero);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * JSPrecompSIMDApproxLog(pArr + 2*j*dim, pArr + 2*(j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of JSs (precomp, one log approx, SIMD) (sparsity:" << pZero << ") per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

bool TestSpearmanRho(size_t N, size_t dim, size_t Rep) {
    int* pArr = new int[N * dim];

    int *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandIntVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    float DiffSum = 0;

    float fract = 1.0/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * SpearmanRho(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard SpearmanRho per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

bool TestSpearmanRhoSIMD(size_t N, size_t dim, size_t Rep) {
    int* pArr = new int[N * dim];

    int *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandIntVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    float DiffSum = 0;

    float fract = 1.0/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * SpearmanRhoSIMD(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SpearmanRhoSIMD per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

bool TestSpearmanFootrule(size_t N, size_t dim, size_t Rep) {
    int* pArr = new int[N * dim];

    int *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandIntVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    float DiffSum = 0;

    float fract = 1.0/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * SpearmanFootrule(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard SpearmanFootrule per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

bool TestSpearmanFootruleSIMD(size_t N, size_t dim, size_t Rep) {
    int* pArr = new int[N * dim];

    int *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandIntVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    float DiffSum = 0;

    float fract = 1.0/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * SpearmanFootruleSIMD(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SpearmanFootruleSIMD per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestSparseLp(size_t N, size_t Rep, T power) {
    unique_ptr<SpaceSparseLp<T>>  space(new SpaceSparseLp<T>(power));
    ObjectVector                  elems;

    space->ReadDataset(elems, NULL, (sampleDataPrefix + "sparse_5K.txt").c_str(), N); 

    N = min(N, elems.size());

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * space->IndexTimeDistance(elems[j-1], elems[j]) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << 
            " # of sparse LP (p=" << power << ") per second: " << (1e6/tDiff) * N * Rep ;

    return true;
}

template <class T>
bool TestSparseAngularDistance(const string& dataFile, size_t N, size_t Rep) {
    unique_ptr<SpaceSparseAngularDistance<T>>  space(new SpaceSparseAngularDistance<T>());
    ObjectVector                  elems;

    space->ReadDataset(elems, NULL, dataFile.c_str(), N); 

    N = min(N, elems.size());

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * space->IndexTimeDistance(elems[j-1], elems[j]) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " File: " << dataFile 
         << " Elapsed: " << tDiff / 1e3 << " ms " << 
            " # of sparse angular dist per second: " << (1e6/tDiff) * N * Rep ;

    return true;
}

bool TestSparseCosineSimilarityFast(const string& dataFile, size_t N, size_t Rep) {
    typedef float T;

    unique_ptr<SpaceSparseCosineSimilarityFast>  space(new SpaceSparseCosineSimilarityFast());
    ObjectVector                                 elems;

    space->ReadDataset(elems, NULL, dataFile.c_str(),  N); 

    N = min(N, elems.size());

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * space->IndexTimeDistance(elems[j-1], elems[j]) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " File: " << dataFile << 
            " Elapsed: " << tDiff / 1e3 << " ms " << 
            " # of (fast) sparse cosine similarity dist second: " << (1e6/tDiff) * N * Rep ;

    return true;
}

template <class T>
bool TestSparseCosineSimilarity(const string& dataFile, size_t N, size_t Rep) {
    unique_ptr<SpaceSparseCosineSimilarity<T>>  space(new SpaceSparseCosineSimilarity<T>());
    ObjectVector                  elems;

    space->ReadDataset(elems, NULL, dataFile.c_str(), N); 

    N = min(N, elems.size());

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * space->IndexTimeDistance(elems[j-1], elems[j]) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " File: " << dataFile 
         << " Elapsed: " << tDiff / 1e3 << " ms " << 
            " # of sparse cosine similarity dist second: " << (1e6/tDiff) * N * Rep ;

    return true;
}

template <class T>
bool TestCosineSimilarity(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;
    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * CosineSimilarity(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard CosineSimilarity per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

template <class T>
bool TestAngularDistance(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, -T(RANGE), T(RANGE));
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;
    T fract = T(1)/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * AngularDistance(pArr + j*dim, pArr + (j-1)*dim, dim) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << typeid(T).name() << " " << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard AngularDistance per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

bool TestBitHamming(size_t N, size_t dim, size_t Rep) {
    size_t WordQty = (dim + 31)/32; 
    uint32_t* pArr = new uint32_t[N * WordQty];

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

    float DiffSum = 0;

    float fract = 1.0/N;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += 0.01 * BitHamming(pArr + j*WordQty, pArr + (j-1)*WordQty, WordQty) / N;
        }
        /* 
         * Multiplying by 0.01 and dividing the sum by N is to prevent Intel from "cheating":
         *
         * http://searchivarius.org/blog/problem-previous-version-intels-library-benchmark
         */
        DiffSum *= fract;
    }

    uint64_t tDiff = t.split();

    LOG(INFO) << "Ignore: " << DiffSum;
    LOG(INFO) << "Elapsed: " << tDiff / 1e3 << " ms " << " # of BitHamming per second: " << (1e6/tDiff) * N * Rep ;

    delete [] pArr;

    return true;
}

TEST(TestSpeed) {
    int nTest  = 0;
    int nFail = 0;

    int dim = 128;

    nTest++;
    nFail += !TestBitHamming(1000, 32, 1000);
    nTest++;
    nFail += !TestBitHamming(1000, 64, 1000);
    nTest++;
    nFail += !TestBitHamming(1000, 128, 1000);
    nTest++;
    nFail += !TestBitHamming(1000, 256, 1000);
    nTest++;
    nFail += !TestBitHamming(1000, 512, 1000);
    nTest++;
    nFail += !TestBitHamming(1000, 1024, 1000);

    double pZero1 = 0.5;
    double pZero2 = 0.25;
    double pZero3 = 0.0;

    nTest++;
    nFail += !TestCosineSimilarity<float>(1000, dim, 1000);
    nTest++;
    nFail += !TestAngularDistance<float>(1000, dim, 1000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestCosineSimilarity<double>(1000, dim, 1000);
    nTest++;
    nFail += !TestAngularDistance<double>(1000, dim, 1000);
#endif

    nTest++;
    nFail += !TestSparseCosineSimilarityFast(sampleDataPrefix + "sparse_5K.txt", 1000, 1000);
    nTest++;
    nFail += !TestSparseCosineSimilarityFast(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 1000);
    nTest++;
    nFail += !TestSparseCosineSimilarityFast(sampleDataPrefix + "sparse_5K.txt", 1000, 1000);
    nTest++;
    nFail += !TestSparseCosineSimilarityFast(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 1000);

    nTest++;
    nFail += !TestSparseCosineSimilarity<float>(sampleDataPrefix + "sparse_5K.txt", 1000, 1000);
    nTest++;
    nFail += !TestSparseCosineSimilarity<float>(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 1000);
    nTest++;
    nFail += !TestSparseAngularDistance<float>(sampleDataPrefix + "sparse_5K.txt", 1000, 1000);
    nTest++;
    nFail += !TestSparseAngularDistance<float>(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 1000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestSparseCosineSimilarity<double>(sampleDataPrefix + "sparse_5K.txt", 1000, 1000);
    nTest++;
    nFail += !TestSparseCosineSimilarity<double>(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 1000);
    nTest++;
    nFail += !TestSparseAngularDistance<double>(sampleDataPrefix + "sparse_5K.txt", 1000, 1000);
    nTest++;
    nFail += !TestSparseAngularDistance<double>(sampleDataPrefix + "sparse_wiki_5K.txt", 1000, 1000);
#endif


    LOG(INFO) << "Single-precision (sparse) LP-distance tests";
    nTest++;
    nFail += !TestSparseLp<float>(1000, 1000, -1);
    nTest++;
    nTest++;
    nFail += !TestSparseLp<float>(1000, 1000, 1/3.0);

#if TEST_SPEED_LP
    float delta = 0.125/2.0;

    for (float power = delta, step = delta; power <= 24; power += step) {
      nTest++;
      // This one should use an optimized LP function
      nFail += !TestSparseLp<float>(1000, 1000, power);
      if (power == 3) step = 0.125;
      if (power == 8) step = 0.5;
    }
    LOG(INFO) << "========================================";

#if TEST_SPEED_DOUBLE
    LOG(INFO) << "Double-precision (sparse) LP-distance tests";
    nFail += !TestSparseLp<double>(1000, 1000, -1);
    nTest++;
    nFail += !TestSparseLp<double>(1000, 1000, 1/3.0);
    nTest++;
    for (double power = delta, step = delta; power <= 24; power += step) {
      nTest++;
      // This one should use an optimized LP function
      nFail += !TestSparseLp<double>(1000, 1000, power);
      if (power == 3) step = 0.125;
      if (power == 8) step = 0.5;
    }
#endif
    LOG(INFO) << "========================================";

    LOG(INFO) << "Single-precision LP-distance tests";
    for (float power = delta, step = delta; power <= 24; power += step) {
      nTest++;
      nFail += !TestLPGeneric<float>(128, dim, 200, power);
      nTest++;
      nFail += !TestLPGenericOptim<float>(128, dim, 200, power);
      if (power == 3) step = 0.125;
      if (power == 8) step = 0.5;
    }
    LOG(INFO) << "========================================";
#if TEST_SPEED_DOUBLE
    LOG(INFO) << "Double-precision LP-distance tests";
    for (double power = delta, step = delta; power <= 24; power += step) {
      nTest++;
      nFail += !TestLPGeneric<double>(128, dim, 200, power);
      nTest++;
      nFail += !TestLPGenericOptim<double>(128, dim, 200, power);
      if (power == 3) step = 0.125;
      if (power == 8) step = 0.5;
    }
    LOG(INFO) << "========================================";
#endif
#endif

    nTest++;
    nFail += !TestSpearmanRho(1024, dim, 2000);

    nTest++;
    nFail += !TestSpearmanRhoSIMD(1024, dim, 2000);

    nTest++;
    nFail += !TestSpearmanFootrule(1024, dim, 2000);

    nTest++;
    nFail += !TestSpearmanFootruleSIMD(1024, dim, 2000);

    nTest++;
    nFail += !TestJSStandard<float>(1024, dim, 1000, pZero1);
    nTest++;
    nFail += !TestJSStandard<float>(1024, dim, 1000, pZero2);
    nTest++;
    nFail += !TestJSStandard<float>(1024, dim, 1000, pZero3);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestJSStandard<double>(1024, dim, 500, pZero1);
    nTest++;
    nFail += !TestJSStandard<double>(1024, dim, 500, pZero2);
    nTest++;
    nFail += !TestJSStandard<double>(1024, dim, 500, pZero3);
#endif

    nTest++;
    nFail += !TestJSPrecomp<float>(1024, dim, 500, pZero1);
    nTest++;
    nFail += !TestJSPrecomp<float>(1024, dim, 500, pZero2);
    nTest++;
    nFail += !TestJSPrecomp<float>(1024, dim, 500, pZero3);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestJSPrecomp<double>(1024, dim, 500, pZero1);
    nTest++;
    nFail += !TestJSPrecomp<double>(1024, dim, 500, pZero2);
    nTest++;
    nFail += !TestJSPrecomp<double>(1024, dim, 500, pZero3);
#endif

    nTest++;
    nFail += !TestJSPrecompApproxLog<float>(1024, dim, 1000, pZero1);
    nTest++;
    nFail += !TestJSPrecompApproxLog<float>(1024, dim, 1000, pZero2);
    nTest++;
    nFail += !TestJSPrecompApproxLog<float>(1024, dim, 1000, pZero3);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestJSPrecompApproxLog<double>(1024, dim, 1000, pZero1);
    nTest++;
    nFail += !TestJSPrecompApproxLog<double>(1024, dim, 1000, pZero2);
    nTest++;
    nFail += !TestJSPrecompApproxLog<double>(1024, dim, 1000, pZero3);
#endif

    nTest++;
    nFail += !TestJSPrecompSIMDApproxLog<float>(1024, dim, 2000, pZero1);
    nTest++;
    nFail += !TestJSPrecompSIMDApproxLog<float>(1024, dim, 2000, pZero2);
    nTest++;
    nFail += !TestJSPrecompSIMDApproxLog<float>(1024, dim, 2000, pZero3);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestJSPrecompSIMDApproxLog<double>(1024, dim, 2000, pZero1);
    nTest++;
    nFail += !TestJSPrecompSIMDApproxLog<double>(1024, dim, 2000, pZero2);
    nTest++;
    nFail += !TestJSPrecompSIMDApproxLog<double>(1024, dim, 2000, pZero3);
#endif

    nTest++;
    nFail += !TestL1Norm<float>(1024, dim, 10000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestL1Norm<double>(1024, dim, 10000);
#endif

    nTest++;
    nFail += !TestL1NormStandard<float>(1024, dim, 10000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestL1NormStandard<double>(1024, dim, 10000);
#endif

    nTest++;
    nFail += !TestL1NormSIMD<float>(1024, dim, 10000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestL1NormSIMD<double>(1024, dim, 10000);
#endif


    nTest++;
    nFail += !TestLInfNorm<float>(1024, dim, 10000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestLInfNorm<double>(1024, dim, 10000);
#endif

    nTest++;
    nFail += !TestLInfNormStandard<float>(1024, dim, 10000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestLInfNormStandard<double>(1024, dim, 10000);
#endif

    nTest++;
    nFail += !TestLInfNormSIMD<float>(1024, dim, 10000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestLInfNormSIMD<double>(1024, dim, 10000);
#endif

    nTest++;
    nFail += !TestItakuraSaitoStandard<float>(1024, dim, 1000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestItakuraSaitoStandard<double>(1024, dim, 1000);
#endif


    nTest++;
    nFail += !TestItakuraSaitoPrecomp<float>(1024, dim, 2000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestItakuraSaitoPrecomp<double>(1024, dim, 2000);
#endif

    nTest++;
    nFail += !TestItakuraSaitoPrecompSIMD<float>(1024, dim, 4000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestItakuraSaitoPrecompSIMD<double>(1024, dim, 4000);
#endif

    nTest++;
    nFail += !TestL2Norm<float>(1024, dim, 10000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestL2Norm<double>(1024, dim, 10000);
#endif

    nTest++;
    nFail += !TestL2NormStandard<float>(1024, dim, 10000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestL2NormStandard<double>(1024, dim, 10000);
#endif

    nTest++;
    nFail += !TestL2NormSIMD<float>(1024, dim, 10000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestL2NormSIMD<double>(1024, dim, 10000);
#endif


    nTest++;
    nFail += !TestKLStandard<float>(1024, dim, 1000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestKLStandard<double>(1024, dim, 1000);
#endif


    nTest++;
    nFail += !TestKLPrecomp<float>(1024, dim, 2000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestKLPrecomp<double>(1024, dim, 2000);
#endif

    nTest++;
    nFail += !TestKLPrecompSIMD<float>(1024, dim, 4000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestKLPrecompSIMD<double>(1024, dim, 4000);
#endif

    nTest++;
    nFail += !TestKLGeneralStandard<float>(1024, dim, 1000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestKLGeneralStandard<double>(1024, dim, 1000);
#endif

    nTest++;
    nFail += !TestKLGeneralPrecomp<float>(1024, dim, 2000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestKLGeneralPrecomp<double>(1024, dim, 2000);
#endif

    nTest++;
    nFail += !TestKLGeneralPrecompSIMD<float>(1024, dim, 2000);
#if TEST_SPEED_DOUBLE
    nTest++;
    nFail += !TestKLGeneralPrecompSIMD<double>(1024, dim, 2000);
#endif

    LOG(INFO) << "Dimensionality " << dim << " " << nTest << " tests performed " << nFail << " failed";

    EXPECT_EQ(0, nFail);
}

}  // namespace similarity

