//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRenderSettings,
        TfType::Bases< UsdRenderSettingsBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("RenderSettings")
    // to find TfType<UsdRenderSettings>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRenderSettings>("RenderSettings");
}

/* virtual */
UsdRenderSettings::~UsdRenderSettings()
{
}

/* static */
UsdRenderSettings
UsdRenderSettings::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderSettings();
    }
    return UsdRenderSettings(stage->GetPrimAtPath(path));
}

/* static */
UsdRenderSettings
UsdRenderSettings::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("RenderSettings");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderSettings();
    }
    return UsdRenderSettings(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdRenderSettings::_GetSchemaKind() const
{
    return UsdRenderSettings::schemaKind;
}

/* static */
const TfType &
UsdRenderSettings::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRenderSettings>();
    return tfType;
}

/* static */
bool 
UsdRenderSettings::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRenderSettings::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRenderSettings::GetIncludedPurposesAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->includedPurposes);
}

UsdAttribute
UsdRenderSettings::CreateIncludedPurposesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->includedPurposes,
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderSettings::GetMaterialBindingPurposesAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->materialBindingPurposes);
}

UsdAttribute
UsdRenderSettings::CreateMaterialBindingPurposesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->materialBindingPurposes,
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderSettings::GetRenderingColorSpaceAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->renderingColorSpace);
}

UsdAttribute
UsdRenderSettings::CreateRenderingColorSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->renderingColorSpace,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdRenderSettings::GetProductsRel() const
{
    return GetPrim().GetRelationship(UsdRenderTokens->products);
}

UsdRelationship
UsdRenderSettings::CreateProductsRel() const
{
    return GetPrim().CreateRelationship(UsdRenderTokens->products,
                       /* custom = */ false);
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
UsdRenderSettings::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRenderTokens->includedPurposes,
        UsdRenderTokens->materialBindingPurposes,
        UsdRenderTokens->renderingColorSpace,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdRenderSettingsBase::GetSchemaAttributeNames(true),
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

#include "pxr/usd/usdRender/product.h"
#include "pxr/usd/usdRender/var.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/frustum.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdRenderSettings
UsdRenderSettings::GetStageRenderSettings(const UsdStageWeakPtr &stage)
{
    if (!stage){
        TF_CODING_ERROR("Invalid UsdStage");
        return UsdRenderSettings();
    }
    if (stage->HasAuthoredMetadata(UsdRenderTokens->renderSettingsPrimPath)){
        std::string pathStr;
        stage->GetMetadata(UsdRenderTokens->renderSettingsPrimPath, &pathStr);
        if (!pathStr.empty()) {
            SdfPath path(pathStr);
            return UsdRenderSettings(stage->GetPrimAtPath(path));
        }
    }
    return UsdRenderSettings();
}

PXR_NAMESPACE_CLOSE_SCOPE
