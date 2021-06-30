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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GPRIM_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GPRIM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "hdPrman/context.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderPass.h"
#include "hdPrman/rixStrings.h"

#include "Riley.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

/// A mix-in template that adds shared gprim behavior to support
/// various HdRprim types.
template <typename BASE>
class HdPrman_Gprim : public BASE {
public:
    typedef BASE BaseType;

    HdPrman_Gprim(SdfPath const& id)
        : BaseType(id)
    {
    }

    virtual ~HdPrman_Gprim() = default;

    void
    Finalize(HdRenderParam *renderParam) override
    {
        HdPrman_Context *context =
            static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

        riley::Riley *riley = context->riley;

        // Release retained conversions of coordSys bindings.
        context->ReleaseCoordSysBindings(BASE::GetId());

        // Delete instances before deleting the prototypes they use.
        for (const auto &id: _instanceIds) {
            if (id != riley::GeometryInstanceId::InvalidId()) {
                riley->DeleteGeometryInstance(
                    riley::GeometryPrototypeId::InvalidId(), id);
            }
        }
        _instanceIds.clear();
        for (const auto &id: _prototypeIds) {
            if (id != riley::GeometryPrototypeId::InvalidId()) {
                riley->DeleteGeometryPrototype(id);
            }
        }
        _prototypeIds.clear();
    }

    void Sync(HdSceneDelegate* sceneDelegate,
         HdRenderParam*   renderParam,
         HdDirtyBits*     dirtyBits,
         TfToken const    &reprToken) override;

protected:
    HdDirtyBits GetInitialDirtyBitsMask() const override = 0;

    HdDirtyBits
    _PropagateDirtyBits(HdDirtyBits bits) const override
    {
        // XXX This is not ideal. Currently Riley requires us to provide
        // all the values anytime we edit a volume. To make sure the values
        // exist in the value cache, we propagte the dirty bits.value cache,
        // we propagte the dirty bits.value cache, we propagte the dirty
        // bits.value cache, we propagte the dirty bits.
        return bits ? (bits | GetInitialDirtyBitsMask()) : bits;
    }

    void
    _InitRepr(TfToken const &reprToken,
              HdDirtyBits *dirtyBits) override
    {
        TF_UNUSED(reprToken);
        TF_UNUSED(dirtyBits);
        // No-op
    }

    // Provide a fallback material.  Default grabs _fallbackMaterial
    // from the context.
    virtual riley::MaterialId
    _GetFallbackMaterial(HdPrman_Context *context)
    {
        return context->fallbackMaterial;
    }

    // Populate primType and primvars.
    virtual RtPrimVarList
    _ConvertGeometry(HdPrman_Context *context,
                      HdSceneDelegate *sceneDelegate,
                      const SdfPath &id,
                      RtUString *primType,
                      std::vector<HdGeomSubset> *geomSubsets) = 0;

    // This class does not support copying.
    HdPrman_Gprim(const HdPrman_Gprim&)             = delete;
    HdPrman_Gprim &operator =(const HdPrman_Gprim&) = delete;

protected:
    std::vector<riley::GeometryPrototypeId> _prototypeIds;
    std::vector<riley::GeometryInstanceId> _instanceIds;
};

template <typename BASE>
void
HdPrman_Gprim<BASE>::Sync(HdSceneDelegate* sceneDelegate,
                          HdRenderParam*   renderParam,
                          HdDirtyBits*     dirtyBits,
                          TfToken const    &reprToken)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    TF_UNUSED(reprToken);

    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

    // Update instance bindings.
    BASE::_UpdateInstancer(sceneDelegate, dirtyBits);

    // Prim id
    SdfPath const& id = BASE::GetId();
    SdfPath const& instancerId = BASE::GetInstancerId();
    const bool isHdInstance = !instancerId.IsEmpty();
    // Prman has a default value for identifier:id of 0 (in case of ray miss),
    // while Hydra treats id -1 as the clear value.  We map Prman primId as
    // (Hydra primId + 1) to get around this, here and in
    // hdPrman/framebuffer.cpp.
    const int32_t primId = BASE::GetPrimId() + 1;

    // Sample transform
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
    sceneDelegate->SampleTransform(id, &xf);

    // Riley API.
    riley::Riley *riley = context->riley;

    // Resolve material binding.  Default to fallbackGprimMaterial.
    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
