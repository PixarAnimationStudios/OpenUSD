//
// Copyright 2023 Pixar
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

#include "pxr/usdImaging/usdImaging/unloadedDrawModeSceneIndex.h"

#include "pxr/usdImaging/usdImaging/modelSchema.h"
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
            UsdImagingModelSchema::GetSchemaToken(),
            UsdImagingModelSchema::Builder()
                .SetApplyDrawMode(
                    HdRetainedTypedSampledDataSource<bool>::New(true))
                .SetDrawMode(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        UsdImagingModelSchemaTokens->bounds))
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
