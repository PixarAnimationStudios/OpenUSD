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

TF_DEFINE_PUBLIC_TOKENS(
    UsdImaging_NiPrototypeSceneIndexTokens,
    USDIMAGING_NI_PROTOTYPE_SCENE_INDEX_TOKENS);

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

HdDataSourceBaseHandle
_ResetXformToIdentityDataSource()
{
    return
        HdXformSchema::Builder()
            .SetMatrix(
                HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                    GfMatrix4d(1.0)))
            .SetResetXformStack(
                HdRetainedTypedSampledDataSource<bool>::New(
                    true))
            .Build();
}

HdContainerDataSourceHandle
_UnderlaySource()
{
    return
        HdRetainedContainerDataSource::New(
            HdInstancedBySchema::GetSchemaToken(),
            UsdImaging_NiPrototypeSceneIndex::GetInstancedByDataSource());
}

HdContainerDataSourceHandle
_PrototypeRootOverlaySource()
{
    return
        HdRetainedContainerDataSource::New(
            HdInstancedBySchema::GetSchemaToken(),
            UsdImaging_NiPrototypeSceneIndex::GetInstancedByDataSource(),
            // The prototypes should always be defined at the origin.
            HdXformSchema::GetSchemaToken(),
            _ResetXformToIdentityDataSource());
}

}

UsdImaging_NiPrototypeSceneIndexRefPtr
UsdImaging_NiPrototypeSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const bool forPrototype)
{
    return TfCreateRefPtr(
        new UsdImaging_NiPrototypeSceneIndex(
            inputSceneIndex, forPrototype));
}

UsdImaging_NiPrototypeSceneIndex::
UsdImaging_NiPrototypeSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const bool forPrototype)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _forPrototype(forPrototype)
{
}

/* static */
const SdfPath &
UsdImaging_NiPrototypeSceneIndex::GetInstancerPath()
{
    static const SdfPath path =
        SdfPath::AbsoluteRootPath()
            .AppendChild(
                UsdImaging_NiPrototypeSceneIndexTokens->instancer);
    return path;
}

/* static */
const SdfPath &
UsdImaging_NiPrototypeSceneIndex::GetPrototypePath()
{
    static const SdfPath path =
        GetInstancerPath()
            .AppendChild(
                UsdImaging_NiPrototypeSceneIndexTokens->prototype);
    return path;
}

/* static */
const HdDataSourceBaseHandle &
UsdImaging_NiPrototypeSceneIndex::GetInstancedByDataSource()
{
    using DataSource = HdRetainedTypedSampledDataSource<VtArray<SdfPath>>;

    static const HdDataSourceBaseHandle &ds =
        HdInstancedBySchema::Builder()
            .SetPaths(
                DataSource::New(
                    { UsdImaging_NiPrototypeSceneIndex::GetInstancerPath() }))
            .SetPrototypeRoots(
                DataSource::New(
                    { UsdImaging_NiPrototypeSceneIndex::GetPrototypePath() }))
            .Build();

    return ds;
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

    if (!_forPrototype) {
        return prim;
    }

    if (!primPath.HasPrefix(GetPrototypePath())) {
        return prim;
    }

    static const size_t n = GetPrototypePath().GetPathElementCount();

    if (primPath.GetPathElementCount() == n) {
        // primPath is /UsdNiInstancer/UsdNiPrototype

        static const HdContainerDataSourceHandle prototypeRootOverlaySource =
            _PrototypeRootOverlaySource();

        prim.dataSource = HdOverlayContainerDataSource::New(
            prototypeRootOverlaySource,
            prim.dataSource);
    } else {
        // primPath is an ancestor of /UsdNiInstancer/UsdNiPrototype

        static const HdContainerDataSourceHandle underlaySource =
            _UnderlaySource();

        prim.dataSource = HdOverlayContainerDataSource::New(
            prim.dataSource,
            underlaySource);
    }

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
