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
#include "nmslib_vector.h"

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
typedef BoolObject (*DataReaderFunc)(PyObject*,int,int);
BoolObject readVector(PyObject* data, int id, int dist_type);
BoolObject readString(PyObject* data, int id, int dist_type);
typedef PyObject* (*DataWriterFunc)(const Object*);
PyObject*  writeVector(const Object*);
PyObject*  writeString(const Object*);

const int kDataVector = 1;
const int kDataString = 2;

const std::map<std::string, int> NMSLIB_DATA_TYPES = {
  {"VECTOR", kDataVector},
  {"STRING", kDataString},
};
const std::map<int, DataReaderFunc> NMSLIB_DATA_READERS = {
  {kDataVector, &readVector},
  {kDataString, &readString},
};
const std::map<int, DataWriterFunc> NMSLIB_DATA_WRITERS = {
  {kDataVector, &writeVector},
  {kDataString, &writeString},
};

const int kDistFloat = 4;
const int kDistInt = 5;
const std::map<std::string, int> NMSLIB_DIST_TYPES = {
  {"FLOAT", kDistFloat},
  {"INT", kDistInt}
};

PyMODINIT_FUNC initnmslib_vector() {
  PyObject* module = Py_InitModule("nmslib_vector", nmslibMethods);
  if (module == NULL) {
    return;
  }
  // data type
  NmslibData_Type.tp_new = PyType_GenericNew;
  NmslibData_Type.tp_name = "nmslib_vector.DataType";
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
  NmslibDist_Type.tp_name = "nmslib_vector.DistType";
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

PyObject* writeString(const Object* obj) {
  unique_ptr<char[]> str_copy;
Py_BEGIN_ALLOW_THREADS
  str_copy.reset(new char[obj->datalength()+1]);
  char *s =str_copy.get();
  s[obj->datalength()] = 0;
  memcpy(s, obj->data(), obj->datalength()*sizeof(char));
Py_END_ALLOW_THREADS
  PyObject* v = PyString_FromString(str_copy.get());
  if (!v) {
    return NULL;
  }
  return v;
}

PyObject* writeVector(const Object* obj) {
  // Could in principal use Py_*ALLOW_THREADS here, but it's not 
  // very useful b/c it would apply only to a very short and fast
  // fragment of code. In that, it seems that we have to start blocking
  // as soon as we start calling Python API functions.
  const float* arr = reinterpret_cast<const float*>(obj->data());
  size_t       qty = obj->datalength() / sizeof(float);
  PyObject* z = PyList_New(qty);
  if (!z) {
    return NULL;
  }
  for (size_t i = 0; i < qty; ++i) {
    PyObject* v = PyFloat_FromDouble(arr[i]);
    if (!v) {
      Py_DECREF(z);
      return NULL;
    }
    PyList_SET_ITEM(z, i, v);
  }
  return z;
}

template <typename T>
class IndexWrapper {
 public:
  IndexWrapper(int dist_type, int data_type, 
               const char* space_type,
               const AnyParams& space_param,
               const char* method_name)
      : dist_type_(dist_type),
        data_type_(data_type),
        space_type_(space_type),
        method_name_(method_name),
        index_(nullptr),
        space_(nullptr)
    {
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
  inline size_t GetDataPointQty() { return data_.size(); }

  void AddDataPoint(const Object* z) {
    data_.push_back(z);
  }

  const Object* GetDataPoint(size_t index) {
    return data_.at(index);
  }



  void CreateIndex(const AnyParams& index_params) {
    // Delete previously created index
    delete index_;
    index_ = MethodFactoryRegistry<T>::Instance()
        .CreateMethod(PRINT_PROGRESS,
                      method_name_, space_type_,
                      *space_, data_);
    index_->CreateIndex(index_params);
  }

  void SaveIndex(const string& fileName) {
    index_->SaveIndex(fileName);
  }

  void LoadIndex(const string& fileName) {
    // Delete previously created index
    delete index_;
    index_ = MethodFactoryRegistry<T>::Instance()
        .CreateMethod(PRINT_PROGRESS,
                      method_name_, space_type_,
                      *space_, data_);
    index_->LoadIndex(fileName);
  }

  void SetQueryTimeParams(const AnyParams& p) {
    index_->SetQueryTimeParams(p);
  }

  PyObject* KnnQuery(int k, const Object* query) {
    IntVector ids;
Py_BEGIN_ALLOW_THREADS
    KNNQueue<T>* res;
    KNNQuery<T> knn(*space_, query, k);
    index_->Search(&knn, -1);
    res = knn.Result()->Clone();
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
      PyList_SET_ITEM(z, i, v);
    }
    return z;
  }

 private:
  const int dist_type_;
  const int data_type_;
  const std::string space_type_;
  const std::string method_name_;
  const AnyParams method_index_param_;
  Index<T>* index_;
  Space<T>* space_;
  ObjectVector data_;
};

inline bool IsDistFloat(PyObject* ptr) {
  return *(reinterpret_cast<int*>(PyLong_AsVoidPtr(ptr))) == kDistFloat;
}

template <typename T>
PyObject* _init(int dist_type,
                     int data_type,
                     const char* space_type,
                     const AnyParams& space_param,
                     const char* method_name) {
  IndexWrapper<T>* 
      index(new IndexWrapper<T>(dist_type, data_type, 
                                space_type, space_param, 
                                method_name));
  if (!index) {
    raise << "failed to create IndexWrapper";
    return NULL;
  }
  return PyLong_FromVoidPtr(reinterpret_cast<void*>(index));
}

PyObject* init(PyObject* self, PyObject* args) {
  char* space_type;
  PyListObject* space_param_list;
  char* method_name;
  int dist_type, data_type;
  if (!PyArg_ParseTuple(args, "sO!sii",
          &space_type, &PyList_Type, &space_param_list, 
          &method_name,
          &data_type, &dist_type)) {
    raise << "Error reading parameters (expecting: space type, space parameter list, index/method name, data type, distance value type)";
    return NULL;
  }

  StringVector space_param;
  if (!readList(space_param_list, space_param, PyString_AsString)) {
    return NULL;
  }

  switch (dist_type) {
    case kDistFloat:
      return _init<float>(
          dist_type, data_type, space_type, space_param,
          method_name);
    case kDistInt:
      return _init<int>(
          dist_type, data_type, space_type, space_param,
          method_name);
    default:
      raise << "unknown dist type - " << dist_type;
      return NULL;
  }
}
template <typename T>
PyObject* _addDataPoint(PyObject* ptr, IdType id, PyObject* data) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  auto iter = NMSLIB_DATA_READERS.find(index->GetDataType());
  if (iter == NMSLIB_DATA_READERS.end()) {
    raise << "unknown data type - " << index->GetDataType();
    return NULL;
  }
  auto res = (*iter->second)(data, id, index->GetDistType());
  if (!res.first) {
    raise << "Cannot create a data-point object!";
    return NULL;
  }
  index->AddDataPoint(res.second);
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* addDataPoint(PyObject* self, PyObject* args) {
  PyObject* ptr;
  PyObject* data;
  int32_t   id;
  if (!PyArg_ParseTuple(args, "OiO", &ptr, &id, &data)) {
    raise << "Error reading parameters (expecting: index ref, object (as a string))";
    return NULL;
  }
  if (IsDistFloat(ptr)) {
    return _addDataPoint<float>(ptr, id, data);
  } else {
    return _addDataPoint<int>(ptr, id, data);
  }
}

template <typename T>
void _createIndex(PyObject* ptr, const AnyParams& index_params) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
Py_BEGIN_ALLOW_THREADS
  index->CreateIndex(index_params);
Py_END_ALLOW_THREADS
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

  if (IsDistFloat(ptr)) {
    _createIndex<float>(ptr, index_params);
  } else {
    _createIndex<int>(ptr, index_params);
  }
  Py_RETURN_NONE;
}

