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
#include "pxr/usd/usdPhysics/massAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsMassAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdPhysicsMassAPI::~UsdPhysicsMassAPI()
{
}

/* static */
UsdPhysicsMassAPI
UsdPhysicsMassAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsMassAPI();
    }
    return UsdPhysicsMassAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdPhysicsMassAPI::_GetSchemaKind() const
{
    return UsdPhysicsMassAPI::schemaKind;
}

/* static */
bool
UsdPhysicsMassAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdPhysicsMassAPI>(whyNot);
}

/* static */
UsdPhysicsMassAPI
UsdPhysicsMassAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdPhysicsMassAPI>()) {
        return UsdPhysicsMassAPI(prim);
    }
    return UsdPhysicsMassAPI();
}

/* static */
const TfType &
UsdPhysicsMassAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsMassAPI>();
    return tfType;
}

/* static */
bool 
UsdPhysicsMassAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsMassAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdPhysicsMassAPI::GetMassAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsMass);
}

UsdAttribute
UsdPhysicsMassAPI::CreateMassAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsMass,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsMassAPI::GetDensityAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsDensity);
}

UsdAttribute
UsdPhysicsMassAPI::CreateDensityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsDensity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsMassAPI::GetCenterOfMassAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsCenterOfMass);
}

UsdAttribute
UsdPhysicsMassAPI::CreateCenterOfMassAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsCenterOfMass,
                       SdfValueTypeNames->Point3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsMassAPI::GetDiagonalInertiaAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsDiagonalInertia);
}

UsdAttribute
UsdPhysicsMassAPI::CreateDiagonalInertiaAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsDiagonalInertia,
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsMassAPI::GetPrincipalAxesAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsPrincipalAxes);
}

UsdAttribute
UsdPhysicsMassAPI::CreatePrincipalAxesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsPrincipalAxes,
                       SdfValueTypeNames->Quatf,
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
UsdPhysicsMassAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsMass,
        UsdPhysicsTokens->physicsDensity,
        UsdPhysicsTokens->physicsCenterOfMass,
        UsdPhysicsTokens->physicsDiagonalInertia,
        UsdPhysicsTokens->physicsPrincipalAxes,
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
