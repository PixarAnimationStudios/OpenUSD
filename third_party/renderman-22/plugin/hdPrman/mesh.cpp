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
#include "hdPrman/context.h"
#include "hdPrman/coordSys.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderPass.h"
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
#include "RixParamList.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_Mesh::HdPrman_Mesh(SdfPath const& id,
                           SdfPath const& instancerId)
    : HdMesh(id, instancerId)
{
}

void
HdPrman_Mesh::Finalize(HdRenderParam *renderParam)
{
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

    riley::Riley *riley = context->riley;

    // Release retained conversions of coordSys bindings.
    context->ReleaseCoordSysBindings(GetId());

    // Delete instances before deleting the masters they use.
    for (const auto &id: _instanceIds) {
        riley->DeleteGeometryInstance(
            riley::GeometryMasterId::k_InvalidId, id);
    }
    _instanceIds.clear();
    for (const auto &id: _masterIds) {
        riley->DeleteGeometryMaster(id);
    }
    _masterIds.clear();
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
        | HdChangeTracker::DirtyInstanceIndex
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits
HdPrman_Mesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // XXX This is not ideal. Currently Riley requires us to provide
    // all the values anytime we edit a mesh. To make sure the values
    // exist in the value cache, we propagte the dirty bits.value cache,
    // we propagte the dirty bits.value cache, we propagte the dirty
    // bits.value cache, we propagte the dirty bits.
    return bits ? (bits | GetInitialDirtyBitsMask()) : bits;
}

void
HdPrman_Mesh::_InitRepr(TfToken const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    TF_UNUSED(reprToken);
    TF_UNUSED(dirtyBits);

    // No-op
}