template <typename T>
void _saveIndex(PyObject* ptr, const string& fileName) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  index->SaveIndex(fileName);
}

PyObject* saveIndex(PyObject* self, PyObject* args) {
  PyObject*     ptr;
  char*         file_name;

  if (!PyArg_ParseTuple(args, "Os", &ptr, &file_name)) {
    raise << "Error reading parameters (expecting: index ref, file name)";
    return NULL;
  }

  if (IsDistFloat(ptr)) {
    _saveIndex<float>(ptr, file_name);
  } else {
    _saveIndex<int>(ptr, file_name);
  }
  Py_RETURN_NONE;
}

template <typename T>
void _loadIndex(PyObject* ptr, const string& fileName) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  index->LoadIndex(fileName);
}

PyObject* loadIndex(PyObject* self, PyObject* args) {
  PyObject*     ptr;
  char*         file_name;

  if (!PyArg_ParseTuple(args, "Os", &ptr, &file_name)) {
    raise << "Error reading parameters (expecting: index ref, file name)";
    return NULL;
  }

  if (IsDistFloat(ptr)) {
    _loadIndex<float>(ptr, file_name);
  } else {
    _loadIndex<int>(ptr, file_name);
  }
  Py_RETURN_NONE;
}

template <typename T>
void _setQueryTimeParams(PyObject* ptr, const AnyParams& qp) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  index->SetQueryTimeParams(qp);
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

  if (IsDistFloat(ptr)) {
    _setQueryTimeParams<float>(ptr, query_time_params);
  } else {
    _setQueryTimeParams<int>(ptr, query_time_params);
  }

  return Py_None;
}

