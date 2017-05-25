//
// Copyright 2016 Pixar
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
#include "pxr/usd/usdUI/nodeGraphNodeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdUINodeGraphNodeAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdUINodeGraphNodeAPI::~UsdUINodeGraphNodeAPI()
{
}

/* static */
UsdUINodeGraphNodeAPI
UsdUINodeGraphNodeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdUINodeGraphNodeAPI();
    }
    return UsdUINodeGraphNodeAPI(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdUINodeGraphNodeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdUINodeGraphNodeAPI>();
    return tfType;
}

/* static */
bool 
UsdUINodeGraphNodeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdUINodeGraphNodeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdUINodeGraphNodeAPI::GetPosAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiNodegraphNodePos);
}

UsdAttribute
UsdUINodeGraphNodeAPI::CreatePosAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiNodegraphNodePos,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdUINodeGraphNodeAPI::GetStackingOrderAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiNodegraphNodeStackingOrder);
}

UsdAttribute
UsdUINodeGraphNodeAPI::CreateStackingOrderAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiNodegraphNodeStackingOrder,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdUINodeGraphNodeAPI::GetDisplayColorAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiNodegraphNodeDisplayColor);
}

UsdAttribute
UsdUINodeGraphNodeAPI::CreateDisplayColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiNodegraphNodeDisplayColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdUINodeGraphNodeAPI::GetIconAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiNodegraphNodeIcon);
}

UsdAttribute
UsdUINodeGraphNodeAPI::CreateIconAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiNodegraphNodeIcon,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdUINodeGraphNodeAPI::GetExpansionStateAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiNodegraphNodeExpansionState);
}

UsdAttribute
UsdUINodeGraphNodeAPI::CreateExpansionStateAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiNodegraphNodeExpansionState,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdUINodeGraphNodeAPI::GetSizeAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiNodegraphNodeSize);
}

UsdAttribute
UsdUINodeGraphNodeAPI::CreateSizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiNodegraphNodeSize,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdUINodeGraphNodeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdUITokens->uiNodegraphNodePos,
        UsdUITokens->uiNodegraphNodeStackingOrder,
        UsdUITokens->uiNodegraphNodeDisplayColor,
        UsdUITokens->uiNodegraphNodeIcon,
        UsdUITokens->uiNodegraphNodeExpansionState,
        UsdUITokens->uiNodegraphNodeSize,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdSchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
