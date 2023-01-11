//
// Copyright 2024 Pixar
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
#include "pxr/usd/usdText/markupText.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextMarkupText,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("MarkupText")
    // to find TfType<UsdTextMarkupText>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdTextMarkupText>("MarkupText");
}

/* virtual */
UsdTextMarkupText::~UsdTextMarkupText()
{
}

/* static */
UsdTextMarkupText
UsdTextMarkupText::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextMarkupText();
    }
    return UsdTextMarkupText(stage->GetPrimAtPath(path));
}

/* static */
UsdTextMarkupText
UsdTextMarkupText::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("MarkupText");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextMarkupText();
    }
    return UsdTextMarkupText(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdTextMarkupText::_GetSchemaKind() const
{
    return UsdTextMarkupText::schemaKind;
}

/* static */
const TfType &
UsdTextMarkupText::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextMarkupText>();
    return tfType;
}

/* static */
bool 
UsdTextMarkupText::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextMarkupText::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdTextMarkupText::GetMarkupStringAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->markupString);
}

UsdAttribute
UsdTextMarkupText::CreateMarkupStringAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->markupString,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetMarkupLanguageAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->markupLanguage);
}

UsdAttribute
UsdTextMarkupText::CreateMarkupLanguageAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->markupLanguage,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetBackgroundColorAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->primvarsBackgroundColor);
}

UsdAttribute
UsdTextMarkupText::CreateBackgroundColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->primvarsBackgroundColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetBackgroundOpacityAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->primvarsBackgroundOpacity);
}

UsdAttribute
UsdTextMarkupText::CreateBackgroundOpacityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->primvarsBackgroundOpacity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetTextMetricsUnitAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->primvarsTextMetricsUnit);
}

UsdAttribute
UsdTextMarkupText::CreateTextMetricsUnitAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->primvarsTextMetricsUnit,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetRendererAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->renderer);
}

UsdAttribute
UsdTextMarkupText::CreateRendererAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->renderer,
                       SdfValueTypeNames->String,
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
UsdTextMarkupText::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTextTokens->markupString,
        UsdTextTokens->markupLanguage,
        UsdTextTokens->primvarsBackgroundColor,
        UsdTextTokens->primvarsBackgroundOpacity,
        UsdTextTokens->primvarsTextMetricsUnit,
        UsdTextTokens->renderer,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomGprim::GetSchemaAttributeNames(true),
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


#include "pxr/usd/usdGeom/boundableComputeExtent.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/usdGeom/pointBased.h"/*
#include "pxr/usd/usdText/columnStyle.h"
#include "pxr/usd/usdText/columnStyleAPI.h"*/

PXR_NAMESPACE_OPEN_SCOPE

static bool
_ComputeExtentForMarkupText(
    const UsdGeomBoundable& boundable,
    const UsdTimeCode& time,
    const GfMatrix4d* transform,
    VtVec3fArray* extent)
{
    const UsdTextMarkupText markupText(boundable);
    if (!TF_VERIFY(markupText)) {
        return false;
    }

    extent->resize(2);
    (*extent)[0] = GfVec3f(0.0f, -500.0f, -1.0f);
    (*extent)[1] = GfVec3f(500.0f, 0.0f, 1.0f);
    return true;
    //UsdPrim& prim = boundable.GetPrim();
    //// Get the columnStyle from the ColumnStyleBinding.
    //if (UsdTextColumnStyleAPI::CanApply(prim))
    //{
    //    UsdTextColumnStyleAPI::ColumnStyleBinding styleBinding =
    //        UsdTextColumnStyleAPI(prim).GetColumnStyleBinding(prim.GetPath());
    //    std::vector<UsdTextColumnStyle> styles = styleBinding.GetColumnStyles();
    //    // The text prim can bind several column styles, and each represents one column.
    //    if (styles.size() > 0)
    //    {
    //        for (auto style : styles)
    //        {
    //            float columnWidth = 0.0f;
    //            float columnHeight = 0.0f;
    //            GfVec2f offset(0.0f, 0.0f);
    //            // The column width, height, and offset must be specified.
    //            if (!style.GetColumnWidthAttr().Get(&columnWidth, time))
    //                return false;
    //            if (!style.GetColumnHeightAttr().Get(&columnHeight, time))
    //                return false;
    //            if (!style.GetOffsetAttr().Get(&offset, time))
    //                return false;

    //            (*extent)[0][0] = ((*extent)[0][0] < offset[0]) ? (*extent)[0][0] : offset[0];
    //            (*extent)[0][1] = ((*extent)[0][1] < (offset[1] - columnHeight)) ? (*extent)[0][1] : (offset[1] - columnHeight);
    //            (*extent)[1][0] = ((*extent)[1][0] > (offset[0] + columnWidth)) ? (*extent)[1][0] : (offset[0] + columnWidth);
    //            (*extent)[1][1] = ((*extent)[1][1] > offset[1]) ? (*extent)[1][1] : offset[1];
    //        }
    //    }
    //    return true;
    //}
    //else {
    //    return false;
    //}
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdTextMarkupText>(
        _ComputeExtentForMarkupText);
}

PXR_NAMESPACE_CLOSE_SCOPE

