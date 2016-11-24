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
typedef BoolObject (*DataReaderFunc)(const Space<float>*, PyObject*, int, int);
BoolObject readDenseVector(const Space<float>*, PyObject*, int, int);
BoolObject readSparseVector(const Space<float>*, PyObject*, int, int);
typedef PyObject* (*DataWriterFunc)(const Object*);
PyObject* writeDenseVector(const Object*);
PyObject* writeSparseVector(const Object*);

const int kDataDenseVector = 1;
const int kDataString = 2;
const int kDataSparseVector = 3;

const std::map<std::string, int> NMSLIB_DATA_TYPES = {
  {"DENSE_VECTOR", kDataDenseVector},
  {"STRING", kDataString},
  {"SPARSE_VECTOR", kDataSparseVector},
};
const std::map<int, DataReaderFunc> NMSLIB_DATA_READERS = {
  {kDataDenseVector, &readDenseVector},
  {kDataSparseVector, &readSparseVector},
};
const std::map<int, DataWriterFunc> NMSLIB_DATA_WRITERS = {
  {kDataDenseVector, &writeDenseVector},
  {kDataSparseVector, &writeSparseVector},
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

BoolObject readDenseVector(const Space<float>* space,
                           PyObject* data,
                           int id,
                           int dist_type) {
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

BoolObject readSparseVector(const Space<float>* space,
                            PyObject* data,
                            int id,
                            int dist_type) {
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

PyObject* writeDenseVector(const Object* obj) {
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

PyObject* writeSparseVector(const Object* obj) {
  const SparseVectElem<float>* arr =
      reinterpret_cast<const SparseVectElem<float>*>(obj->data());
  size_t qty = obj->datalength() / sizeof(SparseVectElem<float>);
  PyObject* z = PyList_New(qty);
  if (!z) {
    return NULL;
  }
  for (size_t i = 0; i < qty; ++i) {
    PyObject* id = PyInt_FromLong(arr[i].id_);
    if (!id) {
      Py_DECREF(z);
      return NULL;
    }
    PyObject* v = PyFloat_FromDouble(arr[i].val_);
    if (!v) {
      Py_DECREF(z);
      Py_DECREF(id);
      return NULL;
    }
    PyObject* p = PyList_New(2);
    if (!p) {
      // TODO(@bileg): need to release all previous p
      Py_DECREF(z);
      Py_DECREF(id);
      Py_DECREF(v);
      return NULL;
    }
    PyList_SET_ITEM(p, 0, id);
    PyList_SET_ITEM(p, 1, v);
    PyList_SET_ITEM(z, i, p);
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
        space_(nullptr) {
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
  inline const Space<T>* GetSpace() const { return space_; }

  size_t AddDataPoint(const Object* z) {
    data_.push_back(z);
    return data_.size() - 1;
  }

  void SetDataPoint(const Object* z, size_t idx) {
    if (idx >= data_.size()) {
      data_.resize(idx + 1);
    }
    data_[idx] = z;
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

  std::vector<IntVector> KnnQueryBatch(const int num_threads, const int k,
                                       const ObjectVector& query_objects) {
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
  IndexWrapper<T>* index(new IndexWrapper<T>(
          dist_type, data_type,
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
      {
        raise << "This version is optimized for vectors. "
              << "Use generic bindings for dist type - " << dist_type;
        return NULL;
      }
    default:
      {
        raise << "unknown dist type - " << dist_type;
        return NULL;
      }
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
  auto res = (*iter->second)(index->GetSpace(), data, id, index->GetDistType());
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
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
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

class NumpyDenseMatrix {
 public:
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

  const int size() const { return num_vec_; }

  const Object* operator[](ssize_t idx) const {
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

class NumpySparseMatrix {
 public:
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

  const int size() const { return n_ - 1; }

  const Object* operator[](ssize_t idx) const {
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

template <typename T, typename N>
PyObject* _addDataPointBatch(IndexWrapper<T>* index,
                             PyArrayObject* ids,
                             PyObject* matrix) {
  try {
    N n(index->GetSpace(), ids, matrix);
    int dims[1];
    dims[0] = n.size();
    PyArrayObject* positions = reinterpret_cast<PyArrayObject*>(
        PyArray_FromDims(1, dims, PyArray_INT));
    if (!positions) {
      raise << "failed to create numpy array for positions";
      return NULL;
    }
    PyArray_ENABLEFLAGS(positions, NPY_ARRAY_OWNDATA);
    int* ptr = reinterpret_cast<int*>(positions->data);
#if 1
    for (int i = 0; i < n.size(); ++i) {
      ptr[i] = index->AddDataPoint(n[i]);
    }
#else
Py_BEGIN_ALLOW_THREADS
    const int num_threads = 10;
    std::queue<std::pair<int,const Object*>> q;
    std::mutex m;
    const size_t num_vec = index->GetDataPointQty();
    for (int i = 0; i < n.size(); ++i) {       // TODO: this can be improved by not adding all (i.e. fixed size thread-pool)
      q.push(std::make_pair(i, n[i]));
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
                    //ptr[p.first] = index->AddDataPoint(p.second);
                    index->SetDataPoint(p.second, num_vec + p.first);
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

template <typename T>
PyObject* _addDataPointBatch(PyObject* ptr,
                             PyArrayObject* ids,
                             PyObject* matrix) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  switch (index->GetDataType()) {
    case kDataDenseVector:
      return _addDataPointBatch<T, NumpyDenseMatrix>(index, ids, matrix);
    case kDataSparseVector:
      return _addDataPointBatch<T, NumpySparseMatrix>(index, ids, matrix);
    default:
      raise << "This version is optimized for DENSE_VECTOR and SPARSE_VECTOR "
            << "Use generic binding for data type " << index->GetDataType();
      return NULL;
  }
}

PyObject* addDataPointBatch(PyObject* self, PyObject* args) {
  PyObject* ptr;
  PyArrayObject* ids;
  PyObject* data;
  if (!PyArg_ParseTuple(args, "OO!O", &ptr, &PyArray_Type, &ids, &data)) {
    raise << "Error reading parameters";
    return NULL;
  }
  if (IsDistFloat(ptr)) {
    return _addDataPointBatch<float>(ptr, ids, data);
  } else {
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
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
    Py_RETURN_NONE;
  } else {
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
  }
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
    Py_RETURN_NONE;
  } else {
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
  }
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
    Py_RETURN_NONE;
  } else {
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
  }
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
    Py_RETURN_NONE;
  } else {
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
  }
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
  auto res = (*iter->second)(index->GetSpace(), data, 0, index->GetDistType());
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
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
  }
}

template <typename T, typename N>
PyObject* _knnQueryBatch(IndexWrapper<T>* index,
                         const int num_threads,
                         const int k,
                         PyObject* matrix) {
  ObjectVector query_objects;
  int dims[2];
  try {
    N n(index->GetSpace(), nullptr, matrix);
    for (int i = 0; i < n.size(); ++i) {
      query_objects.push_back(n[i]);
    }
    dims[0] = n.size();
    dims[1] = k;
  } catch (const ValueException& e) {
    raise << e.what();
    return NULL;
  }

  std::vector<IntVector> query_res;
Py_BEGIN_ALLOW_THREADS
  query_res = index->KnnQueryBatch(num_threads, k, query_objects);
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

template <typename T>
PyObject* _knnQueryBatch(PyObject* ptr,
                         const int num_threads,
                         const int k,
                         PyObject* matrix) {
  IndexWrapper<T>* index = reinterpret_cast<IndexWrapper<T>*>(
      PyLong_AsVoidPtr(ptr));
  switch (index->GetDataType()) {
    case kDataDenseVector:
      return _knnQueryBatch<T, NumpyDenseMatrix>(index, num_threads, k, matrix);
    case kDataSparseVector:
      return _knnQueryBatch<T, NumpySparseMatrix>(index, num_threads, k, matrix);
    default:
      raise << "This version is optimized for DENSE_VECTOR and SPARSE_VECTOR "
            << "Use generic binding for data type " << index->GetDataType();
      return NULL;
  }
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
  if (IsDistFloat(ptr)) {
    return _knnQueryBatch<float>(ptr, num_threads, k, data);
  } else {
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
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
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
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
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
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
    Py_RETURN_NONE;
  } else {
    raise << "This version is optimized for vectors. "
          << "Use generic bindings for dist type - int";
    return NULL;
  }
}

