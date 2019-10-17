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
#include "hdPrman/points.h"

#include "hdPrman/context.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderPass.h"
#include "hdPrman/rixStrings.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"
#include "RixParamList.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_Points::HdPrman_Points(SdfPath const& id,
                           SdfPath const& instancerId)
    : HdPoints(id, instancerId)
    , _masterId(riley::GeometryMasterId::k_InvalidId)
{
}

void
HdPrman_Points::Finalize(HdRenderParam *renderParam)
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

    if (_masterId != riley::GeometryMasterId::k_InvalidId) {
        riley->DeleteGeometryMaster(_masterId);
        _masterId = riley::GeometryMasterId::k_InvalidId;
    }
}

HdDirtyBits
HdPrman_Points::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtPoints(), so it should list every data item
    // that _PopluateRtPoints requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyWidths
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyInstanceIndex
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits
HdPrman_Points::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // XXX This is not ideal. Currently Riley requires us to provide
    // all the values anytime we edit a mesh. To make sure the values
    // exist in the value cache, we propagte the dirty bits.value cache,
    // we propagte the dirty bits.value cache, we propagte the dirty
    // bits.value cache, we propagte the dirty bits.
    return bits ? (bits | GetInitialDirtyBitsMask()) : bits;
}

void
HdPrman_Points::_InitRepr(TfToken const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    TF_UNUSED(reprToken);
    TF_UNUSED(dirtyBits);

    // No-op
}

static RixParamList *
_PopulatePrimvars(const HdPrman_Points &mesh,
                  RixRileyManager *mgr,
                  HdSceneDelegate *sceneDelegate,
                  const SdfPath &id,
                  RtUString *primType)
{
    VtValue pointsVal = sceneDelegate->Get(id, HdTokens->points);
    VtVec3fArray points = pointsVal.Get<VtVec3fArray>();

    RixParamList *primvars = mgr->CreateRixParamList(
         1, /* uniform */
         points.size(), /* vertex */
         points.size(), /* varying */
         points.size()  /* facevarying */);

    RtPoint3 const *pointsData = (RtPoint3 const*)(&points[0]);
    primvars->SetPointDetail(RixStr.k_P, pointsData, RixDetailType::k_vertex);

    *primType = RixStr.k_Ri_Points;

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, 1,
        points.size(), points.size(), points.size());

    return primvars;
}

void
HdPrman_Points::Sync(HdSceneDelegate *sceneDelegate,
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

    SdfPath const& id = GetId();
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

    // Convert (and cache) coordinate systems.
    riley::ScopedCoordinateSystem coordSys = {0, nullptr};
    if (HdPrman_Context::RileyCoordSysIdVecRefPtr convertedCoordSys =
        context->ConvertAndRetainCoordSysBindings(sceneDelegate, id)) {
        coordSys.count = convertedCoordSys->size();
        coordSys.coordsysIds = &(*convertedCoordSys)[0];
    }

    // Hydra dirty bits corresponding to PRMan master primvars
    // and instance attributes.
    const int prmanPrimvarBits =
        HdChangeTracker::DirtyPoints |
        HdChangeTracker::DirtyPrimvar |
        HdChangeTracker::DirtyNormals;
    const int prmanAttrBits =
        HdChangeTracker::DirtyVisibility |
        HdChangeTracker::DirtyTransform;

    // Create or modify geometry master.
    primvars = _PopulatePrimvars(*this, mgr, sceneDelegate, id, &primType);
    if (_masterId == riley::GeometryMasterId::k_InvalidId) {
        _masterId = riley->CreateGeometryMaster(primType, dispId, *primvars);
    } else if (*dirtyBits & prmanPrimvarBits) {
        riley->ModifyGeometryMaster(primType, _masterId, &dispId, primvars);
    }

    // Create or modify geometry instances.
    if (!isInstance) {
        // Simple, non-Hydra-instanced case.
        RtMatrix4x4 xf_rt_values[HDPRMAN_MAX_TIME_SAMPLES];
        for (size_t i=0; i < xf.count; ++i) {
            xf_rt_values[i] = HdPrman_GfMatrixToRtMatrix(xf.values[i]);
        }
        const riley::Transform xform = {
            unsigned(xf.count), xf_rt_values, xf.times};
        attrs = context->ConvertAttributes(sceneDelegate, id);
        // Add "identifier:id" with the hydra prim id, and "identifier:id2"
        // with the instance number.
        attrs->SetInteger(RixStr.k_identifier_id, GetPrimId());
        attrs->SetInteger(RixStr.k_identifier_id2, 0);
        // Truncate extra instances.
        if (_instanceIds.size() > 1) {
            for (size_t i=1; i < _instanceIds.size(); ++i) {
                riley->DeleteGeometryInstance(
                    riley::GeometryMasterId::k_InvalidId, _instanceIds[i]);
            }
            _instanceIds.resize(1);
        }
        // Create or modify single instance.
        if (_instanceIds.empty()) {
            _instanceIds.push_back( riley->CreateGeometryInstance(
                riley::GeometryMasterId::k_InvalidId,
                _masterId, materialId, coordSys, xform, *attrs) );
        } else if (*dirtyBits & prmanAttrBits) {
            riley->ModifyGeometryInstance(
                riley::GeometryMasterId::k_InvalidId,
                _instanceIds[0], &materialId, &coordSys, &xform, attrs);
        }
    } else {
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
                    _masterId, materialId, coordSys, xform, *attrs);
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
