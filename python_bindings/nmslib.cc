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


#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "init.h"
#include "index.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "methodfactory.h"
#include "space.h"
#include "spacefactory.h"
#include "space/space_sparse_vector.h"
#include "thread_pool.h"

namespace py = pybind11;

namespace similarity {
const char * module_name = "nmslib";

enum DistType {
  DISTTYPE_FLOAT,
  DISTTYPE_DOUBLE,
  DISTTYPE_INT
};

enum DataType {
  DATATYPE_DENSE_VECTOR,
  DATATYPE_SPARSE_VECTOR,
  DATATYPE_OBJECT_AS_STRING,
};

// forward references
template <typename dist_t> void exportIndex(py::module * m);
template <typename dist_t> std::string distName();
AnyParams loadParams(py::object o);
void exportLegacyAPI(py::module * m);
void freeObjectVector(ObjectVector * data);

// Wrap a space/objectvector/index together for ease of use
template <typename dist_t>
struct IndexWrapper {
  IndexWrapper(const std::string & method,
        const std::string & space_type,
        py::object space_params,
        DataType data_type,
        DistType dist_type)
      : method(method), space_type(space_type), data_type(data_type), dist_type(dist_type),
        space(SpaceFactoryRegistry<dist_t>::Instance().CreateSpace(space_type,
                                                                   loadParams(space_params))) {
  }

  void createIndex(py::object index_params, bool print_progress = false) {
    AnyParams params = loadParams(index_params);

    py::gil_scoped_release l;
    auto factory = MethodFactoryRegistry<dist_t>::Instance();
    index.reset(factory.CreateMethod(print_progress, method, space_type, *space, data));
    index->CreateIndex(params);
  }

  void loadIndex(const std::string & filename, bool print_progress = false) {
    py::gil_scoped_release l;
    auto factory = MethodFactoryRegistry<dist_t>::Instance();
    index.reset(factory.CreateMethod(print_progress, method, space_type, *space, data));
    index->LoadIndex(filename);

    // querying reloaded indices don't seem to work correctly (at least hnsw ones) until
    // SetQueryTimeParams is called
    index->ResetQueryTimeParams();
  }

  void saveIndex(const std::string & filename) {
    if (!index) {
      throw std::invalid_argument("Must call createIndex or loadIndex before this method");
    }
    py::gil_scoped_release l;
    index->SaveIndex(filename);
  }

  py::object knnQuery(py::object input, size_t k) {
    if (!index) {
      throw std::invalid_argument("Must call createIndex or loadIndex before this method");
    }

    std::unique_ptr<const Object> query(readObject(input));
    KNNQuery<dist_t> knn(*space, query.get(), k);
    {
      py::gil_scoped_release l;
      index->Search(&knn, -1);
    }
    std::unique_ptr<KNNQueue<dist_t>> res(knn.Result()->Clone());
    return convertResult(res.get());
  }

  py::object knnQueryBatch(py::object input, size_t k, int num_threads) {
    if (!index) {
      throw std::invalid_argument("Must call createIndex or loadIndex before this method");
    }

    ObjectVector queries;
    readObjectVector(input, &queries);
    std::vector<std::unique_ptr<KNNQueue<dist_t>>> results(queries.size());
    {
      py::gil_scoped_release l;

      ParallelFor(0, queries.size(), num_threads, [&](size_t query_index) {
        KNNQuery<dist_t> knn(*space, queries[query_index], k);
        index->Search(&knn, -1);
        results[query_index].reset(knn.Result()->Clone());
      });

      // TODO(@benfred): some sort of RAII auto-destroy for this
      freeObjectVector(&queries);
    }

    py::list ret;
    for (auto & result : results) {
      ret.append(convertResult(result.get()));
    }
    return ret;
  }

  py::object convertResult(KNNQueue<dist_t> * res) {
    // Create numpy arrays for the output
    size_t size = res->Size();
    py::array_t<int> ids(size);
    py::array_t<dist_t> distances(size);
    auto raw_ids = ids.mutable_unchecked();
    auto raw_distances = distances.mutable_unchecked();

    while (!res->Empty() && size > 0) {
      // iterating here in reversed order, undo that
      size -= 1;
      raw_ids(size) = res->TopObject()->id();
      raw_distances(size) = res->TopDistance();
      res->Pop();
    }
    return py::make_tuple(ids, distances);
  }

