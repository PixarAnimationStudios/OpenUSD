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
#include "pxr/usdImaging/usdImaging/piPrototypeSceneIndex.h"

#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

bool
_ContainsStrictPrefixOfPath(const SdfPathSet &pathSet,
                            const SdfPath &path)
{
    const auto it = std::lower_bound(
        pathSet.crbegin(), pathSet.crend(),
        path,
        [](const SdfPath &a, const SdfPath &b) {
            return a > b;});
    return it != pathSet.crend() && path.HasPrefix(*it) && path != *it;
}

HdContainerDataSourceHandle
_ComputeUnderlaySource(const SdfPath &instancer, const SdfPath &prototypeRoot)
{
    if (instancer.IsEmpty()) {
        return nullptr;
    }

    using DataSource = HdRetainedTypedSampledDataSource<VtArray<SdfPath>>;

    return
        HdRetainedContainerDataSource::New(
            HdInstancedBySchemaTokens->instancedBy,
            HdInstancedBySchema::Builder()
                .SetPaths(DataSource::New({ instancer }))
                .SetPrototypeRoots(DataSource::New({ prototypeRoot }))
                .Build()); 
}

HdContainerDataSourceHandle
_ComputePrototypeRootOverlaySource(const SdfPath &instancer)
{
    if (instancer.IsEmpty()) {
        return nullptr;
    }
    
    return
        HdRetainedContainerDataSource::New(
            HdXformSchemaTokens->xform,
            HdXformSchema::Builder()
                .SetResetXformStack(
                    HdRetainedTypedSampledDataSource<bool>::New(
                        true))
            .Build());
}

bool
_IsOver(const HdSceneIndexPrim &prim)
{
    UsdImagingUsdPrimInfoSchema schema =
        UsdImagingUsdPrimInfoSchema::GetFromParent(prim.dataSource);
    HdTokenDataSourceHandle const ds = schema.GetSpecifier();
    if (!ds) {
        return false;
    }
    return ds->GetTypedValue(0.0f) == UsdImagingUsdPrimInfoSchemaTokens->over;
}

}

UsdImaging_PiPrototypeSceneIndexRefPtr
UsdImaging_PiPrototypeSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const SdfPath &instancer,
    const SdfPath &prototypeRoot)
{
    return TfCreateRefPtr(
        new UsdImaging_PiPrototypeSceneIndex(
            inputSceneIndex, instancer, prototypeRoot));
}

UsdImaging_PiPrototypeSceneIndex::
UsdImaging_PiPrototypeSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const SdfPath &instancer,
    const SdfPath &prototypeRoot)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _prototypeRoot(prototypeRoot)
  , _underlaySource(
      _ComputeUnderlaySource(instancer, prototypeRoot))
  , _prototypeRootOverlaySource(
      _ComputePrototypeRootOverlaySource(instancer))
{
    _Populate();
}

void
UsdImaging_PiPrototypeSceneIndex::_Populate()
{
    HdSceneIndexPrimView view(_GetInputSceneIndex(), _prototypeRoot);
    for (auto it = view.begin(); it != view.end(); ++it) {
        const SdfPath &path = *it;
        
        HdSceneIndexPrim const prim = _GetInputSceneIndex()->GetPrim(path);
        if (prim.primType == HdPrimTypeTokens->instancer ||
            _IsOver(prim)) {
            _instancersAndOvers.insert(path);
            
            it.SkipDescendants();
        }
    }
}

static
void
_MakeUnrenderable(HdSceneIndexPrim * const prim)
{
    // Force the prim type to empty.
    prim->primType = TfToken();

    if (!prim->dataSource) {
        return;
    }

    // Note that native USD instances are still picked up by the
    // native instance scene indices even when the prim type is empty.
    //
    // We explicitly block the data source indicating a USD instanced.
    //
    // This, unfortuantely, means that a point instancing scene index
    // needs to know about a native instancing token.
    //
    static HdContainerDataSourceHandle const overlaySource =
        HdRetainedContainerDataSource::New(
            UsdImagingUsdPrimInfoSchemaTokens->__usdPrimInfo,
            HdRetainedContainerDataSource::New(
                UsdImagingUsdPrimInfoSchemaTokens->niPrototypePath,
                HdBlockDataSource::New()));
    prim->dataSource = HdOverlayContainerDataSource::New(
        overlaySource,
        prim->dataSource);
}

HdSceneIndexPrim
UsdImaging_PiPrototypeSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (!primPath.HasPrefix(_prototypeRoot)) {
        return prim;
    }

    if (_ContainsStrictPrefixOfPath(_instancersAndOvers, primPath)) {
        // Render all prims under an instancer or over invisible.
        _MakeUnrenderable(&prim);
        return prim;
    }

    if (!prim.dataSource) {
        return prim;
    }

    if (_underlaySource) {
        prim.dataSource = HdOverlayContainerDataSource::New(
            prim.dataSource,
            _underlaySource);
    }

    if (_prototypeRootOverlaySource) {
        if (primPath == _prototypeRoot) {
            prim.dataSource = HdOverlayContainerDataSource::New(
                _prototypeRootOverlaySource,
                prim.dataSource);
        }
    }
    
    return prim;
}

SdfPathVector
UsdImaging_PiPrototypeSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImaging_PiPrototypeSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    HdSceneIndexObserver::AddedPrimEntries newEntries;

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;
        if (_ContainsStrictPrefixOfPath(_instancersAndOvers, path)) {
            newEntries.emplace_back(path, TfToken());
            continue;
        }

        if (entry.primType == HdPrimTypeTokens->instancer ||
            _IsOver(_GetInputSceneIndex()->GetPrim(path))) {
            _instancersAndOvers.insert(path);
        }

        // Note that we do not handle the case that the type of a prim
        // changes and we get a single AddedPrimEntry about it.
        //
        // E.g. if a prim becomes an instancer, we need to re-sync
        // its namespace descendants since their type change to empty.
        // Similarly, if a prim was an instancer.

        newEntries.push_back(entry);
    }

    _SendPrimsAdded(newEntries);
}

void
UsdImaging_PiPrototypeSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

void
UsdImaging_PiPrototypeSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        auto it = _instancersAndOvers.lower_bound(entry.primPath);
        while (it != _instancersAndOvers.end() &&
               it->HasPrefix(entry.primPath)) {
            it = _instancersAndOvers.erase(it);
        }
    }

    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
