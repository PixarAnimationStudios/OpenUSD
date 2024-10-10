//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/primTypeInfo.h"
#include "pxr/usd/usd/pyConversions.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdPrimTypeInfo()
{
    typedef UsdPrimTypeInfo This;
    class_<This, noncopyable>("PrimTypeInfo", no_init)
        .def("GetTypeName", &This::GetTypeName,
             return_value_policy<return_by_value>())
        .def("GetAppliedAPISchemas", &This::GetAppliedAPISchemas,
             return_value_policy<TfPySequenceToList>())
        .def("GetSchemaType", &This::GetSchemaType,
             return_value_policy<return_by_value>())
        .def("GetSchemaTypeName", &This::GetSchemaTypeName,
             return_value_policy<return_by_value>())
        .def("GetPrimDefinition", &This::GetPrimDefinition,
             return_internal_reference<>())
        .def(self == self)
        .def(self != self)

        .def("GetEmptyPrimType", &This::GetEmptyPrimType,
             return_internal_reference<>())
        .staticmethod("GetEmptyPrimType")
        ;
}
