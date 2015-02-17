/*
Copyright (C) 2010,2011 Wei Dong <wdong@wdong.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef WDONG_NNDESCENT_COMMON
#define WDONG_NNDESCENT_COMMON

#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/progress.hpp>
#include <boost/random.hpp>
#define USE_SPINLOCK 1
#ifdef USE_SPINLOCK
#include <mutex>
#include "boost/smart_ptr/detail/spinlock.hpp"
#endif
#ifdef _OPENMP
#include <omp.h>
#endif

namespace similarity {

    using std::vector;
    using std::numeric_limits;

#define SYMMETRIC 1

#ifdef _OPENMP
#if SYMMETRIC
#define NEED_LOCK 1
#endif
#endif

#if NEED_LOCK
#ifdef USE_SPINLOCK
     class Mutex {
         boost::detail::spinlock lock;
     public:
         void init () {
         }
         void set () {
             lock.lock();
         }
 
         void unset () {
             lock.unlock();
         }
         friend class ScopedLock;
     };

     class ScopedLock: public std::lock_guard<boost::detail::spinlock> {
     public:
         ScopedLock (Mutex &mutex): std::lock_guard<boost::detail::spinlock>(mutex.lock) {
 
         }
     };
#else
    class Mutex {
        omp_lock_t *lock;
    public:
        Mutex (): lock(0) {
        }

        void init () {
            lock = new omp_lock_t;
            omp_init_lock(lock);
        }

        ~Mutex () {
            if (lock) {
                omp_destroy_lock(lock);
                delete lock;
            }
        }

        void set () {
            omp_set_lock(lock);
        }

        void unset () {
            omp_unset_lock(lock);
        }

        friend class ScopedLock;
    };
    class ScopedLock {
        omp_lock_t *lock;
    public:
        ScopedLock (Mutex &mutex) {
            lock = mutex.lock;
            omp_set_lock(lock);
        }
        ~ScopedLock () {
            omp_unset_lock(lock);
        }
    };
#endif
#else
    class Mutex {
    public:
        void init () {};
        void set () {};
        void unset () {};
    };
    class ScopedLock {
    public:
        ScopedLock (Mutex &) {
        }
    };
#endif

    struct KNNEntry
    {
        static const int BAD = -1; //numeric_limits<int>::max();
        int key;
        float dist;   
        bool flag;
        bool match (const KNNEntry &e) const { return key == e.key; }
        KNNEntry (int key_, float dist_, bool flag_ = true) : key(key_), dist(dist_), flag(flag_) {}
        KNNEntry () : dist(numeric_limits<float>::max()) { }
        void reset () { key = BAD;  dist = numeric_limits<float>::max(); }
        friend bool operator < (const KNNEntry &e1, const KNNEntry &e2)
        {
            return e1.dist < e2.dist;
        }
    };

    class KNN: public vector<KNNEntry>
    {
        int K;
        Mutex mutex;
    public:
        typedef KNNEntry Element;
        typedef vector<KNNEntry> Base;

        void init (int k) {
            mutex.init();
            K = k;
            this->resize(k);
            BOOST_FOREACH(KNNEntry &e, *this) {
                e.reset();
            }
        }

        int update (Element t)
        {
            //ScopedLock ll(mutex);
            mutex.set();
            int i = this->size() - 1;
            int j;
            if (!(t < this->back())) {
                mutex.unset();
                return -1;
            }
            for (;;)
            {
                if (i == 0) break;
                j = i - 1;
                if (this->at(j).match(t)) {
                    mutex.unset();
                    return -1;
                }
                if (this->at(j) < t) break;
                i = j;
            }

            j = this->size() - 1;
            for (;;)
            {
                if (j == i) break;
                this->at(j) = this->at(j-1);
                --j;
            }
            this->at(i) = t;
            mutex.unset();
            return i;
        }

        void update_unsafe (Element t)
        {
            int i = this->size() - 1;
            int j;
            if (!(t < this->back())) return;
            for (;;)
            {
                if (i == 0) break;
                j = i - 1;
                if (this->at(j).match(t)) {
                    return;
                }
                if (this->at(j) < t) break;
                i = j;
            }

            j = this->size() - 1;
            for (;;)
            {
                if (j == i) break;
                this->at(j) = this->at(j-1);
                --j;
            }
            this->at(i) = t;
        }

        void lock() {
            mutex.set();
        }

        void unlock() {
            mutex.unset();
        }
    };

    static inline float recall (const int *knn, const KNN &ans, int K) {
        int match = 0;
        for (int i = 0; i < K; ++i) {
            for (int j = 0; j < K; ++j) {
                if (knn[i] == ans[j].key) {
                    ++match;
                    break;
                }
            }
        }
        return float(match) / K;
    }

    class Random {
        boost::mt19937 rng;
    public:
        Random () {
        }
        void seed (int s) {
            rng.seed(s);
        }
        ptrdiff_t operator () (ptrdiff_t i) {
            return rng() % i;
        }
    };
}

#endif 

