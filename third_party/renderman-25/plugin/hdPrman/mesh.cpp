//
// Copyright 2019 Pixar
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
#include <numeric> // for std::iota
#include "hdPrman/mesh.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/coordSys.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/imaging/hd/coordSys.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/pxOsd/subdivTags.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/usd/usdRi/rmanUtilities.h"

#include "Riley.h"
#include "RiTypesHelper.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION > 2011
HdPrman_Mesh::HdPrman_Mesh(SdfPath const& id, const bool isMeshLight)
    : BASE(id)
    , _isMeshLight(isMeshLight)
#else
HdPrman_Mesh::HdPrman_Mesh(SdfPath const& id,
                           SdfPath const& instancerId)
    : BASE(id, instancerId)
#endif
{
}

bool
HdPrman_Mesh::_PrototypeOnly()
{
    return _isMeshLight;
}

HdDirtyBits
HdPrman_Mesh::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtMesh(), so it should list every data item
    // that _PopluateRtMesh requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyCullStyle
        | HdChangeTracker::DirtyDoubleSided
        | HdChangeTracker::DirtySubdivTags
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyInstancer
        ;

    return (HdDirtyBits)mask;
}

static
VtIntArray
_Union(const VtIntArray &a, const VtIntArray &b)
{
    if (a.empty()) {
        return b;
    } else if (b.empty()) {
        return a;
    } else {
        VtIntArray aCopy = a;
        VtIntArray bCopy = b;
        std::sort(aCopy.begin(), aCopy.end());
        std::sort(bCopy.begin(), bCopy.end());
        VtIntArray merged;
        std::set_union(aCopy.cbegin(), aCopy.cend(),
                       bCopy.cbegin(), bCopy.cend(),
                       std::back_inserter(merged));
        return merged;
    }
}

