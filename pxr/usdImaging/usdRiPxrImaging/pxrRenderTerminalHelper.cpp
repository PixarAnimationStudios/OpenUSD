//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
