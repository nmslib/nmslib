/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2015
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <Python.h>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <utility>
#include "space.h"
#include "init.h"
#include "index.h"
#include "params.h"
#include "rangequery.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "methodfactory.h"
#include "spacefactory.h"
#include "ztimer.h"
#include "logging.h"
#include "nmslib_generic.h"

#define raise PyException ex;\
              ex.stream()

using namespace similarity;

using IntVector = std::vector<int>;
using FloatVector = std::vector<float>;
using StringVector = std::vector<std::string>;

static PyMethodDef nmslibMethods[] = {
  {"initIndex", initIndex, METH_VARARGS},
  {"addDataPoint", addDataPoint, METH_VARARGS},
  {"buildIndex", buildIndex, METH_VARARGS},
  {"setQueryTimeParams", setQueryTimeParams, METH_VARARGS},
  {"knnQuery", knnQuery, METH_VARARGS},
  {"freeIndex", freeIndex, METH_VARARGS},
  {NULL, NULL}
};

struct NmslibData {
  PyObject_HEAD
};

struct NmslibDist {
  PyObject_HEAD
};

static PyTypeObject NmslibDist_Type = {
  PyObject_HEAD_INIT(NULL)
};

using BoolString = std::pair<bool,string>;

enum DistType {kDistFloat = 4, kDistInt = 5};

const std::map<std::string, DistType> NMSLIB_DIST_TYPES = {
  {"FLOAT", kDistFloat},
  {"INT", kDistInt}
};

PyMODINIT_FUNC initnmslib_generic() {
  PyObject* module = Py_InitModule("nmslib_generic", nmslibMethods);
  if (module == NULL) {
    return;
  }
  // dist type
  NmslibDist_Type.tp_new = PyType_GenericNew;
  NmslibDist_Type.tp_name = "nmslib_generic.DistType";
  NmslibDist_Type.tp_basicsize = sizeof(NmslibDist);
  NmslibDist_Type.tp_flags = Py_TPFLAGS_DEFAULT;
  NmslibDist_Type.tp_dict = PyDict_New();
  for (auto t : NMSLIB_DIST_TYPES) {
    PyObject* tmp = PyInt_FromLong(t.second);
    PyDict_SetItemString(NmslibDist_Type.tp_dict, t.first.c_str(), tmp);
    Py_DECREF(tmp);
  }
  if (PyType_Ready(&NmslibDist_Type) < 0) {
    return;
  }
  Py_INCREF(&NmslibDist_Type);
  PyModule_AddObject(module, "DistType",
        reinterpret_cast<PyObject*>(&NmslibDist_Type));
  initLibrary(LIB_LOGSTDERR, NULL);
  //initLibrary(LIB_LOGNONE, NULL);
}

class PyException {
 public:
  ~PyException() {
    PyErr_SetString(PyExc_ValueError, ss_.str().c_str());
  }
  std::stringstream& stream() { return ss_; }
 private:
  std::stringstream ss_;
};

template <typename T, typename F>
bool readList(PyListObject* lst, std::vector<T>& z, F&& f) {
  PyErr_Clear();
  for (int i = 0; i < PyList_GET_SIZE(lst); ++i) {
    auto value = f(PyList_GET_ITEM(lst, i));
    if (PyErr_Occurred()) {
      raise << "failed to read item from list";
      return false;
    }
    z.push_back(value);
  }
  return true;
}

BoolString ReadString(PyObject* data) {
  if (!PyString_Check(data)) {
    raise << "expected DataType.String";
    return std::make_pair(false, nullptr);
  }
  const char* s = PyString_AsString(data);
  return std::make_pair(true, s);
}

class IndexWrapperBase {
 public:
  IndexWrapperBase(DistType dist_type, 
               const char* space_type,
               const char* method_name)
      : dist_type_(dist_type),
        space_type_(space_type),
        method_name_(method_name) {
  }

  inline int GetDistType() { return dist_type_; }
  inline size_t GetDataPointQty() { return data_.size(); }
  
  virtual void Build(const AnyParams&) = 0;
  virtual void SetQueryTimeParams(const AnyParams&) = 0;
  virtual void AddDataPoint(const Object* z) { data_.push_back(z); }
  virtual PyObject* KnnQuery(int k, const Object* query) = 0;
  virtual unique_ptr<Object> CreateObjFromStr(const string& s, int id) = 0;

  virtual ~IndexWrapperBase() {
    for (auto p : data_) {
      delete p;
    }
  }
protected:
  const DistType dist_type_;
  const std::string space_type_;
  const std::string method_name_;
  ObjectVector data_;
};

template <typename T>
class IndexWrapper : public IndexWrapperBase {
 public:
  IndexWrapper(DistType dist_type, 
               const char* space_type,
               const AnyParams& space_param,
               const char* method_name)
      : IndexWrapperBase(dist_type, space_type, method_name),
        index_(nullptr) {
    space_ = SpaceFactoryRegistry<T>::Instance()
        .CreateSpace(space_type_.c_str(), space_param);
  }

  ~IndexWrapper() {
    delete space_;
    delete index_;
  }


  void Build(const AnyParams& index_params) {
    index_ = MethodFactoryRegistry<T>::Instance()
        .CreateMethod(true /* let's print progress bars for now */, 
                      method_name_, space_type_,
                      *space_, data_);
    index_->CreateIndex(index_params);
  }

