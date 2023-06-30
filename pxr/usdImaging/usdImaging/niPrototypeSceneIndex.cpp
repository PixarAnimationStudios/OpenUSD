//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdImaging/niPrototypeSceneIndex.h"

#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
 
bool
_IsUsdInstance(HdContainerDataSourceHandle const &primSource)
{
    UsdImagingUsdPrimInfoSchema schema =
        UsdImagingUsdPrimInfoSchema::GetFromParent(primSource);
    HdPathDataSourceHandle const pathDs = schema.GetNiPrototypePath();
    if (!pathDs) {
        return false;
    }
    const SdfPath usdPrototypePath = pathDs->GetTypedValue(0.0f);
    return !usdPrototypePath.IsEmpty();
}

HdContainerDataSourceHandle
_ResetXformToIdentityDataSource()
{
    static HdMatrixDataSourceHandle const identity
        = HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
            GfMatrix4d(1.0));
    return HdXformSchema::Builder()
        .SetMatrix(identity)
        .SetResetXformStack(HdRetainedTypedSampledDataSource<bool>::New(true))
        .Build();
}

HdContainerDataSourceHandle
_ComputeUnderlaySource(const SdfPath &prototypeRoot)
{
    if (prototypeRoot.IsEmpty()) {
        return nullptr;
    }

    using DataSource = HdRetainedTypedSampledDataSource<VtArray<SdfPath>>;

    return
        HdRetainedContainerDataSource::New(
            HdInstancedBySchemaTokens->instancedBy,
            HdInstancedBySchema::Builder()
                .SetPaths(DataSource::New({ SdfPath::AbsoluteRootPath() }))
                .SetPrototypeRoots(DataSource::New({ prototypeRoot }))
                .Build(),

            // The prototypes should always be defined at the origin.
            HdXformSchemaTokens->xform,
            _ResetXformToIdentityDataSource());
}

}

UsdImaging_NiPrototypeSceneIndexRefPtr
UsdImaging_NiPrototypeSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const SdfPath &prototypeRoot)
{
    return TfCreateRefPtr(
        new UsdImaging_NiPrototypeSceneIndex(
            inputSceneIndex, prototypeRoot));
}

UsdImaging_NiPrototypeSceneIndex::
UsdImaging_NiPrototypeSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const SdfPath &prototypeRoot)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _underlaySource(_ComputeUnderlaySource(prototypeRoot))
{
}

HdSceneIndexPrim
UsdImaging_NiPrototypeSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (!prim.dataSource) {
        return prim;
    }

    if (_IsUsdInstance(prim.dataSource)) {
        prim.primType = TfToken();
        return prim;
    }

    if (primPath.IsAbsoluteRootPath()) {
        return prim;
    }

    if (!_underlaySource) {
        return prim;
    }

    prim.dataSource = HdOverlayContainerDataSource::New(
        prim.dataSource,
        _underlaySource);

    return prim;
}

SdfPathVector
UsdImaging_NiPrototypeSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImaging_NiPrototypeSceneIndex::_PrimsAdded(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
UsdImaging_NiPrototypeSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

void
UsdImaging_NiPrototypeSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
