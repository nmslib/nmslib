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
#include <stdexcept>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>

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
#include "params_def.h"
#include "utils.h"
#include "space.h"
#include "spacefactory.h"
#include "index.h"
#include "rangequery.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "methodfactory.h"
#include "init.h"
#include "logging.h"
#include "ztimer.h"

#define MAX_SPIN_LOCK_QTY 1000000
#define SLEEP_DURATION    10


const unsigned THREAD_COEFF = 4;

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

using std::string;
using std::unique_ptr;
using std::exception;
using std::mutex;
using std::unique_lock;

using namespace  ::similarity;

class LockedCounterManager {
public:
  LockedCounterManager(int& counter, mutex& mtx) : counter_(counter), mtx_(mtx) 
  {
    unique_lock<mutex> lock(mtx_);
    ++counter_;
  }

  ~LockedCounterManager() {
    unique_lock<mutex> lock(mtx_);
    --counter_;
  }

private:
  int&     counter_; 
  mutex&   mtx_;
};

template <class dist_t>
class QueryServiceHandler : virtual public QueryServiceIf {
 public:
  QueryServiceHandler(
                      bool                               debugPrint,
                      const string&                      SpaceType,
                      const AnyParams&                   SpaceParams,
                      const string&                      DataFile,
                      unsigned                           MaxNumData,
                      const string&                      MethodName,
                      const string&                      LoadIndexLoc,
                      const string&                      SaveIndexLoc,
                      const AnyParams&                   IndexParams,
                      const AnyParams&                   QueryTimeParams) :
    debugPrint_(debugPrint),
    methName_(MethodName),
    space_(SpaceFactoryRegistry<dist_t>::Instance().CreateSpace(SpaceType, SpaceParams)),
    counter_(0)

  {
    unique_ptr<DataFileInputState> inpState(space_->ReadDataset(dataSet_,
                                                                externIds_,
                                                                DataFile,
                                                                MaxNumData));
    space_->UpdateParamsFromFile(*inpState);

    CHECK(dataSet_.size() == externIds_.size());

    index_.reset(MethodFactoryRegistry<dist_t>::Instance().
                                CreateMethod(true /* print progress */,
                                        methName_,
                                        SpaceType,
                                        *space_.get(),
                                        dataSet_));

    if (!LoadIndexLoc.empty() && DoesFileExist(LoadIndexLoc)) {
      LOG(LIB_INFO) << "Loading index from location: " << LoadIndexLoc; 
      index_->LoadIndex(LoadIndexLoc);
      LOG(LIB_INFO) << "The index is loaded!";
    } else {
      LOG(LIB_INFO) << "Creating a new index copy"; 
      index_->CreateIndex(IndexParams);      
      LOG(LIB_INFO) << "The index is created!";
    }

    if (!SaveIndexLoc.empty() && !DoesFileExist(SaveIndexLoc)) {
      LOG(LIB_INFO) << "Saving the index";
      index_->SaveIndex(SaveIndexLoc);
      LOG(LIB_INFO) << "The index is saved!";
    }

    LOG(LIB_INFO) << "Setting query-time parameters";
    index_->SetQueryTimeParams(QueryTimeParams);
  }

  ~QueryServiceHandler() {
    for (auto e: dataSet_) delete e;
  }

  void setQueryTimeParams(const string& queryTimeParamStr) {
    try {
      // Query time parameters are essentially spin-locked
      while (true) {
        for (size_t i = 0; i < MAX_SPIN_LOCK_QTY; ++i) {
          unique_lock<mutex> lock(mtx_);
          if (0 == counter_) {
            vector<string>  desc;
            ParseArg(queryTimeParamStr, desc);
            if (debugPrint_) {
              LOG(LIB_INFO) << "Setting query time parameters (" << queryTimeParamStr << ")";
              for (string s: desc) {
                LOG(LIB_INFO) << s;
              }
            }
            index_->SetQueryTimeParams(AnyParams(desc));
            return;
          }
        } // the lock will be released in the end of the block

        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION));
      }
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

