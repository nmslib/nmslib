#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "QueryService.h"

#include <boost/program_options.hpp>

#ifdef _OPENMP
#include <omp.h>
#endif


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

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

void ParseCommandLineForClient(int argc, char*argv[],
                      string&                 host,
                      int&                    port,
                      int&                    k,
                      int&                    retObjInt
                      ) {
  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    ("help,h", "produce help message")
    ("port,p",          po::value<int>(&port)->required(),
                        "TCP/IP server port number")
    ("addr,a",          po::value<string>(&host)->required(),
                        "TCP/IP server address")
    ("knn,k",             po::value<int>(&k)->required(),
                        "TCP/IP server port number")
    ("retObj",          po::value<int>(&retObjInt)->default_value(0),
                        "Return string representation of found objects?")
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

  if (vm.count("help")  ) {
    Usage(argv[0], ProgOptDesc);
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  string      host;
  int         port = 0;
  int         k;
  int         retObjInt = 0;

  ParseCommandLineForClient(argc, argv,
                      host,
                      port,
                      k,
                      retObjInt);

  // Let's read the query from the input stream
  string        s;
  stringstream  ss;

  while (getline(cin, s)) {
    ss << s << endl;
  }

  string queryObjStr = ss.str();

  boost::shared_ptr<TTransport>   socket(new TSocket(host, port));
  boost::shared_ptr<TTransport>   transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol>    protocol(new TBinaryProtocol(transport));
  QueryServiceClient              client(protocol);

  try {
    transport->open();

    try {
      ReplyEntryList res;

      client.knnQuery(res, k, queryObjStr, retObjInt != 0);

      for (auto e: res) {
        cout << "id=" << e.id << " dist=" << e.dist << endl; 
        if (retObjInt != 0) 
        cout << e.obj << endl;
      }
    } catch (const QueryException& e) {
      cerr << "Query execution error: " << e.message << endl;
      exit(1);
    }
  } catch (const TException& tx) {
    cerr << "Connection error: " << tx.what() << endl;
    exit(1);
  }

  return 0;
}

