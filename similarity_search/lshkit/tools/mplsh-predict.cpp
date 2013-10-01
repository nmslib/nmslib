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
#include <lshkit.h>

/**
  * \file mplsh-predict.cpp
  * \brief Predict MPLSH performance.
  *
  * This program predicts MPLSH performance based on the statistical data
  * gathered by fitdata and the MPLSH parameters.  It extrapolates
  * the performance to a dataset of N points.
  *
  * Either -K or -r, but not both should be used.
  *
\verbatim
Allowed options:
  -h [ --help ]           produce help message.
  -T [ -- ] arg (=1)
  -L [ -- ] arg (=1)
  -M [ -- ] arg (=1)
  -W [ -- ] arg (=1)
  -K [ --topk ] arg (=50)
  -r [ --radius ] arg     radius
  -N [ --size ] arg       size of dataset
  -P [ --param ] arg      data parameter file
\endverbatim
  */

using namespace std;
using namespace lshkit;
namespace po = boost::program_options; 

int main (int argc, char *argv[])
{
    double w;
    unsigned N, K, T, L, M;
    double R;
    string data_param;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message.")
        (",T", po::value<unsigned>(&T)->default_value(1), "")
        (",L", po::value<unsigned>(&L)->default_value(1), "")
        (",M", po::value<unsigned>(&M)->default_value(1), "")
        (",W", po::value<double>(&w)->default_value(1), "")
        ("topk,K", po::value<unsigned>(&K)->default_value(50), "")
        ("radius,r", po::value<double>(&R)->default_value(1.0), "")
        ("size,N", po::value<unsigned>(&N), "size of dataset")
        ("param,P", po::value<string>(&data_param), "data parameter file")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    if (vm.count("help") || (vm.count("param") < 1) || 
            (vm.count("topk") < 1) || (vm.count("size") < 1))
    {
        cout << desc;
        return 0;
    }

    DataParam param(data_param);
    param.scale(w * w);

    MultiProbeLshDataModel model(param, N, K);
    model.setT(T);
    model.setL(L);
    model.setM(M);
    model.setW(1.0);

    double recall = vm.count("radius") > 0 ? model.recall(R) : model.avgRecall();

    cout << recall << '\t' << model.cost() << endl;

    return 0;
}

