//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyObjWrapper.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

struct Tf_PyObjWrapperFromPython {
    Tf_PyObjWrapperFromPython() {
        converter::registry::
            push_back(&_convertible, &_construct,
                      pxr_boost::python::type_id<TfPyObjWrapper>());
    }

private:

    static void*
    _convertible(PyObject *o) {
        // Can always put a python object in a TfPyObjWrapper.
        return o;
    }

    static void
    _construct(PyObject *obj_ptr,
               converter::rvalue_from_python_stage1_data *data) {
        void *storage =
            ((converter::rvalue_from_python_storage<TfPyObjWrapper>*)data)
            ->storage.bytes;
        // Make a TfPyObjWrapper holding the Python object.
        new (storage) TfPyObjWrapper(object(borrowed(obj_ptr)));
        data->convertible = storage;
    }
};

struct Tf_PyObjWrapperToPython {
    static PyObject *
    convert(TfPyObjWrapper const &val) {
        return incref(val.Get().ptr());
    }
};

static TfPyObjWrapper
_RoundTripWrapperTest(TfPyObjWrapper const &wrapper)
{
    return wrapper;
}

static TfPyObjWrapper
_RoundTripWrapperCallTest(TfPyObjWrapper const &wrapper)
{
    return wrapper();
}

static TfPyObjWrapper
_RoundTripWrapperIndexTest(TfPyObjWrapper const &wrapper, int index)
{
    return pxr_boost::python::object(wrapper[index]);
}

} // anonymous namespace 

void wrapPyObjWrapper()
{
    to_python_converter<TfPyObjWrapper, Tf_PyObjWrapperToPython>();
    Tf_PyObjWrapperFromPython();

    def("_RoundTripWrapperTest", _RoundTripWrapperTest);
    def("_RoundTripWrapperCallTest", _RoundTripWrapperCallTest);
    def("_RoundTripWrapperIndexTest", _RoundTripWrapperIndexTest);
}
