#include <memory>
#include <stdexcept>

#include "QueryService.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>

#include <boost/program_options.hpp>

#include "params.h"
#include "utils.h"
#include "space.h"
#include "spacefactory.h"
#include "index.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "methodfactory.h"
#include "init.h"

#define DEBUG_PRINT

#ifdef _OPENMP
#include <omp.h>
#endif


const unsigned THREAD_COEFF = 4;

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

using std::string;
using std::unique_ptr;
using std::exception;

using namespace  ::similarity;

template <class dist_t>
class QueryServiceHandler : virtual public QueryServiceIf {
 public:
  QueryServiceHandler(
                      const string&                      SpaceType,
                      const AnyParams&                   SpaceParams,
                      const string&                      DataFile,
                      unsigned                           MaxNumData,
                      const MethodWithParams&            MethodParams 
  ) :
    space_(SpaceFactoryRegistry<dist_t>::Instance().CreateSpace(SpaceType, SpaceParams))

  {
    space_->ReadDataset(dataSet_,
                       DataFile,
                        0);

    index_.reset(MethodFactoryRegistry<dist_t>::Instance().
                                CreateMethod(true /* print progress */,
                                        MethodParams.methName_,
                                        SpaceType,
                                        space_.get(),
                                        dataSet_, 
                                        MethodParams.methPars_
                                        ));

    LOG(LIB_INFO) << "The index is created!" << endl;
  }

  ~QueryServiceHandler() {
    for (auto e: dataSet_) delete e;
  }

  void knnQuery(ReplyEntryList& _return, const int32_t k, const std::string& queryObjStr, const bool retObj) {
    try {
#ifdef DEBUG_PRINT
      LOG(LIB_INFO) << "Running a " << k << "-NN query";
#endif

      unique_ptr<Object>  queryObj(space_->CreateObjFromStr(0, -1, queryObjStr, NULL));

      KNNQuery<dist_t> knn(space_.get(), queryObj.get(), k);
      index_->Search(&knn);
      unique_ptr<KNNQueue<dist_t>> res(knn.Result()->Clone());

      _return.clear();

#ifdef DEBUG_PRINT
      vector<IdType> ids;
      vector<double> dists;

      LOG(LIB_INFO) << "Results: ";
#endif

      while (!res->Empty()) {
        const Object* topObj = res->TopObject();
        dist_t topDist = res->TopDistance();

        ReplyEntry e;

        e.__set_id(topObj->id());
        e.__set_dist(topDist);
#ifdef DEBUG_PRINT
        ids.insert(ids.begin(), e.id);
        dists.insert(dists.begin(), e.dist);
#endif
        if (retObj) {
          e.__set_obj(space_->CreateStrFromObj(topObj));
        }
        _return.insert(_return.begin(), e);
        res->Pop();
      }
#ifdef DEBUG_PRINT
      for (size_t i = 0; i < ids.size(); ++i) {
       LOG(LIB_INFO) << "id=" << ids[i] << " dist=" << dists[i];
      }
#endif
    } catch (const exception& e) {
        QueryException qe;
        qe.__set_message(e.what());
        throw qe;
    } catch (...) {
        QueryException qe;
        qe.__set_message("Unknown exception");
        throw qe;
    }


  }

 private:
  unique_ptr<Space<dist_t>>   space_;
  unique_ptr<Index<dist_t>>   index_;
  ObjectVector                dataSet_; 
};

namespace po = boost::program_options;

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

