/* 
    Copyright (C) 2009 Wei Dong <wdong@princeton.edu>. All Rights Reserved.
  
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
 * \file embed.cpp
 * \brief Example program of set embedding with random histograms.
 *
 * This program implements the two random histogram embedding methods
 * proposed in the random histogram paper by W. Dong et al.
 *
 * The program reads feature sets from an input text file.  The file
 * is of the following format
 *
 \verbatim
 ID   N                   // ID is the identifier of the set, N is the number of features in the set
 weight   D1  D2  ...     // a weight followed by D dimensions, 1st feature
 weight   D1  D2  ...     // 2nd feature
 ...
 weight   D1  D2  ...     // Nth feature
 ID   N                   // another set
 weight   D1  D2  ...
 \endverbatim
 * ID is string which cannot contain space characters; N is positive integer; weight and the dimension
 * values are floats.
 *
 * The program embedds the input sets into single feature vectors and output
 * them in the following format
 *
 \verbatim
 ID D1  D2 ...              // The input IDs are copied to the output
 ID D1  D2 ...              // Following is the histogram, whose dimensionality is determined by the
 ...                        // input parameters.
 \endverbatim
 *
 * The user is encouraged to modify this program to customize the input and output format.
 *
 * Usage:
 *
 \verbatim
Allowed options:
  -h [ --help ]            produce help message.
  -t [ --type ] arg (=1)   embedding algorithm:
                           1 - stripe embedding [-B, -M, -N, -W],
                           2 - random hyperplane [-B, -M, -N].

  --norm                   normalize the output vector to unit length.
  -I [ --input ] arg (=-)  input file.
  -O [ --output ] arg (=-) output file.
  -D [ --dim ] arg         input dimension.
  -B [ -- ] arg (=8)       #bits per projection.
  -M [ -- ] arg (=1)       take the sum of M.
  -N [ -- ] arg (=10)      repeat N times.
  -W [ -- ] arg (=1)       for type 1 only, LSH window size.
 \endverbatim
 */


#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <lshkit.h>

/*
 The Histogram<> class in LSHKIT doesn't support polymorphism.
 Following we are going to wrap up Histogram<> with Embedder classes
 to support polymorphism, so the user can choose different
 embedder classes in command line.
*/

/// The abstract embedder.
/**
  * An Embedder class only have to implement two virtual functions: dim() and add().
  */
class Embedder {
public:
    /// The dimension of the output histogram.
    virtual unsigned dim () const = 0;
    /// Add a point to the output histogram with weight.
    virtual void add (float *out, const float *in, float weight) const = 0;
    /// Add a point to the output histogram with weight = 1.
    void add (float *out, const float *in) const {
        add(out, in , 1.0);
    }
    /// Initialize the output histogram to zeros.
    void zero (float *out) const {
        std::fill(out, out + dim(), 0);
    }
    /// Scale the output histogram by *s.
    void scale (float *out, float s) const
    {
        for (unsigned i = 0; i < dim(); i++) out[i] *= s;
    }
    /// Normalize the output histogram to a unit vector.
    void norm (float *out) const
    {
        float s = 0.0;
        for (unsigned i = 0; i < dim(); i++) s += out[i] * out[i];
        s = 1.0/sqrtf(s);
        scale(out, s);
    }
};

/// The Stripe LSH. See Section 4.1 of MM08 paper.
typedef lshkit::Repeat<lshkit::LSB<lshkit::GaussianLsh> > StripeLsh;
typedef lshkit::Histogram<StripeLsh> StripeEmbedderBase;

/// Wrapper of the histogram embedder.
class StripeEmbedder: public Embedder, public StripeEmbedderBase {
    typedef StripeEmbedderBase Base;
public:
    struct Parameter: public Base::Parameter {
        unsigned M, N;
        /**
          Inheritec parameters:
          unsigned dim;
          float W;
          unsigned repeat
         */

    };

    StripeEmbedder (const Parameter &param, lshkit::DefaultRng &rng)
        : Base(param.M, param.N, param, rng) {
    }

    virtual unsigned dim () const {
        return Base::dim();
    }

