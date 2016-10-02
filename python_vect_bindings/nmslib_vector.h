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

#ifndef _PY_NMSLIB_H_
#define _PY_NMSLIB_H_

extern "C" void initnmslib_vector();

PyObject* init(PyObject* self, PyObject* args);
PyObject* addDataPoint(PyObject* self, PyObject* args);
PyObject* setQueryTimeParams(PyObject* self, PyObject* args);
PyObject* createIndex(PyObject* self, PyObject* args);
PyObject* saveIndex(PyObject* self, PyObject* args);
PyObject* loadIndex(PyObject* self, PyObject* args);
PyObject* knnQuery(PyObject* self, PyObject* args);
PyObject* getDataPoint(PyObject* self, PyObject* args);
PyObject* getDataPointQty(PyObject* self, PyObject* args);
PyObject* freeIndex(PyObject* self, PyObject* args);

#endif