  const Object * readObject(py::object input, int id = 0) {
    switch (data_type) {
      case DATATYPE_DENSE_VECTOR: {
        py::array_t<dist_t> temp(input);
        return new Object(id, -1, temp.size() * sizeof(dist_t), temp.data(0));
      }
      case DATATYPE_OBJECT_AS_STRING: {
        std::string temp = py::cast<std::string>(input);
        return space->CreateObjFromStr(id, -1, temp.c_str(), NULL).release();
      }
      case DATATYPE_SPARSE_VECTOR: {
        // Sparse vectors are expected to be list of (id, value) tuples
        std::vector<SparseVectElem<dist_t>> output;
        py::list items(input);
        for (size_t i = 0; i < items.size(); ++i) {
          py::tuple current(items[i]);
          output.push_back(SparseVectElem<dist_t>(py::cast<uint32_t>(current[0]),
                                                  py::cast<dist_t>(current[1])));
        }
        std::sort(output.begin(), output.end());
        auto sparse = reinterpret_cast<const SpaceSparseVector<dist_t>*>(space.get());
        return sparse->CreateObjFromVect(id, -1, output);
      }
      default:
        throw std::invalid_argument("Unknown data type for readObject");
    }
  }

  // reads multiple items from a python object and inserts onto a similarity::ObjectVector
  // returns the number of elements inserted
  size_t readObjectVector(py::object input, ObjectVector * output,
                          py::object ids_ = py::none()) {
    std::vector<int> ids;
    if (ids_ != py::none()) {
      ids = py::cast<std::vector<int>>(ids_);
    }

    if (py::isinstance<py::list>(input)) {
      py::list items(input);
      for (size_t i = 0; i < items.size(); ++i) {
        output->push_back(readObject(items[i], ids.size() ? ids.at(i) : i));
      }
      return items.size();

    } else if (data_type == DATATYPE_DENSE_VECTOR) {
      // allow numpy arrays to be returned here too
      py::array_t<dist_t> items(input);
      auto buffer = items.request();
      if (buffer.ndim != 2) throw std::runtime_error("data must be a 2d array");

      size_t rows = buffer.shape[0], features = buffer.shape[1];
      for (size_t row = 0; row < rows; ++row) {
        int id = ids.size() ? ids.at(row) : row;
        output->push_back(new Object(id, -1, features * sizeof(dist_t), items.data(row)));
      }
      return rows;

    } else if (data_type == DATATYPE_SPARSE_VECTOR) {
      // the attr calls will fail with an attribute error, but this fixes the legacy
      // unittest case
      if (!py::hasattr(input, "indptr")) {
        throw py::value_error("expect CSR matrix here");
      }

      // try to intrepret input data as a CSR matrix
      py::array_t<int> indptr(input.attr("indptr"));
      py::array_t<int> indices(input.attr("indices"));
      py::array_t<dist_t> sparse_data(input.attr("data"));

      // read each row from the sparse matrix, and insert
      auto sparse_space = reinterpret_cast<const SpaceSparseVector<dist_t>*>(space.get());
      std::vector<SparseVectElem<dist_t>> sparse_items;
      for (size_t rowid = 0; rowid < indptr.size() - 1; ++rowid) {
        sparse_items.clear();

        for (size_t i = indptr.at(rowid); i < indptr.at(rowid + 1); ++i) {
          sparse_items.push_back(SparseVectElem<dist_t>(indices.at(i),
                                 sparse_data.at(i)));
        }
        std::sort(sparse_items.begin(), sparse_items.end());

        int id = ids.size() ? ids.at(rowid) : rowid;
        output->push_back(sparse_space->CreateObjFromVect(id, -1, sparse_items));
      }
      return indptr.size() - 1;
    }

    throw std::invalid_argument("Unknown data type");
  }

  py::object writeObject(const Object * obj) {
    switch (data_type) {
      case DATATYPE_DENSE_VECTOR: {
        py::list ret;
        const dist_t * values = reinterpret_cast<const dist_t *>(obj->data());
        for (size_t i = 0; i < obj->datalength() / sizeof(dist_t); ++i) {
          ret.append(py::cast(values[i]));
        }
        return ret;
      }
      case DATATYPE_OBJECT_AS_STRING: {
        return py::cast(space->CreateStrFromObj(obj, ""));
      }
      case DATATYPE_SPARSE_VECTOR: {
        auto values = reinterpret_cast<const SparseVectElem<dist_t>*>(obj->data());
        size_t count = obj->datalength() / sizeof(SparseVectElem<dist_t>);
        py::list ret;
        for (size_t i = 0; i < count; ++i) {
          ret.append(py::make_tuple(values[i].id_, values[i].val_));
        }
        return ret;
      }
      default:
        throw std::runtime_error("Unknown data_type");
    }
  }

