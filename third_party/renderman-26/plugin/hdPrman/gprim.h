//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GPRIM_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GPRIM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/gf/matrix4d.h"

#include "hdPrman/gprimbase.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A mix-in template that adds shared gprim behavior to support
/// various HdRprim types.
template <typename BASE>
class HdPrman_Gprim : public BASE, public HdPrman_GprimBase
{
public:
    using BaseType = BASE;

    HdPrman_Gprim(SdfPath const& id)
        : BaseType(id)
    {
    }

    ~HdPrman_Gprim() override = default;

    void
    Finalize(HdRenderParam *renderParam) override
    {
        HdPrman_RenderParam *param =
            static_cast<HdPrman_RenderParam*>(renderParam);
        const SdfPath& id = BASE::GetId();
        riley::Riley *riley = param->AcquireRiley();
        if (!riley) {
            return;
        }

        // Release retained conversions of coordSys bindings.
        param->ReleaseCoordSysBindings(id);

        // Delete instances before deleting the prototypes they use.
        for (const auto &instId: _instanceIds) {
            if (instId != riley::GeometryInstanceId::InvalidId()) {
                riley->DeleteGeometryInstance(
                    riley::GeometryPrototypeId::InvalidId(), instId);
            }
        }
        _instanceIds.clear();

        // delete instances owned by the instancer.
        if (HdPrmanInstancer* instancer = param->GetInstancer(
            BASE::GetInstancerId())) {
            instancer->Depopulate(renderParam, id);
        }

        for (const auto &protoId: _prototypeIds) {
            if (protoId != riley::GeometryPrototypeId::InvalidId()) {
                riley->DeleteGeometryPrototype(protoId);
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
        // By default, just return the same dirty bits we recieved.
        return bits;
    }

    void
    _InitRepr(TfToken const &reprToken,
              HdDirtyBits *dirtyBits) override
    {
        TF_UNUSED(reprToken);
        TF_UNUSED(dirtyBits);
        // No-op
    }

    // We override this member function in mesh.cpp to support the creation
    // of mesh light prototype geometry.
    virtual bool
    _PrototypeOnly()
    {
        return false;
    }

    // Provide a fallback material.  Default grabs _fallbackMaterial
    // from the context.
    virtual riley::MaterialId
    _GetFallbackMaterial(HdPrman_RenderParam *renderParam)
    {
        return renderParam->GetFallbackMaterialId();
    }

    // Populate primType and primvars.
    virtual RtPrimVarList
    _ConvertGeometry(HdPrman_RenderParam *renderParam,
                      HdSceneDelegate *sceneDelegate,
                      const SdfPath &id,
                      RtUString *primType,
                      std::vector<HdGeomSubset> *geomSubsets) = 0;

    // This class does not support copying.
    HdPrman_Gprim(const HdPrman_Gprim&)             = delete;
    HdPrman_Gprim &operator =(const HdPrman_Gprim&) = delete;

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

    // Check if there are any relevant dirtyBits.
    // (See the HdChangeTracker::MarkRprimDirty() note regarding
    // internalDirtyBits used for internal signaling in Hydra.)
    static const HdDirtyBits internalDirtyBits =
        HdChangeTracker::InitRepr |
        HdChangeTracker::Varying |
        HdChangeTracker::NewRepr |
        HdChangeTracker::CustomBitsMask;
    // HdPrman does not make use of the repr concept or customBits.
    *dirtyBits &= ~(HdChangeTracker::NewRepr | HdChangeTracker::CustomBitsMask);
    // If no relevant dirtyBits remain, return early to avoid acquiring write
    // access to Riley, which requires a pause and restart of rendering.
    if (((*dirtyBits & ~internalDirtyBits)
         & HdChangeTracker::AllSceneDirtyBits) == 0) {
        return;
    }

    HdPrman_RenderParam *param =
        static_cast<HdPrman_RenderParam*>(renderParam);

    // Riley API.
    riley::Riley *riley = param->AcquireRiley();

    // Update instance bindings.
    BASE::_UpdateInstancer(sceneDelegate, dirtyBits);

    // Prim id
    SdfPath const& id = BASE::GetId();
    SdfPath const& instancerId = BASE::GetInstancerId();
    const bool isHdInstance = !instancerId.IsEmpty();
    SdfPath primPath = sceneDelegate->GetScenePrimPath(id, 0, nullptr);

    // Prman has a default value for identifier:id of 0 (in case of ray miss),
    // while Hydra treats id -1 as the clear value.  We map Prman primId as
    // (Hydra primId + 1) to get around this, here and in
    // hdPrman/framebuffer.cpp.
    const int32_t primId = BASE::GetPrimId() + 1;

    // Sample transform
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
    sceneDelegate->SampleTransform(id,
#if HD_API_VERSION >= 68
                                   param->GetShutterInterval()[0],
                                   param->GetShutterInterval()[1],
#endif
                                   &xf);

    // Update visibility so thet rprim->IsVisible() will work in render pass
    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        BASE::_UpdateVisibility(sceneDelegate, dirtyBits);
    }

    // Resolve material binding.  Default to fallbackGprimMaterial.
    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
#if HD_API_VERSION < 37
        BASE::_SetMaterialId(sceneDelegate->GetRenderIndex().GetChangeTracker(),
                             sceneDelegate->GetMaterialId(id));
#else
        BASE::SetMaterialId(sceneDelegate->GetMaterialId(id));
#endif
    }
    riley::MaterialId materialId = _GetFallbackMaterial(param);
    riley::DisplacementId dispId = riley::DisplacementId::InvalidId();
    const SdfPath & hdMaterialId = BASE::GetMaterialId();
    HdPrman_ResolveMaterial(sceneDelegate, hdMaterialId, riley, &materialId, &dispId);

