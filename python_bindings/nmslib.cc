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
#include "nmslib.h"

#define raise PyException ex;\
              ex.stream()

using namespace similarity;

using IntVector = std::vector<int>;
using FloatVector = std::vector<float>;
using StringVector = std::vector<std::string>;

static PyMethodDef nmslibMethods[] = {
  {"initIndex", initIndex, METH_VARARGS},
  {"setData", setData, METH_VARARGS},
  {"buildIndex", buildIndex, METH_VARARGS},
  {"knnQuery", knnQuery, METH_VARARGS},
  {"freeIndex", freeIndex, METH_VARARGS},
  {NULL, NULL}
};

struct NmslibData {
  PyObject_HEAD
};

static PyTypeObject NmslibData_Type = {
  PyObject_HEAD_INIT(NULL)
};

struct NmslibDist {
  PyObject_HEAD
};

static PyTypeObject NmslibDist_Type = {
  PyObject_HEAD_INIT(NULL)
};

using BoolObject = std::pair<bool,const Object*>;
typedef BoolObject (*DataParserFunc)(PyObject*,int,int);
BoolObject readVector(PyObject* data, int id, int dist_type);
BoolObject readString(PyObject* data, int id, int dist_type);
BoolObject readSqfd(PyObject* data, int id, int dist_type);

const int kDataVector = 1;
const int kDataString = 2;
const int kDataSqfd = 3;
const std::map<std::string, int> NMSLIB_DATA_TYPES = {
  {"VECTOR", kDataVector},
  {"STRING", kDataString},
  {"SQFD", kDataSqfd}
};
const std::map<int, DataParserFunc> NMSLIB_DATA_PARSERS = {
  {kDataVector, &readVector},
  {kDataString, &readString},
  {kDataSqfd, &readSqfd}
};

const int kDistFloat = 4;
const int kDistInt = 5;
const std::map<std::string, int> NMSLIB_DIST_TYPES = {
  {"FLOAT", kDistFloat},
  {"INT", kDistInt}
};

PyMODINIT_FUNC initnmslib() {
  PyObject* module = Py_InitModule("nmslib", nmslibMethods);
  if (module == NULL) {
    return;
  }
  // data type
  NmslibData_Type.tp_new = PyType_GenericNew;
  NmslibData_Type.tp_name = "nmslib.DataType";
  NmslibData_Type.tp_basicsize = sizeof(NmslibData);
  NmslibData_Type.tp_flags = Py_TPFLAGS_DEFAULT;
  NmslibData_Type.tp_dict = PyDict_New();
  for (auto t : NMSLIB_DATA_TYPES) {
    PyObject* tmp = PyInt_FromLong(t.second);
    PyDict_SetItemString(NmslibData_Type.tp_dict, t.first.c_str(), tmp);
    Py_DECREF(tmp);
  }
  if (PyType_Ready(&NmslibData_Type) < 0) {
    return;
  }
  Py_INCREF(&NmslibData_Type);
  PyModule_AddObject(module, "DataType",
        reinterpret_cast<PyObject*>(&NmslibData_Type));
  // dist type
  NmslibDist_Type.tp_new = PyType_GenericNew;
  NmslibDist_Type.tp_name = "nmslib.DistType";
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

BoolObject readVector(PyObject* data, int id, int dist_type) {
  if (!PyList_Check(data)) {
    raise << "expected DataType.Vector";
    return std::make_pair(false, nullptr);
  }
  PyListObject* l = reinterpret_cast<PyListObject*>(data);
  FloatVector arr;
  if (!readList(l, arr, PyFloat_AsDouble)) {
    return std::make_pair(false, nullptr);
  }
  const Object* z = new Object(id, -1, arr.size()*sizeof(float), &arr[0]);
  return std::make_pair(true, z);
}

BoolObject readString(PyObject* data, int id, int dist_type) {
  if (!PyString_Check(data)) {
    raise << "expected DataType.String";
    return std::make_pair(false, nullptr);
  }
  const char* s = PyString_AsString(data);
  const Object* z = new Object(id, -1, strlen(s)*sizeof(char), s);
  return std::make_pair(true, z);
}

BoolObject readSqfd(PyObject* data, int id, int dist_type) {
  raise << "readSqfd: not implemented yet";
  return std::make_pair(false, nullptr);
}

template <typename T>
class IndexWrapper {
 public:
  IndexWrapper(int dist_type, int data_type, int sz,
               const char* space_type,
               const AnyParams& space_param,
               const char* method_name,
               const AnyParams& method_param)
      : dist_type_(dist_type),
        data_type_(data_type),
        sz_(sz),
        space_type_(space_type),
        method_name_(method_name),
        method_param_(method_param),
        index_(nullptr) {
    data_.resize(sz);
    space_ = SpaceFactoryRegistry<T>::Instance()
        .CreateSpace(space_type_.c_str(), space_param);
  }

  ~IndexWrapper() {
    //cout << "(nmslib) Mopping up" << endl;
    delete space_;
    //cout << "(nmslib) Deleted space" << endl;
    delete index_;
    //cout << "(nmslib) Deleted index" << endl;
    for (auto p : data_) {
      delete p;
    }
    //cout << "(nmslib) Deleted wrapper Object instance" << endl;
    //cout << "(nmslib) Mopping up finished" << endl;
  }

  inline int GetDistType() { return dist_type_; }
  inline int GetDataType() { return data_type_; }
  inline int GetDataSize() { return sz_; }

  void SetData(size_t pos, const Object* z) {
    assert(pos < data_.size());
    data_[pos] = z;
  }

  void Build() {
    index_ = MethodFactoryRegistry<T>::Instance()
        .CreateMethod(true /* let's print progress bars for now */, 
                      method_name_, space_type_,
                      space_, data_, method_param_);
  }

  PyObject* KnnQuery(int k, const Object* query) {
    KNNQuery<T> knn(space_, query, k);
    index_->Search(&knn);
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
      PyList_SET_ITEM(z, i, v);
    }
    return z;
  }

 private:
  const int dist_type_;
  const int data_type_;
  const int sz_;
  const std::string space_type_;
  const std::string method_name_;
  const AnyParams method_param_;
  Index<T>* index_;
  Space<T>* space_;
  ObjectVector data_;
};

