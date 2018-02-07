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
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomModelAPI,
        TfType::Bases< UsdModelAPI > >();
    
}

/* virtual */
UsdGeomModelAPI::~UsdGeomModelAPI()
{
}

/* static */
UsdGeomModelAPI
UsdGeomModelAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomModelAPI();
    }
    return UsdGeomModelAPI(stage->GetPrimAtPath(path));
}


/* static */
UsdGeomModelAPI
UsdGeomModelAPI::Apply(const UsdStagePtr &stage, const SdfPath &path)
{
    // Ensure we have a valid stage, path and prim
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomModelAPI();
    }

    if (path == SdfPath::AbsoluteRootPath()) {
        TF_CODING_ERROR("Cannot apply an api schema on the pseudoroot");
        return UsdGeomModelAPI();
    }

    auto prim = stage->GetPrimAtPath(path);
    if (!prim) {
        TF_CODING_ERROR("Prim at <%s> does not exist.", path.GetText());
        return UsdGeomModelAPI();
    }

    TfToken apiName("GeomModelAPI");  

    // Get the current listop at the edit target
    UsdEditTarget editTarget = stage->GetEditTarget();
    SdfPrimSpecHandle primSpec = editTarget.GetPrimSpecForScenePath(path);
    SdfTokenListOp listOp = primSpec->GetInfo(UsdTokens->apiSchemas)
                                    .UncheckedGet<SdfTokenListOp>();

    // Append our name to the prepend list, if it doesnt exist locally
    TfTokenVector prepends = listOp.GetPrependedItems();
    if (std::find(prepends.begin(), prepends.end(), apiName) != prepends.end()) { 
        return UsdGeomModelAPI();
    }

    SdfTokenListOp prependListOp;
    prepends.push_back(apiName);
    prependListOp.SetPrependedItems(prepends);
    auto result = listOp.ApplyOperations(prependListOp);
    if (!result) {
        TF_CODING_ERROR("Failed to prepend api name to current listop.");
        return UsdGeomModelAPI();
    }

    // Set the listop at the current edit target and return the API prim
    primSpec->SetInfo(UsdTokens->apiSchemas, VtValue(*result));
    return UsdGeomModelAPI(prim);
}

/* static */
const TfType &
UsdGeomModelAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomModelAPI>();
    return tfType;
}

/* static */
bool 
UsdGeomModelAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomModelAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomModelAPI::GetModelDrawModeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelDrawMode);
}

UsdAttribute
UsdGeomModelAPI::CreateModelDrawModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelDrawMode,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomModelAPI::GetModelApplyDrawModeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelApplyDrawMode);
}

UsdAttribute
UsdGeomModelAPI::CreateModelApplyDrawModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelApplyDrawMode,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomModelAPI::GetModelDrawModeColorAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelDrawModeColor);
}

UsdAttribute
UsdGeomModelAPI::CreateModelDrawModeColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelDrawModeColor,
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomModelAPI::GetModelCardGeometryAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelCardGeometry);
}

UsdAttribute
UsdGeomModelAPI::CreateModelCardGeometryAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelCardGeometry,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomModelAPI::GetModelCardTextureXPosAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelCardTextureXPos);
}

UsdAttribute
UsdGeomModelAPI::CreateModelCardTextureXPosAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelCardTextureXPos,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomModelAPI::GetModelCardTextureYPosAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelCardTextureYPos);
}

UsdAttribute
UsdGeomModelAPI::CreateModelCardTextureYPosAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelCardTextureYPos,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomModelAPI::GetModelCardTextureZPosAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelCardTextureZPos);
}

UsdAttribute
UsdGeomModelAPI::CreateModelCardTextureZPosAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelCardTextureZPos,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomModelAPI::GetModelCardTextureXNegAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelCardTextureXNeg);
}

UsdAttribute
UsdGeomModelAPI::CreateModelCardTextureXNegAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelCardTextureXNeg,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomModelAPI::GetModelCardTextureYNegAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelCardTextureYNeg);
}

UsdAttribute
UsdGeomModelAPI::CreateModelCardTextureYNegAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelCardTextureYNeg,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomModelAPI::GetModelCardTextureZNegAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->modelCardTextureZNeg);
}

UsdAttribute
UsdGeomModelAPI::CreateModelCardTextureZNegAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->modelCardTextureZNeg,
                       SdfValueTypeNames->Asset,
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
UsdGeomModelAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->modelDrawMode,
        UsdGeomTokens->modelApplyDrawMode,
        UsdGeomTokens->modelDrawModeColor,
        UsdGeomTokens->modelCardGeometry,
        UsdGeomTokens->modelCardTextureXPos,
        UsdGeomTokens->modelCardTextureYPos,
        UsdGeomTokens->modelCardTextureZPos,
        UsdGeomTokens->modelCardTextureXNeg,
        UsdGeomTokens->modelCardTextureYNeg,
        UsdGeomTokens->modelCardTextureZNeg,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdModelAPI::GetSchemaAttributeNames(true),
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

using std::vector;
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

bool
UsdGeomModelAPI::GetExtentsHint(VtVec3fArray *extents, 
                             const UsdTimeCode &time) const
{
    UsdAttribute extentsHintAttr = 
        GetPrim().GetAttribute(UsdGeomTokens->extentsHint);
    
    if (!extentsHintAttr)
        return false;

    return extentsHintAttr.Get(extents, time);
}