    // Convert (and cache) coordinate systems.
    riley::CoordinateSystemList coordSysList = {0, nullptr};
    if (HdPrman_RenderParam::RileyCoordSysIdVecRefPtr convertedCoordSys =
        param->ConvertAndRetainCoordSysBindings(sceneDelegate, id)) {
        coordSysList.count = convertedCoordSys->size();
        coordSysList.ids = convertedCoordSys->data();
    }

    // Hydra dirty bits corresponding to PRMan prototype attributes (also called
    // "primitive variables" but not synonymous with USD primvars). See prman
    // docs at https://rmanwiki.pixar.com/display/REN24/Primitive+Variables.
    static const HdDirtyBits prmanProtoAttrBits =
        HdChangeTracker::DirtyPoints |
        HdChangeTracker::DirtyNormals |
        HdChangeTracker::DirtyWidths |
        HdChangeTracker::DirtyVolumeField |
        HdChangeTracker::DirtyTopology |
        HdChangeTracker::DirtyPrimvar;

    // Hydra dirty bits corresponding to prman instance attributes. See prman
    // docs at https://rmanwiki.pixar.com/display/REN24/Instance+Attributes.
    static const HdDirtyBits prmanInstAttrBits =
        HdChangeTracker::DirtyMaterialId |
        HdChangeTracker::DirtyTransform |
        HdChangeTracker::DirtyVisibility |
        HdChangeTracker::DirtyDoubleSided |
        HdChangeTracker::DirtySubdivTags |
        HdChangeTracker::DirtyVolumeField |
        HdChangeTracker::DirtyCategories |
        HdChangeTracker::DirtyPrimvar;

    // These two bitmasks intersect, so we check them against dirtyBits
    // prior to clearing either mask.
    const bool prmanProtoAttrBitsWereSet(*dirtyBits & prmanProtoAttrBits);
    const bool prmanInstAttrBitsWereSet(*dirtyBits & prmanInstAttrBits);

