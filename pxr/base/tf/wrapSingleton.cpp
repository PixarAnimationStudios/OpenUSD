//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/raw_function.hpp"
#include "pxr/external/boost/python/tuple.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

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

} // anonymous namespace 

void wrapSingleton() {
    class_<Tf_PySingleton>("Singleton", no_init)
        .def("__new__", _GetSingletonInstance).staticmethod("__new__")
        .def("__init__", raw_function(_DummyInit))
        ;
}