  void rangeQuery(ReplyEntryList& _return, const double r, const string& queryObjStr, 
                  const bool retExternId, const bool retObj) {
    // This will increase the counter and prevent modification of query time parameters.
    LockedCounterManager  mngr(counter_, mtx_);

    try {
      if (debugPrint_) {
        LOG(LIB_INFO) << "Running a range query, r=" << r << " retExternId=" << retExternId << " retObj=" << retObj;
      }
      WallClockTimer wtm;

      wtm.reset();

      unique_ptr<Object>  queryObj(space_->CreateObjFromStr(0, -1, queryObjStr, NULL));

      RangeQuery<dist_t> range(*space_, queryObj.get(), r);
      index_->Search(&range, -1);

      _return.clear();

      wtm.split();

      if (debugPrint_) {
        LOG(LIB_INFO) << "Finished in: " << wtm.elapsed() / 1e3f << " ms";
      }

      vector<IdType> ids;
      vector<double> dists;
      vector<string> externIds;
      vector<string> objs;

     
      if (debugPrint_) { 
        LOG(LIB_INFO) << "Results: ";
      }
      
      const ObjectVector&     vResObjs  = *range.Result(); 
      const vector<dist_t>&   vResDists = *range.ResultDists();

      for (size_t i = 0; i < vResObjs.size(); ++i) {
        const Object* pObj = vResObjs[i];
        dist_t dist        = vResDists[i];

        ReplyEntry e;

        e.__set_id(pObj->id());
        e.__set_dist(dist);

        if (debugPrint_) {
          ids.insert(ids.begin(), e.id);
          dists.insert(dists.begin(), e.dist);
        }

        string externId;

        if (retExternId || retObj) {
          CHECK(e.id < externIds_.size());
          externId = externIds_[e.id];
          e.__set_externId(externId);
          externIds.insert(externIds.begin(), e.externId);
        }

        if (retObj) {
          const string& s = space_->CreateStrFromObj(pObj, externId);
          e.__set_obj(s);
          if (debugPrint_) {
            objs.insert(objs.begin(), s);
          }
        }
        _return.insert(_return.begin(), e);
      }
      if (debugPrint_) {
        for (size_t i = 0; i < ids.size(); ++i) {
          LOG(LIB_INFO) << "id=" << ids[i] << " dist=" << dists[i] << ( retExternId ? " " + externIds[i] : string(""));
          if (retObj) LOG(LIB_INFO) << objs[i]; 
        }
      }
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

  double getDistance(const std::string& objStr1, const std::string& objStr2) {
    try {
      if (debugPrint_) {
        LOG(LIB_INFO) << "Computing the distance between two objects";
      }

      WallClockTimer wtm;

      wtm.reset();

      unique_ptr<Object>  obj1(space_->CreateObjFromStr(0, -1, objStr1, NULL));
      unique_ptr<Object>  obj2(space_->CreateObjFromStr(0, -1, objStr2, NULL));

      double res = space_->IndexTimeDistance(obj1.get(), obj2.get());

      wtm.split();

      if (debugPrint_) {
        LOG(LIB_INFO) << "Result: " << res;
        LOG(LIB_INFO) << "Finished in: " << wtm.elapsed() / 1e3f << " ms ";
      }
      return res;
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

  void knnQuery(ReplyEntryList& _return, const int32_t k, 
                const std::string& queryObjStr, const bool retExternId, const bool retObj) {
    // This will increase the counter and prevent modification of query time parameters.
    LockedCounterManager  mngr(counter_, mtx_);

    try {
      if (debugPrint_) {
        LOG(LIB_INFO) << "Running a " << k << "-NN query" << " retExternId=" << retExternId << " retObj=" << retObj;
      }
      WallClockTimer wtm;

      wtm.reset();

      unique_ptr<Object>  queryObj(space_->CreateObjFromStr(0, -1, queryObjStr, NULL));

      KNNQuery<dist_t> knn(*space_, queryObj.get(), k);
      index_->Search(&knn, -1);
      unique_ptr<KNNQueue<dist_t>> res(knn.Result()->Clone());

      _return.clear();

      wtm.split();

      if (debugPrint_) {
        LOG(LIB_INFO) << "Finished in: " << wtm.elapsed() / 1e3f << " ms";
      }

      vector<IdType> ids;
      vector<double> dists;
      vector<string> objs;
      vector<string> externIds;

     
      if (debugPrint_) { 
        LOG(LIB_INFO) << "Results: ";
      }

      while (!res->Empty()) {
        const Object* topObj = res->TopObject();
        dist_t topDist = res->TopDistance();

        ReplyEntry e;

        e.__set_id(topObj->id());
        e.__set_dist(topDist);

        if (debugPrint_) {
          ids.insert(ids.begin(), e.id);
          dists.insert(dists.begin(), e.dist);
        }

        string externId;

        if (retExternId || retObj) {
          CHECK(e.id < externIds_.size());
          externId = externIds_[e.id];
          e.__set_externId(externId);
          externIds.insert(externIds.begin(), e.externId);
        }

        if (retObj) {
          const string& s = space_->CreateStrFromObj(topObj, externId);
          e.__set_obj(s);
          if (debugPrint_) {
            objs.insert(objs.begin(), s);
          }
        }
        _return.insert(_return.begin(), e);
        res->Pop();
      }
      if (debugPrint_) {
        for (size_t i = 0; i < ids.size(); ++i) {
          LOG(LIB_INFO) << "id=" << ids[i] << " dist=" << dists[i] << ( retExternId ? " " + externIds[i] : string(""));
          if (retObj) LOG(LIB_INFO) << objs[i]; 
        }
      }
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
  bool                        debugPrint_;
  string                      methName_;
  unique_ptr<Space<dist_t>>   space_;
  unique_ptr<Index<dist_t>>   index_;
  vector<string>              externIds_;
  ObjectVector                dataSet_; 

  int                         counter_; 
  mutex                       mtx_;
};

namespace po = boost::program_options;

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

void ParseCommandLineForServer(int argc, char*argv[],
                      bool&                   debugPrint,
                      string&                 LoadIndexLoc,
                      string&                 SaveIndexLoc,
                      int&                    port,
                      size_t&                 threadQty,
                      string&                 LogFile,
                      string&                 DistType,
                      string&                 SpaceType,
                      std::shared_ptr<AnyParams>&  SpaceParams,
                      string&                 DataFile,
                      unsigned&               MaxNumData,
                      string&                         MethodName,
                      std::shared_ptr<AnyParams>&     IndexTimeParams,
                      std::shared_ptr<AnyParams>&     QueryTimeParams) {
  string          methParams;
  size_t          defaultThreadQty = THREAD_COEFF * thread::hardware_concurrency();

  string          indexTimeParamStr;
  string          queryTimeParamStr;
  string          spaceParamStr;

  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    (HELP_PARAM_OPT.c_str(),          HELP_PARAM_MSG.c_str())
    (DEBUG_PARAM_OPT.c_str(),         po::bool_switch(&debugPrint), DEBUG_PARAM_MSG.c_str())
    (PORT_PARAM_OPT.c_str(),          po::value<int>(&port)->required(), PORT_PARAM_MSG.c_str())
    (THREAD_PARAM_OPT.c_str(),        po::value<size_t>(&threadQty)->default_value(defaultThreadQty), THREAD_PARAM_MSG.c_str())
    (LOG_FILE_PARAM_OPT.c_str(),      po::value<string>(&LogFile)->default_value(LOG_FILE_PARAM_DEFAULT), LOG_FILE_PARAM_MSG.c_str())
    (SPACE_TYPE_PARAM_OPT.c_str(),    po::value<string>(&spaceParamStr)->required(),                SPACE_TYPE_PARAM_MSG.c_str())
    (DIST_TYPE_PARAM_OPT.c_str(),     po::value<string>(&DistType)->default_value(DIST_TYPE_FLOAT), DIST_TYPE_PARAM_MSG.c_str())
    (DATA_FILE_PARAM_OPT.c_str(),     po::value<string>(&DataFile)->required(),                     DATA_FILE_PARAM_MSG.c_str())
    (MAX_NUM_DATA_PARAM_OPT.c_str(),  po::value<unsigned>(&MaxNumData)->default_value(MAX_NUM_DATA_PARAM_DEFAULT), MAX_NUM_DATA_PARAM_MSG.c_str())
    (METHOD_PARAM_OPT.c_str(),        po::value<string>(&MethodName)->default_value(METHOD_PARAM_DEFAULT), METHOD_PARAM_MSG.c_str())
    (LOAD_INDEX_PARAM_OPT.c_str(),    po::value<string>(&LoadIndexLoc)->default_value(LOAD_INDEX_PARAM_DEFAULT),   LOAD_INDEX_PARAM_MSG.c_str())
    (SAVE_INDEX_PARAM_OPT.c_str(),    po::value<string>(&SaveIndexLoc)->default_value(SAVE_INDEX_PARAM_DEFAULT),   SAVE_INDEX_PARAM_MSG.c_str())
    (QUERY_TIME_PARAMS_PARAM_OPT.c_str(), po::value<string>(&queryTimeParamStr)->default_value(""), QUERY_TIME_PARAMS_PARAM_MSG.c_str())
    (INDEX_TIME_PARAMS_PARAM_OPT.c_str(), po::value<string>(&indexTimeParamStr)->default_value(""), INDEX_TIME_PARAMS_PARAM_MSG.c_str())
    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(LIB_FATAL) << e.what();
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
  ToLower(spaceParamStr);
  ToLower(MethodName);
 
  try {
    {
      vector<string>     desc;
      ParseSpaceArg(spaceParamStr, SpaceType, desc);
      SpaceParams = shared_ptr<AnyParams>(new AnyParams(desc));
    }

    {
      vector<string>     desc;
      ParseArg(indexTimeParamStr, desc);
      IndexTimeParams = shared_ptr<AnyParams>(new AnyParams(desc));
    }

    {
      vector<string>  desc;

      ParseArg(queryTimeParamStr, desc);
      QueryTimeParams = shared_ptr<AnyParams>(new AnyParams(desc));
    }
    
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
  bool        debugPrint = 0;
  int         port = 0;
  size_t      threadQty = 0;
  string      LogFile;
  string      DistType;
  string      SpaceType;
  std::shared_ptr<AnyParams>  SpaceParams;
  string      DataFile;
  unsigned    MaxNumData;
  
  string                         MethodName;
  std::shared_ptr<AnyParams>     IndexParams;
  std::shared_ptr<AnyParams>     QueryTimeParams;

  string      LoadIndexLoc;
  string      SaveIndexLoc;

  ParseCommandLineForServer(argc, argv,
                      debugPrint,
                      LoadIndexLoc,
                      SaveIndexLoc,
                      port,
                      threadQty,
                      LogFile,
                      DistType,
                      SpaceType,
                      SpaceParams,
                      DataFile,
                      MaxNumData,
                      MethodName,
                      IndexParams,
                      QueryTimeParams
  );

  initLibrary(0, LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

  ToLower(DistType);

  unique_ptr<QueryServiceIf>   queryHandler;

  if (DIST_TYPE_INT == DistType) {
    queryHandler.reset(new QueryServiceHandler<int>(debugPrint,
                                                    SpaceType,
                                                    *SpaceParams,
                                                    DataFile,
                                                    MaxNumData,
                                                    MethodName,
                                                    LoadIndexLoc,
                                                    SaveIndexLoc,
                                                    *IndexParams,
                                                    *QueryTimeParams));
  } else if (DIST_TYPE_FLOAT == DistType) {
    queryHandler.reset(new QueryServiceHandler<float>(debugPrint,
                                                    SpaceType,
                                                    *SpaceParams,
                                                    DataFile,
                                                    MaxNumData,
                                                    MethodName,
                                                    LoadIndexLoc,
                                                    SaveIndexLoc,
                                                    *IndexParams,
                                                    *QueryTimeParams));
  } else if (DIST_TYPE_DOUBLE == DistType) {
    queryHandler.reset(new QueryServiceHandler<double>(debugPrint,
                                                    SpaceType,
                                                    *SpaceParams,
                                                    DataFile,
                                                    MaxNumData,
                                                    MethodName,
                                                    LoadIndexLoc,
                                                    SaveIndexLoc,
                                                    *IndexParams,
                                                    *QueryTimeParams));
  
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
  LOG(LIB_INFO) << "Started a simple server.";
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
  LOG(LIB_INFO) << "Started a server with a " << threadQty << " thread-pool.";
#endif
  server.serve();
  return 0;
}