    //
    // Create or modify Riley geometry prototype(s).
    //
    std::vector<riley::MaterialId> subsetMaterialIds;
    std::vector<SdfPath> subsetPaths;
    {
        RtUString primType;
        HdGeomSubsets geomSubsets;
        RtPrimVarList primvars = _ConvertGeometry(param, sceneDelegate, id,
                         &primType, &geomSubsets);

        // identifier:object is useful for cryptomatte
        primvars.SetString(RixStr.k_identifier_object,
                           RtUString(id.GetName().c_str()));

// In 2311 and beyond, we can use
// HdPrman_PreviewSurfacePrimvarsSceneIndexPlugin.
#if PXR_VERSION < 2311
        // Transfer material opinions of primvars.
        HdPrman_TransferMaterialPrimvarOpinions(sceneDelegate, hdMaterialId,
            primvars);
#endif // PXR_VERSION < 2311

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
            primvars.SetString(RixStr.k_stats_prototypeIdentifier,
                RtUString(primPath.GetText()));
            if (_prototypeIds[0] == riley::GeometryPrototypeId::InvalidId()) {
                TRACE_SCOPE("riley::CreateGeometryPrototype");
                _prototypeIds[0] = riley->CreateGeometryPrototype(
                    riley::UserId(
                        stats::AddDataLocation(primPath.GetText()).GetValue()),
                    primType, dispId, primvars);
            } else if (prmanProtoAttrBitsWereSet) {
                TRACE_SCOPE("riley::ModifyGeometryPrototype");
                riley->ModifyGeometryPrototype(primType, _prototypeIds[0],
                                               &dispId, &primvars);
            }
        } else {
            // Subsets case.
            // We resolve materials here, and hold them in subsetMaterialIds:
            // Displacement networks are passed to the geom prototype;
            // material networks are passed to the instances.
            subsetMaterialIds.reserve(geomSubsets.size());

            // We also cache the subset paths for re-use when creating the instances
            subsetPaths.reserve(geomSubsets.size());

            for (size_t j=0; j < geomSubsets.size(); ++j) {
                auto& prototypeId = _prototypeIds[j];
                HdGeomSubset &subset = geomSubsets[j];

                // Convert indices to int32_t and set as k_shade_faceset.
                std::vector<int32_t> int32Indices(subset.indices.cbegin(),
                                                  subset.indices.cend());
                primvars.SetIntegerArray(RixStr.k_shade_faceset,
                                         int32Indices.data(),
                                         int32Indices.size());
                // Look up material override for the subset (if any)
                riley::MaterialId subsetMaterialId = materialId;
                riley::DisplacementId subsetDispId = dispId;
                if (subset.materialId.IsEmpty()) {
                    subset.materialId = hdMaterialId;
                }
                HdPrman_ResolveMaterial(sceneDelegate, subset.materialId,
                                        riley, &subsetMaterialId, &subsetDispId);
                subsetMaterialIds.push_back(subsetMaterialId);

                // Look up the path for the subset
                SdfPath subsetPath = sceneDelegate->GetScenePrimPath(subset.id, 0, nullptr);
                subsetPaths.push_back(subsetPath);
                primvars.SetString(RixStr.k_stats_prototypeIdentifier,
                    RtUString(subsetPath.GetText()));

                if (prototypeId == riley::GeometryPrototypeId::InvalidId()) {
                    TRACE_SCOPE("riley::CreateGeometryPrototype");
                    prototypeId =
                        riley->CreateGeometryPrototype(
                            riley::UserId(
                                stats::AddDataLocation(subsetPath.GetText()).GetValue()),
                            primType, dispId, primvars);
                } else if (prmanProtoAttrBitsWereSet) {
                    TRACE_SCOPE("riley::ModifyGeometryPrototype");
                    riley->ModifyGeometryPrototype(primType, prototypeId,
                                                &subsetDispId, &primvars);
                }
            }
        }
        *dirtyBits &= ~prmanProtoAttrBits;
    }

    //
    // Stop here, or also create geometry instances?
    //
    if (_PrototypeOnly()) {
        *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
        return;
    }

    //
    // Create or modify Riley geometry instances.
    //

    // Resolve attributes.
    RtParamList attrs = param->ConvertAttributes(sceneDelegate, id, true);

    // Add "identifier:id" with the prim id.
    attrs.SetInteger(RixStr.k_identifier_id, primId);

    // user:__materialid is useful for cryptomatte
    if(!hdMaterialId.IsEmpty()) {
        attrs.SetString(RtUString("user:__materialid"), RtUString(hdMaterialId.GetText()));
    }

    if (!isHdInstance) {
        // Simple case: Singleton instance.
        // Convert transform.
        TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> xf_rt(xf.count);
        for (size_t i=0; i < xf.count; ++i) {
            xf_rt[i] = HdPrman_Utils::GfMatrixToRtMatrix(xf.values[i]);
        }
        const riley::Transform xform = {
            unsigned(xf.count),
            xf_rt.data(),
            xf.times.data()};

        // Add "identifier:id2" with the instance number.
        // XXX Do we want to distinguish facesets here?
        attrs.SetInteger(RixStr.k_identifier_id2, 0);

        // Adjust _instanceIds array.
        const size_t oldCount = _instanceIds.size();
        const size_t newCount = _prototypeIds.size();
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

        // Prepend renderTag to grouping:membership
        param->AddRenderTagToGroupingMembership(
            sceneDelegate->GetRenderTag(id), attrs);

        // Create or modify Riley instances corresponding to a
        // singleton Hydra instance.
        TF_VERIFY(_instanceIds.size() == _prototypeIds.size());
        for (size_t j=0; j < _prototypeIds.size(); ++j) {
            auto const& prototypeId = _prototypeIds[j];
            auto& instanceId = _instanceIds[j];
            auto instanceMaterialId = materialId;
            RtParamList finalAttrs = attrs; // copy

            // If a subset path was cached, use it. If not, use the prim path.
            SdfPath* subsetPath(&primPath);
            if (!subsetPaths.empty()) {
                subsetPath = &subsetPaths[j];
            }

            finalAttrs.SetString(RixStr.k_identifier_name,
                RtUString(subsetPath->GetText()));

            // If a valid subset material was bound, use it.
            if (!subsetMaterialIds.empty()) {
                TF_VERIFY(j < subsetMaterialIds.size());
                instanceMaterialId = subsetMaterialIds[j];
            }

            if (instanceId == riley::GeometryInstanceId::InvalidId()) {
                TRACE_SCOPE("riley::CreateGeometryInstance");
                instanceId = riley->CreateGeometryInstance(
                    riley::UserId(
                        stats::AddDataLocation(subsetPath->GetText()).GetValue()),
                    riley::GeometryPrototypeId::InvalidId(), prototypeId,
                    instanceMaterialId, coordSysList, xform, finalAttrs);
            } else if (prmanInstAttrBitsWereSet) {
                TRACE_SCOPE("riley::ModifyGeometryInstance");
                riley->ModifyGeometryInstance(
                    riley::GeometryPrototypeId::InvalidId(),
                    instanceId, &instanceMaterialId, &coordSysList, &xform,
                    &finalAttrs);
            }
        }
        *dirtyBits &= ~prmanInstAttrBits;
    } else if (prmanInstAttrBitsWereSet
        || HdChangeTracker::IsInstancerDirty(*dirtyBits, instancerId)) {
        // This gprim is a prototype of a hydra instancer. (It is not itself an
        // instancer because it is a gprim.) The riley geometry prototypes have
        // already been synced above, and those are owned by this gprim instance.
        // We need to tell the hdprman instancer to sync its riley instances for
        // these riley prototypes.
        //
        // We won't make any riley instances here. The hdprman instancer will
        // own the riley instances instead.
        //
        // We only need to do this if dirtyBits says the instancer is dirty.

        HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

        // first, sync the hydra instancer and its parents, from the bottom up.
        // (note: this is transitional code, it should be done by the render index...)
        HdInstancer::_SyncInstancerAndParents(renderIndex, instancerId);

        if (subsetMaterialIds.size() == 0) {
            subsetMaterialIds.push_back(materialId);
        }
        if (subsetPaths.size() == 0) {
            subsetPaths.push_back(primPath);
        }
        TF_VERIFY(_prototypeIds.size() == subsetMaterialIds.size() &&
                  _prototypeIds.size() == subsetPaths.size(),
                  "size mismatch (%lu, %lu, %lu)\n", _prototypeIds.size(),
                  subsetMaterialIds.size(), subsetPaths.size());

        // XXX: To avoid a failed verify inside Populate(), we will check the
        // prototype ids for validity here. We don't usually do this, relying on
        // Riley to report invalid prototype ids on instance creation. But
        // Populate() allows and expects an invalid prototype id when instancing
        // lights, so doing this check here lets us make a more informative
        // warning. HYD-3206
        if (std::any_of(_prototypeIds.begin(), _prototypeIds.end(),
            [](const auto& id){
                return id == riley::GeometryPrototypeId::InvalidId();
            })) {
            TF_WARN("Riley geometry prototype creation failed for "
                "instanced gprim <%s>; the prim will not be instanced.",
                id.GetText());
        } else {
            // next, tell the hdprman instancer to sync the riley instances
            HdPrmanInstancer *instancer = static_cast<HdPrmanInstancer*>(
                renderIndex.GetInstancer(instancerId));
            if (instancer) {
                instancer->Populate(
                    renderParam,
                    dirtyBits,
                    id,
                    _prototypeIds,
                    coordSysList,
                    attrs, xf,
                    subsetMaterialIds,
                    subsetPaths);
            }
        }
    }
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GPRIM_H
