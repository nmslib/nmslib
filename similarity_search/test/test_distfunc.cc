/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan, Leonid Boytsov.
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <iostream>

#include "space.h"
#include "scoped_ptr.h"
#include "common.h"
#include "bunit.h"
#include "distcomp.h"
#include "ztimer.h"

namespace similarity {

using namespace std;

//#define TEST_SPEED_DOUBLE

// Agreement test functions
template <class T>
bool TestLInfAgree(size_t N, size_t dim, size_t Rep) {
    T* pVect1 = new T[dim];
    T* pVect2 = new T[dim];

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim);
            GenRandVect(pVect2, dim);

            T val1 = LInfNormStandard(pVect1, pVect2, dim);
            T val2 = LInfNorm(pVect1, pVect2, dim);
            T val3 = LInfNormSIMD(pVect1, pVect2, dim);

            bool bug = false;

            if (fabs(val1 - val2)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug LInf !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << endl; 
            }
            if (fabs(val1 - val3)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug LInf !!! Dim = " << dim << " val1 = " << val1 << " val3 = " << val3 << endl; 
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
            GenRandVect(pVect1, dim);
            GenRandVect(pVect2, dim);

            T val1 = L1NormStandard(pVect1, pVect2, dim);
            T val2 = L1Norm(pVect1, pVect2, dim);
            T val3 = L1NormSIMD(pVect1, pVect2, dim);

            bool bug = false;

            if (fabs(val1 - val2)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L1 !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << endl; 
            }
            if (fabs(val1 - val3)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L1 !!! Dim = " << dim << " val1 = " << val1 << " val3 = " << val3 << endl; 
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
            GenRandVect(pVect1, dim);
            GenRandVect(pVect2, dim);

            T val1 = L2NormStandard(pVect1, pVect2, dim);
            T val2 = L2Norm(pVect1, pVect2, dim);
            T val3 = L2NormSIMD(pVect1, pVect2, dim);

            bool bug = false;

            if (fabs(val1 - val2)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L2 !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << endl; 
            }
            if (fabs(val1 - val3)/max(max(val1,val2),T(1e-18)) > 1e-6) {
                cerr << "Bug L2 !!! Dim = " << dim << " val1 = " << val1 << " val3 = " << val3 << endl; 
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
            GenRandVect(pVect1, dim, T(1e-6), true);
            GenRandVect(pVect2, dim, T(1e-6), true);

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
                cerr << "Bug ItakuraSaito !!! Dim = " << dim << " val1 = " << val1 << " val0 = " << val0 << " Diff: " << (val1 - val0) << " RelDiff1: " << RelDiff1 << " AbsDiff1: " << AbsDiff1 << endl; 
            }

            T AbsDiff2 = fabs(val1 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val1),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug ItakuraSaito !!! Dim = " << dim << " val1 = " << val1 << " val2 = " << val2 << " Diff: " << (val1 - val2) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2 << endl; 
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
            GenRandVect(pVect1, dim, T(1e-6), true);
            GenRandVect(pVect2, dim, T(1e-6), true);

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
            }

            T AbsDiff2 = fabs(val1 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val1),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val2 = " << val2 << " val1 = " << val1 << " Diff: " << (val2 - val1) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2 << endl; 
            }

            T AbsDiff3 = fabs(val1 - val3);
            T RelDiff3 = AbsDiff3/max(max(fabs(val1),fabs(val3)),T(1e-18));
            if (RelDiff3 > 1e-5 && AbsDiff3 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val3 = " << val3 << " val1 = " << val1 << " Diff: " << (val3 - val1) << " RelDiff3: " << RelDiff3 << " AbsDiff3: " << AbsDiff3 << endl; 
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
            GenRandVect(pVect1, dim, T(1e-6), true);
            GenRandVect(pVect2, dim, T(1e-6), true);

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
            }

            T AbsDiff2 = fabs(val3 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val3),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug KL !!! Dim = " << dim << " val2 = " << val2 << " val3 = " << val3 << " Diff: " << (val2 - val3) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2 << endl; 
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

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            GenRandVect(pVect1, dim, T(1e-6), true);
            SetRandZeros(pVect1, dim, pZero);
            GenRandVect(pVect2, dim, T(1e-6), true);
            SetRandZeros(pVect2, dim, pZero);

            copy(pVect1, pVect1 + dim, pPrecompVect1); 
            copy(pVect2, pVect2 + dim, pPrecompVect2); 
    
            PrecompLogarithms(pPrecompVect1, dim);
            PrecompLogarithms(pPrecompVect2, dim);

            T val0 = JSStandard(pVect1, pVect2, dim);
            T val2 = JSPrecomp(pPrecompVect1, pPrecompVect2, dim);
            T val3 = JSPrecomp(pPrecompVect1, pPrecompVect2, dim);

            bool bug = false;

