/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef PARAMS_H
#define PARAMS_H

#include <string>
#include <vector>
#include <limits>
#include <map>
#include <set>
#include <memory>
#include <sstream>
#include <algorithm>

#include "logging.h"
#include "utils.h"

namespace similarity {

using std::string;
using std::vector;
using std::multimap;
using std::set;
using std::stringstream;
using std::shared_ptr;
using std::unique_ptr;


#define FAKE_MAX_LEAVES_TO_VISIT std::numeric_limits<int>::max() 

class AnyParams {
public:
  /* 
   * Each element of the description array is in the form:
   * <param name>=<param value>
   */
  AnyParams(const vector<string>& Desc) :ParamNames(0), ParamValues(0) {
    set<string> seen;
    for (unsigned i = 0; i < Desc.size(); ++i) {
      vector<string>  OneParamPair;
      if (!SplitStr(Desc[i], OneParamPair, '=') ||
          OneParamPair.size() != 2) {
        LOG(LIB_FATAL) << "Wrong format of an argument: '" << Desc[i] << "' should be in the format: <Name>=<Value>";
      }
      const string& Name = OneParamPair[0];
      const string& sVal = OneParamPair[1];

      if (seen.count(Name)) {
        LOG(LIB_FATAL) << "Duplicate parameter: " << Name;
      }
      seen.insert(Name);

      ParamNames.push_back(Name);
      ParamValues.push_back(sVal);
    }
  }
  
  /* 
   * Compare parameters against parameters in the other parameter container.
   * In doing so, IGNORE parameters from the exception list.
   */
  bool equalsIgnoreInList(const AnyParams& that, const vector<string>& ExceptList) {   
   /*
    * These loops are reasonably efficient, unless
    * we have thousands of parameters in the exception
    * list (which realistically won't happen)
    */
    
    vector<pair<string, string>> vals[2], inter;
    const AnyParams*             objRefs[2] = {this, &that};


    for (size_t objId = 0; objId < 2; ++objId) {
      const AnyParams& obj = *objRefs[objId];
 
      for (size_t i = 0; i < obj.ParamNames.size(); ++i) {
        const string& name = obj.ParamNames[i];
        if (find(ExceptList.begin(), ExceptList.end(), name) == ExceptList.end()) {
          vals[objId].push_back(make_pair(name, obj.ParamValues[i]));
        }
      }      
      sort(vals[objId].begin(), vals[objId].end());
    }
    
    inter.resize(ParamNames.size() + that.ParamNames.size());
    size_t qty = set_intersection(vals[0].begin(), vals[0].end(),
                     vals[1].begin(), vals[1].end(),
                     inter.begin()) - inter.begin();
    /*
     * We computed the size of the intersection.
     * If it is equal to the size of either of the original 
     * param lists (entries in the exception lists are excluded
     * at this point), then both sets are equal.
     */
    return qty == vals[0].size() && qty == vals[1].size();    
  }

  string ToString() const {
    stringstream res;
    for (unsigned i = 0; i < ParamNames.size(); ++i) {
      if (i) res << ",";
      res << ParamNames[i] << "=" << ParamValues[i];
    }
    return res.str();
  }

  template <typename ParamType> 
  void ChangeParam(const string& Name, const ParamType& Value) {
    for (unsigned i = 0; i < ParamNames.size(); ++i) 
    if (Name == ParamNames[i]) {
      stringstream str;

      str << Value;
      ParamValues[i] = str.str();
      return;
    }
    LOG(LIB_FATAL) << "Parameter not found: " << Name;
  } 

  AnyParams(){}
  AnyParams(const vector<string>& Names, const vector<string>& Values) 
            : ParamNames(Names), ParamValues(Values) {}
  AnyParams(const AnyParams& that)
            : ParamNames(that.ParamNames), ParamValues(that.ParamValues) {}

  vector<string>  ParamNames;
  vector<string>  ParamValues;

};

class AnyParamManager {
public:
  AnyParamManager(const AnyParams& params) : params(params), seen() {
    if (params.ParamNames.size() != params.ParamValues.size()) {
      LOG(LIB_FATAL) << "Bug: different # of parameters and values";
    }
  }

