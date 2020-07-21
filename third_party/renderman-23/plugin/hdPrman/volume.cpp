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
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderPass.h"
#include "hdPrman/rixStrings.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usdImaging/usdVolImaging/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"
#include "RtParamList.h"
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
    : BASE(id, instancerId)
{
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
        | HdChangeTracker::DirtyInstancer
        ;

    return (HdDirtyBits)mask;
}

namespace { // anonymous namespace

void
_EmitOpenVDBVolume(HdSceneDelegate *sceneDelegate,
                   const SdfPath& id,
                   const HdVolumeFieldDescriptorVector& fields,
                   RtParamList* primvars)
{
    static const RtUString blobbydsoImplOpenVDB("blobbydso:impl_openvdb");

    if (fields.empty()) {
        return;
    }

    auto const& firstfield = fields[0];

    // There is an implicit assumption that all the fields on this volume are
    // extracted from the same .vdb file, which is determined once from the
    // first field.
    VtValue filePath = sceneDelegate->Get(firstfield.fieldId,
                                          HdFieldTokens->filePath);
    SdfAssetPath fileAssetPath = filePath.Get<SdfAssetPath>();

    std::string volumeAssetPath = fileAssetPath.GetResolvedPath();
    if (volumeAssetPath.empty()) {
        volumeAssetPath = fileAssetPath.GetAssetPath();
    }

    primvars->SetString(RixStr.k_Ri_type, blobbydsoImplOpenVDB);

    RtUString sa[2] = {
        RtUString(volumeAssetPath.c_str()),
        RtUString(firstfield.fieldName.GetText())
    };
    primvars->SetStringArray(RixStr.k_blobbydso_stringargs, sa, 2);
}

// Returns the prim type token of a list of fields, if all the fields have the
// same type. If there are no fields or the types are inconsistent it returns
// the empty token.
TfToken
_DetermineConsistentFieldPrimType(const HdVolumeFieldDescriptorVector& fields)
{
    if (fields.empty()) {
        return TfToken();
    }

    HdVolumeFieldDescriptorVector::const_iterator iter = fields.begin();
    TfToken const& fieldPrimType = iter->fieldPrimType;
    ++iter;

    for (; iter != fields.end(); ++iter) {
        if (iter->fieldPrimType != fieldPrimType) {
            return TfToken();
        }
    }

    return fieldPrimType;
}

enum _FieldType {
    FloatType = 0,
    ColorType,
    PointType,
    NormalType,
    VectorType
};

_FieldType
_DetermineFieldType(HdVolumeFieldDescriptor const& field)
{
    // XXX To get one experiement to work, we check for Cd, which is
    //     actually a color field.
    if (field.fieldName.GetString() == "Cd") {
        return ColorType;
    }

    // TODO Only Float grids for now because we do not know the grid type
    // here without reading directly from the grid which would require 
    // opening the file (linking with openvdb) or getting the vdb ptr from
    // houdini (linking with the hdk)
    return FloatType;
}

void
_SetupFieldPrimvars(HdVolumeFieldDescriptorVector const& fields,
                    RtParamList* primvars)
{
    for (HdVolumeFieldDescriptor const& field : fields) {
        RtUString fieldName = RtUString(field.fieldName.GetText());
        RtDetailType detailType = RtDetailType::k_varying;

        _FieldType type = _DetermineFieldType(field);
        switch (type) {
            case FloatType:
                primvars->SetFloatDetail(fieldName, nullptr, detailType);
                break;
            case ColorType:
                primvars->SetColorDetail(fieldName, nullptr, detailType);
                break;
            case PointType:
                primvars->SetPointDetail(fieldName, nullptr, detailType);
                break;
            case NormalType:
                primvars->SetNormalDetail(fieldName, nullptr, detailType);
                break;
            case VectorType:
                primvars->SetVectorDetail(fieldName, nullptr, detailType);
                break;
        }
    }
}

} // end anonymous namespace

/* static */
HdPrman_Volume::_VolumeEmitterMap&
HdPrman_Volume::_GetVolumeEmitterMap()
{
    // Note, the volumeEmitters map is static inside of this method to ensure
    // that it will be initialized the first time this method is called.
    // Initialization of static members of classes or globals is less clearly
    // defined.
    static _VolumeEmitterMap volumeEmitters = {
        // Pre-populate the map with the default implementation for OpenVDB
        {UsdVolImagingTokens->openvdbAsset, _EmitOpenVDBVolume}
    };

    return volumeEmitters;
}

/* static */
bool
HdPrman_Volume::AddVolumeTypeEmitter(TfToken const& fieldPrimType,
                                     HdPrman_VolumeTypeEmitter emitterFunc,
                                     bool overrideExisting)
{
    auto pair = _GetVolumeEmitterMap().insert({fieldPrimType, emitterFunc});
    // Set entry if overriding and there was a previous entry
    if (overrideExisting || pair.second) {
        pair.first->second = emitterFunc;
        return true;
    }

    return false;
}

RtParamList
HdPrman_Volume::_ConvertGeometry(HdPrman_Context *context,
                                 HdSceneDelegate *sceneDelegate,
                                 const SdfPath &id,
                                 RtUString *primType,
                                 std::vector<HdGeomSubset> *geomSubsets)
{
    HdVolumeFieldDescriptorVector fields =
        sceneDelegate->GetVolumeFieldDescriptors(id);

    if (fields.empty()) {
        return RtParamList();
    }

    TfToken fieldPrimType = _DetermineConsistentFieldPrimType(fields);
    if (fieldPrimType.IsEmpty()) {
        TF_WARN("The fields on volume %s have inconsistent types and can't be "
                "emitted as a single volume", id.GetText());
        return RtParamList();
    }

    // Based on the field type we determine the function to emit the volume to
    // Prman
    _VolumeEmitterMap const& volumeEmitters = _GetVolumeEmitterMap();
    auto const iter = volumeEmitters.find(fieldPrimType);
    if (iter == volumeEmitters.end()) {
        TF_WARN("No volume emitter registered for field type '%s' "
                "on prim %s", fieldPrimType.GetText(), id.GetText());
        return RtParamList();
    }

    int32_t const dims[] = { 0, 0, 0 };
    uint64_t const dim = dims[0] * dims[1] * dims[2];

    RtParamList primvars(1, dim, dim, dim);
    primvars.SetIntegerArray(RixStr.k_Ri_dimensions, dims, 3);

    *primType = RixStr.k_Ri_Volume;

    // Setup the volume for Prman with the appropriate DSO and its parameters
    HdPrman_VolumeTypeEmitter emitterFunc = iter->second;
    emitterFunc(sceneDelegate, id, fields, &primvars);

    // The individual fields of this volume need to be declared as primvars
    _SetupFieldPrimvars(fields, &primvars);

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, 1, dim, dim, dim);
    return primvars;
}

PXR_NAMESPACE_CLOSE_SCOPE
