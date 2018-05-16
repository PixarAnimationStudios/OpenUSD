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
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdSkelBindingAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (BindingAPI)
);

/* virtual */
UsdSkelBindingAPI::~UsdSkelBindingAPI()
{
}

/* static */
UsdSkelBindingAPI
UsdSkelBindingAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSkelBindingAPI();
    }
    return UsdSkelBindingAPI(stage->GetPrimAtPath(path));
}

/*virtual*/
bool 
UsdSkelBindingAPI::_IsAppliedAPISchema() const 
{
    return true;
}

/* static */
UsdSkelBindingAPI
UsdSkelBindingAPI::Apply(const UsdPrim &prim)
{
    return UsdAPISchemaBase::_ApplyAPISchema<UsdSkelBindingAPI>(
            prim, _schemaTokens->BindingAPI);
}

/* static */
const TfType &
UsdSkelBindingAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdSkelBindingAPI>();
    return tfType;
}

/* static */
bool 
UsdSkelBindingAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdSkelBindingAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdSkelBindingAPI::GetGeomBindTransformAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->primvarsSkelGeomBindTransform);
}

UsdAttribute
UsdSkelBindingAPI::CreateGeomBindTransformAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->primvarsSkelGeomBindTransform,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdSkelBindingAPI::GetJointsAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->skelJoints);
}

UsdAttribute
UsdSkelBindingAPI::CreateJointsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->skelJoints,
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdSkelBindingAPI::GetJointIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->primvarsSkelJointIndices);
}

UsdAttribute
UsdSkelBindingAPI::CreateJointIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->primvarsSkelJointIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdSkelBindingAPI::GetJointWeightsAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->primvarsSkelJointWeights);
}

UsdAttribute
UsdSkelBindingAPI::CreateJointWeightsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->primvarsSkelJointWeights,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdSkelBindingAPI::GetBlendShapesAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->skelBlendShapes);
}

UsdAttribute
UsdSkelBindingAPI::CreateBlendShapesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->skelBlendShapes,
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdSkelBindingAPI::GetAnimationSourceRel() const
{
    return GetPrim().GetRelationship(UsdSkelTokens->skelAnimationSource);
}

UsdRelationship
UsdSkelBindingAPI::CreateAnimationSourceRel() const
{
    return GetPrim().CreateRelationship(UsdSkelTokens->skelAnimationSource,
                       /* custom = */ false);
}

UsdRelationship
UsdSkelBindingAPI::GetSkeletonRel() const
{
    return GetPrim().GetRelationship(UsdSkelTokens->skelSkeleton);
}

UsdRelationship
UsdSkelBindingAPI::CreateSkeletonRel() const
{
    return GetPrim().CreateRelationship(UsdSkelTokens->skelSkeleton,
                       /* custom = */ false);
}

UsdRelationship
UsdSkelBindingAPI::GetBlendShapeTargetsRel() const
{
    return GetPrim().GetRelationship(UsdSkelTokens->skelBlendShapeTargets);
}

UsdRelationship
UsdSkelBindingAPI::CreateBlendShapeTargetsRel() const
{
    return GetPrim().CreateRelationship(UsdSkelTokens->skelBlendShapeTargets,
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
UsdSkelBindingAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdSkelTokens->primvarsSkelGeomBindTransform,
        UsdSkelTokens->skelJoints,
        UsdSkelTokens->primvarsSkelJointIndices,
        UsdSkelTokens->primvarsSkelJointWeights,
        UsdSkelTokens->skelBlendShapes,
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


#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usd/usdSkel/utils.h"


PXR_NAMESPACE_OPEN_SCOPE


UsdGeomPrimvar
UsdSkelBindingAPI::GetJointIndicesPrimvar() const
{
    return UsdGeomPrimvar(GetJointIndicesAttr());
}


UsdGeomPrimvar
UsdSkelBindingAPI::CreateJointIndicesPrimvar(bool constant,
                                             int elementSize) const
{
    return UsdGeomImageable(GetPrim()).CreatePrimvar(
        UsdSkelTokens->primvarsSkelJointIndices,
        SdfValueTypeNames->IntArray,
        constant ? UsdGeomTokens->constant : UsdGeomTokens->vertex,
        elementSize);
}


UsdGeomPrimvar
UsdSkelBindingAPI::GetJointWeightsPrimvar() const
{
    return UsdGeomPrimvar(GetJointWeightsAttr());
}


UsdGeomPrimvar
UsdSkelBindingAPI::CreateJointWeightsPrimvar(bool constant,
                                             int elementSize) const
{
    return UsdGeomImageable(GetPrim()).CreatePrimvar(
        UsdSkelTokens->primvarsSkelJointWeights,
        SdfValueTypeNames->FloatArray,
        constant ? UsdGeomTokens->constant : UsdGeomTokens->vertex,
        elementSize);
}



bool
UsdSkelBindingAPI::SetRigidJointInfluence(int jointIndex, float weight) const
{
    UsdGeomPrimvar jointIndicesPv =
        CreateJointIndicesPrimvar(/*constant*/ true, /*elementSize*/ 1);
    UsdGeomPrimvar jointWeightsPv =
        CreateJointWeightsPrimvar(/*constant*/ true, /*elementSize*/ 1);

    VtIntArray indices(1);
    indices[0] = jointIndex;

    VtFloatArray weights(1);
    weights[0] = weight;
    
    return jointIndicesPv.Set(indices) && jointWeightsPv.Set(weights);
}


PXR_NAMESPACE_CLOSE_SCOPE