  template <typename ParamType>
  void GetParamRequired(const string&  Name, ParamType& Value) {
    GetParam<ParamType>(Name, Value, true);
  }

  /*
   * The default Value should be set before calling this function.
   * If the parameter not specified, the Value remains unchanged.
   * For instance:
   *    AnyParams ParamManager(SomeParams);
   *    int val = 3;
   *    ParamManager.GetParamOptional("name", val);  
   *     // if name wasn't not present in SomeParams, val remains equal to 3.
   */
  template <typename ParamType>
  void GetParamOptional(const string&  Name, ParamType& Value) {
    GetParam<ParamType>(Name, Value, false);
  }

  /*
   * Takes a list of exceptions and extracts all parameter values, 
   * except parameters from the exceptions' list. The extracted parameters
   * are added to the list of parameters already seen.
   */
  AnyParams ExtractParametersExcept(const vector<string>& ExceptList) {
    set<string> except(ExceptList.begin(), ExceptList.end());

    vector<string> names, values;

    for (size_t i = 0; i < params.ParamNames.size(); ++i) {
      const string& name = params.ParamNames[i];
      if (except.count(name) == 0) { // Not on the exception list
        names.push_back(name);
        values.push_back(params.ParamValues[i]);
        seen.insert(name);
      }
    }

    return AnyParams(names, values);
  }

  ~AnyParamManager() {
    bool bFail = false;
    for (const auto Name: params.ParamNames) 
    if (!seen.count(Name)) {
      LOG(LIB_ERROR) << "Unknown parameter: " << Name;
      bFail = true;
    }
    if (bFail) LOG(LIB_FATAL) << "Unknown parameters found, aborting!";
  }
private:
  const AnyParams&  params;
  set<string>       seen;

  template <typename ParamType>
  void GetParam(const string&  Name, ParamType& Value, bool bRequired) {
    bool bFound = false;
    /* 
     * This loop is reasonably efficient, unless
     * we have thousands of parameters (which realistically won't happen)
     */
    for (size_t i =0; i < params.ParamNames.size(); ++i) 
    if (Name == params.ParamNames[i]) {
      bFound = true; 
      ConvertStrToValue<ParamType>(params.ParamValues[i], Value);
    }

    if (!bFound) {
      if (bRequired) {
        // Here the program terminates
        LOG(LIB_FATAL) << "Mandatory parameter: " << Name << " is missing!";
      }
    }
    //LOG(LIB_INFO) << "@@@ Parameter: " << Name << "=" << Value << " @@@";
    seen.insert(Name);
  }

  template <typename ParamType>
  void ConvertStrToValue(const string& s, ParamType& Value);
};


template <typename ParamType>
inline void AnyParamManager::ConvertStrToValue(const string& s, ParamType& Value) {
  stringstream str(s);

  if (!(str>>Value) || !str.eof()) {
    LOG(LIB_FATAL) << "Failed to convert value '" << s << "' from type: " << typeid(Value).name();
  }
}

template <>
inline void AnyParamManager::ConvertStrToValue<string>(const string& str, string& Value) {
  Value = str;
}

struct MethodWithParams {
	string			methName_;
	AnyParams		methPars_;
	MethodWithParams(const string& methName, const vector<string>& methDesc) :
					methName_(methName),
					methPars_(methDesc) {}
	MethodWithParams(const string& methName, const AnyParams& methPars) :
	                    methName_(methName),
	                    methPars_(methPars) {}				
};

void ParseCommandLine(int argc, char *av[],
                      string&                 LogFile,
                      string&                 DistType,
                      string&                 SpaceType,
                      shared_ptr<AnyParams>&  SpaceParams,
                      unsigned&               dimension,
                      unsigned&               ThreadTestQty,
                      bool&                   DoAppend, 
                      string&                 ResFilePrefix,
                      unsigned&               TestSetQty,
                      string&                 DataFile,
                      string&                 QueryFile,
                      unsigned&               MaxNumData,
                      unsigned&               MaxNumQuery,
                      vector<unsigned>&       knn,
                      float&                  eps,
                      string&                 RangeArg,
                      vector<shared_ptr<MethodWithParams>>& Methods);
};

#endif
