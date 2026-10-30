//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/plug/testPlugBase.h"
#include "pxr/base/tf/pyPtrHelpers.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

template <class T>
void wrap_TestPlugBase(nanobind::module_ &m, const std::string &name)
{
    namespace nb = nanobind;
    typedef T This;
    nb::class_<This>(m, name.c_str(),
                     nb::is_weak_referenceable(), nb::dynamic_attr())
        .def(TfNbRefAndWeakPtr())
        .def(TfNbMakePyConstructor(&This::New))

        // Expose Manufacture as another initializer.
        .def(TfNbMakePyConstructor(&This::Manufacture))

        .def("GetTypeName", &This::GetTypeName)

        .def("TestAcceptPluginSequence", &This::TestAcceptPluginSequence,
             nb::arg("plugins"))
        ;
}

} // anonymous namespace 

void wrap_TestPlugBase(nanobind::module_ &m)
{
    wrap_TestPlugBase<_TestPlugBase1>(m, "_TestPlugBase1");
    wrap_TestPlugBase<_TestPlugBase2>(m, "_TestPlugBase2");
    wrap_TestPlugBase<_TestPlugBase3>(m, "_TestPlugBase3");
    wrap_TestPlugBase<_TestPlugBase4>(m, "_TestPlugBase4");
}
