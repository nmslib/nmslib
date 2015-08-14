/*
Copyright (C) 2010,2011 Wei Dong <wdong@wdong.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef WDONG_NNDESCENT
#define WDONG_NNDESCENT

#include "nndes-common.h"
#include "ported_boost_progress.h"

namespace similarity {

    using std::cout;
    using std::unique_ptr;
    using std::vector;
    using std::swap;

#ifndef NNDES_SHOW_PROGRESS
#define NNDES_SHOW_PROGRESS 1
#endif

    // Normally one would use GRAPH_BOTH,
    // GRAPH_KNN & GRAPH_RNN are for experiments only.
    static const int GRAPH_NONE = 0, GRAPH_KNN = 1, GRAPH_RNN = 2, GRAPH_BOTH = 4;
    typedef int GraphOption;

    // The main NN-Descent class.
    // Instead of the actual dataset, the class takes a distance oracle
    // as input.  Given two data item ids, the oracle returns the distance
    // between the two.
    template <typename ORACLE>
    class NNDescent {
    private:
        const ORACLE &oracle;
        int N;              // # points
        int K;              // K-NN to find
        int S;              // # of NNs to use for exploration
        GraphOption option;
        vector<KNN> nn;     // K-NN approximation

        // We maintain old and newly added KNN/RNN items
        // separately for incremental processing:
        // we need to compare two new ones
        // and a new one to an old one, but not two old ones as they
        // must have been compared already.
        vector<vector<int> > nn_old;
        vector<vector<int> > nn_new;
        vector<vector<int> > rnn_old;
        vector<vector<int> > rnn_new;

        // total number of comparisons done.
        long long int cost;

        // This function decides of it's necessary to compare two
        // points.  Obviously a point should not compare against itself.
        // Another potential usage of this function is to record all
        // pairs that have already be compared, so that when seen in the future,
        // then same pair doesn't have be compared again.
        bool mark (int p1, int p2) {
            return p1 == p2;
        }

        // Compare two points and update their KNN list of necessary.
        // Return the number of comparisons done (0 or 1).
        int update (int p1, int p2) {
            if (mark(p1, p2)) return 0;
            // KNN::update is synchronized by a lock
            // keep an order is necessary to avoid deadlock.
            if (p1 > p2) swap(p1, p2);
            float dist =  oracle(p1, p2);
            nn[p1].update(KNN::Element(p2, dist, true));
            nn[p2].update(KNN::Element(p1, dist, true));
            return 1;
        }

    public:
        const vector<KNN> &getNN() const {
            return nn;
        }

        long long int getCost () const {
            return cost;
        }

        NNDescent (int N_, int K_, float S_, const ORACLE &oracle_,
                GraphOption opt = GRAPH_BOTH)
            : oracle(oracle_), N(N_), K(K_), S(K * S_), option(opt), nn(N_),
              nn_old(N_), nn_new(N_), rnn_old(N_), rnn_new(N_), cost(0)
        {
            for (int i = 0; i < N; ++i) {
                nn[i].init(K);
                // random initial edges
                if ((option & GRAPH_KNN) || (option & GRAPH_BOTH)) {
                    nn_new[i].resize(S);
                    BOOST_FOREACH(int &u, nn_new[i]) {
                        u = RandomInt() % N;
                    }
                }
                if ((option & GRAPH_RNN) || (option & GRAPH_BOTH)) {
                    rnn_new[i].resize(S);
                    BOOST_FOREACH(int &u, rnn_new[i]) {
                        u = RandomInt() % N;
                    }
                }
            }
        }

        // An iteration contains two parts:
        //      local join
        //      identify the newly detected NNs.
        int iterate (bool PrintProgress) {

#if NNDES_SHOW_PROGRESS
            unique_ptr<ProgressDisplay>  progress(
                    PrintProgress? new ProgressDisplay(N, cerr): NULL);
#endif

            long long int cc = 0;
            // local joins
#pragma omp parallel for default(shared) reduction(+:cc)
            for (int i = 0; i < N; ++i) {

                // The following loops are bloated to deal with all
                // the experimental setups.  Otherwise they should
                // be really simple.
                if (option & (GRAPH_KNN | GRAPH_BOTH)) {
                    BOOST_FOREACH(int j, nn_new[i]) {
                        BOOST_FOREACH(int k, nn_new[i]) {
                            if (j >= k) continue;
                            cc += update(j, k);
                        }
                        BOOST_FOREACH(int k, nn_old[i]) {
                            cc += update(j, k);
                        }
                    }
                }
                if (option & (GRAPH_RNN | GRAPH_BOTH)) {
                    BOOST_FOREACH(int j, rnn_new[i]) {
                        BOOST_FOREACH(int k, rnn_new[i]) {
                            if (j >= k) continue;
                            cc += update(j, k);
                        }
                        BOOST_FOREACH(int k, rnn_old[i]) {
                            cc += update(j, k);
                        }
                    }
                }
                if (option & GRAPH_BOTH) {
                    BOOST_FOREACH(int j, nn_new[i]) {
                        BOOST_FOREACH(int k, rnn_old[i]) {
                            cc += update(j, k);
                        }
                        BOOST_FOREACH(int k, rnn_new[i]) {
                            cc += update(j, k);
                        }
                    }
                    BOOST_FOREACH(int j, nn_old[i]) {
                        BOOST_FOREACH(int k, rnn_new[i]) {
                            cc += update(j, k);
                        }
                    }
                }

#if NNDES_SHOW_PROGRESS
#pragma omp critical 
                if (progress) ++(*progress);
#endif
            }

            cost += cc;

            int t = 0;
//#pragma omp parallel for default(shared) reduction(+:t)
            for (int i = 0; i < N; ++i) {

                nn_old[i].clear();
                nn_new[i].clear();
                rnn_old[i].clear();
                rnn_new[i].clear();

                // find the new ones
                for (int j = 0; j < K; ++j) {
                    KNN::Element &e = nn[i][j];
                    if (e.key == KNN::Element::BAD) continue;
                    if (e.flag){
                        nn_new[i].push_back(j);
                    }
                    else {
                        nn_old[i].push_back(e.key);
                    }
                }

                t += nn_new[i].size();
                // sample
                if (nn_new[i].size() > unsigned(S)) {
                    random_shuffle(nn_new[i].begin(), nn_new[i].end());
                    nn_new[i].resize(S);
                }
                BOOST_FOREACH(int &v, nn_new[i]) {
                    nn[i][v].flag = false;
                    v = nn[i][v].key;
                }
            }

            // symmetrize
            if ((option & GRAPH_RNN) || (option & GRAPH_BOTH)) {
                for (int i = 0; i < N; ++i) {
                    BOOST_FOREACH(int e, nn_old[i]) {
                        rnn_old[e].push_back(i);
                    }
                    BOOST_FOREACH(int e, nn_new[i]) {
                        rnn_new[e].push_back(i);
                    }
                }
            }

//#pragma omp parallel for default(shared) reduction(+:t)
            for (int i = 0; i < N; ++i) {
                if (rnn_old[i].size() > unsigned(S)) {
                    random_shuffle(rnn_old[i].begin(), rnn_old[i].end());
                    rnn_old[i].resize(S);
                }
                if (rnn_new[i].size() > unsigned(S)) {
                    random_shuffle(rnn_new[i].begin(), rnn_new[i].end());
                    rnn_new[i].resize(S);
                }
            }

            return t;
        }
    };


}

#endif 

