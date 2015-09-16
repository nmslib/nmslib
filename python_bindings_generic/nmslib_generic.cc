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
               int sz,
               const char* space_type,
               const char* method_name,
               const AnyParams& method_param)
      : dist_type_(dist_type),
        sz_(sz),
        space_type_(space_type),
        method_name_(method_name),
        method_param_(method_param) {
    data_.resize(sz);
  }

  inline int GetDistType() { return dist_type_; }
  inline int GetDataSize() { return sz_; }

  virtual void Build() = 0;
  virtual void AddDataPoint(size_t pos, const Object* z) = 0;
  virtual PyObject* KnnQuery(int k, const Object* query) = 0;
  virtual unique_ptr<Object> CreateObjFromStr(const string& s, int id) = 0;

  virtual ~IndexWrapperBase() {
    for (auto p : data_) {
      delete p;
    }
  }
protected:
  const DistType dist_type_;
  const int sz_;
  const std::string space_type_;
  const std::string method_name_;
  const AnyParams method_param_;
  ObjectVector data_;
};

template <typename T>
class IndexWrapper : public IndexWrapperBase {
 public:
  IndexWrapper(DistType dist_type, 
               int sz,
               const char* space_type,
               const AnyParams& space_param,
               const char* method_name,
               const AnyParams& method_param)
      : IndexWrapperBase(dist_type, sz, space_type, method_name, method_param),
        index_(nullptr) {
    space_ = SpaceFactoryRegistry<T>::Instance()
        .CreateSpace(space_type_.c_str(), space_param);
  }

  ~IndexWrapper() {
    delete space_;
    delete index_;
  }

  virtual void AddDataPoint(size_t pos, const Object* z) {
    if (pos >= data_.size()) {
      raise << "Bug, the object index " << pos << " is too large, the # of points: " << data_.size();
    } else {
      data_[pos] = z;
    }
  }

  void Build() {
    index_ = MethodFactoryRegistry<T>::Instance()
        .CreateMethod(true /* let's print progress bars for now */, 
                      method_name_, space_type_,
                      space_, data_, method_param_);
  }

  unique_ptr<Object> CreateObjFromStr(const string& s, int id) {
    return space_->CreateObjFromStr(id, -1,  s, NULL);
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
                     int sz,
                     const char* space_type,
                     const AnyParams& space_param,
                     const char* method_name,
                     const AnyParams& method_param) {
  IndexWrapper<T>* index(new IndexWrapper<T>(
      dist_type, sz, space_type, space_param,
      method_name, method_param));
  if (!index) {
    raise << "failed to create IndexWrapper";
    return NULL;
  }
  return PyLong_FromVoidPtr(reinterpret_cast<void*>(static_cast<IndexWrapperBase*>(index)));
}

PyObject* initIndex(PyObject* self, PyObject* args) {
  int sz;
  char* space_type;
  PyListObject* space_param_list;
  char* method_name;
  PyListObject* method_param_list;
  DistType dist_type;
  if (!PyArg_ParseTuple(args, "isO!sO!i",
          &sz, &space_type, &PyList_Type, &space_param_list, &method_name,
          &PyList_Type, &method_param_list, &dist_type)) {
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
          dist_type, sz, space_type, space_param,
          method_name, method_param);
    case kDistInt:
      return _initIndex<int>(
          dist_type, sz, space_type, space_param,
          method_name, method_param);
    default:
      raise << "unknown dist type - " << dist_type;
      return NULL;
  }
}

PyObject* addDataPoint(PyObject* self, PyObject* args) {
  PyObject* ptr;
  PyObject* dataPoint;
  int pos;
  if (!PyArg_ParseTuple(args, "OiO", &ptr, &pos, &dataPoint)) {
    return NULL;
  }

  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  if (pos < 0 || pos >= pIndex->GetDataSize()) {
    raise << "pos (" << pos << ") should be in [0, "
          << pIndex->GetDataSize() << ")";
    return NULL;
  }

  auto res = ReadString(dataPoint);

  if (!res.first) {
    return NULL;
  }

  pIndex->AddDataPoint(pos, pIndex->CreateObjFromStr(res.second, pos).release());
  Py_INCREF(Py_None);
  return Py_None;
}


PyObject* buildIndex(PyObject* self, PyObject* args) {
  PyObject* ptr;
  if (!PyArg_ParseTuple(args, "O", &ptr)) {
    return NULL;
  }
  IndexWrapperBase* pIndex = reinterpret_cast<IndexWrapperBase*>(PyLong_AsVoidPtr(ptr));
  pIndex->Build();
  Py_RETURN_NONE;
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

