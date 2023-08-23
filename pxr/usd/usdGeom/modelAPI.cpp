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

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomModelAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
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


/* virtual */
UsdSchemaKind UsdGeomModelAPI::_GetSchemaKind() const
{
    return UsdGeomModelAPI::schemaKind;
}

/* static */
bool
UsdGeomModelAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdGeomModelAPI>(whyNot);
}

/* static */
UsdGeomModelAPI
UsdGeomModelAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdGeomModelAPI>()) {
        return UsdGeomModelAPI(prim);
    }
    return UsdGeomModelAPI();
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
                                const UsdTimeCode &time) const
{
    const size_t extSize = extents.size();
    const TfTokenVector &purposeTokens
        = UsdGeomImageable::GetOrderedPurposeTokens();
    if (extSize % 2 || extSize < 2 || extSize > 2 * purposeTokens.size()) {
        TF_CODING_ERROR(
            "invalid extents size (%zu) - must be an even number >= 2 and <= "
            "2 * UsdGeomImageable::GetOrderedPurposeTokens().size() (%zu)",
            extSize, 2 * purposeTokens.size());
        return false;
    }

    UsdAttribute extentsHintAttr = 
        GetPrim().CreateAttribute(UsdGeomTokens->extentsHint, 
                                  SdfValueTypeNames->Float3Array,
                                  /* custom = */ false);

    return extentsHintAttr && extentsHintAttr.Set(extents, time);
}

UsdAttribute 
UsdGeomModelAPI::GetExtentsHintAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->extentsHint);
}

VtVec3fArray
UsdGeomModelAPI::ComputeExtentsHint(UsdGeomBBoxCache& bboxCache) const
{
    static const TfTokenVector &purposeTokens =
        UsdGeomImageable::GetOrderedPurposeTokens();

    if (!TF_VERIFY(!purposeTokens.empty(), "we have no purpose!")) {
        return {};
    }

    VtVec3fArray extents;
    
    // If this model is itself a boundable, we call ComputeExtentFromPlugins().
    UsdGeomBoundable boundable(GetPrim());
    if (boundable) {
        if (UsdGeomBoundable::ComputeExtentFromPlugins(
                boundable, bboxCache.GetTime(), &extents) && extents.size()) {
            // Replicate the bounds across all the purposes for now.  Seems like
            // 'extent' for aggregate boundables should support per-purpose
            // extent, like extentsHint.
            extents.resize(2 * purposeTokens.size());
            for (size_t i = 1; i != purposeTokens.size(); ++i) {
                extents[2*i] = extents[0];
                extents[2*i+1] = extents[1];
            }
        }
        else {
            // Leave a single empty range.
            extents.resize(2);
            extents[0] = GfRange3f().GetMin();
            extents[1] = GfRange3f().GetMax();
        }
        return extents;
    }

    // This model is not a boundable, so use the bboxCache.
    extents.resize(2 * purposeTokens.size());

    // It would be possible to parallelize this loop in the future if it becomes
    // a bottleneck.
    std::vector<TfToken> purposeTokenVec(1);
    size_t lastNotEmpty = 0;
    for (size_t i = 0, end = purposeTokens.size(); i != end; ++i) {

        // Set the gprim purpose that we are interested in computing the bbox
        // for. This doesn't cause the cache to be blown.
        purposeTokenVec[0] = purposeTokens[i];
        bboxCache.SetIncludedPurposes(purposeTokenVec);

        const GfRange3d range = bboxCache
            .ComputeUntransformedBound(GetPrim())
            .ComputeAlignedBox();

        extents[2*i] = GfVec3f(range.GetMin());
        extents[2*i+1] = GfVec3f(range.GetMax());

        if (!range.IsEmpty()) {
            lastNotEmpty = i;
        }
    }

    // Trim any trailing empty boxes, but leave at least one.
    extents.resize(2 * (lastNotEmpty + 1));
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

namespace {
static 
bool
_GetAuthoredDrawMode(const UsdPrim &prim, TfToken *drawMode)
{
    // Only check for the attribute on models; don't check the pseudo-root.
    if (!prim.IsModel() || !prim.GetParent()) {
        return false;
    }

    UsdGeomModelAPI modelAPI(prim);
    UsdAttribute attr = modelAPI.GetModelDrawModeAttr();
    return attr && attr.Get(drawMode);
}
}

TfToken
UsdGeomModelAPI::ComputeModelDrawMode(const TfToken &parentDrawMode) const
{
    TfToken drawMode = UsdGeomTokens->inherited;

    if (_GetAuthoredDrawMode(GetPrim(), &drawMode) &&
        drawMode != UsdGeomTokens->inherited) {
        return drawMode;
    }

    if (!parentDrawMode.IsEmpty()) {
        return parentDrawMode;
    }

    // Find the closest applicable model:drawMode among this prim's ancestors.
    for (UsdPrim curPrim = GetPrim().GetParent(); 
         curPrim; 
         curPrim = curPrim.GetParent()) {

        if (_GetAuthoredDrawMode(curPrim, &drawMode) &&
            drawMode != UsdGeomTokens->inherited) {
            return drawMode;
        }
    }

    // If the attribute isn't set on any ancestors, return "default".
    return UsdGeomTokens->default_;
}


PXR_NAMESPACE_CLOSE_SCOPE