bool
UsdGeomModelAPI::SetExtentsHint(VtVec3fArray const &extents, 
                             const UsdTimeCode &time)
{
    if (!TF_VERIFY(extents.size() >= 2 &&
                      extents.size() <= (2 *
                      UsdGeomImageable::GetOrderedPurposeTokens().size())))
        return false;

    UsdAttribute extentsHintAttr = 
        GetPrim().CreateAttribute(UsdGeomTokens->extentsHint, 
                                  SdfValueTypeNames->Float3Array,
                                  /* custom = */ false);

    if (!extentsHintAttr)
        return false;

    VtVec3fArray currentExtentsHint;
    extentsHintAttr.Get(&currentExtentsHint, time);
   
    return extentsHintAttr.Set(extents, time);
}

UsdAttribute 
UsdGeomModelAPI::GetExtentsHintAttr()
{
    return GetPrim().GetAttribute(UsdGeomTokens->extentsHint);
}

VtVec3fArray
UsdGeomModelAPI::ComputeExtentsHint(
        UsdGeomBBoxCache& bboxCache) const
{
    static const TfTokenVector &purposeTokens =
        UsdGeomImageable::GetOrderedPurposeTokens();

    VtVec3fArray extents(purposeTokens.size() * 2);
    size_t lastNonEmptyBbox = std::numeric_limits<size_t>::max();

    // We should be able execute this loop in parallel since the
    // bounding box computation can be multi-threaded. However, most 
    // conversion processes are run on the farm and are limited to one
    // CPU, so there may not be a huge benefit from doing this. Also, 
    // we expect purpose 'default' to be the most common purpose value 
    // and in some cases the only purpose value. Computing bounds for 
    // the rest of the purpose values should be very fast.
    for(size_t bboxType = purposeTokens.size(); bboxType-- != 0; ) {

        // Set the gprim purpose that we are interested in computing the 
        // bbox for. This doesn't cause the cache to be blown.
        bboxCache.SetIncludedPurposes(
            std::vector<TfToken>(1, purposeTokens[bboxType]));

        GfBBox3d bbox = bboxCache.
            ComputeUntransformedBound(GetPrim());

        const GfRange3d range = bbox.ComputeAlignedBox();

        if (!range.IsEmpty() && lastNonEmptyBbox == std::numeric_limits<size_t>::max())
            lastNonEmptyBbox = bboxType;
        
        const GfVec3d &min = range.GetMin();
        const GfVec3d &max = range.GetMax();

        size_t index = bboxType * 2;
        extents[index] = GfVec3f(min[0], min[1], min[2]);
        extents[index + 1] = GfVec3f(max[0], max[1], max[2]);
    }

    // If all the extents are empty. Author a single empty range.
    if (lastNonEmptyBbox == std::numeric_limits<size_t>::max())
        lastNonEmptyBbox = 0;

    // Shrink the array to only include non-empty bounds. 
    // If all the bounds are empty, we still need to author one empty 
    // bound.
    extents.resize(2 * (lastNonEmptyBbox + 1));
    return extents;
}

UsdGeomConstraintTarget 
UsdGeomModelAPI::GetConstraintTarget(
    const std::string &constraintName) const
{
    const TfToken &constraintAttrName = 
        UsdGeomConstraintTarget::GetConstraintAttrName(constraintName);

    return UsdGeomConstraintTarget(GetPrim().GetAttribute(constraintAttrName));
}

UsdGeomConstraintTarget 
UsdGeomModelAPI::CreateConstraintTarget(
    const string &constraintName) const
{
    const TfToken &constraintAttrName = 
        UsdGeomConstraintTarget::GetConstraintAttrName(constraintName);

    // Check if the constraint target attribute already exists.
    UsdAttribute constraintAttr = GetPrim().GetAttribute(constraintAttrName);
    if (!constraintAttr) {
        // Create the attribute, if it doesn't exist.
        constraintAttr = GetPrim().CreateAttribute(constraintAttrName, 
            SdfValueTypeNames->Matrix4d, 
            /* custom */ false, 
            SdfVariabilityVarying);
    }

    return UsdGeomConstraintTarget(constraintAttr);
}

vector<UsdGeomConstraintTarget> 
UsdGeomModelAPI::GetConstraintTargets() const
{
    vector<UsdGeomConstraintTarget> constraintTargets;

    const vector<UsdAttribute> &attributes = GetPrim().GetAttributes();
    TF_FOR_ALL(attrIt, attributes) {
        UsdGeomConstraintTarget constraintTarget(*attrIt);

        // Add it to the list, if it is a valid constraint target.
        if (constraintTarget) {
            constraintTargets.push_back(constraintTarget);
        }
    }

    return constraintTargets;
}

TfToken
UsdGeomModelAPI::ComputeModelDrawMode() const
{
    // Find the closest applicable model:drawMode among this prim's ancestors.
    for (UsdPrim curPrim = GetPrim(); curPrim; curPrim = curPrim.GetParent()) {
        // Only check for the attribute on models; don't check the pseudo-root.
        if (!curPrim.IsModel() || !curPrim.GetParent()) {
            continue;
        }

        // If model:drawMode is set, use its value; we want the first attribute
        // we find.
        UsdGeomModelAPI curModel(curPrim);
        UsdAttribute attr;
        TfToken drawMode;

        if ((attr = curModel.GetModelDrawModeAttr()) && attr &&
            attr.Get(&drawMode)) {
            return drawMode;
        }
    }

    // If the attribute isn't set on any ancestors, return "default".
    return UsdGeomTokens->default_;
}

PXR_NAMESPACE_CLOSE_SCOPE

