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
#include "pxr/usd/usdGeom/cone.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomCone,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename to associate it with the TfType, under
    // UsdSchemaBase. This enables one to call TfType::FindByName("Cone") to find
    // TfType<UsdGeomCone>, which is how IsA queries are answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomCone>("Cone");
}

/* virtual */
UsdGeomCone::~UsdGeomCone()
{
}

/* static */
UsdGeomCone
UsdGeomCone::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (not stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCone();
    }
    return UsdGeomCone(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomCone
UsdGeomCone::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Cone");
    if (not stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCone();
    }
    return UsdGeomCone(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdGeomCone::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomCone>();
    return tfType;
}

/* static */
bool 
UsdGeomCone::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomCone::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomCone::GetHeightAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->height);
}

UsdAttribute
UsdGeomCone::CreateHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->height,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCone::GetRadiusAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->radius);
}

UsdAttribute
UsdGeomCone::CreateRadiusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->radius,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCone::GetAxisAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->axis);
}

UsdAttribute
UsdGeomCone::CreateAxisAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->axis,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCone::GetExtentAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->extent);
}

UsdAttribute
UsdGeomCone::CreateExtentAttr(VtValue const &defaultValue, bool writeSparsely) const
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
UsdGeomCone::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->height,
        UsdGeomTokens->radius,
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

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