  void SetQueryTimeParams(const AnyParams& p) {
    index_->SetQueryTimeParams(p);
  }

  unique_ptr<Object> CreateObjFromStr(const string& s, int id) {
    return space_->CreateObjFromStr(id, -1,  s, NULL);
  }

  PyObject* KnnQuery(int k, const Object* query) {
    KNNQuery<T> knn(*space_, query, k);
    index_->Search(&knn, -1);
    KNNQueue<T>* res = knn.Result()->Clone();
    IntVector ids;
    while (!res->Empty()) {
      ids.insert(ids.begin(), res->TopObject()->id());
      res->Pop();
    }
    delete res;
    PyObject* z = PyList_New(ids.size());
    if (!z) {
      return NULL;
    }
    for (int i = static_cast<int>(ids.size())-1; i >= 0; --i) {
      PyObject* v = PyInt_FromLong(ids[i]);
      if (!v) {
        Py_DECREF(z);
        return NULL;
      }
      PyList_SetItem(z, i, v);
    }
    return z;
  }

 private:
  Index<T>* index_;
  Space<T>* space_;
};

template <typename T>
PyObject* _initIndex(DistType dist_type,
                     const char* space_type,
                     const AnyParams& space_param,
                     const char* method_name) {
  IndexWrapper<T>* index(new IndexWrapper<T>(
                          dist_type, space_type, space_param,
                          method_name));
  if (!index) {
    raise << "failed to create IndexWrapper";
    return NULL;
  }
  return PyLong_FromVoidPtr(reinterpret_cast<void*>(static_cast<IndexWrapperBase*>(index)));
}

PyObject* initIndex(PyObject* self, PyObject* args) {
  char* space_type;
  PyListObject* space_param_list;
  char* method_name;
  DistType dist_type;
  if (!PyArg_ParseTuple(args, "sO!si",
          &space_type, &PyList_Type, &space_param_list, 
          &method_name, &dist_type)) {
    raise << "Error reading parameters (expecting: space type, space parameter list, index/method name, distance value type";
    return NULL;
  }

  StringVector space_param;
  if (!readList(space_param_list, space_param, PyString_AsString)) {
    return NULL;
  }

  switch (dist_type) {
    case kDistFloat:
      return _initIndex<float>(
          dist_type, 
          space_type, space_param,
          method_name);
    case kDistInt:
      return _initIndex<int>(
          dist_type, 
          space_type, space_param,
          method_name);
    default:
      raise << "unknown dist type - " << dist_type;
      return NULL;
  }
}

PyObject* addDataPoint(PyObject* self, PyObject* args) {
  PyObject* ptr;
  PyObject* dataPoint;

  if (!PyArg_ParseTuple(args, "OO", &ptr, &dataPoint)) {
    raise << "Error reading parameters (expecting: index ref, object (as a string))";
    return NULL;
  }

  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));

  auto res = ReadString(dataPoint);

  if (!res.first) {
    raise << "Cannot convert an argument to a string";
    return NULL;
  }

  IdType id = pIndex->GetDataPointQty(); 
  pIndex->AddDataPoint(pIndex->CreateObjFromStr(res.second, id).release());
  Py_INCREF(Py_None);
  return Py_None;
}


PyObject* buildIndex(PyObject* self, PyObject* args) {
  PyObject*     ptr;
  PyListObject* param_list;

  if (!PyArg_ParseTuple(args, "OO!", &ptr, &PyList_Type, &param_list)) {
    raise << "Error reading parameters (expecting: index ref, parameter list)";
     return NULL;
   }

  StringVector index_params;
  if (!readList(param_list, index_params, PyString_AsString)) {
    raise << "Cannot convert an argument to a list";
    return NULL;
  }

  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  pIndex->Build(index_params);
  Py_RETURN_NONE;
}

PyObject* setQueryTimeParams(PyObject* self, PyObject* args) {
  PyListObject* param_list;
  PyObject*     ptr;

  if (!PyArg_ParseTuple(args, "OO!", &ptr, &PyList_Type, &param_list)) {
    raise << "Error reading parameters (expecting: index ref, parameter list)";
    return NULL;
  }

  StringVector query_time_params;
  if (!readList(param_list, query_time_params, PyString_AsString)) {
    raise << "Cannot convert an argument to a list";
    return NULL;
  }

  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  pIndex->SetQueryTimeParams(query_time_params);

  return Py_None;
}

PyObject* knnQuery(PyObject* self, PyObject* args) {
  PyObject* ptr;
  int k;
  PyObject* data;
  if (!PyArg_ParseTuple(args, "OiO", &ptr, &k, &data)) {
    raise << "Error reading parameters (expecting: index ref, K as in-KNN, query)";
    return NULL;
  }
  if (k < 1) {
    raise << "K (" << k << ") should be >=1";
    return NULL;
  }

  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));

  auto res = ReadString(data);
  if (!res.first) {
    return NULL;
  }
  std::unique_ptr<const Object> query_obj(pIndex->CreateObjFromStr(res.second, 0));
  return pIndex->KnnQuery(k, query_obj.get());
}

PyObject* freeIndex(PyObject* self, PyObject* args) {
  PyObject* ptr;
  if (!PyArg_ParseTuple(args, "O", &ptr)) {
    return NULL;
  }
  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  delete pIndex;
  Py_RETURN_NONE;
}

