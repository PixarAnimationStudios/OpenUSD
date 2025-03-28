//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdRiPxrImaging/pxrRenderTerminalHelper.h"
#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (inputs)
);


static TfToken
_GetNodeTypeId(
    UsdPrim const& prim,
    TfToken const& shaderIdToken,
    TfToken const& primTypeToken)
{
    UsdAttribute attr = prim.GetAttribute(shaderIdToken);
    if (attr) {
        VtValue value;
        if (attr.Get(&value)) {
            if (value.IsHolding<TfToken>()) {
                return value.UncheckedGet<TfToken>();
            }
        }
    }
    return primTypeToken;
}

/* static */
HdMaterialNode2
UsdRiPxrImagingRenderTerminalHelper::CreateHdMaterialNode2(
    UsdPrim const& prim,
    TfToken const& shaderIdToken,
    TfToken const& primTypeToken)
{
    HdMaterialNode2 materialNode;
    materialNode.nodeTypeId = _GetNodeTypeId(prim, shaderIdToken, primTypeToken);

    UsdAttributeVector attrs = prim.GetAuthoredAttributes();
    for (const auto& attr : attrs) {
        VtValue value;
        const std::pair<std::string, bool> inputName =
            SdfPath::StripPrefixNamespace(attr.GetName(), _tokens->inputs);
        if (inputName.second && attr.Get(&value)) {
            materialNode.parameters[TfToken(inputName.first)] = value;
        }
    }
    return materialNode;
}

PXR_NAMESPACE_CLOSE_SCOPE
