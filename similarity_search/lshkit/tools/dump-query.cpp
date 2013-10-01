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

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message.")
        ("data,D", po::value<string>(&data_file), "data file")
        ("benchmark,B", po::value<string>(&benchmark), "benchmark file")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    FloatMatrix data(data_file);

        Benchmark<> bench;
        bench.load(benchmark);

    for (unsigned i = 0; i < bench.getQ(); ++i) {
        const float *p = data[bench.getQuery(i)];
        cout << p[0];
        for (int d = 1; d < data.getDim(); ++d) {
            cout << ' ' << p[d];
        }
        cout << endl;
    }

    return 0;
}

