/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef METHOD_FACTORY_H
#define METHOD_FACTORY_H

#include <string>
#include <iostream>

#include "index.h"
#include "space.h"
#include "logging.h"
#include "params.h"

namespace similarity {

using std::string;

#define REGISTER_METHOD_CREATOR(type, name, func)\
  namespace MethFact##type##name##func {\
    struct DummyStruct { DummyStruct()\
      { MethodFactoryRegistry<type>::Instance().Register(name, func); }\
    };\
    DummyStruct DummyVar;\
  }
 

template <typename dist_t>
class MethodFactoryRegistry {
public:
  typedef Index<dist_t>* (*CreateFuncPtr)(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& MethPars);

  static MethodFactoryRegistry& Instance() {
    static MethodFactoryRegistry elem;

    return elem;
  }

  void Register(const string& MethodName, CreateFuncPtr func) {
    LOG(INFO) << "Registering at the factory, method: " << MethodName << " distance type: " << DistTypeName<dist_t>();
    Creators_[MethodName] = func;
  }

  Index<dist_t>* CreateMethod(bool PrintProgress,
                            const string& MethName,
                            const string& SpaceType,
                            const Space<dist_t>* space,
                            const ObjectVector& DataObjects,
                            const AnyParams& MethPars) {
    if (Creators_.count(MethName)) {
      return Creators_[MethName](PrintProgress, SpaceType, space, DataObjects, MethPars);
    } else {
      LOG(FATAL) << "It looks like the method " << MethName << 
                    " is not defined for the distance type : " << DistTypeName<dist_t>();
    }
    return NULL;
  }
private:
  map<string, CreateFuncPtr>  Creators_;
};

}


#endif
