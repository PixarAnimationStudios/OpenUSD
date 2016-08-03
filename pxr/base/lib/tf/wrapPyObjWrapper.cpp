//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <boost/python.hpp>

#include "pxr/base/tf/pyObjWrapper.h"

using namespace boost::python;

struct Tf_PyObjWrapperFromPython {
    Tf_PyObjWrapperFromPython() {
        converter::registry::
            push_back(&_convertible, &_construct,
                      boost::python::type_id<TfPyObjWrapper>());
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
    return boost::python::object(wrapper[index]);
}

void wrapPyObjWrapper()
{
    to_python_converter<TfPyObjWrapper, Tf_PyObjWrapperToPython>();
    Tf_PyObjWrapperFromPython();

    def("_RoundTripWrapperTest", _RoundTripWrapperTest);
    def("_RoundTripWrapperCallTest", _RoundTripWrapperCallTest);
    def("_RoundTripWrapperIndexTest", _RoundTripWrapperIndexTest);
}
