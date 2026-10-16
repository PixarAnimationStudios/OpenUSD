//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Need an empty class to serve as the singleton base class wrapped out to
// python.
struct Tf_PySingleton {};

static nanobind::object
_GetSingletonInstance(nanobind::object const &classObj,
                      nanobind::args,
                      nanobind::kwargs) {
    // Try to get existing instance from this class.
    nanobind::object instance =
        classObj.attr("__dict__").attr("get")("__instance", nanobind::none());

    if (instance.is_none()) {
        // Create instance.
        instance = nanobind::inst_alloc_zero(classObj);

        // Store singleton instance in class.
        nanobind::setattr(classObj, "__instance", instance);

        // If there's an 'init' method, call it.
        if (!nanobind::getattr(instance, "init", nanobind::none()).is_none())
            instance.attr("init")();
    }

    // Return instance.
    return instance;
}

} // anonymous namespace 

void wrapSingleton(nanobind::module_ &m) {
    namespace nb = nanobind;
    nb::class_<Tf_PySingleton>(m, "Singleton", nb::dynamic_attr())
        .def_static("__new__", _GetSingletonInstance)
        .def("__init__", [](nb::handle, nb::args, nb::kwargs) {})
        ;
}