void ParseCommandLineForServer(int argc, char*argv[],
                      int&                    port,
                      size_t&                 threadQty,
                      string&                 LogFile,
                      string&                 DistType,
                      string&                 SpaceType,
                      std::shared_ptr<AnyParams>&  SpaceParams,
                      string&                 DataFile,
                      unsigned&               MaxNumData,
                      std::shared_ptr<MethodWithParams>& pars) {

  string          methParams;
  size_t          defaultThreadQty = THREAD_COEFF; 
#ifdef _OPENMP
  defaultThreadQty = omp_get_max_threads() * THREAD_COEFF;
#endif


  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    ("help,h", "produce help message")
    ("port,p",          po::value<int>(&port)->required(),
                        "TCP/IP port number")
    ("threadQty",       po::value<size_t>(&threadQty)->default_value(defaultThreadQty),
                        "A number of server threads")
    ("logFile,l",       po::value<string>(&LogFile)->default_value(""),
                        "log file")
    ("spaceType,s",     po::value<string>(&SpaceType)->required(),
                        "space type, e.g., l1, l2, lp:p=0.5")
    ("distType",        po::value<string>(&DistType)->default_value(DIST_TYPE_FLOAT),
                        "distance value type: int, float, double")
    ("dataFile,i",      po::value<string>(&DataFile)->required(),
                        "input data file")
    ("maxNumData",      po::value<unsigned>(&MaxNumData)->default_value(0),
                        "if non-zero, only the first maxNumData elements are used")
    ("method,m",        po::value<string>(&methParams)->required(),
                        "one method with comma-separated parameters in the format:\n"
                        "<method name>:<param1>,<param2>,...,<paramK>")
    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(LIB_FATAL) << e.what() << endl;
  }

  if (vm.count("help")  ) {
    Usage(argv[0], ProgOptDesc);
    exit(0);
  }

  if (vm.count("method") != 1) {
    Usage(argv[0], ProgOptDesc);
    LOG(LIB_FATAL) << "There should be exactly one method specified!";
  }

  ToLower(DistType);
  ToLower(SpaceType);
  
  try {
    {
      vector<string> SpaceDesc;
      string str = SpaceType;
      ParseSpaceArg(str, SpaceType, SpaceDesc);
      SpaceParams = std::shared_ptr<AnyParams>(new AnyParams(SpaceDesc));
    }

    string          MethName;
    vector<string>  MethodDesc;
    ParseMethodArg(methParams, MethName, MethodDesc);
    pars = std::shared_ptr<MethodWithParams>(new MethodWithParams(MethName, MethodDesc));
    
    if (DataFile.empty()) {
      LOG(LIB_FATAL) << "data file is not specified!";
    }

    if (!DoesFileExist(DataFile)) {
      LOG(LIB_FATAL) << "data file " << DataFile << " doesn't exist";
    }
  } catch (const exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  }
}

int main(int argc, char *argv[]) {
  int         port = 0;
  size_t      threadQty = 0;
  string      LogFile;
  string      DistType;
  string      SpaceType;
  std::shared_ptr<AnyParams>  SpaceParams;
  string      DataFile;
  unsigned    MaxNumData;
  std::shared_ptr<MethodWithParams> MethodParams;

  ParseCommandLineForServer(argc, argv,
                      port,
                      threadQty,
                      LogFile,
                      DistType,
                      SpaceType,
                      SpaceParams,
                      DataFile,
                      MaxNumData,
                      MethodParams);

  initLibrary(LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

  ToLower(DistType);

  unique_ptr<QueryServiceIf>   queryHandler;

  if (DIST_TYPE_INT == DistType) {
    queryHandler.reset(new QueryServiceHandler<int>(
                                                    SpaceType,
                                                    *SpaceParams,
                                                    DataFile,
                                                    MaxNumData,
                                                    *MethodParams));
  } else if (DIST_TYPE_FLOAT == DistType) {
    queryHandler.reset(new QueryServiceHandler<float>(
                                                    SpaceType,
                                                    *SpaceParams,
                                                    DataFile,
                                                    MaxNumData,
                                                    *MethodParams));
  } else if (DIST_TYPE_DOUBLE == DistType) {
    queryHandler.reset(new QueryServiceHandler<double>(
                                                    SpaceType,
                                                    *SpaceParams,
                                                    DataFile,
                                                    MaxNumData,
                                                    *MethodParams));
  } else {
    LOG(LIB_FATAL) << "Unknown distance value type: " << DistType;
  }

  boost::shared_ptr<QueryServiceIf> handler(queryHandler.get());
  boost::shared_ptr<TProcessor> processor(new QueryServiceProcessor(handler));
  boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

#if SIMPLE_SERVER
  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  LOG(LIB_INFO) << "Starting a simple server.";
#else
  boost::shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(threadQty);
  boost::shared_ptr<PosixThreadFactory> threadFactory = boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  threadManager->threadFactory(threadFactory);
  threadManager->start();
  TThreadPoolServer server(processor,
                           serverTransport,
                           transportFactory,
                           protocolFactory,
                           threadManager);
  LOG(LIB_INFO) << "Starting a server with a " << threadQty << " thread-pool.";
#endif
  server.serve();
  return 0;
}

