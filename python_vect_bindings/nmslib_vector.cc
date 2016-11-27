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
#include <numpy/arrayobject.h>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <utility>
#include <thread>
#include <queue>
#include <mutex>
#include <type_traits>
#include "space.h"
#include "space/space_sparse_vector.h"
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
  {"addDataPointBatch", addDataPointBatch, METH_VARARGS},
  {"createIndex", createIndex, METH_VARARGS},
  {"saveIndex", saveIndex, METH_VARARGS},
  {"loadIndex", loadIndex, METH_VARARGS},
  {"setQueryTimeParams", setQueryTimeParams, METH_VARARGS},
  {"knnQuery", knnQuery, METH_VARARGS},
  {"knnQueryBatch", knnQueryBatch, METH_VARARGS},
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
using BoolString = std::pair<bool,string>;
using BoolPyObject = std::pair<bool,PyObject*>;

const int kDataDenseVector = 1;
const int kDataString = 2;
const int kDataSparseVector = 3;
const int kDataObjectAsString = 4;

const std::map<std::string, int> NMSLIB_DATA_TYPES = {
  {"DENSE_VECTOR", kDataDenseVector},
  {"STRING", kDataString},
  {"SPARSE_VECTOR", kDataSparseVector},
  {"OBJECT_AS_STRING", kDataObjectAsString},
};

const int kDistFloat = 14;
const int kDistInt = 15;
const std::map<std::string, int> NMSLIB_DIST_TYPES = {
  {"FLOAT", kDistFloat},
  {"INT", kDistInt}
};

