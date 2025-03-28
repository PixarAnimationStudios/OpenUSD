//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/usdImaging/usdImaging/unloadedDrawModeSceneIndex.h"

#include "pxr/usdImaging/usdImaging/geomModelSchema.h"
#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingUnloadedDrawModeSceneIndexRefPtr
UsdImagingUnloadedDrawModeSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return TfCreateRefPtr(
        new UsdImagingUnloadedDrawModeSceneIndex(inputSceneIndex));
}

UsdImagingUnloadedDrawModeSceneIndex::UsdImagingUnloadedDrawModeSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

UsdImagingUnloadedDrawModeSceneIndex::
~UsdImagingUnloadedDrawModeSceneIndex() = default;

static
bool
_IsPrimLoaded(HdContainerDataSourceHandle const &primSource)
{
    UsdImagingUsdPrimInfoSchema schema =
        UsdImagingUsdPrimInfoSchema::GetFromParent(primSource);
    if (HdBoolDataSourceHandle const ds = schema.GetIsLoaded()) {
        return ds->GetTypedValue(0.0f);
    }
    return true;
}

static
HdContainerDataSourceHandle const &
_DataSourceForcingBoundsDrawMode()
{
    static HdContainerDataSourceHandle result =
        HdRetainedContainerDataSource::New(
            UsdImagingGeomModelSchema::GetSchemaToken(),
            UsdImagingGeomModelSchema::Builder()
                .SetApplyDrawMode(
                    HdRetainedTypedSampledDataSource<bool>::New(true))
                .SetDrawMode(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        UsdImagingGeomModelSchemaTokens->bounds))
                .Build());
    return result;
}

HdSceneIndexPrim
UsdImagingUnloadedDrawModeSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (!_IsPrimLoaded(prim.dataSource)) {
        prim.dataSource = HdOverlayContainerDataSource::New(
            _DataSourceForcingBoundsDrawMode(),
            prim.dataSource);
    }

    return prim;
}

SdfPathVector
UsdImagingUnloadedDrawModeSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImagingUnloadedDrawModeSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
UsdImagingUnloadedDrawModeSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    // Loading/unloading prims forces a resync (prims removed
    // and added). So nothing to do here.

    _SendPrimsDirtied(entries);
}

void
UsdImagingUnloadedDrawModeSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
