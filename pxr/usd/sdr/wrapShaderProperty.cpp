//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapShaderProperty(nanobind::module_ &m)
{
    typedef SdrShaderProperty This;

    TF_NB_WRAP_PUBLIC_TOKENS(
        m, "PropertyTypes", SdrPropertyTypes, SDR_PROPERTY_TYPE_TOKENS
    );

    nb::class_<This>(m, "ShaderProperty")
        .def("__repr__", &This::GetInfoString)
        .def("GetName", &This::GetName)
        .def("GetType", &This::GetType)
        .def("GetDefaultValue", &This::GetDefaultValue)
        .def("IsOutput", &This::IsOutput)
        .def("IsArray", &This::IsArray)
        .def("IsDynamicArray", &This::IsDynamicArray)
        .def("GetArraySize", &This::GetArraySize)
        .def("GetTupleSize", &This::GetTupleSize)
        .def("GetInfoString", &This::GetInfoString)
        .def("GetMetadata", &This::GetMetadata)
        .def("GetMetadataObject", &This::GetMetadataObject)
        .def("IsConnectable", &This::IsConnectable)
        .def("CanConnectTo", &This::CanConnectTo)
        .def("GetTypeAsSdfType", &This::GetTypeAsSdfType)
        .def("GetDefaultValueAsSdfType", &This::GetDefaultValueAsSdfType)
        .def("GetLabel", &This::GetLabel)
        .def("GetHelp", &This::GetHelp)
        .def("GetPage", &This::GetPage)
        .def("GetWidget", &This::GetWidget)
        .def("GetHints", &This::GetHints)
        .def("GetOptions", &This::GetOptions)
        .def("GetImplementationName", &This::GetImplementationName)
        .def("GetShownIf", &This::GetShownIf)
        .def("GetVStructMemberOf", &This::GetVStructMemberOf)
        .def("GetVStructMemberName", &This::GetVStructMemberName)
        .def("GetVStructConditionalExpr",
            &This::GetVStructConditionalExpr)
        .def("IsVStructMember", &This::IsVStructMember)
        .def("IsVStruct", &This::IsVStruct)
        .def("GetValidConnectionTypes", &This::GetValidConnectionTypes)
        .def("IsAssetIdentifier", &This::IsAssetIdentifier)
        .def("IsDefaultInput", &This::IsDefaultInput)
        ;
}
