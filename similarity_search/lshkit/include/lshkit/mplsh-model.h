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
#ifndef __LSHKIT_MPLSH_MODEL__
#define __LSHKIT_MPLSH_MODEL__

#include <fstream>
#include <sstream>
#include <boost/math/distributions/gamma.hpp>
#include <boost/math/distributions/normal.hpp>
#include <lshkit/matrix.h>

/**
 * \file mplsh-model.h
 * \brief Model of Multi-probe LSH.
 *
 * The modeling code is not well documented because I find it hard to do so.
 * The code is essentially a translation of our CIKM'08 paper.  If you do wish
 * to look into the code, beware that when the distance is used, sometime I
 * mean L2 distance (when Gaussian distribution of the distance between
 * the random projection of two points is considered)
 * and sometime I mean L2sqr distance because L2sqr distance between two points
 * in the database follows Gamma distribution.
 */

namespace lshkit {

typedef boost::math::normal_distribution<double> GaussianDouble;
typedef boost::math::gamma_distribution<double> GammaDouble;

/// Maximum likelihood estimation of gamma distribution.
/**
  * @param M sample mean.
  * @param G geometric mean of sample.
  */
GammaDouble GammaDoubleMLE (double M, double G);

/// Data parameter.
/**
  * This class represents the parameters extracted from the dataset.
  */
// We use l2sqr here.
class DataParam
{
    double M, G;
    double a_M, b_M, c_M;
    double a_G, b_G, c_G;

    /*
    DataParam () {}
    DataParam (const DataParam &dp)
        : M(dp.M), G(dp.G),
        a_M(dp.a_M), b_M(dp.b_M), c_M(dp.c_M),
        a_G(dp.a_G), b_G(dp.b_G), c_G(dp.c_G) {}
        */
public:
    /// Constructor.
    /**
      * For now, parameters can only be loaded from a file.
      */
    DataParam (const std::string &path)
    {
        std::ifstream is(path.c_str());
        is >> M >> G >> a_M >> b_M >> c_M
             >> a_G >> b_G >> c_G;
    }

    // read from string
    DataParam(const std::string& fit_data, int dummy) {
      std::stringstream ss(fit_data);
      ss >> M >> G >> a_M >> b_M >> c_M
             >> a_G >> b_G >> c_G;
    }

    /// Estimate the global distance distribution.
    GammaDouble globalDist () const
    {
        return GammaDoubleMLE(M, G);
    }

    /// Estimate the distance distribution of the K-th NN.
    /**
      * @param N size of the (extraplolated) dataset.
      */
    GammaDouble topkDist (unsigned N, unsigned K) const
    {
        double m, g;
        m = std::exp(a_M) * std::pow(double(N), b_M) * std::pow(double(K), c_M);
        g = std::exp(a_G) * std::pow(double(N), b_G) * std::pow(double(K), c_G);
        return GammaDoubleMLE(m,g);
    }

    void scale (double s) {
        M /= s;
        G /= s;
        a_M -= std::log(s);
        a_G -= std::log(s);
    }

    double scale () {
        double s = M;
        scale(s);
        return s;
    }

};

/// Multi-probe LSH parameters.
// we use l2 here.
class MultiProbeLshModel
{
    unsigned L_;
    double W_;
    unsigned M_, T_;
public:
    MultiProbeLshModel(unsigned L, double W, unsigned M, unsigned T)
        : L_(L), W_(W), M_(M), T_(T)
    {}

    double recall (double l2) const;

    void setL (unsigned L) { L_ = L; }
    void setW (double W) { W_ = W; }
    void setT (unsigned T) { T_ = T; }
    void setM (unsigned M) { M_ = M; }

    unsigned getT () const {return T_; }

};

// we use l2sqr here.
class MultiProbeLshDataModel: public MultiProbeLshModel
{
// temporary, not thread safe
    GammaDouble globalDist_;
    std::vector<GammaDouble> topkDists_;

public:
    MultiProbeLshDataModel(const DataParam &param, unsigned N, unsigned K)
        : MultiProbeLshModel(0,0,0,0), globalDist_(1.0),
        topkDists_(K, globalDist_)
    {
        globalDist_ = param.globalDist();
        for (unsigned k = 0; k < K; k++)
        {
            topkDists_[k] = param.topkDist(N, k + 1);
        }
    }

    double avgRecall () const;
    double cost () const;
};

// we use l2 here.
class MultiProbeLshRecallTable
{
    unsigned step_;
    double min_, max_;
    double lmin_, lmax_;
    Matrix<float> table_;

public:

    void load (std::istream &is) {
        is.read((char *)&min_, sizeof(min_));
        is.read((char *)&max_, sizeof(max_));
        table_.load(is);

        step_ = table_.getDim();
        lmin_ = log(min_);
        lmax_ = log(max_);
    }

    void save (std::ostream &os) {
        os.write((const char *)&min_, sizeof(min_));
        os.write((const char *)&max_, sizeof(max_));
        table_.save(os);
    }

    void reset (MultiProbeLshModel model, unsigned d_step, double d_min, double d_max)
    {
        if (d_min <= 0 || d_max <= 0) {
            throw std::logic_error("Make sure a distance is positive.");
        }

        step_ = d_step;

        min_ = d_min;
        max_ = d_max;
        lmin_ = log(d_min);
        lmax_ = log(d_max);
        table_.reset(d_step, model.getT());

        unsigned T = model.getT();
        double delta = (lmax_ - lmin_) / step_;
        for (unsigned t = 0; t < T; ++t)
        {
            model.setT(t+1);
            for (unsigned d = 0; d < step_; ++d)
            {
                table_[t][d] = model.recall(exp(lmin_ + delta * d));
            }
        }
    }

    float lookup (float dist, int T) const
    {
        unsigned d;
        if (dist < min_) return 1.0;
        if (!(dist < max_)) return 0.0;
        d = std::floor((log(dist) - lmin_) * step_ / (lmax_ - lmin_) + 0.5);
        return table_[T-1][d];
    }

};

}

#endif
