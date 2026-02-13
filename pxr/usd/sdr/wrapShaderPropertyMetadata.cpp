//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/usd/sdr/shaderPropertyMetadata.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapShaderPropertyMetadata()
{
    typedef SdrShaderPropertyMetadata This;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "PropertyMetadata", SdrPropertyMetadata, SDR_PROPERTY_METADATA_TOKENS
    );

    TF_PY_WRAP_PUBLIC_TOKENS(
        "PropertyRole",
        SdrPropertyRole,
        SDR_PROPERTY_ROLE_TOKENS
    );

    class_<This>("ShaderPropertyMetadata", no_init)
        .def("HasItem", &This::HasItem)
        .def("GetItemValue", &This::GetItemValue)
        .def("GetItems",
             +[](const SdrShaderPropertyMetadata& metadata) {
                return metadata.GetItems();
             }, return_value_policy<return_by_value>())
        .def("HasLabel", &This::HasLabel)
        .def("GetLabel", &This::GetLabel)
        .def("HasHelp", &This::HasHelp)
        .def("GetHelp", &This::GetHelp)
        .def("HasPage", &This::HasPage)
        .def("GetPage", &This::GetPage)
        .def("HasRenderType", &This::HasRenderType)
        .def("GetRenderType", &This::GetRenderType)
        .def("HasRole", &This::HasRole)
        .def("GetRole", &This::GetRole)
        .def("HasWidget", &This::HasWidget)
        .def("GetWidget", &This::GetWidget)
        .def("HasIsDynamicArray", &This::HasIsDynamicArray)
        .def("GetIsDynamicArray", &This::GetIsDynamicArray)
        .def("HasTupleSize", &This::HasTupleSize)
        .def("GetTupleSize", &This::GetTupleSize)
        .def("HasConnectable", &This::HasConnectable)
        .def("GetConnectable", &This::GetConnectable)
        .def("HasShownIf", &This::HasShownIf)
        .def("GetShownIf", &This::GetShownIf)
        .def("HasValidConnectionTypes", &This::HasValidConnectionTypes)
        .def("GetValidConnectionTypes", &This::GetValidConnectionTypes)
        .def("HasIsAssetIdentifier", &This::HasIsAssetIdentifier)
        .def("GetIsAssetIdentifier", &This::GetIsAssetIdentifier)
        .def("HasImplementationName", &This::HasImplementationName)
        .def("GetImplementationName", &This::GetImplementationName)
        .def("HasSdrUsdDefinitionType", &This::HasSdrUsdDefinitionType)
        .def("GetSdrUsdDefinitionType", &This::GetSdrUsdDefinitionType)
        .def("HasDefaultInput", &This::HasDefaultInput)
        .def("GetDefaultInput", &This::GetDefaultInput)
        .def("HasColorspace", &This::HasColorspace)
        .def("GetColorspace", &This::GetColorspace);
}