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

#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <boost/format.hpp>
#include <boost/timer.hpp>
#include <lshkit.h>

/**
  * \file mplsh-run.cpp
  * \brief Example of using MPLSH.
  *
  * This program is an example of using MPLSH index.
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
  -h [ --help ]                   produce help message.
  -W [ -- ] arg (=1)
  -M [ -- ] arg (=1)
  -T [ -- ] arg (=1)              # probes
  -L [ -- ] arg (=1)              # hash tables
  -Q [ -- ] arg (=100)            # queries
  -K [ -- ] arg (=50)             # nearest neighbor to retrieve
  -R [ -- ] arg (=3.40282347e+38) R-NN distance range
  --recall arg                    desired recall
  -D [ --data ] arg               data file
  -B [ --benchmark ] arg          benchmark file
  --index arg                     index file
  -H [ -- ] arg (=1017881)        hash table size, use the default value.
\endverbatim
  */

using namespace std;
using namespace lshkit;
namespace po = boost::program_options; 

/*
    You must provide an access class to query the MPLSH.
    MPLSH only saves keys (pointers to the real feature vectors) in the
    hash tables and it will rely on the accessor class to retrieve
    the feature vector.

    An accessor must provide three methods:

        bool mark (unsigned key);

        mark that key has been accessed.  If key has already been marked, return false,
        otherwise return true.  MPLSH will use this to avoid scanning the data more than
        one time per query.

        void reset ();

        to clear all the marks.
        
        const float *operator () (unsigned key);

        given a key, return the pointer to a feature vector.

    The MatrixAccessor class is used to access feature vectors stored in a Matrix.
*/

/* This class has been merged to include/lshkit/matrix.h */
/*
class MatrixAccessor
{
    const Matrix<float> &matrix_;
    boost::dynamic_bitset<> flags_;
public:
    typedef unsigned Key;
    MatrixAccessor(const Matrix<float> &matrix)
        : matrix_(matrix), flags_(matrix.getSize()) {}

    MatrixAccessor(const MatrixAccessor &accessor)
        : matrix_(accessor.matrix_), flags_(matrix_.getSize()) {}

    void reset ()
    {
        flags_.reset();
    }

    bool mark (unsigned key)
    {
        if (flags_[key]) return false;
        flags_.set(key);
        return true;
    }

    const float *operator () (unsigned key)
    {
        return matrix_[key];
    }
};
*/

int main (int argc, char *argv[])
{
    string data_file;
    string benchmark;
    string index_file;

    float W, R, desired_recall = 1.0;
    unsigned M, L, H;
    unsigned Q, K, T;
    bool do_recall = false;
    bool do_benchmark = true;
    bool use_index = false; // load the index from a file

    boost::timer timer;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message.")
        (",W", po::value<float>(&W)->default_value(1.0), "")
        (",M", po::value<unsigned>(&M)->default_value(1), "")
        (",T", po::value<unsigned>(&T)->default_value(1), "# probes")
        (",L", po::value<unsigned>(&L)->default_value(1), "# hash tables")
        (",Q", po::value<unsigned>(&Q)->default_value(100), "# queries")
        (",K", po::value<unsigned>(&K)->default_value(0), "# nearest neighbor to retrieve")
        ("radius,R", po::value<float>(&R)->default_value(numeric_limits<float>::max()), "R-NN distance range (L2)")
        ("recall", po::value<float>(&desired_recall), "desired recall")
        ("data,D", po::value<string>(&data_file), "data file")
        ("benchmark,B", po::value<string>(&benchmark), "benchmark file")
        ("index", po::value<string>(&index_file), "index file")
        (",H", po::value<unsigned>(&H)->default_value(1017881), "hash table size, use the default value.")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    if (vm.count("help") || (vm.count("data") < 1))
    {
        cout << desc;
        return 0;
    }

    if (vm.count("radius") >= 1) {
        R *= R; // we use L2sqr in the program.
    }

    if (vm.count("recall") >= 1)
    {
        do_recall = true;
        if (K == 0) {
            cerr << "Automatic probing does not support R-NN query." << endl;
        }
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

    typedef MultiProbeLshIndex<unsigned> Index;

    FloatMatrix::Accessor accessor(data);
    Index index;

    // try loading index
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
        Index::Parameter param;

        // Setup the parameters.  Note that L is not provided here.
        param.W = W;
        param.range = H; // See H in the program parameters.  You can just use the default value.
        param.repeat = M;
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
        metric::l2sqr<float> l2sqr(data.getDim());
        TopkScanner<FloatMatrix::Accessor, metric::l2sqr<float> > query(accessor, l2sqr, K, R);
        vector<Topk<unsigned> > topks(Q);

        timer.restart();
        if (do_recall)
            // Specify the required recall
            // and let MPLSH to guess how many bins to probe.
        {
            boost::progress_display progress(Q);
            for (unsigned i = 0; i < Q; ++i)
            {
                // Query for one point.
                query.reset(data[bench.getQuery(i)]);
                index.query_recall(data[bench.getQuery(i)], desired_recall, query);
                cost << double(query.cnt())/double(data.getSize());
                topks[i].swap(query.topk());
                ++progress;
            }
        }
        else
            // specify how many bins to probe.
        {
            boost::progress_display progress(Q);
            for (unsigned i = 0; i < Q; ++i)
            {
                query.reset(data[bench.getQuery(i)]);
                index.query(data[bench.getQuery(i)], T, query);
                cost << double(query.cnt())/double(data.getSize());
                topks[i].swap(query.topk());
                ++progress;
            }
        }

        for (unsigned i = 0; i < Q; ++i) {
            recall << bench.getAnswer(i).recall(topks[i]);
        }

        cout << boost::format("QUERY TIME: %1%s.") % timer.elapsed() << endl;

        cout << "[RECALL] " << recall.getAvg() << " +/- " << recall.getStd() << endl;
        cout << "[COST] " << cost.getAvg() << " +/- " << cost.getStd() << endl;

    }

    return 0;
}

