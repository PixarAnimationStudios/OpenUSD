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
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomMotionAPI,
        TfType::Bases< UsdSchemaBase > >();
    
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


/* static */
UsdGeomMotionAPI
UsdGeomMotionAPI::Apply(const UsdStagePtr &stage, const SdfPath &path)
{
    // Ensure we have a valid stage, path and prim
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomMotionAPI();
    }

    if (path == SdfPath::AbsoluteRootPath()) {
        TF_CODING_ERROR("Cannot apply an api schema on the pseudoroot");
        return UsdGeomMotionAPI();
    }

    auto prim = stage->GetPrimAtPath(path);
    if (!prim) {
        TF_CODING_ERROR("Prim at <%s> does not exist.", path.GetText());
        return UsdGeomMotionAPI();
    }

    TfToken apiName("MotionAPI");  

    // Get the current listop at the edit target
    UsdEditTarget editTarget = stage->GetEditTarget();
    SdfPrimSpecHandle primSpec = editTarget.GetPrimSpecForScenePath(path);
    SdfTokenListOp listOp = primSpec->GetInfo(UsdTokens->apiSchemas)
                                    .UncheckedGet<SdfTokenListOp>();

    // Append our name to the prepend list, if it doesnt exist locally
    TfTokenVector prepends = listOp.GetPrependedItems();
    if (std::find(prepends.begin(), prepends.end(), apiName) != prepends.end()) { 
        return UsdGeomMotionAPI();
    }

    SdfTokenListOp prependListOp;
    prepends.push_back(apiName);
    prependListOp.SetPrependedItems(prepends);
    auto result = listOp.ApplyOperations(prependListOp);
    if (!result) {
        TF_CODING_ERROR("Failed to prepend api name to current listop.");
        return UsdGeomMotionAPI();
    }

    // Set the listop at the current edit target and return the API prim
    primSpec->SetInfo(UsdTokens->apiSchemas, VtValue(*result));
    return UsdGeomMotionAPI(prim);
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
        UsdGeomTokens->motionVelocityScale,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdSchemaBase::GetSchemaAttributeNames(true),
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

float UsdGeomMotionAPI::ComputeVelocityScale(UsdTimeCode time) const
{
    UsdPrim prim = GetPrim();
    UsdPrim pseudoRoot = prim.GetStage()->GetPseudoRoot();
    float  velocityScale = 1.0;

    while (prim != pseudoRoot){
        UsdAttribute vsAttr = 
            prim.GetAttribute(UsdGeomTokens->motionVelocityScale);
        if (vsAttr.HasAuthoredValueOpinion() && 
            vsAttr.Get(&velocityScale, time)){
            return velocityScale;
        }
        prim = prim.GetParent();
    }

    return velocityScale;
}

PXR_NAMESPACE_CLOSE_SCOPE
