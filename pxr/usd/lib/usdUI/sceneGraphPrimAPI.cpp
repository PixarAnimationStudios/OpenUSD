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
#include "pxr/usd/usdUI/sceneGraphPrimAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdUISceneGraphPrimAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdUISceneGraphPrimAPI::~UsdUISceneGraphPrimAPI()
{
}

/* static */
UsdUISceneGraphPrimAPI
UsdUISceneGraphPrimAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdUISceneGraphPrimAPI();
    }
    return UsdUISceneGraphPrimAPI(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdUISceneGraphPrimAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdUISceneGraphPrimAPI>();
    return tfType;
}

/* static */
bool 
UsdUISceneGraphPrimAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdUISceneGraphPrimAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdUISceneGraphPrimAPI::GetDisplayNameAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiDisplayName);
}

UsdAttribute
UsdUISceneGraphPrimAPI::CreateDisplayNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiDisplayName,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdUISceneGraphPrimAPI::GetDisplayGroupAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiDisplayGroup);
}

UsdAttribute
UsdUISceneGraphPrimAPI::CreateDisplayGroupAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiDisplayGroup,
                       SdfValueTypeNames->Token,
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
UsdUISceneGraphPrimAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdUITokens->uiDisplayName,
        UsdUITokens->uiDisplayGroup,
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
