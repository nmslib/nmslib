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

#ifndef _PY_NMSLIB_GENERIC_H_
#define _PY_NMSLIB_GENERIC_H_

extern "C" void initnmslib_generic();

PyObject* initIndex(PyObject* self, PyObject* args);
PyObject* addDataPoint(PyObject* self, PyObject* args);
PyObject* buildIndex(PyObject* self, PyObject* args);
PyObject* setQueryTimeParams(PyObject* self, PyObject* args);
PyObject* knnQuery(PyObject* self, PyObject* args);
PyObject* freeIndex(PyObject* self, PyObject* args);

#endif