#if HD_API_VERSION < 37
        BASE::_SetMaterialId(sceneDelegate->GetRenderIndex().GetChangeTracker(),
                             sceneDelegate->GetMaterialId(id));
#else
        BASE::SetMaterialId(sceneDelegate->GetMaterialId(id));
#endif
    }
    riley::MaterialId materialId = _GetFallbackMaterial(context);
    riley::DisplacementId dispId = riley::DisplacementId::InvalidId();
    const SdfPath & hdMaterialId = BASE::GetMaterialId();
    HdPrman_ResolveMaterial(sceneDelegate, hdMaterialId, &materialId, &dispId);

    // Convert (and cache) coordinate systems.
    riley::CoordinateSystemList coordSysList = {0, nullptr};
    if (HdPrman_Context::RileyCoordSysIdVecRefPtr convertedCoordSys =
        context->ConvertAndRetainCoordSysBindings(sceneDelegate, id)) {
        coordSysList.count = convertedCoordSys->size();
        coordSysList.ids = &(*convertedCoordSys)[0];
    }

    // Hydra dirty bits corresponding to PRMan prototype primvars
    // and instance attributes.
    const int prmanPrimvarBits =
        HdChangeTracker::DirtyPrimvar;
    const int prmanAttrBits =
        HdChangeTracker::DirtyVisibility |
        HdChangeTracker::DirtyTransform;

    //
    // Create or modify Riley geometry prototype(s).
    //
    std::vector<riley::MaterialId> subsetMaterialIds;
    {
        RtUString primType;
        HdGeomSubsets geomSubsets;
        RtPrimVarList primvars = _ConvertGeometry(context, sceneDelegate, id,
                         &primType, &geomSubsets);

        // Adjust _prototypeIds array.
        const size_t oldCount = _prototypeIds.size();
        const size_t newCount = std::max((size_t) 1, geomSubsets.size());
        if (newCount != oldCount) {
            for (const auto &oldPrototypeId: _prototypeIds) {
                if (oldPrototypeId != riley::GeometryPrototypeId::InvalidId()) {
                    riley->DeleteGeometryPrototype(oldPrototypeId);
                }
            }
            _prototypeIds.resize(newCount,
                              riley::GeometryPrototypeId::InvalidId());
        }

        // Update Riley geom prototypes.
        if (geomSubsets.empty()) {
            // Common case: no subsets.
            TF_VERIFY(newCount == 1);
            TF_VERIFY(_prototypeIds.size() == 1);
            if (_prototypeIds[0] == riley::GeometryPrototypeId::InvalidId()) {
                _prototypeIds[0] =
                    riley->CreateGeometryPrototype(riley::UserId::DefaultId(),
                                                primType, dispId,
                                                primvars);
            } else if (*dirtyBits & prmanPrimvarBits) {
                riley->ModifyGeometryPrototype(primType, _prototypeIds[0],
                                            &dispId, &primvars);
            }
        } else {
            // Subsets case.
            // We resolve materials here, and hold them in subsetMaterialIds:
            // Displacement networks are passed to the geom prototype;
            // material networks are passed to the instances.
            subsetMaterialIds.reserve(geomSubsets.size());
            for (size_t j=0; j < geomSubsets.size(); ++j) {
                auto& prototypeId = _prototypeIds[j];
                HdGeomSubset &subset = geomSubsets[j];
                // Convert indices to int32_t and set as k_shade_faceset.
                std::vector<int32_t> int32Indices(subset.indices.begin(),
                                                  subset.indices.end());
                primvars.SetIntegerArray(RixStr.k_shade_faceset,
                                          &int32Indices[0],
                                          int32Indices.size());
                // Look up material override for the subset (if any)
                riley::MaterialId subsetMaterialId = materialId;
                riley::DisplacementId subsetDispId = dispId;
                if (subset.materialId.IsEmpty()) {
                    subset.materialId = hdMaterialId;
                }
                HdPrman_ResolveMaterial(sceneDelegate, subset.materialId,
                                        &subsetMaterialId, &subsetDispId);
                subsetMaterialIds.push_back(subsetMaterialId);
                if (prototypeId == riley::GeometryPrototypeId::InvalidId()) {
                    prototypeId =
                        riley->CreateGeometryPrototype(
                            riley::UserId::DefaultId(),
                            primType, dispId, primvars);
                } else if (*dirtyBits & prmanPrimvarBits) {
                    riley->ModifyGeometryPrototype(primType, prototypeId,
                                                &dispId, &primvars);
                }
            }
        }
    }

    //
    // Create or modify Riley geometry instances.
    //
    // Resolve attributes.
    RtParamList attrs = context->ConvertAttributes(sceneDelegate, id);
    if (!isHdInstance) {
        // Simple case: Singleton instance.
        // Convert transform.
        TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> xf_rt(xf.count);
        for (size_t i=0; i < xf.count; ++i) {
            xf_rt[i] = HdPrman_GfMatrixToRtMatrix(xf.values[i]);
        }
        const riley::Transform xform = {
            unsigned(xf.count), 
            xf_rt.data(), 
            xf.times.data()};

        // Add "identifier:id" with the hydra prim id, and "identifier:id2"
        // with the instance number.
        // XXX Do we want to distinguish facesets here?
        attrs.SetInteger(RixStr.k_identifier_id, primId);
        attrs.SetInteger(RixStr.k_identifier_id2, 0);
        // Adjust _instanceIds array.
        const size_t newNumHdInstances = 1u;
        const size_t oldCount = _instanceIds.size();
        const size_t newCount = newNumHdInstances * _prototypeIds.size();
        if (newCount != oldCount) {
            for (const auto &oldInstanceId: _instanceIds) {
                if (oldInstanceId != riley::GeometryInstanceId::InvalidId()) {
                    riley->DeleteGeometryInstance(
                        riley::GeometryPrototypeId::InvalidId(), oldInstanceId);
                }
            }
            _instanceIds.resize(
                newCount,
                riley::GeometryInstanceId::InvalidId());
        }
        // Create or modify Riley instances corresponding to a
        // singleton Hydra instance.
        TF_VERIFY(_instanceIds.size() == _prototypeIds.size());
        for (size_t j=0; j < _prototypeIds.size(); ++j) {
            auto const& prototypeId = _prototypeIds[j];
            auto& instanceId = _instanceIds[j];
            auto instanceMaterialId = materialId;
            // If a valid subset material was bound, use it.
            if (!subsetMaterialIds.empty()) {
                TF_VERIFY(j < subsetMaterialIds.size());
                instanceMaterialId = subsetMaterialIds[j];
            }
            if (instanceId == riley::GeometryInstanceId::InvalidId()) {
                instanceId = riley->CreateGeometryInstance(
                        riley::UserId::DefaultId(),
                        riley::GeometryPrototypeId::InvalidId(),
                        prototypeId, instanceMaterialId, coordSysList,
                        xform, attrs);
            } else if (*dirtyBits & prmanAttrBits) {
                riley->ModifyGeometryInstance(
                    riley::GeometryPrototypeId::InvalidId(),
                    instanceId, &instanceMaterialId, &coordSysList,
                    &xform, &attrs);
            }
        }
    } else {
        // Hydra Instancer case.
        HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

        // Sync the hydra instancer (note: this is transitional code, it should
        // be done by the render index...)
        HdInstancer::_SyncInstancerAndParents(renderIndex, instancerId);

        HdPrmanInstancer *instancer = static_cast<HdPrmanInstancer*>(
            renderIndex.GetInstancer(instancerId));
        VtIntArray instanceIndices =
            sceneDelegate->GetInstanceIndices(instancerId, id);

        // Sample per-instance transforms.
        HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES> ixf;
        instancer->SampleInstanceTransforms(id, instanceIndices, &ixf);

        // Adjust _instanceIds array.
        // Each Hydra instance produces a Riley instance for each
        // geometry prototype.  The number of geometry prototypes is
        // based on the number of geometry subsets.
        const size_t newNumHdInstances =
            (ixf.count > 0) ? ixf.values[0].size() : 0;
        const size_t oldCount = _instanceIds.size();
        const size_t newCount = newNumHdInstances * _prototypeIds.size();
        if (newCount != oldCount) {
            for (const auto &oldInstanceId: _instanceIds) {
                riley->DeleteGeometryInstance(
                    riley::GeometryPrototypeId::InvalidId(),
                    oldInstanceId);
            }
            _instanceIds.resize(newCount,
                                riley::GeometryInstanceId::InvalidId());
        }

        // Add "identifier:id" with the hydra prim id.
        attrs.SetInteger(RixStr.k_identifier_id, primId);

        // Retrieve instance categories.
        std::vector<VtArray<TfToken>> instanceCategories =
            sceneDelegate->GetInstanceCategories(instancerId);

        // Process each Hydra instance.
        for (size_t i=0; i < newNumHdInstances; ++i) {
            // XXX: Add support for nested instancing instance primvars.
            size_t instanceIndex = 0;
            if (i < instanceIndices.size()) {
                instanceIndex = instanceIndices[i];
            }

            // Create a copy of the instancer attrs.
            RtParamList instanceAttrs = attrs;
            instancer->GetInstancePrimvars(id, instanceIndex, instanceAttrs);
            // Add "identifier:id2" with the instance number.
            instanceAttrs.SetInteger(RixStr.k_identifier_id2, i);

            // Convert categories.
            if (instanceIndex < instanceCategories.size()) {
                context->ConvertCategoriesToAttributes(
                    id, instanceCategories[instanceIndex], instanceAttrs);
            }

            // Convert transform.
            // PRMan does not allow transforms on geometry prototypes,
            // so we apply that transform (xf) to all the instances, here.
            TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> 
                rt_xf(ixf.count);

            if (xf.count == 0 ||
                (xf.count == 1 && (xf.values[0] == GfMatrix4d(1)))) {
                // Expected case: prototype xf is constant & exactly identity.
                for (size_t j=0; j < ixf.count; ++j) {
                    rt_xf[j] = HdPrman_GfMatrixToRtMatrix(ixf.values[j][i]);
                }
            } else {
                // Multiply resampled prototype xf against instance xforms.
                for (size_t j=0; j < ixf.count; ++j) {
                    GfMatrix4d xf_j = xf.Resample(ixf.times[j]);
                    rt_xf[j] = 
                        HdPrman_GfMatrixToRtMatrix(xf_j * ixf.values[j][i]);
                }
            }
            const riley::Transform xform = 
                { unsigned(ixf.count), rt_xf.data(), ixf.times.data() };

            // Create or modify Riley instances corresponding to this
            // Hydra instance.
            for (size_t j=0; j < _prototypeIds.size(); ++j) {
                auto const& prototypeId = _prototypeIds[j];
                auto& instanceId = _instanceIds[i*_prototypeIds.size() + j];
                auto instanceMaterialId = materialId;
                // If a valid subset material was bound, use it.
                if (!subsetMaterialIds.empty()) {
                    TF_VERIFY(j < subsetMaterialIds.size());
                    instanceMaterialId = subsetMaterialIds[j];
                }
                if (instanceId == riley::GeometryInstanceId::InvalidId()) {
                    instanceId = riley->CreateGeometryInstance(
                        riley::UserId::DefaultId(),
                        riley::GeometryPrototypeId::InvalidId(),
                        prototypeId, instanceMaterialId, coordSysList,
                        xform, instanceAttrs);
                } else if (*dirtyBits & prmanAttrBits) {
                    riley->ModifyGeometryInstance(
                        riley::GeometryPrototypeId::InvalidId(),
                        instanceId, &instanceMaterialId, &coordSysList,
                        &xform, &instanceAttrs);
                }
            }
        }
    }
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GPRIM_H
