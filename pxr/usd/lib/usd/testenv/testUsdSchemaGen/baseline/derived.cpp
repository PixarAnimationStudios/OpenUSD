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
#include "pxr/usd/usdContrived/derived.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedDerived,
        TfType::Bases< UsdContrivedBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Derived")
    // to find TfType<UsdContrivedDerived>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdContrivedDerived>("Derived");
}

/* virtual */
UsdContrivedDerived::~UsdContrivedDerived()
{
}

/* static */
UsdContrivedDerived
UsdContrivedDerived::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedDerived();
    }
    return UsdContrivedDerived(stage->GetPrimAtPath(path));
}

/* static */
UsdContrivedDerived
UsdContrivedDerived::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Derived");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedDerived();
    }
    return UsdContrivedDerived(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdContrivedDerived::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedDerived>();
    return tfType;
}

/* static */
bool 
UsdContrivedDerived::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedDerived::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdContrivedDerived::GetPivotPositionAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->pivotPosition);
}

UsdAttribute
UsdContrivedDerived::CreatePivotPositionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->pivotPosition,
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedDerived::GetMyVecfArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->myVecfArray);
}

UsdAttribute
UsdContrivedDerived::CreateMyVecfArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->myVecfArray,
                       SdfValueTypeNames->Float3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedDerived::GetHoleIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->holeIndices);
}

UsdAttribute
UsdContrivedDerived::CreateHoleIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->holeIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedDerived::GetCornerIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->cornerIndices);
}

UsdAttribute
UsdContrivedDerived::CreateCornerIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->cornerIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedDerived::GetCornerSharpnessesAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->cornerSharpnesses);
}

UsdAttribute
UsdContrivedDerived::CreateCornerSharpnessesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->cornerSharpnesses,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedDerived::GetCreaseLengthsAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->creaseLengths);
}

UsdAttribute
UsdContrivedDerived::CreateCreaseLengthsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->creaseLengths,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedDerived::GetTransformAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->transform);
}

UsdAttribute
UsdContrivedDerived::CreateTransformAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->transform,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedDerived::GetTestingAssetAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->testingAsset);
}

UsdAttribute
UsdContrivedDerived::CreateTestingAssetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->testingAsset,
                       SdfValueTypeNames->AssetArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedDerived::GetNamespacedPropertyAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->namespacedProperty);
}

UsdAttribute
UsdContrivedDerived::CreateNamespacedPropertyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->namespacedProperty,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedDerived::GetJustDefaultAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->justDefault);
}

UsdAttribute
UsdContrivedDerived::CreateJustDefaultAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->justDefault,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdContrivedDerived::GetBindingRel() const
{
    return GetPrim().GetRelationship(UsdContrivedTokens->binding);
}

UsdRelationship
UsdContrivedDerived::CreateBindingRel() const
{
    return GetPrim().CreateRelationship(UsdContrivedTokens->binding,
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
UsdContrivedDerived::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdContrivedTokens->pivotPosition,
        UsdContrivedTokens->myVecfArray,
        UsdContrivedTokens->holeIndices,
        UsdContrivedTokens->cornerIndices,
        UsdContrivedTokens->cornerSharpnesses,
        UsdContrivedTokens->creaseLengths,
        UsdContrivedTokens->transform,
        UsdContrivedTokens->testingAsset,
        UsdContrivedTokens->namespacedProperty,
        UsdContrivedTokens->justDefault,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdContrivedBase::GetSchemaAttributeNames(true),
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
