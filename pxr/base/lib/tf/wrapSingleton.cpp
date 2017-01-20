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

#include "pxr/pxr.h"

#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/tuple.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

// Need an empty class to serve as the singleton base class wrapped out to
// python.
struct Tf_PySingleton {};


static object
_GetSingletonInstance(object const &classObj) {

    // Try to get existing instance from this class.
    object instance = classObj.attr("__dict__").attr("get")("__instance");

    if (TfPyIsNone(instance)) {
        // Create instance.  Use our first base class in the method resolution
        // order (mro) to create it.
        instance = TfPyGetClassObject
            <Tf_PySingleton>().attr("__mro__")[1].attr("__new__")(classObj);

        // Store singleton instance in class.
        setattr(classObj, "__instance", instance);

        // If there's an 'init' method, call it.
        if (!TfPyIsNone(getattr(instance, "init", object())))
            instance.attr("init")();
    }

    // Return instance.
    return instance;
}


// Need an init method that accepts any arguments and does nothing.
static object _DummyInit(tuple const &, dict const &) { return object(); }


void wrapSingleton() {
    class_<Tf_PySingleton>("Singleton", no_init)
        .def("__new__", _GetSingletonInstance).staticmethod("__new__")
        .def("__init__", raw_function(_DummyInit))
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