inline bool IsDistFloat(PyObject* ptr) {
  return *(reinterpret_cast<int*>(PyLong_AsVoidPtr(ptr))) == kDistFloat;
}

template <typename T>
PyObject* _initIndex(int dist_type,
                     int data_type,
                     int sz,
                     const char* space_type,
                     const AnyParams& space_param,
                     const char* method_name,
                     const AnyParams& method_param) {
  IndexWrapper<T>* index(new IndexWrapper<T>(
      dist_type, data_type, sz, space_type, space_param,
      method_name, method_param));
  if (!index) {
    raise << "failed to create IndexWrapper";
    return NULL;
  }
  return PyLong_FromVoidPtr(reinterpret_cast<void*>(index));
}

PyObject* initIndex(PyObject* self, PyObject* args) {
  int sz;
  char* space_type;
  PyListObject* space_param_list;
  char* method_name;
  PyListObject* method_param_list;
  int dist_type, data_type;
  if (!PyArg_ParseTuple(args, "isO!sO!ii",
          &sz, &space_type, &PyList_Type, &space_param_list, &method_name,
          &PyList_Type, &method_param_list, &data_type, &dist_type)) {
    return NULL;
  }
  if (sz <= 0) {
    raise << "# of objects sould be greater than 0";
    return NULL;
  }
  StringVector space_param;
  if (!readList(space_param_list, space_param, PyString_AsString)) {
    return NULL;
  }
  StringVector method_param;
  if (!readList(method_param_list, method_param, PyString_AsString)) {
    return NULL;
  }
  switch (dist_type) {
    case kDistFloat:
      return _initIndex<float>(
          dist_type, data_type, sz, space_type, space_param,
          method_name, method_param);
    case kDistInt:
      return _initIndex<int>(
          dist_type, data_type, sz, space_type, space_param,
          method_name, method_param);
    default:
      raise << "unknown dist type - " << dist_type;
      return NULL;
  }
}
template <typename T>
PyObject* _setData(PyObject* ptr, int pos, PyObject* data) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  if (pos < 0 || pos >= index->GetDataSize()) {
    raise << "pos (" << pos << ") should be in [0, "
          << index->GetDataSize() << ")";
    return NULL;
  }
  auto iter = NMSLIB_DATA_PARSERS.find(index->GetDataType());
  if (iter == NMSLIB_DATA_PARSERS.end()) {
    raise << "unknown data type - " << index->GetDataType();
    return NULL;
  }
  auto res = (*iter->second)(data, pos, index->GetDistType());
  if (!res.first) {
    return NULL;
  }
  index->SetData(pos, res.second);
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* setData(PyObject* self, PyObject* args) {
  PyObject* ptr;
  PyObject* data;
  int pos;
  if (!PyArg_ParseTuple(args, "OiO", &ptr, &pos, &data)) {
    return NULL;
  }
  if (IsDistFloat(ptr)) {
    return _setData<float>(ptr, pos, data);
  } else {
    return _setData<int>(ptr, pos, data);
  }
}

template <typename T>
void _buildIndex(PyObject* ptr) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  index->Build();
}

PyObject* buildIndex(PyObject* self, PyObject* args) {
  PyObject* ptr;
  if (!PyArg_ParseTuple(args, "O", &ptr)) {
    return NULL;
  }
  if (IsDistFloat(ptr)) {
    _buildIndex<float>(ptr);
  } else {
    _buildIndex<int>(ptr);
  }
  Py_RETURN_NONE;
}

template <typename T>
PyObject* _knnQuery(PyObject* ptr, int k, PyObject* data) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  auto iter = NMSLIB_DATA_PARSERS.find(index->GetDataType());
  if (iter == NMSLIB_DATA_PARSERS.end()) {
    raise << "unknown data type - " << index->GetDataType();
    return NULL;
  }
  auto res = (*iter->second)(data, 0, index->GetDistType());
  if (!res.first) {
    return NULL;
  }
  std::unique_ptr<const Object> query_obj(res.second);
  return index->KnnQuery(k, query_obj.get());
}

PyObject* knnQuery(PyObject* self, PyObject* args) {
  PyObject* ptr;
  int k;
  PyObject* data;
  if (!PyArg_ParseTuple(args, "OiO", &ptr, &k, &data)) {
    return NULL;
  }
  if (k < 1) {
    raise << "k (" << k << ") should be >=1";
    return NULL;
  }
  if (IsDistFloat(ptr)) {
    return _knnQuery<float>(ptr, k, data);
  } else {
    return _knnQuery<int>(ptr, k, data);
  }
}

template <typename T>
void _freeIndex(PyObject* ptr) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  delete index;
}

PyObject* freeIndex(PyObject* self, PyObject* args) {
  PyObject* ptr;
  if (!PyArg_ParseTuple(args, "O", &ptr)) {
    return NULL;
  }
  if (IsDistFloat(ptr)) {
    _freeIndex<float>(ptr);
  } else {
    _freeIndex<int>(ptr);
  }
  Py_RETURN_NONE;
}

