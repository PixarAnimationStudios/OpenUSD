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
#include "pxr/usd/usdGeom/visibilityAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomVisibilityAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdGeomVisibilityAPI::~UsdGeomVisibilityAPI()
{
}

/* static */
UsdGeomVisibilityAPI
UsdGeomVisibilityAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomVisibilityAPI();
    }
    return UsdGeomVisibilityAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdGeomVisibilityAPI::_GetSchemaKind() const
{
    return UsdGeomVisibilityAPI::schemaKind;
}

/* static */
bool
UsdGeomVisibilityAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdGeomVisibilityAPI>(whyNot);
}

/* static */
UsdGeomVisibilityAPI
UsdGeomVisibilityAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdGeomVisibilityAPI>()) {
        return UsdGeomVisibilityAPI(prim);
    }
    return UsdGeomVisibilityAPI();
}

/* static */
const TfType &
UsdGeomVisibilityAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomVisibilityAPI>();
    return tfType;
}

/* static */
bool 
UsdGeomVisibilityAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomVisibilityAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomVisibilityAPI::GetGuideVisibilityAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->guideVisibility);
}

UsdAttribute
UsdGeomVisibilityAPI::CreateGuideVisibilityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->guideVisibility,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomVisibilityAPI::GetProxyVisibilityAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->proxyVisibility);
}

UsdAttribute
UsdGeomVisibilityAPI::CreateProxyVisibilityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->proxyVisibility,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomVisibilityAPI::GetRenderVisibilityAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->renderVisibility);
}

UsdAttribute
UsdGeomVisibilityAPI::CreateRenderVisibilityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->renderVisibility,
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
UsdGeomVisibilityAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->guideVisibility,
        UsdGeomTokens->proxyVisibility,
        UsdGeomTokens->renderVisibility,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
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

PXR_NAMESPACE_OPEN_SCOPE

UsdAttribute
UsdGeomVisibilityAPI::GetPurposeVisibilityAttr(
    const TfToken &purpose) const
{
    if (purpose == UsdGeomTokens->guide) {
        return GetGuideVisibilityAttr();
    }
    if (purpose == UsdGeomTokens->proxy) {
        return GetProxyVisibilityAttr();
    }
    if (purpose == UsdGeomTokens->render) {
        return GetRenderVisibilityAttr();
    }

    TF_CODING_ERROR(
        "Unexpected purpose '%s' getting purpose visibility attribute for "
        "<%s>.",
        purpose.GetText(),
        GetPrim().GetPath().GetText());
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
