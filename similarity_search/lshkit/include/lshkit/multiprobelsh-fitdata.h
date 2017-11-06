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
 *  \file fitdata.cpp
 *  \brief Gather statistics from dataset for MPLSH tuning.
 *
 *  This program gahters statistical data from a small sample dataset
 *  for automatic MPLSH parameter tuning.  It carries out the following
 *  steps:
 *  -# Sample N points from the dataset. Only those N points will be used for future computation.
 *  -# Sample P pairs of points from the sample, calculate the distance for each pair.
 *  -# Sample Q points from the sample as queries points.
 *  -# Divide the sample into F folds.
 *  -# For i = 1 to F, take i folds and run K-NN search, so the query points
 *     will be searched against sample datasets of N/F, 2N/F, ..., N/F points.
 *
 *  The statistical data is printed to standard output after the progress display.
 *
\verbatim
Allowed options:
  -h [ --help ]          produce help message.
  -N [ -- ] arg (=0)     number of points to use
  -P [ -- ] arg (=50000) number of pairs to sample
  -Q [ -- ] arg (=1000)  number of queries to sample
  -K [ -- ] arg (=100)   search for K nearest neighbors
  -F [ -- ] arg (=10)    divide the sample to F folds
  -D [ --data ] arg      data file
\endverbatim
 */

#ifndef _MPLSH_FITDATA_H_
#define _MPLSH_FITDATA_H_

#include "logging.h"

#include <cstdlib>
#include <sstream>
#include <gsl/gsl_multifit.h>

namespace lshkit {

inline
bool is_good_value (double v) {
    return ((v > -std::numeric_limits<double>::max()) &&
            (v < std::numeric_limits<double>::max()));
}

inline
std::string FitData(const FloatMatrix& data,
                    unsigned N,            // number of points to use
                    unsigned P,            // number of pairs to sample
                    unsigned Q,            // number of queries to sample
                    unsigned K,            // search for K neighbors neighbors
                    unsigned F             // divide the sample to F folds
                   )
{
    LOG(LIB_INFO) << "started running FitData";

    std::vector<unsigned> idx(data.getSize());
    for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    std::random_shuffle(idx.begin(), idx.end());

    if (N > 0 && N < static_cast<unsigned>(data.getSize())) idx.resize(N);

    metric::l2sqr<float> l2sqr(data.getDim());

    DefaultRng rng;
    boost::variate_generator<DefaultRng &, UniformUnsigned> gen(rng,
             UniformUnsigned(0, idx.size()-1));

    double gM = 0.0;
    double gG = 0.0;
    {
        // sample P pairs of points
        for (unsigned k = 0; k < P; ++k)
        {
            double dist, logdist;
            for (;;)
            {
                unsigned i = gen();
                unsigned j = gen();
                if (i == j) continue;
                dist = l2sqr(data[idx[i]], data[idx[j]]);
                logdist = log(dist);
                if (is_good_value(logdist)) break;
            }
            gM += dist;
            gG += logdist;
        }
        gM /= P;
        gG /= P;
        gG = exp(gG);
    }

    if (Q > idx.size()) Q = idx.size();
    if (K > idx.size() - Q) K = idx.size() - Q;
    /* sample query */
    std::vector<unsigned> qry(Q);

    SampleQueries(&qry, idx.size(), rng);

    /* do the queries */
    std::vector<Topk<unsigned> > topks(Q);
    for (unsigned i = 0; i < Q; ++i) topks[i].reset(K);

    /* ... */
    gsl_matrix *X = gsl_matrix_alloc(F * K, 3);
    gsl_vector *yM = gsl_vector_alloc(F * K);
    gsl_vector *yG = gsl_vector_alloc(F * K);
    gsl_vector *pM = gsl_vector_alloc(3);
    gsl_vector *pG = gsl_vector_alloc(3);
    gsl_matrix *cov = gsl_matrix_alloc(3,3);

    std::vector<double> M(K);
    std::vector<double> G(K);

    unsigned m = 0;
    for (unsigned l = 0; l < F; l++)
    {
        // Scan
        for (unsigned i = l; i< idx.size(); i += F)
        {
            for (unsigned j = 0; j < Q; j++)
            {
                unsigned id = qry[j];
                if (i != id)
                {
                    float d = l2sqr(data[idx[id]], data[idx[i]]);
                    if (is_good_value(log(double(d))))
                      topks[j] << Topk<unsigned>::Element(i, d);
                }
            }
        }

        fill(M.begin(), M.end(), 0.0);
        fill(G.begin(), G.end(), 0.0);

        for (unsigned i = 0; i < Q; i++)
        {
            for (unsigned k = 0; k < K; k++)
            {
                M[k] += topks[i][k].dist;
                G[k] += log(topks[i][k].dist);
            }
        }

        for (unsigned k = 0; k < K; k++)
        {
            M[k] = log(M[k]/Q);
            G[k] /= Q;
            gsl_matrix_set(X, m, 0, 1.0);
            gsl_matrix_set(X, m, 1, log(double(data.getSize() * (l + 1)) / double(F)));
            gsl_matrix_set(X, m, 2, log(double(k + 1)));
            gsl_vector_set(yM, m, M[k]);
            gsl_vector_set(yG, m, G[k]);
            ++m;
        }

        //++progress;
    }

    gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc(F * K, 3);

    double chisq;

    gsl_multifit_linear(X, yM, pM, cov, &chisq, work);
    gsl_multifit_linear(X, yG, pG, cov, &chisq, work);

    std::stringstream ss;
    ss << gM << " " << gG << std::endl;
    ss << gsl_vector_get(pM, 0) << " "
       << gsl_vector_get(pM, 1) << " "
       << gsl_vector_get(pM, 2) << std::endl;
    ss << gsl_vector_get(pG, 0) << " "
       << gsl_vector_get(pG, 1) << " "
       << gsl_vector_get(pG, 2) << std::endl;

    gsl_matrix_free(X);
    gsl_matrix_free(cov);
    gsl_vector_free(pM);
    gsl_vector_free(pG);
    gsl_vector_free(yM);
    gsl_vector_free(yG);

    LOG(LIB_INFO) << ss.str(); 
    LOG(LIB_INFO) << "finished FitData";

    return ss.str();
}

}     // namespace lshkit

#endif
