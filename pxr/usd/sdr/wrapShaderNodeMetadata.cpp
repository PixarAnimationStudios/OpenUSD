//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/usd/sdr/shaderNodeMetadata.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapShaderNodeMetadata()
{
    typedef SdrShaderNodeMetadata This;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "NodeMetadata", SdrNodeMetadata, SDR_NODE_METADATA_TOKENS
    );
    TF_PY_WRAP_PUBLIC_TOKENS(
        "NodeContext", SdrNodeContext, SDR_NODE_CONTEXT_TOKENS
    );

    TF_PY_WRAP_PUBLIC_TOKENS(
        "NodeRole", SdrNodeRole, SDR_NODE_ROLE_TOKENS
    );

    class_<This>("ShaderNodeMetadata", no_init)
        .def("HasItem", &This::HasItem)
        .def("GetItemValue", &This::GetItemValue)
        .def("GetItems",
             +[](const SdrShaderNodeMetadata& metadata) {
                return metadata.GetItems();
             }, return_value_policy<return_by_value>())
        .def("HasLabel", &This::HasLabel)
        .def("GetLabel", &This::GetLabel)
        .def("HasCategory", &This::HasCategory)
        .def("GetCategory", &This::GetCategory)
        .def("HasRole", &This::HasRole)
        .def("GetRole", &This::GetRole)
        .def("HasHelp", &This::HasHelp)
        .def("GetHelp", &This::GetHelp)
        .def("HasDepartments", &This::HasDepartments)
        .def("GetDepartments", &This::GetDepartments)
        .def("HasPages", &This::HasPages)
        .def("GetPages", &This::GetPages)
        .def("HasOpenPages", &This::HasOpenPages)
        .def("GetOpenPages", &This::GetOpenPages)
        .def("HasPagesShownIf", &This::HasPagesShownIf)
        .def("GetPagesShownIf", &This::GetPagesShownIf)
        .def("HasPrimvars", &This::HasPrimvars)
        .def("GetPrimvars", &This::GetPrimvars)
        .def("HasImplementationName", &This::HasImplementationName)
        .def("GetImplementationName", &This::GetImplementationName)
        .def("HasSdrUsdEncodingVersion", &This::HasSdrUsdEncodingVersion)
        .def("GetSdrUsdEncodingVersion", &This::GetSdrUsdEncodingVersion)
        .def("HasSdrDefinitionNameFallbackPrefix",
             &This::HasSdrDefinitionNameFallbackPrefix)
        .def("GetSdrDefinitionNameFallbackPrefix",
             &This::GetSdrDefinitionNameFallbackPrefix);
}