PyMODINIT_FUNC initnmslib_vector() {
  PyObject* module = Py_InitModule("nmslib_vector", nmslibMethods);
  if (module == NULL) {
    return;
  }
  import_array();
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

BoolObject readDenseVector(const Space<float>* space, PyObject* data, int id) {
  if (!PyList_Check(data)) {
    raise << "expected DataType.DENSE_VECTOR";
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

BoolObject readSparseVector(const Space<float>* space, PyObject* data, int id) {
  if (!PyList_Check(data)) {
    raise << "expected DataType.SPARSE_VECTOR";
    return std::make_pair(false, nullptr);
  }
  PyListObject* l = reinterpret_cast<PyListObject*>(data);
  std::vector<SparseVectElem<float>> arr;
  PyErr_Clear();
  for (int i = 0; i < PyList_GET_SIZE(l); ++i) {
    PyObject* item = PyList_GET_ITEM(l, i);
    if (PyErr_Occurred()) {
      raise << "failed to read item from list";
      return std::make_pair(false, nullptr);
    }
    if (!PyList_Check(item)) {
      raise << "expected list of list pair [index, value]";
      return std::make_pair(false, nullptr);
    }
    PyListObject* lst = reinterpret_cast<PyListObject*>(item);
    if (PyList_GET_SIZE(lst) != 2) {
      raise << "expected list of list pair [index, value]";
      return std::make_pair(false, nullptr);
    }
    auto index = PyInt_AsLong(PyList_GET_ITEM(lst, 0));
    if (PyErr_Occurred()) {
      raise << "expected int index";
      return std::make_pair(false, nullptr);
    }
    auto value = PyFloat_AsDouble(PyList_GET_ITEM(lst, 1));
    if (PyErr_Occurred()) {
      raise << "expected double value";
      return std::make_pair(false, nullptr);
    }
    arr.push_back(SparseVectElem<float>(
            static_cast<uint32_t>(index), static_cast<float>(value)));
  }
  std::sort(arr.begin(), arr.end());
  const Object* z = reinterpret_cast<const SpaceSparseVector<float>*>(
      space)->CreateObjFromVect(id, -1, arr);
  return std::make_pair(true, z);
}

template <typename T>
BoolObject readString(const Space<T>* space, PyObject* data, int id) {
  if (!PyString_Check(data)) {
    raise << "expected DataType.STRING";
    return std::make_pair(false, nullptr);
  }
  const char* s = PyString_AsString(data);
  const Object* z = new Object(id, -1, strlen(s), s);
  return std::make_pair(true, z);
}

template <typename T>
BoolObject readObjectAsString(const Space<T>* space, PyObject* data, int id) {
  if (!PyString_Check(data)) {
    raise << "expected DataType.OBJECT_AS_STRING";
    return std::make_pair(false, nullptr);
  }
  const char* s = PyString_AsString(data);
  const Object* z = space->CreateObjFromStr(id, -1,  s, NULL).release();
  return std::make_pair(true, z);
}

template <typename T>
BoolObject readObject(const int data_type,
                      const Space<T>* space,
                      PyObject* data,
                      const int id,
                      const int dist_type) {
  raise << "not implemented for data_type "
        << data_type << " and dist_type "
        << dist_type;
  return std::make_pair(false, nullptr);
}

template <>
BoolObject readObject(const int data_type,
                      const Space<float>* space,
                      PyObject* data,
                      const int id,
                      const int dist_type) {
  if (dist_type != kDistFloat) {
    raise << "expected float dist_type";
    return std::make_pair(false, nullptr);
  }
  switch (data_type) {
    case kDataDenseVector:
      return readDenseVector(space, data, id);
    case kDataSparseVector:
      return readSparseVector(space, data, id);
    case kDataString:
      return readString(space, data, id);
    case kDataObjectAsString:
      return readObjectAsString(space, data, id);
    default:
      raise << "not implemented";
      return std::make_pair(false, nullptr);
  }
}

template <>
BoolObject readObject(const int data_type,
                      const Space<int>* space,
                      PyObject* data,
                      const int id,
                      const int dist_type) {
  if (dist_type != kDistInt) {
    raise << "expected int dist_type";
    return std::make_pair(false, nullptr);
  }
  switch (data_type) {
    case kDataString:
      return readString(space, data, id);
    case kDataObjectAsString:
      return readObjectAsString(space, data, id);
    default:
      raise << "not implemented";
      return std::make_pair(false, nullptr);
  }
}

template <typename T>
BoolPyObject writeString(const Space<T>* space, const Object* obj) {
  unique_ptr<char[]> str_copy;
Py_BEGIN_ALLOW_THREADS
  str_copy.reset(new char[obj->datalength()+1]);
  char *s =str_copy.get();
  s[obj->datalength()] = 0;
  memcpy(s, obj->data(), obj->datalength()*sizeof(char));
Py_END_ALLOW_THREADS
  PyObject* v = PyString_FromString(str_copy.get());
  if (!v) {
    return std::make_pair(false, nullptr);
  }
  return std::make_pair(true, v);
}

template <typename T>
BoolPyObject writeDenseVector(const Space<T>* space, const Object* obj) {
  // Could in principal use Py_*ALLOW_THREADS here, but it's not
  // very useful b/c it would apply only to a very short and fast
  // fragment of code. In that, it seems that we have to start blocking
  // as soon as we start calling Python API functions.
  const float* arr = reinterpret_cast<const float*>(obj->data());
  size_t       qty = obj->datalength() / sizeof(float);
  PyObject* z = PyList_New(qty);
  if (!z) {
    return std::make_pair(false, nullptr);
  }
  for (size_t i = 0; i < qty; ++i) {
    PyObject* v = PyFloat_FromDouble(arr[i]);
    if (!v) {
      Py_DECREF(z);
      return std::make_pair(false, nullptr);
    }
    PyList_SET_ITEM(z, i, v);
  }
  return std::make_pair(true, z);
}

template <typename T>
BoolPyObject writeSparseVector(const Space<T>* space, const Object* obj) {
  const SparseVectElem<float>* arr =
      reinterpret_cast<const SparseVectElem<float>*>(obj->data());
  size_t qty = obj->datalength() / sizeof(SparseVectElem<float>);
  PyObject* z = PyList_New(qty);
  if (!z) {
    return std::make_pair(false, nullptr);
  }
  for (size_t i = 0; i < qty; ++i) {
    PyObject* id = PyInt_FromLong(arr[i].id_);
    if (!id) {
      Py_DECREF(z);
      return std::make_pair(false, nullptr);
    }
    PyObject* v = PyFloat_FromDouble(arr[i].val_);
    if (!v) {
      Py_DECREF(z);
      Py_DECREF(id);
      return std::make_pair(false, nullptr);
    }
    PyObject* p = PyList_New(2);
    if (!p) {
      // TODO(@bileg): need to release all previous p
      Py_DECREF(z);
      Py_DECREF(id);
      Py_DECREF(v);
      return std::make_pair(false, nullptr);
    }
    PyList_SET_ITEM(p, 0, id);
    PyList_SET_ITEM(p, 1, v);
    PyList_SET_ITEM(z, i, p);
  }
  return std::make_pair(true, z);
}

template <typename T>
BoolPyObject writeObjectAsString(const Space<T> *space, const Object* obj) {
  unique_ptr<char[]> str_copy;
Py_BEGIN_ALLOW_THREADS
  std::stringstream ss;
  ss << obj->id();
  std::string str = space->CreateStrFromObj(obj, ss.str());
  str_copy.reset(new char[str.size()+1]);
  memcpy(str_copy.get(), str.c_str(), str.size()+1);
Py_END_ALLOW_THREADS
  PyObject* v = PyString_FromString(str_copy.get());
  if (!v) {
    return std::make_pair(false, nullptr);
  }
  return std::make_pair(true, v);
}

template <typename T>
BoolPyObject writeObject(const int data_type,
                         const Space<T>* space,
                         const Object* obj) {
  raise << "writeObject is not implemented";
  return std::make_pair(false, nullptr);
}

template <>
BoolPyObject writeObject(const int data_type,
                         const Space<float>* space,
                         const Object* obj) {
  switch (data_type) {
    case kDataDenseVector:
      return writeDenseVector(space, obj);
    case kDataSparseVector:
      return writeSparseVector(space, obj);
    case kDataString:
      return writeString(space, obj);
    case kDataObjectAsString:
      return writeObjectAsString(space, obj);
    default:
      raise << "write function is not implemented for data type "
            << data_type << " and dist type float";
      return std::make_pair(false, nullptr);
  }
}

template <>
BoolPyObject writeObject(int data_type,
                         const Space<int>* space,
                         const Object* obj) {
  switch (data_type) {
    case kDataDenseVector:
      return writeDenseVector(space, obj);
    case kDataSparseVector:
      return writeSparseVector(space, obj);
    case kDataString:
      return writeString(space, obj);
    case kDataObjectAsString:
      return writeObjectAsString(space, obj);
    default:
      raise << "write function is not implemented for data type "
            << data_type << " and dist type int";
      return std::make_pair(false, nullptr);
  }
}

class ValueException : std::exception {
 public:
  ValueException(const std::string& msg) : msg_(msg) {}
  virtual ~ValueException() {}
  virtual const char* what() const throw() {
    return msg_.c_str();
  }
 private:
  std::string msg_;
};

class BatchObjects {
 public:
  virtual ~BatchObjects() {}
  virtual const int size() const = 0;
  virtual const Object* operator[](ssize_t idx) const = 0;
};

class NumpyDenseMatrix : public BatchObjects {
 public:
  NumpyDenseMatrix(const Space<int>* space,
                   PyArrayObject* ids,
                   PyObject* matrix) {
    throw ValueException("NumpyDenseMatrix is not implemented for int dist");
  }

  NumpyDenseMatrix(const Space<float>* space,
                   PyArrayObject* ids,
                   PyObject* matrix)
      : space_(space) {
    if (ids) {
      if (ids->descr->type_num != NPY_INT32 || ids->nd != 1) {
        throw ValueException("id field should be 1 dimensional int32 vector");
      }
      id_ = reinterpret_cast<int*>(ids->data);
    } else {
      id_ = nullptr;
    }
    if (!PyArray_Check(matrix)) {
      throw ValueException("expected numpy float32 matrix");
    }
    PyArrayObject* data = reinterpret_cast<PyArrayObject*>(matrix);
    if (data->flags & NPY_FORTRAN) {
      throw ValueException("the order of matrix should be C not FORTRAN");
    }
    if (data->descr->type_num != NPY_FLOAT32 || data->nd != 2) {
      throw ValueException("expected numpy float32 matrix");
    }
    num_vec_ = PyArray_DIM(data, 0);
    num_dim_ = PyArray_DIM(data, 1);
    if (id_ && num_vec_ != PyArray_DIM(ids, 0)) {
      std::stringstream ss;
      ss << "ids contains " << PyArray_DIM(ids, 0) << " elements "
         << "whereas matrix contains " << num_vec_ << " elements";
      throw ValueException(ss.str());
    }
    for (int i = 0; i < num_vec_; ++i) {
      const float* buf = reinterpret_cast<float*>(
          data->data + i * data->strides[0]);
      data_.push_back(buf);
    }
  }

  ~NumpyDenseMatrix() {}

  const int size() const override { return num_vec_; }

  const Object* operator[](ssize_t idx) const override {
    int id = id_ ? id_[idx] : 0;
    return new Object(id, -1, num_dim_ * sizeof(float), data_[idx]);
  }

 private:
  const Space<float>* space_;
  int num_vec_;
  int num_dim_;
  const int* id_;
  std::vector<const float*> data_;
};

template <int T>
PyArrayObject* GetAttrAsNumpyArray(PyObject* obj,
                                   const std::string& attr_name) {
  PyObject* attr = PyObject_GetAttrString(obj, attr_name.c_str());
  if (!attr) {
    std::stringstream ss;
    ss << "failed to get attribute " << attr_name;
    throw ValueException(ss.str());
  }
  if (!PyArray_Check(attr)) {
    std::stringstream ss;
    ss << "expected scipy float32 csr_matrix: no attribute " << attr_name;
    throw ValueException(ss.str());
  }
  PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(attr);
  if (!arr) {
    std::stringstream ss;
    ss << "expected scipy float32 csr_matrix: attribute "
       << attr_name << " is not numpy array";
    throw ValueException(ss.str());
  }
  if (arr->descr->type_num != T || arr->nd != 1) {
    throw ValueException("expected scipy float32 csr_matrix");
  }
  if (!(arr->flags & NPY_C_CONTIGUOUS)) {
    std::stringstream ss;
    ss << "scipy csr_matrix's " << attr_name << " has to be NPY_C_CONTIGUOUS";
    throw ValueException(ss.str());
  }
  return arr;
}

class NumpySparseMatrix : public BatchObjects {
 public:
  NumpySparseMatrix(const Space<int>* space,
                    PyArrayObject* ids,
                    PyObject* matrix) {
    throw ValueException("NumpySparseMatrix is not implemented for int dist");
  }

  NumpySparseMatrix(const Space<float>* space,
                    PyArrayObject* ids,
                    PyObject* matrix)
      : space_(reinterpret_cast<const SpaceSparseVector<float>*>(space)) {
    if (ids) {
      if (ids->descr->type_num != NPY_INT32 || ids->nd != 1) {
        throw ValueException("id field should be 1 dimensional int32 vector");
      }
      id_ = reinterpret_cast<int*>(ids->data);
    } else {
      id_ = nullptr;
    }
    PyArrayObject* data = GetAttrAsNumpyArray<NPY_FLOAT>(matrix, "data");
    PyArrayObject* indices = GetAttrAsNumpyArray<NPY_INT>(matrix, "indices");
    PyArrayObject* indptr = GetAttrAsNumpyArray<NPY_INT>(matrix, "indptr");
    n_ = PyArray_DIM(indptr, 0);
    indices_ = reinterpret_cast<int*>(indices->data);
    indptr_ = reinterpret_cast<int*>(indptr->data);
    data_ = reinterpret_cast<float*>(data->data);
  }

  ~NumpySparseMatrix() {}

  const int size() const override { return n_ - 1; }

  const Object* operator[](ssize_t idx) const override {
    std::vector<SparseVectElem<float>> arr;
    const int beg_ptr = indptr_[idx];
    const int end_ptr = indptr_[idx+1];
    for (int k = beg_ptr; k < end_ptr; ++k) {
      const int j = indices_[k];
      //std::cout << "[" << idx << " " << j << " " << data_[k] << "] ";
      if (std::isnan(data_[k])) {
        throw ValueException("Bug: nan in NumpySparseMatrix");
      }
      arr.push_back(SparseVectElem<float>(static_cast<uint32_t>(j), data_[k]));
    }
    if (arr.empty()) {
      // TODO(@bileg): should we allow this?
      throw ValueException("sparse marix's row is empty (ie, all zero values)");
    }
    std::sort(arr.begin(), arr.end());
    int id = id_ ? id_[idx] : 0;
    return space_->CreateObjFromVect(id, -1, arr);
  }

 private:
  const SpaceSparseVector<float>* space_;
  int n_;
  const int* id_;
  const int* indices_;
  const int* indptr_;
  const float* data_;
};

class BatchStrings : public BatchObjects {
 public:
  BatchStrings(const Space<int>* space,
               PyArrayObject* ids,
               PyObject* data) {
    Init(ids, data);
  }

  BatchStrings(const Space<float>* space,
               PyArrayObject* ids,
               PyObject* data) {
    Init(ids, data);
  }

  ~BatchStrings() {}

  const int size() const override { return num_str_; }

  const Object* operator[](ssize_t idx) const override {
    int id = id_ ? id_[idx] : 0;
    return new Object(id, -1, strlen(data_[idx]), data_[idx]);
  }

 protected:
  void Init(PyArrayObject* ids, PyObject* data) {
    if (ids) {
      if (ids->descr->type_num != NPY_INT32 || ids->nd != 1) {
        throw ValueException("id field should be 1 dimensional int32 vector");
      }
      id_ = reinterpret_cast<int*>(ids->data);
    } else {
      id_ = nullptr;
    }
    if (!PyList_Check(data)) {
      throw ValueException("expected list of strings");
    }
    PyListObject* l = reinterpret_cast<PyListObject*>(data);
    PyErr_Clear();
    num_str_ = PyList_GET_SIZE(l);
    for (int i = 0; i < num_str_; ++i) {
      PyObject* item = PyList_GET_ITEM(l, i);
      if (PyErr_Occurred()) {
        throw ValueException("failed to read string from list");
      }
      if (!PyString_Check(item)) {
        throw ValueException("expected DataType.STRING");
      }
      data_.push_back(PyString_AsString(item));
    }
  }

  int num_str_;
  const int* id_;
  std::vector<const char*> data_;
};

class BatchObjectStrings : public BatchStrings {
 public:
  BatchObjectStrings(const Space<int>* space,
                     PyArrayObject* ids,
                     PyObject* data)
      : BatchStrings(space, ids, data),
        space_int_(space),
        space_float_(nullptr) {
    Init(ids, data);
  }

  BatchObjectStrings(const Space<float>* space,
                     PyArrayObject* ids,
                     PyObject* data)
      : BatchStrings(space, ids, data),
        space_int_(nullptr),
        space_float_(space) {
    Init(ids, data);
  }

  ~BatchObjectStrings() {}

  const Object* operator[](ssize_t idx) const override {
    int id = id_ ? id_[idx] : 0;
    if (space_int_) {
      return space_int_->CreateObjFromStr(id, -1,  data_[idx], NULL).release();
    } else {
      return space_float_->CreateObjFromStr(id, -1,  data_[idx], NULL).release();
    }
  }

 private:
  const Space<int>* space_int_;        // TODO(@bileg) this one is a bit ugly
  const Space<float>* space_float_;    // but there's no workaround
};

class IndexWrapperBase {
 public:
  IndexWrapperBase(int dist_type,
                   int data_type,
                   const char* space_type,
                   const char* method_name)
      : dist_type_(dist_type),
        data_type_(data_type),
        space_type_(space_type),
        method_name_(method_name) {
  }

  virtual ~IndexWrapperBase() {
    for (auto p : data_) {
      delete p;
    }
  }

  inline int GetDistType() const { return dist_type_; }
  inline int GetDataType() const { return data_type_; }
  inline size_t GetDataPointQty() const { return data_.size(); }

  virtual size_t AddDataPoint(const Object* z) {
    data_.push_back(z);
    return data_.size() - 1;
  }

  virtual void SetDataPoint(const Object* z, size_t idx) {
    if (idx >= data_.size()) {
      data_.resize(idx + 1);
    }
    data_[idx] = z;
  }

  virtual const BoolObject ReadObject(int id, PyObject* data) = 0;
  virtual const BoolPyObject WriteObject(size_t index) = 0;
  virtual void CreateIndex(const AnyParams& index_params) = 0;
  virtual void SaveIndex(const string& fileName) = 0;
  virtual void LoadIndex(const string& fileName) = 0;
  virtual void SetQueryTimeParams(const AnyParams& p) = 0;
  virtual PyObject* KnnQuery(int k, const Object* query) = 0;
  virtual std::vector<IntVector> KnnQueryBatch(const int num_threads,
                                               const int k,
                                               const ObjectVector& query_objects) = 0;

  virtual PyObject* AddDataPointBatch(PyArrayObject* ids,
                                      PyObject* data) = 0;

  virtual PyObject* KnnQueryBatch(const int num_threads,
                                  const int k,
                                  PyObject* data) = 0;

 protected:
  const int dist_type_;
  const int data_type_;
  const std::string space_type_;
  const std::string method_name_;
  ObjectVector data_;
};

template <typename T>
class IndexWrapper : public IndexWrapperBase {
 public:
  IndexWrapper(int dist_type,
               int data_type,
               const char* space_type,
               const AnyParams& space_param,
               const char* method_name)
      : IndexWrapperBase(dist_type, data_type, space_type, method_name),
        index_(nullptr),
        space_(nullptr) {
    space_ = SpaceFactoryRegistry<T>::Instance()
        .CreateSpace(space_type_.c_str(), space_param);
  }

  ~IndexWrapper() {
    delete space_;
    delete index_;
  }

  const BoolObject ReadObject(int id, PyObject* data) override {
    return readObject(data_type_, space_, data, id, dist_type_);
  }

  const BoolPyObject WriteObject(size_t index) override {
    return writeObject(data_type_, space_, data_[index]);
  }

  void CreateIndex(const AnyParams& index_params) override {
    // Delete previously created index
    delete index_;
    index_ = MethodFactoryRegistry<T>::Instance()
        .CreateMethod(PRINT_PROGRESS,
                      method_name_, space_type_,
                      *space_, data_);
    index_->CreateIndex(index_params);
  }

  void SaveIndex(const string& fileName) override {
    index_->SaveIndex(fileName);
  }

  void LoadIndex(const string& fileName) override {
    // Delete previously created index
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

  PyObject* KnnQuery(int k, const Object* query) override {
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
    for (size_t i = 0; i < ids.size(); ++i) {
      PyObject* v = PyInt_FromLong(ids[i]);
      if (!v) {
        Py_DECREF(z);
        return NULL;
      }
      PyList_SET_ITEM(z, i, v);
    }
    return z;
  }

  std::vector<IntVector> KnnQueryBatch(const int num_threads,
                                       const int k,
                                       const ObjectVector& query_objects) override {
    std::vector<IntVector> query_res(query_objects.size());
    std::queue<std::pair<size_t, const Object*>> q;
    std::mutex m;
    for (size_t i = 0; i < query_objects.size(); ++i) {       // TODO: this can be improved by not adding all (ie. fixed size thread-pool)
      q.push(std::make_pair(i, query_objects[i]));
    }
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
      threads.push_back(std::thread(
              [&]() {
                for (;;) {
                  std::pair<size_t, const Object*> query;
                  {
                    std::unique_lock<std::mutex> lock(m);
                    if (q.empty()) {
                      break;
                    }
                    query = q.front();
                    q.pop();
                  }
                  IntVector& ids = query_res[query.first];
                  KNNQueue<T>* res;
                  KNNQuery<T> knn(*space_, query.second, k);
                  index_->Search(&knn, -1);
                  res = knn.Result()->Clone();
                  while (!res->Empty()) {
                    ids.insert(ids.begin(), res->TopObject()->id());
                    res->Pop();
                  }
                  delete res;
                }
              }));
    }
    for (auto& thread : threads) {
      thread.join();
    }
    return query_res;
  }

  PyObject* AddDataPointBatch(PyArrayObject* ids,
                              PyObject* data) override {
    try {
      std::unique_ptr<BatchObjects> n;
      switch (data_type_) {
        case kDataDenseVector:
          n.reset(new NumpyDenseMatrix(space_, ids, data));
          break;
        case kDataSparseVector:
          n.reset(new NumpySparseMatrix(space_, ids, data));
          break;
        case kDataString:
          n.reset(new BatchStrings(space_, ids, data));
          break;
        case kDataObjectAsString:
          n.reset(new BatchObjectStrings(space_, ids, data));
          break;
        default:
          raise << "AddDataPointBatch is not yet implemented for data type "
                << data_type_;
          return NULL;
      }
      int dims[1];
      dims[0] = n->size();
      PyArrayObject* positions = reinterpret_cast<PyArrayObject*>(
          PyArray_FromDims(1, dims, PyArray_INT));
      if (!positions) {
        raise << "failed to create numpy array for positions";
        return NULL;
      }
      PyArray_ENABLEFLAGS(positions, NPY_ARRAY_OWNDATA);
      int* ptr = reinterpret_cast<int*>(positions->data);
#if 1
      for (int i = 0; i < n->size(); ++i) {
        ptr[i] = AddDataPoint((*(n.get()))[i]);
      }
#else
Py_BEGIN_ALLOW_THREADS
      const int num_threads = 10;
      std::queue<std::pair<int,const Object*>> q;
      std::mutex m;
      const size_t num_vec = GetDataPointQty();
      for (int i = 0; i < n->size(); ++i) {       // TODO: this can be improved by not adding all (i.e. fixed size thread-pool)
        q.push(std::make_pair(i, (*(n.get()))[i]));
      }
      std::mutex md;
      std::vector<std::thread> threads;
      for (int i = 0; i < num_threads; ++i) {
      threads.push_back(std::thread(
              [&]() {
                for (;;) {
                  std::pair<int,const Object*> p;
                  {
                    std::unique_lock<std::mutex> lock(m);
                    if (q.empty()) {
                      break;
                    }
                    p = q.front();
                    q.pop();
                  }
                  {
                    std::unique_lock<std::mutex> lock(md);
                    //ptr[p.first] = AddDataPoint(p.second);
                    SetDataPoint(p.second, num_vec + p.first);
                    ptr[p.first] = num_vec + p.first;
                  }
                }
              }));
      }
      for (auto& thread : threads) {
        thread.join();
      }
Py_END_ALLOW_THREADS
#endif
      return PyArray_Return(positions);
    } catch (const ValueException& e) {
      raise << e.what();
      return NULL;
    }
  }

  PyObject* KnnQueryBatch(const int num_threads,
                          const int k,
                          PyObject* data) override {
    ObjectVector query_objects;
    int dims[2];
    try {
      std::unique_ptr<BatchObjects> n;
      switch (data_type_) {
        case kDataDenseVector:
          n.reset(new NumpyDenseMatrix(space_, nullptr, data));
          break;
        case kDataSparseVector:
          n.reset(new NumpySparseMatrix(space_, nullptr, data));
          break;
        case kDataString:
          n.reset(new BatchStrings(space_, nullptr, data));
          break;
        case kDataObjectAsString:
          n.reset(new BatchObjectStrings(space_, nullptr, data));
          break;
        default:
          raise << "KnnQueryBatch is not yet implemented for data type "
                << data_type_;
          return NULL;
      }
      for (int i = 0; i < n->size(); ++i) {
        query_objects.push_back((*(n.get()))[i]);
      }
      dims[0] = n->size();
      dims[1] = k;
    } catch (const ValueException& e) {
      raise << e.what();
      return NULL;
    }

    std::vector<IntVector> query_res;
Py_BEGIN_ALLOW_THREADS
    query_res = KnnQueryBatch(num_threads, k, query_objects);
Py_END_ALLOW_THREADS

    PyArrayObject* ret = reinterpret_cast<PyArrayObject*>(
        PyArray_FromDims(2, dims, PyArray_INT));
    if (!ret) {
      raise << "failed to create numpy result array";
      return NULL;
    }
    PyArray_ENABLEFLAGS(ret, NPY_ARRAY_OWNDATA);
    for (size_t i = 0; i < query_res.size(); ++i) {
      for (size_t j = 0; j < query_res[i].size() && j < k; ++j) {
        *reinterpret_cast<int*>(PyArray_GETPTR2(ret, i, j)) = query_res[i][j];
      }
    }
    return PyArray_Return(ret);
  }

 private:
  Index<T>* index_;
  Space<T>* space_;
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
  IndexWrapper<T>* index(new IndexWrapper<T>(
          dist_type, data_type,
          space_type, space_param,
          method_name));
  if (!index) {
    raise << "failed to create IndexWrapper";
    return NULL;
  }
  return PyLong_FromVoidPtr(reinterpret_cast<void*>(
          static_cast<IndexWrapperBase*>(index)));
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
    raise << "Error reading parameters (expecting: space type, space parameter "
          << "list, index/method name, data type, distance value type)";
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

PyObject* addDataPoint(PyObject* self, PyObject* args) {
  PyObject* ptr;
  PyObject* data;
  int32_t   id;
  if (!PyArg_ParseTuple(args, "OiO", &ptr, &id, &data)) {
    raise << "Error reading parameters (expecting: index ref, object (as a string))";
    return NULL;
  }
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
  if (index->GetDataType() != kDataDenseVector &&
      index->GetDataType() != kDataSparseVector &&
      index->GetDataType() != kDataString &&
      index->GetDataType() != kDataObjectAsString) {
    raise << "unknown data type - " << index->GetDataType();
    return NULL;
  }
  auto res = index->ReadObject(id, data);
  if (!res.first) {
    raise << "Cannot create a data-point object!";
    return NULL;
  }
  PyObject* pos = PyInt_FromLong(index->AddDataPoint(res.second));
  if (pos == NULL) {
    raise << "failed to create PyObject";
    return NULL;
  }
  return pos;
}

PyObject* addDataPointBatch(PyObject* self, PyObject* args) {
  PyObject* ptr;
  PyArrayObject* ids;
  PyObject* data;
  if (!PyArg_ParseTuple(args, "OO!O", &ptr, &PyArray_Type, &ids, &data)) {
    raise << "Error reading parameters";
    return NULL;
  }
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
  return index->AddDataPointBatch(ids, data);
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
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
Py_BEGIN_ALLOW_THREADS
  index->CreateIndex(index_params);
Py_END_ALLOW_THREADS
  Py_RETURN_NONE;
}

PyObject* saveIndex(PyObject* self, PyObject* args) {
  PyObject*     ptr;
  char*         file_name;

  if (!PyArg_ParseTuple(args, "Os", &ptr, &file_name)) {
    raise << "Error reading parameters (expecting: index ref, file name)";
    return NULL;
  }
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
Py_BEGIN_ALLOW_THREADS
  index->SaveIndex(file_name);
Py_END_ALLOW_THREADS
  Py_RETURN_NONE;
}

PyObject* loadIndex(PyObject* self, PyObject* args) {
  PyObject*     ptr;
  char*         file_name;

  if (!PyArg_ParseTuple(args, "Os", &ptr, &file_name)) {
    raise << "Error reading parameters (expecting: index ref, file name)";
    return NULL;
  }
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
Py_BEGIN_ALLOW_THREADS
  index->LoadIndex(file_name);
Py_END_ALLOW_THREADS
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
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
Py_BEGIN_ALLOW_THREADS
  index->SetQueryTimeParams(query_time_params);
Py_END_ALLOW_THREADS
  Py_RETURN_NONE;
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
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
  if (index->GetDataType() != kDataDenseVector &&
      index->GetDataType() != kDataSparseVector &&
      index->GetDataType() != kDataString &&
      index->GetDataType() != kDataObjectAsString) {
    raise << "unknown data type - " << index->GetDataType();
    return NULL;
  }
  auto res = index->ReadObject(0, data);
  std::unique_ptr<const Object> query_obj(res.second);
  return index->KnnQuery(k, query_obj.get());
}

PyObject* knnQueryBatch(PyObject* self, PyObject* args) {
  PyObject* ptr;
  int num_threads;
  int k;
  PyObject* data;
  if (!PyArg_ParseTuple(args, "OiiO", &ptr, &num_threads,
                        &k, &data)) {
    raise << "Error reading parameters";
    return NULL;
  }
  if (k < 1) {
    raise << "k (" << k << ") should be >=1";
    return NULL;
  }
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
  return index->KnnQueryBatch(num_threads, k, data);
}

PyObject* getDataPoint(PyObject* self, PyObject* args) {
  PyObject* ptr;
  int       id;
  if (!PyArg_ParseTuple(args, "Oi", &ptr, &id)) {
    raise << "Error reading parameters (expecting: index ref, object index)";
    return NULL;
  }
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
  if (id < 0 || static_cast<size_t>(id) >= index->GetDataPointQty()) {
    raise << "The data point index should be >= 0 & < " << index->GetDataPointQty();
    return NULL;
  }
  auto res = index->WriteObject(id);
  if (!res.first) {
    return NULL;
  }
  return res.second;
}

PyObject* getDataPointQty(PyObject* self, PyObject* args) {
  PyObject* ptr;
  if (!PyArg_ParseTuple(args, "O", &ptr)) {
    raise << "Error reading parameters (expecting: index ref)";
    return NULL;
  }
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
  PyObject* tmp = PyInt_FromLong(index->GetDataPointQty());
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
  IndexWrapperBase* index = reinterpret_cast<IndexWrapperBase*>(
      PyLong_AsVoidPtr(ptr));
  delete index;
  Py_RETURN_NONE;
}

