//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/ndr/node.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapNode()
{
    typedef NdrNode This;
    typedef NdrNodePtr ThisPtr;

    return_value_policy<copy_const_reference> copyRefPolicy;

    class_<This, ThisPtr, noncopyable>("Node", no_init)
        .def("__repr__", &This::GetInfoString)
        .def("__bool__", &This::IsValid)
        .def("GetIdentifier", &This::GetIdentifier, copyRefPolicy)
        .def("GetVersion", &This::GetVersion)
        .def("GetName", &This::GetName, copyRefPolicy)
        .def("GetFamily", &This::GetFamily, copyRefPolicy)
        .def("GetContext", &This::GetContext, copyRefPolicy)
        .def("GetSourceType", &This::GetSourceType, copyRefPolicy)
        .def("GetResolvedDefinitionURI", &This::GetResolvedDefinitionURI, copyRefPolicy)
        .def("GetResolvedImplementationURI", &This::GetResolvedImplementationURI, copyRefPolicy)
        .def("IsValid", &This::IsValid)
        .def("GetInfoString", &This::GetInfoString)
        .def("GetInput", &This::GetInput, return_internal_reference<>())
        .def("GetInputNames", &This::GetInputNames, copyRefPolicy)
        .def("GetOutput", &This::GetOutput, return_internal_reference<>())
        .def("GetOutputNames", &This::GetOutputNames, copyRefPolicy)
        .def("GetSourceCode", &This::GetSourceCode, copyRefPolicy)
        .def("GetMetadata", &This::GetMetadata,
            return_value_policy<TfPyMapToDictionary>())
        ;
}
