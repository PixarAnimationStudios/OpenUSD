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
UsdSchemaType UsdRenderSettings::_GetSchemaType() const {
    return UsdRenderSettings::schemaType;
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
