//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/volumeAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceVolume.h"
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

TfTokenVector
UsdImagingVolumeAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingVolumeAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->volume;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingVolumeAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceVolumePrim::New(
            prim.GetPath(),
            prim,
            stageGlobals);
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingVolumeAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceVolumePrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
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
        prim, index, GetMaterialUsdPath(prim), instancerContext);
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
                    // XXX(UsdImagingPaths): Using usdPath directly
                    // as cachePath here -- we should do the correct
                    // mapping in order for instancing to work.
                    SdfPath const& cachePath = fieldUsdPrim.GetPath();
                    descriptors.push_back(
                        HdVolumeFieldDescriptor(it->first, fieldPrimType,
                            _ConvertCachePathToIndexPath(cachePath)));
                }
            }
        }
    }

    return descriptors;
}

PXR_NAMESPACE_CLOSE_SCOPE

