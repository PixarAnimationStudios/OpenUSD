//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/usd/sdr/shaderNodeMetadata.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapShaderNodeMetadata(nanobind::module_ &m)
{
    typedef SdrShaderNodeMetadata This;

    TF_NB_WRAP_PUBLIC_TOKENS(
        m, "NodeMetadata", SdrNodeMetadata, SDR_NODE_METADATA_TOKENS
    );
    TF_NB_WRAP_PUBLIC_TOKENS(
        m, "NodeDomain", SdrNodeDomain, SDR_NODE_DOMAIN_TOKENS
    );
    TF_NB_WRAP_PUBLIC_TOKENS(
        m, "NodeSubdomain", SdrNodeSubdomain, SDR_NODE_SUBDOMAIN_TOKENS
    );
    TF_NB_WRAP_PUBLIC_TOKENS(
        m, "NodeContext", SdrNodeContext, SDR_NODE_CONTEXT_TOKENS
    );
    TF_NB_WRAP_PUBLIC_TOKENS(
        m, "NodeRole", SdrNodeRole, SDR_NODE_ROLE_TOKENS
    );

    nb::class_<This>(m, "ShaderNodeMetadata")
        .def("HasItem", &This::HasItem)
        .def("GetItemValue", &This::GetItemValue)
        .def("GetItems",
             [](const SdrShaderNodeMetadata& metadata) {
                return metadata.GetItems();
             })
        .def_static("GetDefaultValue", &This::GetDefaultValue)
        .def_static("GetDefaultValues", &This::GetDefaultValues)
        .def("HasLabel", &This::HasLabel)
        .def("GetLabel", &This::GetLabel)
        .def("HasCategory", &This::HasCategory)
        .def("GetCategory", &This::GetCategory)
        .def("HasDomain", &This::HasDomain)
        .def("GetDomain", &This::GetDomain)
        .def("HasSubdomain", &This::HasSubdomain)
        .def("GetSubomain", &This::GetSubdomain)
        .def("HasContext", &This::HasContext)
        .def("GetContext", &This::GetContext)
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