/*
    Copyright (C) 2008 Wei Dong <wdong@princeton.edu>. All Rights Reserved.

    This file is part of LSHKIT.

    LSHKIT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LSHKIT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LSHKIT.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * \file mplsh-tune.cpp
 * \brief Automatic parameter tuning for MPLSH.
 *
 * Assume you have a sample datafile sample.data.  You need to take the following two steps for parameter tuning:
 * \section tune_1 1. Create a model of data distribution.
 *
 * Use the following command
 *
 \verbatim
 fitdata -D sample.data
 \endverbatim
 *
 * The following options are acceptable:
 \verbatim
 Allowed options:
    -h [ --help ]            produce help message.
    -N [ -- ] arg (=0)     # of points to use.  if N = 0, then the whole sample dataset is used.
    -P [ -- ] arg (=50000) # pairs of points to sample
    -Q [ -- ] arg (=1000)  # queries to sample
    -K [ -- ] arg (=100)   Top K
    -F [ -- ] arg (=10)
    -D [ --data ] arg      data file
  \endverbatim
 *
 * For example, we use the following command to generate statistics from audio.data:
 *
 \verbatim
 fitdata -D audio.data
 \endverbatim
 *
 * We get the following output
 *
  \verbatim
  0%   10   20   30   40   50   60   70   80   90   100%
  |----|----|----|----|----|----|----|----|----|----|
  ***************************************************
  3.28031 2.90649
  0.831193        -0.160539       0.150609
  0.825921        -0.186676       0.177095
  \endverbatim
 *
 * Cut and paste the last three lines into a file named audio.param.
 *
 * \section tune_2 2. Use mplsh-tune to tune parameters.
 *
 * There are four parameters in MPLSH: L, T, M, W.  You'll have to choose L and
 * T and let mplsh-tune to find the optimal M and W.   For example, we use the
 * following command to tune M and W for the audio dataset.
 *
 * \verbatim
 mplsh-tune -P audio.param -N 54387 -L 8 -T 20 -R 0.8 -K 50

  -P is the parameter file we generated in the 1st step;
  -N is the whole dataset size.  Note that in the first step we only use a subset of the data to generate the model.
  -L # hash tables to use.
  -T # of bins probed in each hash table
  -R required recall.
  -r radius
  -K K-NNs to find.
 \endverbatim
 *
 * mplsh-tune will output best W and M that will meet the required recall.  For
 * the audio data, we get the following output:
 *
 \verbatim
 L = 8   T = 20  M = 20  W = 4.32        recall = 0.805276       cost = 0.0968405
 \endverbatim
 *
 * We can then run the benchmark and see how close is the prediction to the real number:
 *
 \verbatim
 mplsh-run -D audio.data -B audio.query -L 8 -T 20 -W 4.32 -M 20 -Q 100 -K 50
 \endverbatim
 *
 \verbatim
 Loading data...done.
 Loading benchmark...done.
 Initializing index...
 done.
 Populating index...

 0%   10   20   30   40   50   60   70   80   90   100%
 |----|----|----|----|----|----|----|----|----|----|
 ***************************************************
 CREATE TIME: 3.19s
 Running queries...

 0%   10   20   30   40   50   60   70   80   90   100%
 |----|----|----|----|----|----|----|----|----|----|
 ***************************************************
 QUERY TIME: 0.57s
 [RECALL] 0.8192 +/- 0.234635
 [COST] 0.115853 +/- 0.0980032
\endverbatim
 *
 * (Note that the audio dataset is a pretty hard one.)
 *
 * \section tune_LT How to choose L and T?
 *
 * L is the number of hash tables maintained in the memory and generally larger
 * L results in better performance (smaller cost to reach certain recall).
 * Hash tables only stores pointers to the feature vectors.  So on a 64-bit
 * machine, N points will take 8N bytes plus some overhead.  For our audio
 * dataset, where there are about 55K points, one hash table takes about 500KB
 * of memory and 8 hash tables will take 4MB.
 *
 * T is the number of buckets to probe in each hash table.  A number from 10 ~
 * 100 would be fine.  Larger T results in lower cost.  Yes, more buckets are
 * probed in each hash table, but that would allow us to make each hash bucket
 * smaller, and the overall effect is less points are scanned to reach the
 * required recall.  However, our model doesn't consider the cost of generating
 * the probe sequence and when T is very large, that cost can be significant.
 * So in practice T should not really be much larger than 100.
 *
 *
 */

#ifndef _MPLSH_TUNE_H_
#define _MPLSH_TUNE_H_

#include "logging.h"

#include <boost/format.hpp>
#include <lshkit.h>
#include <lshkit/tune.h>