RtPrimVarList
HdPrman_Mesh::_ConvertGeometry(HdPrman_RenderParam *renderParam,
                                HdSceneDelegate *sceneDelegate,
                                const SdfPath &id,
                                RtUString *primType,
                                HdGeomSubsets *geomSubsets)
{
    // Pull topology.
    const HdMeshTopology topology = GetMeshTopology(sceneDelegate);
    const size_t npoints = topology.GetNumPoints();
    const VtIntArray verts = topology.GetFaceVertexIndices();
    const VtIntArray nverts = topology.GetFaceVertexCounts();

    // If the geometry has been partitioned into subsets, add an
    // additional subset representing anything left over.
    *geomSubsets = topology.GetGeomSubsets();
    if (!geomSubsets->empty()) {
        const int numFaces = topology.GetNumFaces();
        std::vector<bool> faceMask(numFaces, true);
        size_t numUnusedFaces = faceMask.size();
        for (HdGeomSubset const& subset: *geomSubsets) {
            for (int index: subset.indices) {
                if (TF_VERIFY(index < numFaces) && faceMask[index]) {
                    faceMask[index] = false;
                    numUnusedFaces--;
                }
            }
        }
        if (numUnusedFaces) {
            geomSubsets->push_back(HdGeomSubset());
            HdGeomSubset &unusedSubset = geomSubsets->back();
            unusedSubset.type = HdGeomSubset::TypeFaceSet;
            unusedSubset.id = id;
            // Use an empty material ID as a placeholder to indicate
            // that we wish to re-use the mesh-level material binding.
            unusedSubset.materialId = SdfPath();
            unusedSubset.indices.resize(numUnusedFaces);
            size_t count = 0;
            for (size_t i=0;
                 i < faceMask.size() && count < numUnusedFaces; ++i) {
                if (faceMask[i]) {
                    unusedSubset.indices[count] = i;
                    count++;
                }
            }
        }
    }

    RtPrimVarList primvars(
         nverts.size(), /* uniform */
         npoints, /* vertex */
         npoints, /* varying */
         verts.size()  /* facevarying */);

    //
    // Point positions (P)
    //
    float primvarTime = 0.0f;
    if( HdPrman_RenderParam::HasSceneIndexPlugin(
            HdPrmanPluginTokens->velocityBlur) ) {
        HdPrman_ConvertPointsPrimvar(sceneDelegate, id, primvars, npoints);
    } else {
        primvarTime = renderParam->ConvertPositions(
            sceneDelegate, id, npoints, primvars);
    }
    // Topology.
    primvars.SetIntegerDetail(RixStr.k_Ri_nvertices, nverts.cdata(),
                              RtDetailType::k_uniform);
    primvars.SetIntegerDetail(RixStr.k_Ri_vertices, verts.cdata(),
                              RtDetailType::k_facevarying);
    if (topology.GetScheme() == PxOsdOpenSubdivTokens->catmullClark) {
        *primType = RixStr.k_Ri_SubdivisionMesh;
        primvars.SetString(RixStr.k_Ri_scheme, RixStr.k_catmullclark);
    } else if (topology.GetScheme() == PxOsdOpenSubdivTokens->loop) {
        *primType = RixStr.k_Ri_SubdivisionMesh;
        primvars.SetString(RixStr.k_Ri_scheme, RixStr.k_loop);
    } else if (topology.GetScheme() == PxOsdOpenSubdivTokens->bilinear) {
        *primType = RixStr.k_Ri_SubdivisionMesh;
        primvars.SetString(RixStr.k_Ri_scheme, RixStr.k_bilinear);
    } else { // if scheme == PxOsdOpenSubdivTokens->none
        *primType = RixStr.k_Ri_PolygonMesh;
    }

    // Invisible faces will be handled by treating them as holes.  Since there
    // may also be explicitly specified hole indices, we use the union of the
    // two lists as the hole indices for the mesh.
    const VtIntArray invisibleFaces = topology.GetInvisibleFaces();
    const VtIntArray explicitHoleIndices = topology.GetHoleIndices();
    const VtIntArray holeIndices = _Union(invisibleFaces, explicitHoleIndices);

    if (*primType == RixStr.k_Ri_PolygonMesh &&
        !holeIndices.empty()) {
        // Poly meshes with holes are promoted to bilinear subdivs, to
        // make riley respect the holes.
        *primType = RixStr.k_Ri_SubdivisionMesh;
        primvars.SetString(RixStr.k_Ri_scheme, RixStr.k_bilinear);
    }
  
    // Orientation, aka winding order.
    // Because PRMan uses a left-handed coordinate system, and USD/Hydra
    // use a right-handed coordinate system, the meaning of orientation
    // also flips when we convert between them.  So LH<->RH.
    if (topology.GetOrientation() == PxOsdOpenSubdivTokens->leftHanded) {
        primvars.SetString(RixStr.k_Ri_Orientation, RixStr.k_rh);
    } else {
        primvars.SetString(RixStr.k_Ri_Orientation, RixStr.k_lh);
    }

    // Subdiv tags
    if (*primType == RixStr.k_Ri_SubdivisionMesh) {
        std::vector<RtUString> tagNames;
        std::vector<RtInt> tagArgCounts;
        std::vector<RtInt> tagIntArgs;
        std::vector<RtFloat> tagFloatArgs;
        std::vector<RtToken> tagStringArgs;

        // Holes
        if (!holeIndices.empty()) {
            tagNames.push_back(RixStr.k_hole);
            tagArgCounts.push_back(holeIndices.size()); // num int args
            tagArgCounts.push_back(0); // num float args
            tagArgCounts.push_back(0); // num str args
            tagIntArgs.insert(tagIntArgs.end(),
                              holeIndices.begin(), holeIndices.end());
        }

        PxOsdSubdivTags osdTags = GetSubdivTags(sceneDelegate);

        // Creases
        VtIntArray creaseLengths = osdTags.GetCreaseLengths();
        VtIntArray creaseIndices = osdTags.GetCreaseIndices();
        VtFloatArray creaseWeights = osdTags.GetCreaseWeights();
        if (!creaseIndices.empty()) {
            const bool weightPerCrease = 
                creaseWeights.size() == creaseLengths.size();
            for (int creaseLength: creaseLengths) {
                tagNames.push_back(RixStr.k_crease);
                tagArgCounts.push_back(creaseLength); // num int args
                if (weightPerCrease) {
                    // one weight for each crease
                    tagArgCounts.push_back(1); // num float args
                } else {
                    // one weight for each crease edge
                    tagArgCounts.push_back(creaseLength-1); // num float args
                }
                tagArgCounts.push_back(0); // num str args
            }
            tagIntArgs.insert(tagIntArgs.end(),
                    creaseIndices.begin(), creaseIndices.end());
            tagFloatArgs.insert(tagFloatArgs.end(),
                    creaseWeights.begin(), creaseWeights.end());
        }

        // Corners
        VtIntArray cornerIndices = osdTags.GetCornerIndices();
        VtFloatArray cornerWeights = osdTags.GetCornerWeights();
        if (cornerIndices.size()) {
            tagNames.push_back(RixStr.k_corner);
            tagArgCounts.push_back(cornerIndices.size()); // num int args
            tagArgCounts.push_back(cornerWeights.size()); // num float args
            tagArgCounts.push_back(0); // num str args
            tagIntArgs.insert(tagIntArgs.end(),
                    cornerIndices.begin(), cornerIndices.end());
            tagFloatArgs.insert(tagFloatArgs.end(),
                    cornerWeights.begin(), cornerWeights.end());
        }

        // Vertex Interpolation (aka interpolateboundary)
        TfToken vInterp = osdTags.GetVertexInterpolationRule();
        if (vInterp.IsEmpty()) {
            vInterp = PxOsdOpenSubdivTokens->edgeAndCorner;
        }
        if (UsdRiConvertToRManInterpolateBoundary(vInterp) != 0) {
            tagNames.push_back(RixStr.k_interpolateboundary);
            tagArgCounts.push_back(0); // num int args
            tagArgCounts.push_back(0); // num float args
            tagArgCounts.push_back(0); // num str args
        }

        // Face-varying Interpolation (aka facevaryinginterpolateboundary)
        TfToken fvInterp = osdTags.GetFaceVaryingInterpolationRule();
        if (fvInterp.IsEmpty()) {
            fvInterp = PxOsdOpenSubdivTokens->cornersPlus1;
        }
        tagNames.push_back(RixStr.k_facevaryinginterpolateboundary);
        tagArgCounts.push_back(1); // num int args
        tagArgCounts.push_back(0); // num float args
        tagArgCounts.push_back(0); // num str args
        tagIntArgs.push_back(
                UsdRiConvertToRManFaceVaryingLinearInterpolation(fvInterp));

        // Triangle subdivision rule
        TfToken triSubdivRule = osdTags.GetTriangleSubdivision();
        if (triSubdivRule == PxOsdOpenSubdivTokens->smooth) {
            tagNames.push_back(RixStr.k_smoothtriangles);
            tagArgCounts.push_back(1); // num int args
            tagArgCounts.push_back(0); // num float args
            tagArgCounts.push_back(0); // num str args
            tagIntArgs.push_back(
                    UsdRiConvertToRManTriangleSubdivisionRule(triSubdivRule));
        }
        primvars.SetStringArray(RixStr.k_Ri_subdivtags,
                                 &tagNames[0], tagNames.size());
        primvars.SetIntegerArray(RixStr.k_Ri_subdivtagnargs,
                                  &tagArgCounts[0], tagArgCounts.size());
        primvars.SetFloatArray(RixStr.k_Ri_subdivtagfloatargs,
                                &tagFloatArgs[0], tagFloatArgs.size());
        primvars.SetIntegerArray(RixStr.k_Ri_subdivtagintargs,
                                  &tagIntArgs[0], tagIntArgs.size());
    }

    // Set element ID.
    std::vector<int32_t> elementId(nverts.size());
    std::iota(elementId.begin(), elementId.end(), 0);
    primvars.SetIntegerDetail(RixStr.k_faceindex, elementId.data(),
                              RtDetailType::k_uniform);

    HdPrman_ConvertPrimvars(
        sceneDelegate, id, primvars, nverts.size(), npoints, npoints,
        verts.size(), primvarTime);

    return primvars;
}

PXR_NAMESPACE_CLOSE_SCOPE
