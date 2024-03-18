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
#include "pxr/usd/usdText/columnStyle.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextColumnStyle,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ColumnStyle")
    // to find TfType<UsdTextColumnStyle>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdTextColumnStyle>("ColumnStyle");
}

/* virtual */
UsdTextColumnStyle::~UsdTextColumnStyle()
{
}

/* static */
UsdTextColumnStyle
UsdTextColumnStyle::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextColumnStyle();
    }
    return UsdTextColumnStyle(stage->GetPrimAtPath(path));
}

/* static */
UsdTextColumnStyle
UsdTextColumnStyle::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ColumnStyle");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextColumnStyle();
    }
    return UsdTextColumnStyle(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdTextColumnStyle::_GetSchemaKind() const
{
    return UsdTextColumnStyle::schemaKind;
}

/* static */
const TfType &
UsdTextColumnStyle::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextColumnStyle>();
    return tfType;
}

/* static */
bool 
UsdTextColumnStyle::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextColumnStyle::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdTextColumnStyle::GetColumnWidthAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->columnWidth);
}

UsdAttribute
UsdTextColumnStyle::CreateColumnWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->columnWidth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetColumnHeightAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->columnHeight);
}

UsdAttribute
UsdTextColumnStyle::CreateColumnHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->columnHeight,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetOffsetAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->offset);
}

UsdAttribute
UsdTextColumnStyle::CreateOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->offset,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetMarginsAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->margins);
}

UsdAttribute
UsdTextColumnStyle::CreateMarginsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->margins,
                       SdfValueTypeNames->Float4,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetBlockAlignmentAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->blockAlignment);
}

UsdAttribute
UsdTextColumnStyle::CreateBlockAlignmentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->blockAlignment,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetDirectionAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->direction);
}

UsdAttribute
UsdTextColumnStyle::CreateDirectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->direction,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetLinesFlowDirectionAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->linesFlowDirection);
}

UsdAttribute
UsdTextColumnStyle::CreateLinesFlowDirectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->linesFlowDirection,
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
UsdTextColumnStyle::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTextTokens->columnWidth,
        UsdTextTokens->columnHeight,
        UsdTextTokens->offset,
        UsdTextTokens->margins,
        UsdTextTokens->blockAlignment,
        UsdTextTokens->direction,
        UsdTextTokens->linesFlowDirection,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
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
