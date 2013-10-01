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
  * \file lsh-run.cpp
  * \brief Example of using RandomThresholding LSH index for L1 distance.
  *
  * This program is an example of using LSH index.  The LSH used is
  * Tail<RepeatHash<ThresholdingLsh> >.  The ThresholdingLSH is repeated
  * M times and then randomly hashed to a integer within the range of
  * [0, H).  The size of one hash table is H.  The LSH family approximates
  * L1 distance.
  *
  * The program reconstruct the LSH index by default.  You can give the
  * --index option to make the program save the LSH index.  The next
  * time you run the program with the same --index option, the program
  * will try to load the previously saved index.  When a saved index is
  * used, you need to make sure that the dataset and other parameters match
  * the previous run.  However, the benchmark file, Q and K can be different.
  *
\verbatim
Allowed options:
  -h [ --help ]            produce help message.
  -M [ -- ] arg (=20)
  -L [ -- ] arg (=1)       number of hash tables
  -Q [ -- ] arg (=100)     number of queries to use
  -K [ -- ] arg (=50)      number of nearest neighbors to retrieve
  -D [ --data ] arg        dataset path
  -B [ --benchmark ] arg   benchmark path
  --index arg              index file
  -H [ -- ] arg (=1017881) hash table size, use the default.
\endverbatim
  * 
  */

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
    string benchmark;
    string index_file;

    float R;
    unsigned M, L, H;
    unsigned Q, K;
    bool do_benchmark = true;
    bool use_index = false; // load the index from a file

    boost::timer timer;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message.")
        (",M", po::value<unsigned>(&M)->default_value(20), "")
        (",L", po::value<unsigned>(&L)->default_value(1), "number of hash tables")
        (",Q", po::value<unsigned>(&Q)->default_value(100), "number of queries to use")
        (",K", po::value<unsigned>(&K)->default_value(50), "number of nearest neighbors to retrieve")
        (",R", po::value<float>(&R)->default_value(numeric_limits<float>::max()), "R-NN distance range")
        ("data,D", po::value<string>(&data_file), "dataset path")
        ("benchmark,B", po::value<string>(&benchmark), "benchmark path")
        ("index", po::value<string>(&index_file), "index file")
        (",H", po::value<unsigned>(&H)->default_value(1017881), "hash table size, use the default.")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    if (vm.count("help") || (vm.count("data") < 1))
    {
        cout << desc;
        return 0;
    }

    if ((Q == 0) || (vm.count("benchmark") == 0)) {
        do_benchmark = false;
    }

    if (vm.count("index") == 1) {
        use_index = true;
    }

    cout << "LOADING DATA..." << endl;
    timer.restart();
    FloatMatrix data(data_file);
    cout << boost::format("LOAD TIME: %1%s.") % timer.elapsed() << endl;

    //typedef Tail<RepeatHash<CauchyLsh> > MyLsh;

    typedef Tail<RepeatHash<ThresholdingLsh> > MyLsh;
    typedef LshIndex<MyLsh, unsigned> Index;

    metric::l1<float> l1(data.getDim());
    FloatMatrix::Accessor accessor(data);
    Index index;

    bool index_loaded = false;

    if (use_index) {
        ifstream is(index_file.c_str(), ios_base::binary);
        if (is) {
            is.exceptions(ios_base::eofbit | ios_base::failbit | ios_base::badbit);
            cout << "LOADING INDEX..." << endl;
            timer.restart();
            index.load(is);
            BOOST_VERIFY(is);
            cout << boost::format("LOAD TIME: %1%s.") % timer.elapsed() << endl;
            index_loaded = true;
        }
    }

    if (!index_loaded) {
        // We define a short name for the MPLSH index.
        float min = numeric_limits<float>::max();
        float max = -numeric_limits<float>::max();

        for (int i = 0; i < data.getSize(); ++i) {
            for (int j = 0; j < data.getDim(); ++j) {
                if (data[i][j] > max) max = data[i][j];
                if (data[i][j] < min) min = data[i][j];
            }
        }

        Index::Parameter param;

        // Setup the parameters.  Note that L is not provided here.
        param.range = H;
        param.repeat = M;
        param.min = min;
        param.max = max;
        param.dim = data.getDim();
        DefaultRng rng;

        index.init(param, rng, L);
        // The accessor.

        // Initialize the index structure.  Note L is passed here.
        cout << "CONSTRUCTING INDEX..." << endl;

        timer.restart();
        {
            boost::progress_display progress(data.getSize());
            for (int i = 0; i < data.getSize(); ++i)
            {
                // Insert an item to the hash table.
                // Note that only the key is passed in here.
                // MPLSH will get the feature from the accessor.
                index.insert(i, data[i]);
                ++progress;
            }
        }
        cout << boost::format("CONSTRUCTION TIME: %1%s.") % timer.elapsed() << endl;

        if (use_index) {
            timer.restart();
            cout << "SAVING INDEX..." << endl;
            {
                ofstream os(index_file.c_str(), ios_base::binary);
                os.exceptions(ios_base::eofbit | ios_base::failbit | ios_base::badbit);
                index.save(os);
                BOOST_VERIFY(os);
            }
            cout << boost::format("SAVING TIME: %1%s") % timer.elapsed() << endl;
        }
    }

    if (do_benchmark) {

        Benchmark<> bench;
        cout << "LOADING BENCHMARK..." << endl;
        bench.load(benchmark);
        bench.resize(Q, K);
        cout << "DONE." << endl;

        for (unsigned i = 0; i < Q; ++i)
        {
            for (unsigned j = 0; j < K; ++j)
            {
                assert(bench.getAnswer(i)[j].key < data.getSize());
            }
        }

        cout << "RUNNING QUERIES..." << endl;

        Stat recall;
        Stat cost;

        timer.restart();
        {
            TopkScanner<FloatMatrix::Accessor, metric::l1<float> > query(accessor, l1, K, R);
            boost::progress_display progress(Q);
            for (unsigned i = 0; i < Q; ++i)
            {
                query.reset(data[bench.getQuery(i)]);
                index.query(data[bench.getQuery(i)], query);
                recall << bench.getAnswer(i).recall(query.topk());
                cost << double(query.cnt())/double(data.getSize());
                ++progress;
            }
        }
        cout << boost::format("QUERY TIME: %1%s.") % timer.elapsed() << endl;

        cout << "[RECALL] " << recall.getAvg() << " +/- " << recall.getStd() << endl;
        cout << "[COST] " << cost.getAvg() << " +/- " << cost.getStd() << endl;

    }

    return 0;
}

