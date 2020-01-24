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

RtParamList
HdPrman_Volume::_ConvertGeometry(HdPrman_Context *context,
                                  HdSceneDelegate *sceneDelegate,
                                  const SdfPath &id,
                                  RtUString *primType,
                                  std::vector<HdGeomSubset> *geomSubsets)
{
    static const RtUString density("density");
    static const RtUString blobbydsoImplOpenVDB("blobbydso:impl_openvdb");

    HdVolumeFieldDescriptorVector fields =
        sceneDelegate->GetVolumeFieldDescriptors(id);

    if (fields.empty()) return RtParamList();
    auto const& firstfield = fields[0];

    VtValue filePath = sceneDelegate->Get(firstfield.fieldId,
                                          HdFieldTokens->filePath);
    SdfAssetPath fileAssetPath = filePath.Get<SdfAssetPath>();

    int32_t const dims[] = { 0, 0, 0 };
    uint64_t const dim = dims[0] * dims[1] * dims[2];

    RtParamList primvars(1, dim, dim, dim);

    // TODO Only VDB for now
    primvars.SetString(RixStr.k_Ri_type, blobbydsoImplOpenVDB);
    primvars.SetIntegerArray(RixStr.k_Ri_dimensions, dims, 3);

    RtUString const fieldName(firstfield.fieldName.GetText());
    RtUString sa[2];
    std::string path = fileAssetPath.GetResolvedPath();
    if (path.empty()) 
    {
        path = fileAssetPath.GetAssetPath();
    }
    sa[0] = RtUString(path.c_str());
    sa[1] = RtUString(firstfield.fieldName.GetText());
    primvars.SetStringArray(RixStr.k_blobbydso_stringargs, sa, 2);
    *primType = RixStr.k_Ri_Volume;

    // TODO Only Float grids for now because we do not know the grid type
    // here without reading directly from the grid which would require 
    // opening the file (linking with openvdb) or getting the vdb ptr from
    // houdini (linking with the hdk)
    for (int i = 0; i < fields.size(); ++i)
    {
        HdVolumeFieldDescriptor const& field = fields[i]; 
        RtUString fieldName = RtUString(field.fieldName.GetText());
        primvars.SetFloatDetail(fieldName, nullptr, RtDetailType::k_varying);
    }

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, 1, dim, dim, dim);
    return std::move(primvars);
}

PXR_NAMESPACE_CLOSE_SCOPE
