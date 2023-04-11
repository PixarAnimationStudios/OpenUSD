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
#include "pxr/usd/usdGeom/motionAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomMotionAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdGeomMotionAPI::~UsdGeomMotionAPI()
{
}

/* static */
UsdGeomMotionAPI
UsdGeomMotionAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomMotionAPI();
    }
    return UsdGeomMotionAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdGeomMotionAPI::_GetSchemaKind() const
{
    return UsdGeomMotionAPI::schemaKind;
}

/* static */
bool
UsdGeomMotionAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdGeomMotionAPI>(whyNot);
}

/* static */
UsdGeomMotionAPI
UsdGeomMotionAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdGeomMotionAPI>()) {
        return UsdGeomMotionAPI(prim);
    }
    return UsdGeomMotionAPI();
}

/* static */
const TfType &
UsdGeomMotionAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomMotionAPI>();
    return tfType;
}

/* static */
bool 
UsdGeomMotionAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomMotionAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomMotionAPI::GetMotionBlurScaleAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->motionBlurScale);
}

UsdAttribute
UsdGeomMotionAPI::CreateMotionBlurScaleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->motionBlurScale,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMotionAPI::GetVelocityScaleAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->motionVelocityScale);
}

UsdAttribute
UsdGeomMotionAPI::CreateVelocityScaleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->motionVelocityScale,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMotionAPI::GetNonlinearSampleCountAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->motionNonlinearSampleCount);
}

UsdAttribute
UsdGeomMotionAPI::CreateNonlinearSampleCountAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->motionNonlinearSampleCount,
                       SdfValueTypeNames->Int,
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
UsdGeomMotionAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->motionBlurScale,
        UsdGeomTokens->motionVelocityScale,
        UsdGeomTokens->motionNonlinearSampleCount,
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

template <class T> 
static T _ComputeInheritedMotionAttr(UsdPrim const &queriedPrim,
                                     TfToken const &attrName,
                                     T const &fallbackValue,
                                     UsdTimeCode time)
{
    UsdPrim prim(queriedPrim);
    UsdPrim pseudoRoot = prim.GetStage()->GetPseudoRoot();
    T val = fallbackValue;

    while (prim != pseudoRoot){
        if (prim.HasAPI<UsdGeomMotionAPI>()){
            UsdAttribute attr = 
                prim.GetAttribute(attrName);
            if (attr.HasAuthoredValue() && 
                attr.Get(&val, time)){
                return val;
            }
        }
        prim = prim.GetParent();
    }

    return val;
}

float UsdGeomMotionAPI::ComputeVelocityScale(UsdTimeCode time) const
{
    return _ComputeInheritedMotionAttr<float>(
                  GetPrim(), 
                  UsdGeomTokens->motionVelocityScale,
                  1.0,
                  time);
}

int UsdGeomMotionAPI::ComputeNonlinearSampleCount(UsdTimeCode time) const
{
    return _ComputeInheritedMotionAttr<int>(
                  GetPrim(), 
                  UsdGeomTokens->motionNonlinearSampleCount,
                  3,
                  time);
}

float UsdGeomMotionAPI::ComputeMotionBlurScale(UsdTimeCode time) const
{
    return _ComputeInheritedMotionAttr<float>(
                  GetPrim(), 
                  UsdGeomTokens->motionBlurScale,
                  1.0,
                  time);
}

PXR_NAMESPACE_CLOSE_SCOPE
