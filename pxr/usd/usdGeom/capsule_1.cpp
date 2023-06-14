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
#include "pxr/usd/usdGeom/capsule_1.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomCapsule_1,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Capsule_1")
    // to find TfType<UsdGeomCapsule_1>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomCapsule_1>("Capsule_1");
}

/* virtual */
UsdGeomCapsule_1::~UsdGeomCapsule_1()
{
}

/* static */
UsdGeomCapsule_1
UsdGeomCapsule_1::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCapsule_1();
    }
    return UsdGeomCapsule_1(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomCapsule_1
UsdGeomCapsule_1::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Capsule_1");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCapsule_1();
    }
    return UsdGeomCapsule_1(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomCapsule_1::_GetSchemaKind() const
{
    return UsdGeomCapsule_1::schemaKind;
}

/* static */
const TfType &
UsdGeomCapsule_1::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomCapsule_1>();
    return tfType;
}

/* static */
bool 
UsdGeomCapsule_1::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomCapsule_1::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomCapsule_1::GetHeightAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->height);
}

UsdAttribute
UsdGeomCapsule_1::CreateHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->height,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCapsule_1::GetRadiusTopAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->radiusTop);
}

UsdAttribute
UsdGeomCapsule_1::CreateRadiusTopAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->radiusTop,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCapsule_1::GetRadiusBottomAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->radiusBottom);
}

UsdAttribute
UsdGeomCapsule_1::CreateRadiusBottomAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->radiusBottom,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCapsule_1::GetAxisAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->axis);
}

UsdAttribute
UsdGeomCapsule_1::CreateAxisAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->axis,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCapsule_1::GetExtentAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->extent);
}

UsdAttribute
UsdGeomCapsule_1::CreateExtentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->extent,
                       SdfValueTypeNames->Float3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
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
UsdGeomCapsule_1::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->height,
        UsdGeomTokens->radiusTop,
        UsdGeomTokens->radiusBottom,
        UsdGeomTokens->axis,
        UsdGeomTokens->extent,
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