template <typename T>
PyObject* _knnQuery(PyObject* ptr, int k, PyObject* data) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  auto iter = NMSLIB_DATA_READERS.find(index->GetDataType());
  if (iter == NMSLIB_DATA_READERS.end()) {
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
    raise << "Error reading parameters (expecting: index ref, K as in-KNN, query)";
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
PyObject* _getDataPoint(PyObject* ptr, int id) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  if (id < 0 || static_cast<size_t>(id) >= index->GetDataPointQty()) {
    raise << "The data point index should be >= 0 & < " << index->GetDataPointQty();
    return NULL;
  }
  auto iter = NMSLIB_DATA_WRITERS.find(index->GetDataType());
  if (iter == NMSLIB_DATA_WRITERS.end()) {
    raise << "unknown data type - " << index->GetDataType();
    return NULL;
  }
  const Object* obj = index->GetDataPoint(id);
  return (*iter->second)(obj);
}

PyObject* getDataPoint(PyObject* self, PyObject* args) {
  PyObject* ptr;
  int       index;
  if (!PyArg_ParseTuple(args, "Oi", &ptr, &index)) {
    raise << "Error reading parameters (expecting: index ref, object index)";
    return NULL;
  }

  if (IsDistFloat(ptr)) {
    return _getDataPoint<float>(ptr, index);
  } else {
    return _getDataPoint<int>(ptr, index);
  }
}

template <typename T>
PyObject* _getDataPointQty(PyObject* ptr) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  PyObject* tmp = PyInt_FromLong(index->GetDataPointQty());
  if (tmp == NULL) {
    return NULL;
  }
  return tmp;
}

PyObject* getDataPointQty(PyObject* self, PyObject* args) {
  PyObject* ptr;
  if (!PyArg_ParseTuple(args, "O", &ptr)) {
    raise << "Error reading parameters (expecting: index ref)";
    return NULL;
  }

  if (IsDistFloat(ptr)) {
    return _getDataPointQty<float>(ptr);
  } else {
    return _getDataPointQty<int>(ptr);
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

