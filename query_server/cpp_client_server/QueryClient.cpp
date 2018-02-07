/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2013-2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "QueryService.h"

#include "ztimer.h"
#include "params_def.h"

#include <boost/program_options.hpp>



using std::string;
using std::exception;
using std::stringstream;
using std::cerr;
using std::cout;
using std::cin;
using std::endl;
using std::unique_ptr;

using namespace  ::similarity;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace po = boost::program_options;

enum SearchType {
  kNoSearch, kKNNSearch, kRangeSearch
};

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

void ParseCommandLineForClient(int argc, char*argv[],
                      string&                 host,
                      int&                    port,
                      SearchType&             searchType,
                      int&                    k,
                      double&                 r,
                      bool&                   retExternId,
                      bool&                   retObj,
                      string&                 queryTimeParams
                      ) {
  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    (HELP_PARAM_OPT.c_str(),    HELP_PARAM_MSG.c_str())
    (PORT_PARAM_OPT.c_str(),    po::value<int>(&port)->required(), PORT_PARAM_MSG.c_str())
    (ADDR_PARAM_OPT.c_str(),    po::value<string>(&host)->required(), ADDR_PARAM_MSG.c_str())
    (KNN_PARAM_OPT.c_str(),     po::value<int>(&k), KNN_PARAM_MSG.c_str())
    (RANGE_PARAM_OPT.c_str(),   po::value<double>(&r), RANGE_PARAM_MSG.c_str())
    (QUERY_TIME_PARAMS_PARAM_OPT.c_str(), po::value<string>(&queryTimeParams)->default_value(""), QUERY_TIME_PARAMS_PARAM_MSG.c_str())
    (RET_EXT_ID_PARAM_OPT.c_str(),   RET_EXT_ID_PARAM_MSG.c_str())
    (RET_OBJ_PARAM_OPT.c_str(), RET_EXT_ID_PARAM_MSG.c_str())
    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    cerr << e.what() << endl;
    exit(1);
  }

  if (vm.count("knn") == 1) {
    if (vm.count("range") != 0) {
      cerr << "Range search is not allowed if the KNN search is specified!";
      Usage(argv[0], ProgOptDesc);
      exit(1);
    }
    searchType = kKNNSearch;
  } else if (vm.count("range") == 1) {
    if (vm.count("knn") != 0) {
      cerr << "KNN search is not allowed if the range search is specified";
      Usage(argv[0], ProgOptDesc);
      exit(1);
    }
    searchType = kRangeSearch;
  } else {
    searchType = kNoSearch;
  }

  retExternId = vm.count("retExternId") != 0;

  retObj = vm.count("retObj") != 0;

  if (vm.count("help")  ) {
    Usage(argv[0], ProgOptDesc);
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  string      host;
  int         port = 0;
  int         k;
  double      r;
  bool        retExternId;
  bool        retObj;
  SearchType  searchType;
  string      queryTimeParams;

  ParseCommandLineForClient(argc, argv,
                      host,
                      port,
                      searchType,
                      k, r,
                      retExternId,
                      retObj,
                      queryTimeParams);

  // Let's read the query from the input stream
  string        s;
  stringstream  ss;

  if (kNoSearch != searchType) {
    while (getline(cin, s)) {
      ss << s << endl;
    }
  }

  string queryObjStr = ss.str();

  boost::shared_ptr<TTransport>   socket(new TSocket(host, port));
  boost::shared_ptr<TTransport>   transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol>    protocol(new TBinaryProtocol(transport));
  QueryServiceClient              client(protocol);

  try {
    transport->open();

    try {

      if (!queryTimeParams.empty()) {
        client.setQueryTimeParams(queryTimeParams);
      }

      WallClockTimer wtm;
      wtm.reset();

      ReplyEntryList res;

      if (kKNNSearch == searchType) {
        cout << "Running a " << k << "-NN query" << endl;;
        client.knnQuery(res, k, queryObjStr, retExternId, retObj);
      } 
      if (kRangeSearch == searchType) {
        cout << "Running a range query with radius = " << r << endl;
        client.rangeQuery(res, r, queryObjStr, retExternId, retObj);
      }

      wtm.split();

      cout << "Finished in: " << wtm.elapsed() / 1e3f << " ms" << endl;

      for (auto e: res) {
        cout << "id=" << e.id << " dist=" << e.dist << ( retExternId ? " externId=" + e.externId : string("")) << endl; 
        if (retObj) cout << e.obj << endl;
      }
    } catch (const QueryException& e) {
      cerr << "Query execution error: " << e.message << endl;
      exit(1);
    }

    transport->close(); // Close transport !!!
  } catch (const TException& tx) {
    cerr << "Connection error: " << tx.what() << endl;
    exit(1);
  }

  return 0;
}

