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
#include "hdPrman/volume.h"

#include "hdPrman/context.h"
#include "hdPrman/material.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderPass.h"
#include "hdPrman/rixStrings.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"
#include "RixParamList.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_Field::HdPrman_Field(TfToken const& typeId, SdfPath const& id)
    : HdField(id), _typeId(typeId)
{
}

void HdPrman_Field::Sync(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdDirtyBits *dirtyBits)
{
}

void HdPrman_Field::Finalize(HdRenderParam *renderParam)
{
}

HdDirtyBits HdPrman_Field::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtVolume(), so it should list every data item
    // that _PopluateRtVolume requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyMaterialId
        ;
    return (HdDirtyBits)mask;    
}

HdPrman_Volume::HdPrman_Volume(SdfPath const& id,
                           SdfPath const& instancerId)
    : HdVolume(id, instancerId)
    , _masterId(riley::GeometryMasterId::k_InvalidId)
    , _instanceId(riley::GeometryInstanceId::k_InvalidId)
{
}

void
HdPrman_Volume::Finalize(HdRenderParam *renderParam)
{
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

    riley::Riley *riley = context->riley;

    // Release retained conversions of coordSys bindings.
    context->ReleaseCoordSysBindings(GetId());

    if (_masterId != riley::GeometryMasterId::k_InvalidId) {
        riley->DeleteGeometryMaster(_masterId);
        _masterId = riley::GeometryMasterId::k_InvalidId;
    }
    if (_instanceId != riley::GeometryInstanceId::k_InvalidId) {
        riley->DeleteGeometryInstance(
            riley::GeometryMasterId::k_InvalidId, // no group
            _instanceId);
        _instanceId = riley::GeometryInstanceId::k_InvalidId;
    }
}

HdDirtyBits
HdPrman_Volume::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtVolume(), so it should list every data item
    // that _PopluateRtVolume requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyMaterialId
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits
HdPrman_Volume::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // XXX This is not ideal. Currently Riley requires us to provide
    // all the values anytime we edit a volume. To make sure the values
    // exist in the value cache, we propagte the dirty bits.value cache,
    // we propagte the dirty bits.value cache, we propagte the dirty
    // bits.value cache, we propagte the dirty bits.
    return bits ? (bits | GetInitialDirtyBitsMask()) : bits;
}

void
HdPrman_Volume::_InitRepr(TfToken const &reprToken,
                          HdDirtyBits *dirtyBits)
{
    TF_UNUSED(reprToken);
    TF_UNUSED(dirtyBits);

    // No-op
}

static RixParamList *
_PopulatePrimvars(const HdPrman_Volume &volume,
                  RixRileyManager *mgr,
                  HdSceneDelegate *sceneDelegate,
                  const SdfPath &id,
                  RtUString *primType)
{
    static const RtUString density("density");
    static const RtUString blobbydsoImplOpenVDB("blobbydso:impl_openvdb");

    HdVolumeFieldDescriptorVector fields =
        sceneDelegate->GetVolumeFieldDescriptors(id);

    // TODO Only one field for now
    if (fields.empty()) return nullptr;
    auto const& field = fields[0];

    VtValue filePath = sceneDelegate->Get(field.fieldId,
                                          HdVolumeTokens->filePath);
    SdfAssetPath fileAssetPath = filePath.Get<SdfAssetPath>();

    int32_t const dims[] = { 0, 0, 0 };
    uint64_t const dim = dims[0] * dims[1] * dims[2];

    RixParamList *primvars = mgr->CreateRixParamList(1, dim, dim, dim);

    // TODO Only VDB for now
    primvars->SetString(RixStr.k_Ri_type, blobbydsoImplOpenVDB);
    primvars->SetIntegerArray(RixStr.k_Ri_dimensions, dims, 3);

    RtUString const fieldName(field.fieldName.GetText());
    RtUString sa[2];
    sa[0] = RtUString(fileAssetPath.GetResolvedPath().c_str());
    sa[1] = RtUString(field.fieldName.GetText());
    primvars->SetStringArray(RixStr.k_blobbydso_stringargs, sa, 2);
    primvars->SetFloatDetail(density, nullptr,
                             RixDetailType::k_varying);
    *primType = RixStr.k_Ri_Volume;

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, 1, dim, dim, dim);

    return primvars;
}

void
HdPrman_Volume::Sync(HdSceneDelegate *sceneDelegate,
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

    float const zerotime = 0.0f;
    RtMatrix4x4 matrix =
        HdPrman_GfMatrixToRtMatrix(sceneDelegate->GetTransform(id));
    riley::Transform xform = { 1, &matrix, &zerotime };

    RixRileyManager *mgr = context->mgr;
    riley::Riley *riley = context->riley;

    RixParamList *primvars = nullptr;
    RixParamList *attrs = nullptr;
    RtUString primType;

    // Look up material binding.  Default to fallbackVolumeMaterial.
    riley::MaterialId materialId = context->fallbackVolumeMaterial;
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
        HdChangeTracker::DirtyPrimvar;
    const int prmanAttrBits =
        HdChangeTracker::DirtyVisibility |
        HdChangeTracker::DirtyTransform;

    // Lazily initialize _primType.
    if (_masterId == riley::GeometryMasterId::k_InvalidId) {
        // Lazily initialize _masterId.
        primvars = _PopulatePrimvars(*this, mgr, sceneDelegate, id, &primType);
        _masterId = riley->CreateGeometryMaster(
            primType,
            dispId,
            *primvars);
    } else if (*dirtyBits & prmanPrimvarBits) {
        // Modify existing master.
        primvars = _PopulatePrimvars(*this, mgr, sceneDelegate, id, &primType);
        riley->ModifyGeometryMaster(
            primType,
            _masterId,
            &dispId,
            primvars);
    }

    if (_instanceId == riley::GeometryInstanceId::k_InvalidId) {
        // Lazily initialize _instanceId.
        attrs = context->ConvertAttributes(sceneDelegate, id);
        // Add "identifier:id" with the hydra prim id, and "identifier:id2"
        // with the instance number.
        attrs->SetInteger(RixStr.k_identifier_id, GetPrimId());
        attrs->SetInteger(RixStr.k_identifier_id2, 0);
        _instanceId = riley->CreateGeometryInstance(
            riley::GeometryMasterId::k_InvalidId, // no group
            _masterId,
            materialId,
            coordSys, xform, *attrs);
    } else if (*dirtyBits & prmanAttrBits) {
        // Modify existing instance.
        attrs = context->ConvertAttributes(sceneDelegate, id);
        // Add "identifier:id" with the hydra prim id, and "identifier:id2"
        // with the instance number.
        attrs->SetInteger(RixStr.k_identifier_id, GetPrimId());
        attrs->SetInteger(RixStr.k_identifier_id2, 0);
        riley->ModifyGeometryInstance(
            riley::GeometryMasterId::k_InvalidId, // no group
            _instanceId,
            &materialId,
            &coordSys,
            &xform,
            attrs);
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
