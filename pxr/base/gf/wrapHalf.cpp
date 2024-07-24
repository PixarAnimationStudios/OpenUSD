//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/half.h"

#include <boost/python/def.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/converter/from_python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Registers to and from python conversions with boost.python for half.
struct HalfPythonConversions
{
    static void Register() {
        // to-python
        to_python_converter<GfHalf, HalfPythonConversions>();
        // from-python
        converter::registry::push_back(&_convertible, &_construct,
                                       boost::python::type_id<GfHalf>());
    }

    // to-python
    static PyObject *convert(GfHalf h) { return PyFloat_FromDouble(h); }

private:
    // from-python
    static void *_convertible(PyObject *obj_ptr) {
        // Must be number-like.
        if (!PyNumber_Check(obj_ptr))
            return NULL;
        // Try to convert to python float: if we can, then we can make a GfHalf.
        if (PyObject *flt = PyNumber_Float(obj_ptr))
            return flt;
        // Otherwise we cannot produce a GfHalf.  Clear any python exception
        // raised above when attempting to create the float.
        if (PyErr_Occurred())
            PyErr_Clear();
        return NULL;
    }
    static void _construct(PyObject *obj_ptr, converter::
                           rvalue_from_python_stage1_data *data) {
        // Pull out the python float we returned from _convertible().
        PyObject *flt = (PyObject *)data->convertible;
        // Turn the python float into a C++ double, make a GfHalf from that
        // double, and store it where boost.python expects it.
        void *storage =
            ((converter::rvalue_from_python_storage<GfHalf>*)data)->storage.bytes;
        new (storage) GfHalf(static_cast<float>(PyFloat_AsDouble(flt)));
        data->convertible = storage;
        // Drop our reference to the python float we created.
        Py_DECREF(flt);
    }
};

// Simple test function that takes and returns a GfHalf.
static GfHalf _HalfRoundTrip(GfHalf in) { return in; }

} // anonymous namespace 

void wrapHalf()
{
    HalfPythonConversions::Register();
    boost::python::def("_HalfRoundTrip", _HalfRoundTrip);
}