static RixParamList *
_PopulatePrimvars(const HdPrman_Mesh &mesh,
                  HdPrman_Context *context,
                  RixRileyManager *mgr,
                  HdSceneDelegate *sceneDelegate,
                  const SdfPath &id,
                  RtUString *primType,
                  HdGeomSubsets *geomSubsets,
                  int *numFaces)
{
    // Pull topology.
    const HdMeshTopology topology = mesh.GetMeshTopology(sceneDelegate);
    const size_t npoints = topology.GetNumPoints();
    const VtIntArray verts = topology.GetFaceVertexIndices();
    const VtIntArray nverts = topology.GetFaceVertexCounts();
    const int refineLevel = sceneDelegate->GetDisplayStyle(id).refineLevel;
    *geomSubsets = topology.GetGeomSubsets();
    *numFaces = topology.GetNumFaces();

    RixParamList *primvars = mgr->CreateRixParamList(
         nverts.size(), /* uniform */
         npoints, /* vertex */
         npoints, /* varying */
         verts.size()  /* facevarying */);

    //
    // Point positions (P)
    //
    HdTimeSampleArray<VtVec3fArray, HDPRMAN_MAX_TIME_SAMPLES> points;
    {
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedPoints;
        sceneDelegate->SamplePrimvar(id, HdTokens->points, &boxedPoints);
        points.UnboxFrom(boxedPoints);
    }
    // Assume 1 time sample until and unless we detect animated P.
    if (points.count == 1) {
        // Fast path: single sample.
        if (points.values[0].size() == npoints) {
            primvars->SetPointDetail(RixStr.k_P,
                                     (RtPoint3*) points.values[0].cdata(),
                                     RixDetailType::k_vertex);
        } else {
            TF_WARN("<%s> primvar 'points' size (%zu) did not match expected "
                    "(%zu)", id.GetText(), points.values[0].size(), npoints);
        }
    } else if (points.count > 1) {
        // P is animated, so promote the RixParamList to use the
        // configured sample times.
        const std::vector<float> &configuredSampleTimes =
            context->GetTimeSamplesForId(id);
        primvars->SetTimeSamples(configuredSampleTimes.size(),
                                 &configuredSampleTimes[0]);
        // Resample P at configured times.
        for (size_t i=0; i < configuredSampleTimes.size(); ++i) {
            VtVec3fArray p = points.Resample(configuredSampleTimes[i]);
            if (p.size() == npoints) {
                primvars->SetPointDetail(RixStr.k_P, (RtPoint3*) p.cdata(),
                                         RixDetailType::k_vertex, i);
            } else {
                TF_WARN("<%s> primvar 'points' size (%zu) did not match "
                        "expected (%zu)", id.GetText(), p.size(), npoints);
            }
        }
    }

    // Topology.
    primvars->SetIntegerDetail(RixStr.k_Ri_nvertices, nverts.cdata(),
                               RixDetailType::k_uniform);
    primvars->SetIntegerDetail(RixStr.k_Ri_vertices, verts.cdata(),
                               RixDetailType::k_facevarying);
    if (topology.GetScheme() == PxOsdOpenSubdivTokens->catmullClark) {
        *primType = RixStr.k_Ri_SubdivisionMesh;
        primvars->SetString(RixStr.k_Ri_scheme, RixStr.k_catmullclark);
    } else if (topology.GetScheme() == PxOsdOpenSubdivTokens->loop) {
        *primType = RixStr.k_Ri_SubdivisionMesh;
        primvars->SetString(RixStr.k_Ri_scheme, RixStr.k_loop);
    } else if (topology.GetScheme() == PxOsdOpenSubdivTokens->bilinear) {
        *primType = RixStr.k_Ri_SubdivisionMesh;
        primvars->SetString(RixStr.k_Ri_scheme, RixStr.k_bilinear);
    } else { // if scheme == PxOsdOpenSubdivTokens->none
        *primType = RixStr.k_Ri_PolygonMesh;
    }

    // Topology overrides.
    if (refineLevel == 0) {
        // If the refine level is 0, treat this as a polymesh, since the
        // scene won't be supplying subdiv tags.
        *primType = RixStr.k_Ri_PolygonMesh;
    }

    VtIntArray holeIndices = topology.GetHoleIndices();
    if (*primType == RixStr.k_Ri_PolygonMesh &&
        !holeIndices.empty()) {
        // Poly meshes with holes are promoted to bilinear subdivs, to
        // make riley respect the holes.
        *primType = RixStr.k_Ri_SubdivisionMesh;
        primvars->SetString(RixStr.k_Ri_scheme, RixStr.k_bilinear);
    }

    if (mesh.IsDoubleSided(sceneDelegate)) {
        primvars->SetInteger(RixStr.k_Ri_Sides, 2);
    }
    // Orientation, aka winding order.
    // Because PRMan uses a left-handed coordinate system, and USD/Hydra
    // use a right-handed coordinate system, the meaning of orientation
    // also flips when we convert between them.  So LH<->RH.
    if (topology.GetOrientation() == PxOsdOpenSubdivTokens->leftHanded) {
        primvars->SetString(RixStr.k_Ri_Orientation, RixStr.k_rh);
    } else {
        primvars->SetString(RixStr.k_Ri_Orientation, RixStr.k_lh);
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

        // If refineLevel is 0, the scene treats the mesh as a polymesh and
        // isn't required to compute subdiv tags; so only add subdiv tags for
        // nonzero refine level.
        if (refineLevel > 0) {
            PxOsdSubdivTags osdTags = mesh.GetSubdivTags(sceneDelegate);
            // Creases
            VtIntArray creaseLengths = osdTags.GetCreaseLengths();
            VtIntArray creaseIndices = osdTags.GetCreaseIndices();
            VtFloatArray creaseWeights = osdTags.GetCreaseWeights();
            if (!creaseIndices.empty()) {
                for (int creaseLength: creaseLengths) {
                    tagNames.push_back(RixStr.k_crease);
                    tagArgCounts.push_back(creaseLength); // num int args
                    tagArgCounts.push_back(1); // num float args
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
        }
        primvars->SetStringArray(RixStr.k_Ri_subdivtags,
                                 &tagNames[0], tagNames.size());
        primvars->SetIntegerArray(RixStr.k_Ri_subdivtagnargs,
                                  &tagArgCounts[0], tagArgCounts.size());
        primvars->SetFloatArray(RixStr.k_Ri_subdivtagfloatargs,
                                &tagFloatArgs[0], tagFloatArgs.size());
        primvars->SetIntegerArray(RixStr.k_Ri_subdivtagintargs,
                                  &tagIntArgs[0], tagIntArgs.size());
    }

    // Set element ID.
    std::vector<int32_t> elementId(nverts.size());
    std::iota(elementId.begin(), elementId.end(), 0);
    primvars->SetIntegerDetail(RixStr.k_faceindex, elementId.data(),
                               RixDetailType::k_uniform);

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, nverts.size(),
        npoints, npoints, verts.size());

    return primvars;
}

void
HdPrman_Mesh::Sync(HdSceneDelegate *sceneDelegate,
                   HdRenderParam   *renderParam,
                   HdDirtyBits     *dirtyBits,
                   TfToken const   &reprToken)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(reprToken);

    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        _SetMaterialId(sceneDelegate->GetRenderIndex().GetChangeTracker(),
                       sceneDelegate->GetMaterialId(GetId()));
    }

    const SdfPath & id = GetId();
    const bool isInstance = !GetInstancerId().IsEmpty();

    // Sample transform
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
    sceneDelegate->SampleTransform(id, &xf);

    RixRileyManager *mgr = context->mgr;
    riley::Riley *riley = context->riley;

    RixParamList *primvars = nullptr;
    RixParamList *attrs = nullptr;
    RtUString primType;

    // Look up material binding.  Default to fallbackMaterial.
    riley::MaterialId materialId = context->fallbackMaterial;
    riley::DisplacementId dispId = riley::DisplacementId::k_InvalidId;
    const SdfPath & hdMaterialId = GetMaterialId();
    HdPrman_ResolveMaterial(sceneDelegate, hdMaterialId, &materialId, &dispId);

    // XXX Workaround for possible prman bug: Starting at refinelevel 0
    // and then going to 1 (i.e. switching mesh->subdiv) via
    // ModifyGeometryMaster() does not run the displacement shader.
    // Work around this by deleting & re-creating the geometry instead.
    for (const auto &instanceId: _instanceIds) {
        riley->DeleteGeometryInstance(
            riley::GeometryMasterId::k_InvalidId, // no group
            instanceId);
    }
    _instanceIds.clear();
    for (const auto &masterId: _masterIds) {
        riley->DeleteGeometryMaster(masterId);
    }
    _masterIds.clear();

    // Look up primvars.
    HdGeomSubsets geomSubsets;
    std::vector<riley::MaterialId> subsetMaterialIds;
    int numFaces = 0;
    primvars = _PopulatePrimvars(*this, context, mgr, sceneDelegate,
                                 id, &primType, &geomSubsets, &numFaces);

    // Convert (and cache) coordinate systems.
    riley::ScopedCoordinateSystem coordSys = {0, nullptr};
    if (HdPrman_Context::RileyCoordSysIdVecRefPtr convertedCoordSys =
        context->ConvertAndRetainCoordSysBindings(sceneDelegate, id)) {
        coordSys.count = convertedCoordSys->size();
        coordSys.coordsysIds = &(*convertedCoordSys)[0];
    }

    // If the geometry has been partitioned into subsets, add an
    // additional subset representing anything left over.
    if (!geomSubsets.empty()) {
        std::vector<bool> faceIsUnused(numFaces, true);
        size_t numUnusedFaces = faceIsUnused.size();
        for (HdGeomSubset const& subset: geomSubsets) {
            for (int index: subset.indices) {
                if (TF_VERIFY(index < numFaces) && faceIsUnused[index]) {
                    faceIsUnused[index] = false;
                    numUnusedFaces--;
                }
            }
        }
        // If we found any unused faces, build a final subset with those faces.
        // Use the material bound to the parent mesh.
        if (numUnusedFaces) {
            geomSubsets.push_back(HdGeomSubset());
            HdGeomSubset &unusedSubset = geomSubsets.back();
            unusedSubset.type = HdGeomSubset::TypeFaceSet;
            unusedSubset.id = id;
            unusedSubset.materialId = hdMaterialId;
            unusedSubset.indices.resize(numUnusedFaces);
            size_t count = 0;
            for (size_t i=0;
                 i < faceIsUnused.size() && count < numUnusedFaces; ++i) {
                if (faceIsUnused[i]) {
                    unusedSubset.indices[count] = i;
                    count++;
                }
            }
        }
    }

    // Create Riley master(s).
    if (geomSubsets.empty()) {
        _masterIds.push_back(
            riley->CreateGeometryMaster(primType, dispId, *primvars));
    } else {
        for (HdGeomSubset const& subset: geomSubsets) {
            // Convert indices to int32_t and set as k_shade_faceset.
            std::vector<int32_t> int32Indices(subset.indices.begin(),
                                              subset.indices.end());
            primvars->SetIntegerArray(RixStr.k_shade_faceset,
                                      &int32Indices[0],
                                      int32Indices.size());
            // Look up material override for the subset (if any)
            riley::MaterialId subsetMaterialId = materialId;
            riley::DisplacementId subsetDispId = dispId;
            HdPrman_ResolveMaterial(sceneDelegate, subset.materialId,
                                    &subsetMaterialId, &subsetDispId);
            _masterIds.push_back(
                riley->CreateGeometryMaster(primType, subsetDispId, *primvars));
            // Hold the material for later, when we create the
            // Riley instances below.
            subsetMaterialIds.push_back(subsetMaterialId);
        }
    }

    // Create or modify instances.
    if (isInstance) {
        // Hydra Instancer case.
        HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
        HdPrmanInstancer *instancer = static_cast<HdPrmanInstancer*>(
            renderIndex.GetInstancer(GetInstancerId()));
        VtIntArray instanceIndices =
            sceneDelegate->GetInstanceIndices(GetInstancerId(), GetId());

        instancer->SyncPrimvars();

        HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES> ixf;
        instancer->SampleInstanceTransforms(GetId(), instanceIndices, &ixf);

        // Retrieve instance categories.
        std::vector<VtArray<TfToken>> instanceCategories =
            sceneDelegate->GetInstanceCategories(GetInstancerId());

        // Adjust size of PRMan instance array.
        const size_t oldSize = _instanceIds.size();
        const size_t newSize = (ixf.count > 0) ? ixf.values[0].size() : 0;
        if (newSize != oldSize) {
            for (size_t i=newSize; i < oldSize; ++i) {
                riley->DeleteGeometryInstance(
                    riley::GeometryMasterId::k_InvalidId, // no group
                    _instanceIds[i]);
            }
            _instanceIds.resize(newSize);
        }

        // We can only retrieve the primvars from Hydra once.
        RixParamList *instancerAttrs =
            context->ConvertAttributes(sceneDelegate, id);
        // Add "identifier:id" with the hydra prim id.
        instancerAttrs->SetInteger(RixStr.k_identifier_id, GetPrimId());

        // Create or modify PRMan instances.
        for (size_t i=0; i < newSize; ++i) {
            // XXX: Add support for nested instancing instance primvars.
            size_t instanceIndex = 0;
            if (i < instanceIndices.size()) {
                instanceIndex = instanceIndices[i];
            }

            // Create a copy of the instancer attrs.
            RixParamList *attrs = mgr->CreateRixParamList();
            instancer->GetInstancePrimvars(id, instanceIndex, attrs);
            // Inherit instancer attributes under the instance attrs.
            attrs->Inherit(*instancerAttrs);
            // Add "identifier:id2" with the instance number.
            attrs->SetInteger(RixStr.k_identifier_id2, i);

            // Convert categories.
            if (instanceIndex < instanceCategories.size()) {
                context->ConvertCategoriesToAttributes(
                    id, instanceCategories[instanceIndex], attrs);
            }

            // PRMan does not allow transforms on geometry masters,
            // so we apply that transform (xf) to all the instances, here.
            RtMatrix4x4 rt_xf[HDPRMAN_MAX_TIME_SAMPLES];
            if (xf.count == 0 ||
                (xf.count == 1 && (xf.values[0] == GfMatrix4d(1)))) {
                // Expected case: master xf is constant & exactly identity.
                for (size_t j=0; j < ixf.count; ++j) {
                    rt_xf[j] = HdPrman_GfMatrixToRtMatrix(ixf.values[j][i]);
                }
            } else {
                // Multiply resampled master xf against instance xforms.
                for (size_t j=0; j < ixf.count; ++j) {
                    GfMatrix4d xf_j = xf.Resample(ixf.times[j]);
                    rt_xf[j] =
                        HdPrman_GfMatrixToRtMatrix(xf_j * ixf.values[j][i]);
                }
            }

            const riley::Transform xform = 
                { unsigned(ixf.count), rt_xf, ixf.times };

            if (i >= oldSize) {
                riley::GeometryInstanceId id = riley->CreateGeometryInstance(
                    riley::GeometryMasterId::k_InvalidId, // no group
                    _masterIds[0], materialId, coordSys, xform, *attrs);
                // This can fail when inserting meshes with nans (for example)
                if (TF_VERIFY(id != riley::GeometryInstanceId::k_InvalidId, 
                    "HdPrman failed to create geometry %s", 
                    GetId().GetText())) {
                        _instanceIds[i] = id;
                }
            } else {
                riley->ModifyGeometryInstance(
                    riley::GeometryMasterId::k_InvalidId, // no group
                    _instanceIds[i],
                    &materialId,
                    &coordSys,
                    &xform, attrs);
            }
            mgr->DestroyRixParamList(attrs);
            attrs = nullptr;
        }
        mgr->DestroyRixParamList(instancerAttrs);
        instancerAttrs = nullptr;
    } else {
        // Single, non-Hydra-instanced case.
        RtMatrix4x4 xf_rt_values[HDPRMAN_MAX_TIME_SAMPLES];
        for (size_t i=0; i < xf.count; ++i) {
            xf_rt_values[i] = HdPrman_GfMatrixToRtMatrix(xf.values[i]);
        }
        const riley::Transform xform = {
            unsigned(xf.count), xf_rt_values, xf.times};

        // Destroy old instances.
        if (!_instanceIds.empty()) {
            for (size_t i=0; i < _instanceIds.size(); ++i) {
                riley->DeleteGeometryInstance(
                    riley::GeometryMasterId::k_InvalidId,
                    _instanceIds[i]);
            }
            _instanceIds.clear();
        }
        // Create new Riley instances. 
        attrs = context->ConvertAttributes(sceneDelegate, id);
        // Add "identifier:id" with the hydra prim id, and "identifier:id2"
        // with the instance number.
        attrs->SetInteger(RixStr.k_identifier_id, GetPrimId());
        attrs->SetInteger(RixStr.k_identifier_id2, 0);

        if (geomSubsets.empty()) {
            riley::GeometryInstanceId id = riley->CreateGeometryInstance(
                riley::GeometryMasterId::k_InvalidId,
                _masterIds[0], materialId, coordSys, xform, *attrs);
            
            // This can fail when inserting meshes with nans (for example)
            if (TF_VERIFY(id != riley::GeometryInstanceId::k_InvalidId, 
                "HdPrman failed to create geometry %s", GetId().GetText())) {
                    _instanceIds.push_back(id);
            }
        } else {
            // If subsets exist, create one Riley instance for each subset.
            for (size_t i=0; i < geomSubsets.size(); ++i) {
                riley::GeometryInstanceId id = riley->CreateGeometryInstance(
                    riley::GeometryMasterId::k_InvalidId,
                    _masterIds[i], subsetMaterialIds[i], coordSys,
                    xform, *attrs);

                // This can fail when inserting meshes with nans (for example)
                if (TF_VERIFY(id != riley::GeometryInstanceId::k_InvalidId, 
                    "HdPrman failed to create geometry %s", 
                    GetId().GetText())){
                        _instanceIds.push_back(id);
                }
            }
        }
    }

    if (primvars) {
        mgr->DestroyRixParamList(primvars);
        primvars = nullptr;
    }
    if (attrs) {
        mgr->DestroyRixParamList(attrs);
        attrs = nullptr;
    }

    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
