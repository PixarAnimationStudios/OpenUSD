//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/pyAsset.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapAsset()
{
    typedef Ar_PyAsset This;

    class_<This>
        ("Ar_PyAsset", init<std::shared_ptr<ArAsset>>())

        .def("GetBuffer", &This::GetBuffer)

        .def("__bool__", &This::IsValid)
        .def("__enter__", &This::Enter, return_self<>())
        .def("__exit__", &This::Exit)
        ;
}