  size_t addDataPoint(int id, py::object input) {
    data.push_back(readObject(input, id));
    return data.size() - 1;
  }

  size_t addDataPointBatch(py::object input, py::object ids = py::none()) {
    return readObjectVector(input, &data, ids);
  }

  inline size_t size() const { return data.size(); }

  py::object at(size_t pos) { return writeObject(data.at(pos)); }

  dist_t getDistance(size_t pos1, size_t pos2) const {
    py::gil_scoped_release l;
    return space->IndexTimeDistance(data.at(pos1), data.at(pos2));
  }

  std::string repr() const {
    std::stringstream ret;
    ret << "<" << module_name << "." << distName<dist_t>() << "Index method='" << method
        << "' space='" << space_type <<  "' at " << this << ">";
    return ret.str();
  }

  ~IndexWrapper() {
    freeObjectVector(&data);
  }

  std::string method;
  std::string space_type;
  DataType data_type;
  DistType dist_type;
  std::unique_ptr<Space<dist_t>> space;
  std::unique_ptr<Index<dist_t>> index;
  ObjectVector data;
};

PYBIND11_PLUGIN(nmslib) {
  // TODO(@benfred): configurable logging
  initLibrary(LIB_LOGSTDERR, NULL);

  py::module m(module_name, "Bindings for Non-Metric Space Library (NMSLIB)");

#ifdef VERSION_INFO
  m.attr("__version__") = py::str(VERSION_INFO);
#else
  m.attr("__version__") = py::str("dev");
#endif

  py::enum_<DistType>(m, "DistType")
    .value("FLOAT", DISTTYPE_FLOAT)
    .value("DOUBLE", DISTTYPE_DOUBLE)
    .value("INT", DISTTYPE_INT);

  py::enum_<DataType>(m, "DataType")
    .value("DENSE_VECTOR", DATATYPE_DENSE_VECTOR)
    .value("SPARSE_VECTOR", DATATYPE_SPARSE_VECTOR)
    .value("OBJECT_AS_STRING", DATATYPE_OBJECT_AS_STRING);

  // Initializes a new index. Param ordering here is set to be consistent with the previous
  // version of the bindings
  m.def("init",
    [](const std::string & space, py::object space_params, const std::string & method,
       DataType data_type, DistType dtype) {
      py::object ret = py::none();
      switch (dtype) {
        case DISTTYPE_FLOAT: {
          auto index = new IndexWrapper<float>(method, space, space_params, data_type, dtype);
          ret = py::cast(index);
          break;
        }
        case DISTTYPE_DOUBLE: {
          auto index = new IndexWrapper<double>(method, space, space_params, data_type, dtype);
          ret = py::cast(index);
          break;
        }
        case DISTTYPE_INT: {
          auto index = new IndexWrapper<int>(method, space, space_params, data_type, dtype);
          ret = py::cast(index);
          break;
        }
        default:
          // should never happen
          throw std::invalid_argument("Invalid DistType");
      }
      return ret;
    },
    py::arg("space") = "cosinesimil",
    py::arg("space_params") = py::none(),
    py::arg("method") = "hnsw",
    py::arg("data_type") = DATATYPE_DENSE_VECTOR,
    py::arg("dtype") = DISTTYPE_FLOAT,
    "This function initializes a new NMSLIB index\n\n"
    "Parameters\n"
    "----------\n"
    "space: str optional\n"
    "    The metric space to create for this index\n"
    "space_params: dict optional\n"
    "    Parameters for configuring the space\n"
    "method: str optional\n"
    "    The index method to use\n"
    "data_type: nmslib.DataType optional\n"
    "    The type of data to index (dense/sparse/string vectors)\n"
    "dist_type: nmslib.DistType optional\n"
    "    The type of index to create (float/double/int)\n"
    "\n"
    "Returns\n"
    "----------\n"
    "    A new NMSLIB Index.\n");

  // Export Different Types of NMS Indices and spaces
  // hiding in a submodule to avoid cluttering up main namespace
  py::module dist_module = m.def_submodule("dist",
    "Contains Indexes and Spaces for different Distance Types");
  exportIndex<int>(&dist_module);
  exportIndex<float>(&dist_module);
  exportIndex<double>(&dist_module);

  exportLegacyAPI(&m);
  return m.ptr();
}

template <typename dist_t>
void exportIndex(py::module * m) {
  // Export the index
  std::string index_name = distName<dist_t>() + "Index";
  py::class_<IndexWrapper<dist_t>>(*m, index_name.c_str())
    .def("createIndex", &IndexWrapper<dist_t>::createIndex,
      py::arg("index_params") = py::none(),
      py::arg("print_progress") = false,
      "Creates the index, and makes it available for querying\n\n"
      "Parameters\n"
      "----------\n"
      "index_params: dict optional\n"
      "    Dictionary of optional parameters to use in indexing\n"
      "print_progress: bool optional\n"
      "    Whether or not to display progress bar when creating index\n")

    .def("knnQuery", &IndexWrapper<dist_t>::knnQuery,
      py::arg("vector"), py::arg("k") = 10,
      "Finds the approximate K nearest neighbours of a vector in the index \n\n"
      "Parameters\n"
      "----------\n"
      "vector: array_like\n"
      "    A 1D vector to query for.\n"
      "k: int optional\n"
      "    The number of neighbours to return\n"
      "\n"
      "Returns\n"
      "----------\n"
      "ids: array_like.\n"
      "    A 1D vector of the ids of each nearest neighbour.\n"
      "distances: array_like.\n"
      "    A 1D vector of the distance to each nearest neigbhour.\n")

    .def("knnQueryBatch", &IndexWrapper<dist_t>::knnQueryBatch,
      py::arg("queries"), py::arg("k") = 10, py::arg("num_threads") = 0,
      "Performs multiple queries on the index, distributing the work over \n"
      "a thread pool\n"
      "Parameters\n"
      "----------\n"
      "input: list\n"
      "    A list of queries to query for\n"
      "k: int optional\n"
      "    The number of neighbours to return\n"
      "num_threads: int optional\n"
      "    The number of threads to use\n"
      "\n"
      "Returns\n"
      "----------\n"
      "list:\n"
      "   A list of tuples of (ids, distances)\n ")

    .def("loadIndex", &IndexWrapper<dist_t>::loadIndex,
      py::arg("filename"),
      py::arg("print_progress") = false,
      "Loads the index from disk\n\n"
      "Parameters\n"
      "----------\n"
      "filename: str\n"
      "    The filename to read from\n",
      "print_progress: bool optional\n"
      "    Whether or not to display progress bar when creating index\n")

    .def("saveIndex", &IndexWrapper<dist_t>::saveIndex,
      py::arg("filename"),
      "Saves the index to disk\n\n"
      "Parameters\n"
      "----------\n"
      "filename: str\n"
      "    The filename to save to\n")

    .def("setQueryTimeParams",
      [](IndexWrapper<dist_t> * self, py::object params) {
        self->index->SetQueryTimeParams(loadParams(params));
      }, py::arg("params") = py::none(),
      "Sets parameters used in knnQuery.\n\n"
      "Parameters\n"
      "----------\n"
      "params: dict\n"
      "    A dictionary of params to use in querying. Setting params to None will reset\n")

    .def("addDataPoint", &IndexWrapper<dist_t>::addDataPoint,
      py::arg("id"),
      py::arg("data"),
      "Saves the index to disk\n\n"
      "Parameters\n"
      "----------\n"
      "id: int\n"
      "    The id of the object to add\n"
      "data: object\n"
      "    The object to add to the index.\n"
      "Returns\n"
      "----------\n"
      "int\n"
      "    The position the item was added at\n")

    .def("addDataPointBatch", &IndexWrapper<dist_t>::addDataPointBatch,
      py::arg("data"),
      py::arg("ids") = py::none(),
      "Saves the index to disk\n\n"
      "Parameters\n"
      "----------\n"
      "data: object\n"
      "    The objects to add to the index.\n"
      "ids: array_like optional\n"
      "    The ids of the object being inserted. If not set will default to the \n"
      "    row id of each object in the dataset\n"
      "Returns\n"
      "----------\n"
      "int\n"
      "    The number of items added\n")

    .def_readonly("dataType", &IndexWrapper<dist_t>::data_type)
    .def_readonly("distType", &IndexWrapper<dist_t>::dist_type)
    .def("__len__", &IndexWrapper<dist_t>::size)
    .def("__getitem__", &IndexWrapper<dist_t>::at)
    .def("getDistance", &IndexWrapper<dist_t>::getDistance)
    .def("__repr__", &IndexWrapper<dist_t>::repr);
}

template <> std::string distName<int>() { return "Int"; }
template <> std::string distName<float>() { return "Float"; }
template <> std::string distName<double>() { return "Double"; }

void freeObjectVector(ObjectVector * data) {
  for (auto datum : *data) {
    delete datum;
  }
}

AnyParams loadParams(py::object o) {
  if (o.is_none()) {
    return AnyParams();
  }

  // if we're given a list of strings like "['key=value', 'key2=value2']",
  if (py::isinstance<py::list>(o)) {
    return AnyParams(py::cast<std::vector<std::string>>(o));
  }

  if (py::isinstance<py::dict>(o)) {
    AnyParams ret;
    py::dict items(o);
    for (auto & item : items) {
      std::string key = py::cast<std::string>(item.first);
      auto & value = item.second;

      // allow param values to be string/int/double
      if (py::isinstance<py::int_>(value)) {
        ret.AddChangeParam(key, py::cast<int>(value));
      } else if (py::isinstance<py::float_>(value)) {
        ret.AddChangeParam(key, py::cast<double>(value));
      } else if (py::isinstance<py::str>(value)) {
        ret.AddChangeParam(key, py::cast<std::string>(value));
      } else {
        std::stringstream err;
        err << "Unknown type for parameter '" << key << "'";
        throw std::invalid_argument(err.str());
      }
    }
    return ret;
  }

  throw std::invalid_argument("Unknown type for parameters");
}

/// Function Definitions for backwards compatibility
void exportLegacyAPI(py::module * m) {
  m->def("addDataPoint", [](py::object self, int id, py::object datum) {
    return self.attr("addDataPoint")(id, datum);
  });
  m->def("addDataPointBatch", [](py::object self, py::object ids, py::object data) {
    // There are multiple unittests that expect this function to raise a ValueError
    // if the types aren't numpy arrays. (the newer api will work and convert
    // the types if lists are passed in etc - while the legacy api expects an exception)
    if (!py::isinstance<py::array_t<int>>(ids)) {
      throw py::value_error("Invalid datatype for ids in addDataPointBatch");
    }

    // Also ensure data is a matrix of the right type
    DataType data_type = py::cast<DataType>(self.attr("dataType"));
    if (data_type == DATATYPE_DENSE_VECTOR) {
      DistType dist_type = py::cast<DistType>(self.attr("distType"));
      if (((dist_type == DISTTYPE_FLOAT) && (!py::isinstance<py::array_t<float>>(data)))  ||
          ((dist_type == DISTTYPE_DOUBLE) && (!py::isinstance<py::array_t<double>>(data)))  ||
          ((dist_type == DISTTYPE_INT) && (!py::isinstance<py::array_t<int>>(data)))) {
        throw py::value_error("Invalid datatype for data in addDataPointBatch");
      }
    }

    size_t offset = py::len(self);
    int insertions = py::cast<int>(self.attr("addDataPointBatch")(data, ids));

    py::array_t<int> positions(insertions);
    for (int i = 0; i < insertions; ++i) {
      positions.mutable_at(i) = offset + i;
    }
    return positions;
  });
  m->def("setQueryTimeParams", [](py::object self, py::object params) {
    return self.attr("setQueryTimeParams")(params);
  });
  m->def("createIndex", [](py::object self, py::object index_params) {
    return self.attr("createIndex")(index_params);
  });
  m->def("saveIndex", [](py::object self, py::object filename) {
    return self.attr("saveIndex")(filename);
  });
  m->def("loadIndex", [](py::object self, py::object filename) {
    return self.attr("loadIndex")(filename);
  });
  m->def("knnQuery", [](py::object self, size_t k, py::object query) {
    // knnQuery now returns a tuple of ids/distances numpy arrays
    // previous version returns list of just ids. convert
    py::tuple ret = self.attr("knnQuery")(query, k);
    py::list ids(ret[0]);
    return ids;
  });
  m->def("getDataPoint", [](py::object self, size_t pos) {
    return self.attr("__getitem__")(pos);
  });
  m->def("getDataPointQty", [](py::object self) {
    return py::len(self);
  });
  m->def("getDistance", [](py::object self, size_t pos1, size_t post2) {
    return self.attr("getDistance")(pos1, post2);
  });

  m->def("knnQueryBatch", [](py::object self, int num_threads, int k, py::object queries) {
    py::list results = self.attr("knnQueryBatch")(queries, k, num_threads);

    // return plain lists of just the ids
    py::list ret;
    for (size_t i = 0; i < results.size(); ++i) {
      py::tuple current(results[i]);
      ret.append(py::list(current[0]));
    }
    return ret;
  });

  m->def("freeIndex", [](py::object self) { });
}
}  // namespace similarity
