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

#include "logging.h"

#include <cmath>
#include <boost/functional.hpp>
#include <boost/math/special_functions/digamma.hpp>
#include <boost/math/tools/roots.hpp>
#include <gsl/gsl_integration.h>

#include <lshkit/common.h>
#include <lshkit/mplsh.h>

#define ABS_ERROR   1e-5l
#define REL_ERROR   1e-5l
#define MAX_SHAPE   1000.0l
#define LIMIT       40000

namespace lshkit {

static GaussianDouble normal;

/* Maximum Likelihood Estimation of gamma distribution */
class GammaDoubleMleHelper
{
    double rate_; // G / M
public:
    GammaDoubleMleHelper(double rate): rate_(rate) {}

    double operator () (double k)
    {
        return std::log(k) - boost::math::digamma(k) + std::log(rate_);
    }
};

bool GammaDoubleMleTol (double first, double second)
{
    return (second - first) < ABS_ERROR;
}

GammaDouble GammaDoubleMLE (double M, double G)
{
    GammaDoubleMleHelper hlp(G/M);
    // Conversion to double is due to an Intel's bug with __builtin_signbit being undefined for float
    std::pair<double,double> pair = boost::math::tools::bisect(hlp, (double)ABS_ERROR, (double)MAX_SHAPE, GammaDoubleMleTol);
    double k = (pair.first + pair.second) / 2.0;
    return GammaDouble(k, M/k);
}

/* estimate the cost and miss of LSH */
static inline double col_helper (double x)
{
    double result;
    result = 2.0*boost::math::cdf(normal, x) - 1.0;
    result += std::sqrt(2.0/M_PI) * (std::exp(-x*x/2.0)-1.0)/x;
    return result; 
}

static inline double p_col_helper (double x, double k)
{
    return boost::math::cdf(normal,(1.0 + k) * x)
        - boost::math::cdf(normal, k*x);
}

double MultiProbeLshModel::recall (double x) const
{
    double x2 = W_ / x;
    double p = col_helper(x2);


    unsigned MT =  __probeSequenceTemplates[M_].size();
    if (MT > T_) MT = T_;
    
    double result = 0;
    for (unsigned i = 0; i < MT; i++)
    {
        double r = 1.0;
        for (unsigned j = 0; j < M_; j++)
        {
            Probe &probe = __probeSequenceTemplates[M_][i];
            if (probe.mask & leftshift(j))
            {
                double delta = (j + 1.0) / (M_ + 1.0) * 0.5; // expected value
                if (probe.shift & leftshift(j))
                {
                    r *= p_col_helper(x2, 1.0 - delta);
                }
                else
                {
                    r *= p_col_helper(x2, delta);
                }
            }
            else r *= p;
        }
        result += r;
    }
    return 1.0 - std::exp(std::log(1.0 - result) * L_);
}

struct __MpLshMdlHlpr
{
    const MultiProbeLshModel *model;
    const GammaDouble *gamma;
};

static double recall_helper (double xsqr, void *_param)
{
    __MpLshMdlHlpr *param = reinterpret_cast<__MpLshMdlHlpr
        *>(_param);
    return boost::math::pdf(*param->gamma, xsqr) * param->model->recall(std::sqrt(xsqr));
}

static double recall (__MpLshMdlHlpr *param)
{
    static gsl_integration_workspace *workspace = NULL;
    double f, error;
    gsl_function I;
    if (workspace == NULL)
    {
        workspace = gsl_integration_workspace_alloc(LIMIT);
        BOOST_VERIFY(workspace != NULL);
    }
    I.params = param;
    I.function = recall_helper;
    //if (gsl_integration_qagiu(&I, 0.0, ABS_ERROR, REL_ERROR, LIMIT, workspace, &f, &error) != 0) f = 1.0;
    if (gsl_integration_qagiu(&I, 0.0, ABS_ERROR, REL_ERROR, LIMIT, workspace, &f, &error) != 0) {
      LOG(LIB_INFO) << "The integral didn't converge, assuming the recall is zero.";
      f = 0.0;
    }
    return f;
}

double MultiProbeLshDataModel::avgRecall () const
{
    double f = 0.0;
    __MpLshMdlHlpr param;
    param.model = this;
    for (unsigned k = 0; k < topkDists_.size(); k++)
    {
        param.gamma = &topkDists_[k];
        f += lshkit::recall(&param);
    }
    f /= topkDists_.size();
    return f;
}

double MultiProbeLshDataModel::cost () const
{
    __MpLshMdlHlpr param;
    param.model = this;
    param.gamma = &globalDist_;
    return lshkit::recall(&param);
}

}
