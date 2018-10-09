//
// Copyright 2018 Pixar
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

#include "pxr/usdImaging/usdImaging/volumeAdapter.h"
#include "pxr/usdImaging/usdImaging/fieldAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/volume.h"

#include "pxr/usd/usdVol/tokens.h"
#include "pxr/usd/usdVol/volume.h"
#include "pxr/usd/usdVol/fieldBase.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingVolumeAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingVolumeAdapter::~UsdImagingVolumeAdapter() 
{
}

bool
UsdImagingVolumeAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->volume);
}

bool
UsdImagingVolumeAdapter::_GatherVolumeData(UsdPrim const& prim, 
                                      UsdVolVolume::FieldMap *fieldMap) const
{
    UsdVolVolume volume(prim);

    if (volume) {
        // Gather all relationships in the "field" namespace to figure out 
        // which field primitives make up this volume.
        UsdVolVolume::FieldMap  fields = volume.GetFieldPaths();
        fieldMap->swap(fields);
    }

    return !fieldMap->empty();
}

SdfPath
UsdImagingVolumeAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return  _AddRprim(HdPrimTypeTokens->volume,
        prim, index, GetMaterialId(prim), instancerContext);
}

void 
UsdImagingVolumeAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const* 
                                              instancerContext) const
{
    // Just call the base class to test for a time-varying transform.
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    // Relationships can't be time varying, so we don't need to worry
    // about the mapping from field names to field prim paths being
    // time varying.
}

// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingVolumeAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
    // Call the base class to update the transform.
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);
}

HdVolumeFieldDescriptorVector
UsdImagingVolumeAdapter::GetVolumeFieldDescriptors(UsdPrim const& usdPrim,
                                                    SdfPath const &id,
                                                    UsdTimeCode time) const
{
    HdVolumeFieldDescriptorVector descriptors;
    std::map<TfToken, SdfPath> fieldMap;

    // Build HdVolumeFieldDescriptors for all our fields.
    if (_GatherVolumeData(usdPrim, &fieldMap)) {
        for (auto it = fieldMap.begin(); it != fieldMap.end(); ++it) {
            UsdPrim fieldUsdPrim(_GetPrim(it->second));
            UsdVolFieldBase fieldPrim(fieldUsdPrim);

            if (fieldPrim) {
                TfToken fieldPrimType;
                UsdImagingPrimAdapterSharedPtr adapter
                    = _GetPrimAdapter(fieldUsdPrim);
                UsdImagingFieldAdapter *fieldAdapter;

                fieldAdapter = dynamic_cast<UsdImagingFieldAdapter *>(
                    adapter.get());
                if (TF_VERIFY(fieldAdapter)) {
                    fieldPrimType = fieldAdapter->GetPrimTypeToken();
                    descriptors.push_back(
                    HdVolumeFieldDescriptor(it->first, fieldPrimType,
                        _GetPathForIndex(fieldUsdPrim.GetPath())));
                }
            }
        }
    }

    return descriptors;
}

PXR_NAMESPACE_CLOSE_SCOPE