    virtual void add (float *out, const float *in, float weight) const {
        Base::add(out, const_cast<float *>(in), weight);
    }
};

/// Random hyperplane LSH. See Section 4.2 of MM08 paper.
typedef lshkit::Repeat<lshkit::HyperPlaneLsh> HyperPlaneLsh;
typedef lshkit::Histogram<HyperPlaneLsh> HyperPlaneEmbedderBase;

/// Wrapper of the histogram embedder.
class HyperPlaneEmbedder: public Embedder, public HyperPlaneEmbedderBase {
    typedef HyperPlaneEmbedderBase Base;
public:
    struct Parameter: public Base::Parameter {
        unsigned M, N;
        /**
          Inheritec parameters:
          unsigned dim;
          unsigned repeat
         */
    };

    HyperPlaneEmbedder (const Parameter &param, lshkit::DefaultRng &rng)
        : Base(param.M, param.N, param, rng) {
    }

    virtual unsigned dim () const {
        return Base::dim();
    }

    virtual void add (float *out, const float *in, float weight) const {
        Base::add(out, const_cast<float *>(in), weight);
    }
};

using namespace std;
using namespace lshkit;
namespace po = boost::program_options; 

#define EMBEDDER_STRIP  1
#define EMBEDDER_RP 2

int main (int argc, char *argv[])
{
    int type;

    string input;
    string output;

    unsigned D, B, M, N;
    float W;
    bool norm = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message.")
        ("type,t", po::value(&type)->default_value(1), "embedding algorithm:\n"
            "\t1 - stripe embedding [-B, -M, -N, -W],\n"
            "\t2 - random hyperplane [-B, -M, -N].\n"
         )
        ("norm", "normalize the output vector to unit length.")
        ("input,I", po::value(&input)->default_value("-"), "input file.")
        ("output,O", po::value(&output)->default_value("-"), "output file.")
        ("dim,D", po::value(&D), "input dimension.")
        (",B", po::value(&B)->default_value(8), "#bits per projection.")
        (",M", po::value(&M)->default_value(1), "take the sum of M.")
        (",N", po::value(&N)->default_value(10), "repeat N times.")
        (",W", po::value(&W)->default_value(1.0), "for type 1 only, LSH window size.")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    if (vm.count("help") || (vm.count("dim") < 1))
    {
        cout << desc;
        return 0;
    }

    if (vm.count("norm")) norm = true;

    DefaultRng rng;
    Embedder *emb;

    switch (type)
    {
        case 1: {
                    StripeEmbedder::Parameter param;
                    param.dim = D;
                    param.W = W;
                    param.repeat = B;
                    param.M = M;
                    param.N = N;
                    emb = new StripeEmbedder(param, rng);
                    break;
                }
        case 2: {

                    HyperPlaneEmbedder::Parameter param;
                    param.dim = D;
                    param.repeat = B;
                    param.M = M;
                    param.N = N;
                    emb = new HyperPlaneEmbedder(param, rng);
                    break;
                }
        default:
                throw invalid_argument("INVALID EMBEDDER TYPE.");
    }
    
    ifstream is(input.c_str());
    ofstream os(output.c_str());

    float *in = new float[D];
    float *out = new float[emb->dim()];

    for (;;)
    {
/* !!!!!!!!!!!!!!!!! Modify here for different input format !!!!!!!!!!!!!! */
        unsigned n;  /* number of feature vectors for the next object */
        string id;
        is >> id >> n;
        if (!is) break;
        emb->zero(out);
        // process the n vectors
        for (unsigned i = 0; i < n; ++i)
        {
            float weight;
            is >> weight;
            /* read a feature vector */
            for (unsigned j = 0; j < D; ++j) {
                is >> in[j];
            }
            emb->add(out, in, weight);
        }

/* !!!!!!!!!!!!!!!!! Modify here for different output format !!!!!!!!!!!!!! */
        if (norm) emb->norm(out);
        os << id;
        for (unsigned j = 0; j < emb->dim(); ++j) {
            os << '\t' << out[j];
        }
        os << endl;
    }

    delete emb;
    delete []in;
    delete []out;

    return 0;
}