namespace lshkit {

static const int MIN_L = 1;
static const int MAX_L = 20;

static const int MIN_T = 1;
static const int MAX_T = Probe::MAX_T;

static const int MIN_M = 1;
static const int MAX_M = Probe::MAX_M;

static const double MIN_W = 0.01;
static const double MAX_W = 10;
static const double NUM_W = 400;
static const double DELTA_W = (MAX_W - MIN_W) / NUM_W;

// L, T, M, W
static tune::Interval intervals[] = {
                            {MIN_L, MAX_L + 1},
                            {MIN_T, MAX_T + 1},
                            {0, MAX_M - MIN_M + 1},
                            {0, static_cast<unsigned>(NUM_W) + 1}};

static double target_recall;

static MultiProbeLshDataModel *model;
static double R;

inline
double recall_K (const tune::Input &x) {
    model->setL(x[0]);
    model->setT(x[1]);
    model->setM(MAX_M - x[2]);
    model->setW(MIN_W + DELTA_W * x[3]);
    return model->avgRecall();
}

inline
double recall_R (const tune::Input &x) {
    model->setL(x[0]);
    model->setT(x[1]);
    model->setM(MAX_M - x[2]);
    model->setW(MIN_W + DELTA_W * x[3]);
    return model->recall(R);
}

inline
double cost (const tune::Input &x) {
    model->setL(x[0]);
    model->setT(x[1]);
    model->setM(MAX_M - x[2]);
    model->setW(MIN_W + DELTA_W * x[3]);
    return model->cost();
}

inline
bool constraint_K (const tune::Input &x) {
    return recall_K(x) > target_recall;
}

inline
bool constraint_R (const tune::Input &x) {
    return recall_R(x) > target_recall;
}

inline
void MPLSHTune(unsigned N,                    // dataset size
               std::string data_param,        // data parameter
               int T,
               int L,
               double desired_recall,         // desired recall
               unsigned K,                    // topk
               int& M,
               float& W
               //unsigned r                   // radius
              )
{
    LOG(LIB_INFO) << "started running MPLSHTune" ;
    gsl_set_error_handler_off();

    // restore initial values
    intervals[0].begin = MIN_L;  intervals[0].end = MAX_L + 1;
    intervals[1].begin = MIN_T;  intervals[1].end = MAX_T + 1;
    intervals[2].begin = 0;      intervals[2].end = MAX_M - MIN_M + 1;
    intervals[3].begin = 0;      intervals[3].end = NUM_W + 1;

    target_recall = desired_recall;
    model = NULL;
    R = 0.0;  // radius

    /*
    int T, L, M;
    double W;
    unsigned N, K;
    bool do_K = true;
    string data_param;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message.")
        (",T", po::value<int>(&T)->default_value(-1), "")
        (",L", po::value<int>(&L)->default_value(-1), "")
        (",M", po::value<int>(&M)->default_value(-1), "set to -1 for tuning.")
        (",W", po::value<double>(&W)->default_value(-1), "set to -1 for tuning.")
        ("size,N", po::value<unsigned>(&N), "dataset size.")
        ("param,P", po::value<string>(&data_param), "data parameter file.")
        ("recall,R", po::value<double>(&target_recall)->default_value(0.9), "desired recall.")
        ("topk,K", po::value<unsigned>(&K)->default_value(20), "")
        ("radius,r", po::value<double>(&R), "")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") || (vm.count("param") < 1) || (vm.count("size") < 1) || T < 1 || L < 1)
    {
        cout << desc;
        return 0;
    }

    if (vm.count("radius")) {
        do_K = false;
    }
    */

    if (L <= 0 || T <= 0) {
        LOG(LIB_INFO) << "You need to specify L and T." ;
        exit(1);
    }

    if (L > 0) {
        intervals[0].begin = L;
        intervals[0].end = L + 1;
    }

    if (T > 0) {
        intervals[1].begin = T;
        intervals[1].end = T + 1;
    }

    /*
    if (M > 0) {
        intervals[2].begin = MAX_M - M;
        intervals[2].end = MAX_M - M + 1;
    }

    if (W > 0) {
        intervals[3].begin = (W - MIN_W) / DELTA_W;
        intervals[3].end = intervals[3].begin + 1;
    }
    */

    DataParam param(data_param, 0);
    double scale = param.scale();

    MultiProbeLshDataModel local_model(param, N, K);
    model = &local_model;

    int begin_M = intervals[2].begin;
    int end_M = intervals[2].end;

    LOG(LIB_INFO) << "iter limits: begin_M " << begin_M << " end_M " << end_M ;

    double best_recall = 0.0;
    double best_cost = 1.0;
    M = -1;
    W = -1.0;
    bool found = false;

    for (int m = begin_M; m < end_M; ++m) {
        intervals[2].begin = m;
        intervals[2].end = m + 1;

        tune::Range range(intervals, intervals + sizeof intervals /sizeof intervals[0]);
        tune::Input input;
        //bool ok = do_K ? tune::Tune(range, constraint_K, &input)
        //               : tune::Tune(range, constraint_R, &input);

        bool ok = tune::Tune(range, constraint_K, &input);

        if (ok) {
            double CurrCost = cost(input);
            LOG(LIB_INFO) << "iter " << m << " "
            << boost::format("L = %d\tT = %d\tM = %d\tW = %g\trecall = %g\tcost = %g")
            % input[0] % input[1] % (MAX_M - input[2]) % ((MIN_W + DELTA_W * input[3]) * sqrt(scale))
            % ( recall_K(input)) % CurrCost 
            //% (do_K ? recall_K(input) : recall_R(input)) % cost(input)
            ;


            double recall = recall_K(input);
            if (//best_recall < recall && 
                recall >= target_recall
                && CurrCost < best_cost) {
              best_recall = recall;
              best_cost = CurrCost;
              found = true;
              M = MAX_M - input[2];
              W = (MIN_W + DELTA_W * input[3]) * sqrt(scale);
            }

        } else {
          LOG(LIB_INFO) << "Failed. iter " << m ;
        }
    }

    if (found) {
      LOG(LIB_INFO) << "best_recall = " << best_recall << " ";
      LOG(LIB_INFO) << "best_cost   = " << best_cost << " ";
      LOG(LIB_INFO) << "M = " << M << " W = " << W ;
    } else {
      LOG(LIB_INFO) << "could not tune M & W" ;
      exit(1);
    }
    LOG(LIB_INFO) << "MPLSHTune finished." ;
}

}     // namespace lshkit

#endif
