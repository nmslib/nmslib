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

const bool PRINT_PROGRESS=true;

#define raise PyException ex;\
              ex.stream()

using namespace similarity;

using IntVector = std::vector<int>;
using FloatVector = std::vector<float>;
using StringVector = std::vector<std::string>;

static PyMethodDef nmslibMethods[] = {
  {"init", init, METH_VARARGS},
  {"addDataPoint", addDataPoint, METH_VARARGS},
  {"createIndex", createIndex, METH_VARARGS},
  {"saveIndex", saveIndex, METH_VARARGS},
  {"loadIndex", loadIndex, METH_VARARGS},
  {"setQueryTimeParams", setQueryTimeParams, METH_VARARGS},
  {"knnQuery", knnQuery, METH_VARARGS},
  {"getDataPoint", getDataPoint, METH_VARARGS},
  {"getDataPointQty", getDataPointQty, METH_VARARGS},
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
  
  virtual void CreateIndex(const AnyParams&) = 0;
  virtual void SaveIndex(const string&) = 0;
  virtual void LoadIndex(const string&) = 0;
  virtual void SetQueryTimeParams(const AnyParams&) = 0;
  virtual void AddDataPoint(const Object* z) { data_.push_back(z); }
  virtual PyObject* GetDataPoint(size_t index) = 0;
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
        index_(nullptr),
        space_(nullptr) 
  {
    space_ = SpaceFactoryRegistry<T>::Instance()
        .CreateSpace(space_type_.c_str(), space_param);
  }

  ~IndexWrapper() {
    delete space_;
    delete index_;
  }


  void CreateIndex(const AnyParams& index_params) override {
Py_BEGIN_ALLOW_THREADS
    delete index_;
    index_ = MethodFactoryRegistry<T>::Instance()
        .CreateMethod(PRINT_PROGRESS,
                      method_name_, space_type_,
                      *space_, data_);
    index_->CreateIndex(index_params);
Py_END_ALLOW_THREADS
  }

  void SaveIndex(const string& fileName) override {
    index_->SaveIndex(fileName);
  }

  void LoadIndex(const string& fileName) override {
    delete index_;
    index_ = MethodFactoryRegistry<T>::Instance()
        .CreateMethod(PRINT_PROGRESS,
                      method_name_, space_type_,
                      *space_, data_);
    index_->LoadIndex(fileName);
  }

  void SetQueryTimeParams(const AnyParams& p) override {
    index_->SetQueryTimeParams(p);
  }

  unique_ptr<Object> CreateObjFromStr(const string& s, int id) override {
    return space_->CreateObjFromStr(id, -1,  s, NULL);
  }

  PyObject* KnnQuery(int k, const Object* query) override {
    IntVector ids;
Py_BEGIN_ALLOW_THREADS
    KNNQuery<T> knn(*space_, query, k);
    index_->Search(&knn, -1);
    KNNQueue<T>* res = knn.Result()->Clone();
    while (!res->Empty()) {
      ids.insert(ids.begin(), res->TopObject()->id());
      res->Pop();
    }
    delete res;
Py_END_ALLOW_THREADS
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

  PyObject* GetDataPoint(size_t index) override {
    const Object* obj = data_.at(index);
    unique_ptr<char[]> str_copy;
Py_BEGIN_ALLOW_THREADS
    string str = space_->CreateStrFromObj(obj, ConvertToString(obj->id()));
    str_copy.reset(new char[str.size()+1]);
    memcpy(str_copy.get(), str.c_str(), str.size()+1);
Py_END_ALLOW_THREADS
    PyObject* v = PyString_FromString(str_copy.get());
    if (!v) {
      return NULL;
    }
    return v;
  }

 private:
  Index<T>* index_;
  Space<T>* space_;
};

template <typename T>
PyObject* _init(DistType dist_type,
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

PyObject* init(PyObject* self, PyObject* args) {
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
      return _init<float>(
          dist_type, 
          space_type, space_param,
          method_name);
    case kDistInt:
      return _init<int>(
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
  int32_t   id;
  PyObject* dataPoint;

  if (!PyArg_ParseTuple(args, "OiO", &ptr, &id, &dataPoint)) {
    raise << "Error reading parameters (expecting: index ref, object (as a string))";
    return NULL;
  }

  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  unique_ptr<Object> obj;

Py_BEGIN_ALLOW_THREADS

  auto res = ReadString(dataPoint);

  if (!res.first) {
    raise << "Cannot convert an argument to a string";
    return NULL;
  }

  obj.reset(pIndex->CreateObjFromStr(res.second, id).release());

Py_END_ALLOW_THREADS

  pIndex->AddDataPoint(obj.release());
  Py_INCREF(Py_None);
  return Py_None;
}


PyObject* createIndex(PyObject* self, PyObject* args) {
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
  pIndex->CreateIndex(index_params);
  Py_RETURN_NONE;
}

PyObject* saveIndex(PyObject* self, PyObject* args) {
  PyObject*     ptr;
  char*         file_name;

  if (!PyArg_ParseTuple(args, "Os", &ptr, &file_name)) {
    raise << "Error reading parameters (expecting: index ref, file name)";
    return NULL;
  }

  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  pIndex->SaveIndex(file_name);
  Py_RETURN_NONE;
}

PyObject* loadIndex(PyObject* self, PyObject* args) {
  PyObject*     ptr;
  char*         file_name;

  if (!PyArg_ParseTuple(args, "Os", &ptr, &file_name)) {
    raise << "Error reading parameters (expecting: index ref, file name)";
    return NULL;
  }

  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  pIndex->LoadIndex(file_name);
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

PyObject* getDataPoint(PyObject* self, PyObject* args) {
  PyObject* ptr;
  int       index;
  if (!PyArg_ParseTuple(args, "Oi", &ptr, &index)) {
    raise << "Error reading parameters (expecting: index ref, object index)";
    return NULL;
  }
  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  if (index < 0 || static_cast<size_t>(index) >= pIndex->GetDataPointQty()) {
    raise << "The data point index should be >= 0 & < " << pIndex->GetDataPointQty();
    return NULL;
  }
  return pIndex->GetDataPoint(index);
}

PyObject* getDataPointQty(PyObject* self, PyObject* args) {
  PyObject* ptr;
  if (!PyArg_ParseTuple(args, "O", &ptr)) {
    raise << "Error reading parameters (expecting: index ref)";
    return NULL;
  }
  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  PyObject* tmp = PyInt_FromLong(pIndex->GetDataPointQty());
  if (tmp == NULL) {
    return NULL;
  }
  return tmp;
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

