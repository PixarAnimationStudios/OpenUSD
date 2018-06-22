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
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdSkelSkeleton,
        TfType::Bases< UsdGeomBoundable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Skeleton")
    // to find TfType<UsdSkelSkeleton>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdSkelSkeleton>("Skeleton");
}

/* virtual */
UsdSkelSkeleton::~UsdSkelSkeleton()
{
}

/* static */
UsdSkelSkeleton
UsdSkelSkeleton::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSkelSkeleton();
    }
    return UsdSkelSkeleton(stage->GetPrimAtPath(path));
}

/* static */
UsdSkelSkeleton
UsdSkelSkeleton::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Skeleton");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSkelSkeleton();
    }
    return UsdSkelSkeleton(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdSkelSkeleton::_GetSchemaType() const {
    return UsdSkelSkeleton::schemaType;
}

/* static */
const TfType &
UsdSkelSkeleton::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdSkelSkeleton>();
    return tfType;
}

/* static */
bool 
UsdSkelSkeleton::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdSkelSkeleton::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdSkelSkeleton::GetJointsAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->joints);
}

UsdAttribute
UsdSkelSkeleton::CreateJointsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->joints,
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdSkelSkeleton::GetRestTransformsAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->restTransforms);
}

UsdAttribute
UsdSkelSkeleton::CreateRestTransformsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->restTransforms,
                       SdfValueTypeNames->Matrix4dArray,
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
UsdSkelSkeleton::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdSkelTokens->joints,
        UsdSkelTokens->restTransforms,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomBoundable::GetSchemaAttributeNames(true),
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


#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/boundableComputeExtent.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"
#include "pxr/usd/usdSkel/utils.h"


PXR_NAMESPACE_OPEN_SCOPE


/// Plugin extent method.
static bool
_ComputeExtent(const UsdGeomBoundable& boundable,
               const UsdTimeCode& time,
               const GfMatrix4d* transform,
               VtVec3fArray* extent)
{
    UsdSkelSkeleton skel(boundable);
    if (!TF_VERIFY(skel)) {
        return false;
    }

    UsdSkelCache skelCache;

    UsdSkelSkeletonQuery skelQuery =
        skelCache.GetSkelQuery(UsdSkelSkeleton(boundable.GetPrim()));

    // We compute joint transforms in skel space, which is given as:
    //
    //    animationSource.animTransform * skelPrim.localToWorldTransform
    //
    // UsdGeomBoundable, however, wants extent with respect to the skel
    // primitive itself (I.e., with the animation source's transform baked in).
    // Compute a root transform that accounts for the animation source.
    GfMatrix4d rootXform;
    if (skelQuery.ComputeAnimTransform(&rootXform, time)) {
        if(transform)
            rootXform *= *transform;
    } else {
        if(transform)
            rootXform = *transform;
        else
            rootXform.SetIdentity();
    }
    
    if (TF_VERIFY(skelQuery)) {
        // Compute skel-space joint transforms.
        // The extent for this skel is based on the pivots of all joints.
        VtMatrix4dArray skelXforms;
        if (skelQuery.ComputeJointSkelTransforms(&skelXforms, time)) {
            return UsdSkelComputeJointsExtent(skelXforms, extent,
                                              /*padding*/ 0, &rootXform);
        }
    }
    return true;
}


TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdSkelSkeleton>(_ComputeExtent);
}


PXR_NAMESPACE_CLOSE_SCOPE
