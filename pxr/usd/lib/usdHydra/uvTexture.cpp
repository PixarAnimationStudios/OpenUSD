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
#include "pxr/usd/usdHydra/uvTexture.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdHydraUvTexture,
        TfType::Bases< UsdHydraTexture > >();
    
}

/* virtual */
UsdHydraUvTexture::~UsdHydraUvTexture()
{
}

/* static */
UsdHydraUvTexture
UsdHydraUvTexture::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdHydraUvTexture();
    }
    return UsdHydraUvTexture(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdHydraUvTexture::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdHydraUvTexture>();
    return tfType;
}

/* static */
bool 
UsdHydraUvTexture::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdHydraUvTexture::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdHydraUvTexture::GetUvAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->uv);
}

UsdAttribute
UsdHydraUvTexture::CreateUvAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->uv,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraUvTexture::GetWrapSAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->wrapS);
}

UsdAttribute
UsdHydraUvTexture::CreateWrapSAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->wrapS,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraUvTexture::GetWrapTAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->wrapT);
}

UsdAttribute
UsdHydraUvTexture::CreateWrapTAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->wrapT,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraUvTexture::GetMinFilterAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->minFilter);
}

UsdAttribute
UsdHydraUvTexture::CreateMinFilterAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->minFilter,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraUvTexture::GetMagFilterAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->magFilter);
}

UsdAttribute
UsdHydraUvTexture::CreateMagFilterAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->magFilter,
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
UsdHydraUvTexture::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdHydraTokens->uv,
        UsdHydraTokens->wrapS,
        UsdHydraTokens->wrapT,
        UsdHydraTokens->minFilter,
        UsdHydraTokens->magFilter,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdHydraTexture::GetSchemaAttributeNames(true),
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
