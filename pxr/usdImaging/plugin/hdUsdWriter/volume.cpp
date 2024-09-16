//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#include "pxr/usdImaging/plugin/hdUsdWriter/volume.h"

#include "pxr/usd/usdVol/volume.h"

PXR_NAMESPACE_OPEN_SCOPE

HdUsdWriterVolume::HdUsdWriterVolume(SdfPath const& id) : HdUsdWriterRprim<HdVolume>(id)
{
}

HdDirtyBits HdUsdWriterVolume::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::Clean | HdChangeTracker::DirtyVolumeField |
           _GetInitialDirtyBitsMask();
}

void HdUsdWriterVolume::Sync(HdSceneDelegate* sceneDelegate,
                        HdRenderParam* renderParam,
                        HdDirtyBits* dirtyBits,
                        TfToken const& reprToken)
{
    const auto& id = GetId();
    // To avoid syncing primvars for now.
    *dirtyBits = *dirtyBits & ~(HdChangeTracker::DirtyPrimvar);
    _Sync(sceneDelegate, id, dirtyBits);

    if (*dirtyBits & HdChangeTracker::DirtyVolumeField)
    {
        _volumeFieldDescriptors = sceneDelegate->GetVolumeFieldDescriptors(id);
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void HdUsdWriterVolume::SerializeToUsd(const UsdStagePtr &stage)
{
    const auto volume = UsdVolVolume::Define(stage, GetId());
    _SerializeToUsd(volume.GetPrim(), [](const auto&) -> auto { return false; });

    HdUsdWriterPopOptional(_volumeFieldDescriptors,
        [&volume](const auto& volumeFieldDescriptors)
        {
            for (const auto& volumeFieldDescriptor : volumeFieldDescriptors)
            {
                volume.CreateFieldRelationship(
                    volumeFieldDescriptor.fieldName, volumeFieldDescriptor.fieldId);
            }
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
