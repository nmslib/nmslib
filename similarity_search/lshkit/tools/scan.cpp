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
 * \file scan.cpp
 * \brief Linear scan dataset and construct benchmark.
 *
 * This program randomly picks Q points from a dataset as queries, and
 * then linear-scan the database to find K-NN/R-NN for each query to produce
 * a benchmark file.  For each query, the query point itself is excluded
 * from the K-NN/R-NN list.
 *
 * You can specify both K and R and the prorgram will search for the K
 * points closest to queries which are within distance range of R.
 * If K = 0, then all points within distance range of R are returned.
 * The default value of R is the maximal value of float.
 *
\verbatim
Allowed options:
  -h [ --help ]          produce help message.
  -Q [ -- ] arg (=1)     number of queries to sample.
  -K [ -- ] arg (=1)     number of nearest neighbors.
  --metric arg (=2)      1: L1; 2: L2
  -D [ --data ] arg      dataset path
  -B [ --benchmark ] arg output benchmark file path
\endverbatim
 *
 */
const char *help = "This program searches for K-NNs by linear scan and generate a benchmark file.";

#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <boost/format.hpp>
#include <boost/timer.hpp>
#include <lshkit.h>

using namespace std;
using namespace lshkit;
namespace po = boost::program_options; 


int main (int argc, char *argv[])
{
    string data_file;
    string query_file;

    unsigned K, Q, metric, seed;
    float R;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message.")
        (",Q", po::value<unsigned>(&Q)->default_value(1), "number of queries to sample.")
        (",K", po::value<unsigned>(&K)->default_value(0), "number of nearest neighbors.")
        (",R", po::value<float>(&R)->default_value(numeric_limits<float>::max()), "distance range to search for")
        ("seed", po::value<unsigned>(&seed)->default_value(0), "random number seed, 0 to use default.")
        ("metric", po::value<unsigned>(&metric)->default_value(2), "1: L1; 2: L2")
        ("data,D", po::value<string>(&data_file), "dataset path")
        ("benchmark,B", po::value<string>(&query_file), "output benchmark file path")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    if (vm.count("help") || (vm.count("data") < 1) || (vm.count("benchmark") < 1))
    {
        cout << help << endl;
        cout << desc;
        return 0;
    }

    Matrix<float> data(data_file);

    Benchmark<unsigned> bench;
    bench.init(Q, data.getSize(), seed);
    boost::timer timer;

    timer.restart();
    if (metric == 1) {

        metric::l1<float> l1(data.getDim());

        boost::progress_display progress(Q);
        for (unsigned i = 0; i < Q; ++i)
        {
            int q = static_cast<int>(bench.getQuery(i));
            Topk<unsigned> &topk = bench.getAnswer(i);
            topk.reset(K, R);
            for (int j = 0; j < data.getSize(); ++j)
            {
                if (q == j) continue;
                topk << Topk<unsigned>::Element(j, l1(data[q],
                                        data[j]));
            }
            ++progress;
        }
    }
    else if (metric == 2) {

        metric::l2<float> l2(data.getDim());

        boost::progress_display progress(Q);
        for (unsigned i = 0; i < Q; ++i)
        {
            int q = static_cast<int>(bench.getQuery(i));
            Topk<unsigned> &topk = bench.getAnswer(i);
            topk.reset(K, R);
            for (int j = 0; j < data.getSize(); ++j)
            {
                if (q == j) continue;
                topk << Topk<unsigned>::Element(j, l2(data[q],
                                        data[j]));
            }
            ++progress;
        }
    }
    else {
        cout << "METRIC NOT SUPPORTED." << endl;
    }

    cout << boost::format("QUERY TIME: %1%s.") % timer.elapsed() << endl;
    bench.save(query_file);

    //timer.tuck("QUERY");

    return 0;
}