            T AbsDiff1 = fabs(val2 - val0);
            T RelDiff1 = AbsDiff1/max(max(fabs(val2),fabs(val0)),T(1e-18));
            if (RelDiff1 > 1e-5 && AbsDiff1 > 1e-5) {
                cerr << "Bug JS !!! Dim = " << dim << " val0 = " << val0 << " val2 = " << val2 << " Diff: " << (val0 - val2) << " RelDiff1: " << RelDiff1 << " AbsDiff1: " << AbsDiff1 << endl; 
            }

            T AbsDiff2 = fabs(val3 - val2);
            T RelDiff2 = AbsDiff2/max(max(fabs(val3),fabs(val2)),T(1e-18));
            if (RelDiff2 > 1e-5 && AbsDiff2 > 1e-5) {
                cerr << "Bug JS !!! Dim = " << dim << " val2 = " << val2 << " val3 = " << val3 << " Diff: " << (val2 - val3) << " RelDiff2: " << RelDiff2 << " AbsDiff2: " << AbsDiff2 << endl; 
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

TEST(TestAgree) {
    int nTest  = 0;
    int nFail = 0;

    srand48(0);

    for (unsigned dim = 1; dim <= 64; ++dim) {
        cout << "Dim = " << dim << endl;

        nTest++;
        nFail = !TestJSAgree<float>(1024, dim, 10, 0.5);
        nTest++;
        nFail = !TestJSAgree<double>(1024, dim, 10, 0.5);

        nTest++;
        nFail = !TestKLGeneralAgree<float>(1024, dim, 10);
        nTest++;
        nFail = !TestKLGeneralAgree<double>(1024, dim, 10);

        nTest++;
        nFail = !TestLInfAgree<float>(1024, dim, 10);
        nTest++;
        nFail = !TestLInfAgree<double>(1024, dim, 10);

        nTest++;
        nFail = !TestL1Agree<float>(1024, dim, 10);
        nTest++;
        nFail = !TestL1Agree<double>(1024, dim, 10);

        nTest++;
        nFail = !TestL2Agree<float>(1024, dim, 10);
        nTest++;
        nFail = !TestL2Agree<double>(1024, dim, 10);

        nTest++;
        nFail = !TestKLAgree<float>(1024, dim, 10);
        nTest++;
        nFail = !TestKLAgree<double>(1024, dim, 10);

        nTest++;
        nFail = !TestItakuraSaitoAgree<float>(1024, dim, 10);
        nTest++;
        nFail = !TestItakuraSaitoAgree<double>(1024, dim, 10);
    }

    cout << nTest << " (sub) tests performed " << nFail << " failed" << endl;

    EXPECT_EQ(0, nFail);
}

// Efficiency test functions

template <class T>
bool TestLInfNormStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += LInfNormStandard(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard LInfs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestLInfNorm(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += LInfNorm(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of optim. LInfs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestLInfNormSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];
    T* p = pArr;

    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += LInfNormSIMD(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD LInfs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}


template <class T>
bool TestL1NormStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += L1NormStandard(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard L1s per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL1Norm(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += L1Norm(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of optim. L1s per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL1NormSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];
    T* p = pArr;

    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += L1NormSIMD(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD L1s per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL2NormStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += L2NormStandard(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of standard L2s per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL2Norm(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += L2Norm(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of optim. L2s per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestL2NormSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];
    T* p = pArr;

    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

     

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += L2NormSIMD(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD L2s per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestItakuraSaitoPrecomp(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += ItakuraSaitoPrecomp(pArr + j*dim*2, pArr + (j-1)*dim*2, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of precomp. ItakuraSaito per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestItakuraSaitoPrecompSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += ItakuraSaitoPrecompSIMD(pArr + j*dim*2, pArr + (j-1)*dim*2, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD precomp. ItakuraSaito per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestItakuraSaitoStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += ItakuraSaito(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of ItakuraSaito per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}


template <class T>
bool TestKLPrecomp(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += KLPrecomp(pArr + j*dim*2, pArr + (j-1)*dim*2, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of precomp. KLs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestKLPrecompSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += KLPrecompSIMD(pArr + j*dim*2, pArr + (j-1)*dim*2, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD precomp. KLs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestKLStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += KLStandard(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of KLs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}


template <class T>
bool TestKLGeneralPrecomp(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += KLGeneralPrecomp(pArr + j*dim*2, pArr + (j-1)*dim*2, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of precomp. general. KLs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestKLGeneralPrecompSIMD(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim * 2];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= 2 * dim) {
        GenRandVect(p, dim);
        PrecompLogarithms(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += KLGeneralPrecompSIMD(pArr + j*dim*2, pArr + (j-1)*dim*2, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of SIMD precomp. general. KLs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestKLGeneralStandard(size_t N, size_t dim, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += KLGeneralStandard(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of general. KLs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestJSStandard(size_t N, size_t dim, float pZero, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, T(0), true);
        SetRandZeros(p, dim, pZero);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += JSStandard(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of JSs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

template <class T>
bool TestJSPrecomp(size_t N, size_t dim, float pZero, size_t Rep) {
    T* pArr = new T[N * dim];

    T *p = pArr;
    for (size_t i = 0; i < N; ++i, p+= dim) {
        GenRandVect(p, dim, T(0), true);
        SetRandZeros(p, dim, pZero);
    }

    WallClockTimer  t;

    t.reset();

    T DiffSum = 0;

    for (size_t i = 0; i < Rep; ++i) {
        for (size_t j = 1; j < N; ++j) {
            DiffSum += JSPrecomp(pArr + j*dim, pArr + (j-1)*dim, dim);
        }
    }

    uint64_t tDiff = t.split();

    cout << "Ignore: " << DiffSum << endl;
    cout << "Elapsed: " << tDiff / 1e3 << " ms " << " # of JSs per second: " << (1e6/tDiff) * N * Rep  << endl;

    delete [] pArr;

    return true;
}

TEST(TestSpeed) {
    int nTest  = 0;
    int nFail = 0;

    srand48(0);

    int N = 128;
    double pZero = 0.5;


    nTest++;
    nFail = !TestJSStandard<float>(1024, N, 10000, pZero);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestJSStandard<double>(1024, N, 10000, pZero);
#endif

    nTest++;
    nFail = !TestJSPrecomp<float>(1024, N, 10000, pZero);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestJSPrecomp<double>(1024, N, 10000, pZero);
#endif


    nTest++;
    nFail = !TestL1Norm<float>(1024, N, 100000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestL1Norm<double>(1024, N, 100000);
#endif

    nTest++;
    nFail = !TestL1NormStandard<float>(1024, N, 100000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestL1NormStandard<double>(1024, N, 100000);
#endif

    nTest++;
    nFail = !TestL1NormSIMD<float>(1024, N, 100000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestL1NormSIMD<double>(1024, N, 100000);
#endif


    nTest++;
    nFail = !TestLInfNorm<float>(1024, N, 100000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestLInfNorm<double>(1024, N, 100000);
#endif

    nTest++;
    nFail = !TestLInfNormStandard<float>(1024, N, 100000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestLInfNormStandard<double>(1024, N, 100000);
#endif

    nTest++;
    nFail = !TestLInfNormSIMD<float>(1024, N, 100000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestLInfNormSIMD<double>(1024, N, 100000);
#endif

    nTest++;
    nFail = !TestItakuraSaitoStandard<float>(1024, N, 10000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestItakuraSaitoStandard<double>(1024, N, 10000);
#endif


    nTest++;
    nFail = !TestItakuraSaitoPrecomp<float>(1024, N, 10000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestItakuraSaitoPrecomp<double>(1024, N, 10000);
#endif

    nTest++;
    nFail = !TestItakuraSaitoPrecompSIMD<float>(1024, N, 10000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestItakuraSaitoPrecompSIMD<double>(1024, N, 10000);
#endif

    nTest++;
    nFail = !TestL2Norm<float>(1024, N, 100000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestL2Norm<double>(1024, N, 100000);
#endif

    nTest++;
    nFail = !TestL2NormStandard<float>(1024, N, 100000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestL2NormStandard<double>(1024, N, 100000);
#endif

    nTest++;
    nFail = !TestL2NormSIMD<float>(1024, N, 100000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestL2NormSIMD<double>(1024, N, 100000);
#endif


    nTest++;
    nFail = !TestKLStandard<float>(1024, N, 10000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestKLStandard<double>(1024, N, 10000);
#endif


    nTest++;
    nFail = !TestKLPrecomp<float>(1024, N, 10000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestKLPrecomp<double>(1024, N, 10000);
#endif

    nTest++;
    nFail = !TestKLPrecompSIMD<float>(1024, N, 10000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestKLPrecompSIMD<double>(1024, N, 10000);
#endif

    nTest++;
    nFail = !TestKLGeneralStandard<float>(1024, N, 10000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestKLGeneralStandard<double>(1024, N, 10000);
#endif

    nTest++;
    nFail = !TestKLGeneralPrecomp<float>(1024, N, 10000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestKLGeneralPrecomp<double>(1024, N, 10000);
#endif

    nTest++;
    nFail = !TestKLGeneralPrecompSIMD<float>(1024, N, 10000);
#ifdef TEST_SPEED_DOUBLE
    nTest++;
    nFail = !TestKLGeneralPrecompSIMD<double>(1024, N, 10000);
#endif

    cout << "Dimensionality " << N << " " << nTest << " tests performed " << nFail << " failed" << endl;

    EXPECT_EQ(0, nFail);
}

}  // namespace similarity

