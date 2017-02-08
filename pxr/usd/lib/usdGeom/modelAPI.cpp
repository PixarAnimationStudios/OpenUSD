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
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/modelAPI.h"

#include "pxr/usd/usdGeom/constraintTarget.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usd/schemaRegistry.h"

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
const TfType &
UsdGeomModelAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomModelAPI>();
    return tfType;
}

/* virtual */
const TfType &
UsdGeomModelAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdGeomModelAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdModelAPI::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
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
    for(int bboxType = (purposeTokens.size() - 1); bboxType >= 0; bboxType--) {

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

        int index = bboxType * 2;
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

PXR_NAMESPACE_CLOSE_SCOPE

