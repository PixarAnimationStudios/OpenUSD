//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

namespace std {
template <>
struct is_copy_constructible<PXR_NS::SdrShaderNode> : std::false_type {};
template <>
struct is_move_constructible<PXR_NS::SdrShaderNode> : std::false_type {};
} // namespace std

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapShaderNode(nanobind::module_ &m)
{
    typedef SdrShaderNode This;

    TF_NB_WRAP_PUBLIC_TOKENS(
        m, "NodeFieldKey", SdrNodeFieldKey, SDR_NODE_FIELD_KEY_TOKENS
    );

    nb::class_<This>(m, "ShaderNode")
        .def("__repr__", &This::GetInfoString)
        .def("__bool__", &This::IsValid)
        .def("GetIdentifier", &This::GetIdentifier)
        .def("GetShaderVersion", &This::GetShaderVersion)
        .def("GetName", &This::GetName)
        .def("GetFamily", &This::GetFamily)
        .def("GetFunction", &This::GetFunction)
        .def("GetShadingSystem", &This::GetShadingSystem)
        .def("GetSourceType", &This::GetSourceType)
        .def("GetResolvedDefinitionURI", &This::GetResolvedDefinitionURI)
        .def("GetResolvedImplementationURI",
            &This::GetResolvedImplementationURI)
        .def("IsValid", &This::IsValid)
        .def("GetInfoString", &This::GetInfoString)
        .def("GetSourceCode", &This::GetSourceCode)
        .def("GetMetadata", &This::GetMetadata)
        .def("GetMetadataObject", &This::GetMetadataObject)
        .def("GetShaderInput", &This::GetShaderInput,
            nb::rv_policy::reference_internal)
        .def("GetShaderOutput", &This::GetShaderOutput,
            nb::rv_policy::reference_internal)
        .def("GetShaderInputNames", &This::GetShaderInputNames)
        .def("GetShaderOutputNames", &This::GetShaderOutputNames)
        .def("GetAssetIdentifierInputNames",
            &This::GetAssetIdentifierInputNames)
        .def("GetDefaultInput", &This::GetDefaultInput,
            nb::rv_policy::reference_internal)
        .def("GetDomain", &This::GetDomain)
        .def("GetSubdomain", &This::GetSubdomain)
        .def("GetContext", &This::GetContext)
        .def("GetRole", &This::GetRole)
        .def("GetLabel", &This::GetLabel)
        .def("GetCategory", &This::GetCategory)
        .def("GetHelp", &This::GetHelp)
        .def("GetDepartments", &This::GetDepartments)
        .def("GetPages", &This::GetPages)
        .def("GetOpenPages", &This::GetOpenPages)
        .def("GetPagesShownIf", &This::GetPagesShownIf)
        .def("GetPrimvars", &This::GetPrimvars)
        .def("GetAdditionalPrimvarProperties",
            &This::GetAdditionalPrimvarProperties)
        .def("GetImplementationName", &This::GetImplementationName)
        .def("GetPropertyNamesForPage", &This::GetPropertyNamesForPage)
        .def("GetAllVstructNames", &This::GetAllVstructNames)
        .def("GetDataForKey", &This::GetDataForKey)
        ;
}
