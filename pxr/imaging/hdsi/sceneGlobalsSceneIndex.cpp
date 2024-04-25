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
//

#include "pxr/imaging/hdsi/sceneGlobalsSceneIndex.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

// -----------------------------------------------------------------------------
// _SceneGlobalsDataSource
// -----------------------------------------------------------------------------
class _SceneGlobalsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SceneGlobalsDataSource);

    TfTokenVector
    GetNames() override;

    HdDataSourceBaseHandle
    Get(const TfToken &name) override;

private:
    _SceneGlobalsDataSource(HdsiSceneGlobalsSceneIndex const * const si)
    : _si(si) {}

    const HdsiSceneGlobalsSceneIndex * const  _si;
};

TfTokenVector
_SceneGlobalsDataSource::GetNames()
{
    static const TfTokenVector names = {
        HdSceneGlobalsSchemaTokens->activeRenderPassPrim,
        HdSceneGlobalsSchemaTokens->activeRenderSettingsPrim
    };

    return names;
}

HdDataSourceBaseHandle
_SceneGlobalsDataSource::Get(const TfToken &name)
{
    if (name == HdSceneGlobalsSchemaTokens->activeRenderPassPrim) {
        SdfPath const &path = _si->_activeRenderPassPrimPath;
        return HdRetainedTypedSampledDataSource<SdfPath>::New(path);
    }
    if (name == HdSceneGlobalsSchemaTokens->activeRenderSettingsPrim) {
        SdfPath const &path = _si->_activeRenderSettingsPrimPath;
        return HdRetainedTypedSampledDataSource<SdfPath>::New(path);
    }

    return nullptr;
}

// -----------------------------------------------------------------------------
// HdsiSceneGlobalsSceneIndex
// -----------------------------------------------------------------------------

/* static */
HdsiSceneGlobalsSceneIndexRefPtr
HdsiSceneGlobalsSceneIndex::New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(new HdsiSceneGlobalsSceneIndex(inputSceneIndex));
}

void
HdsiSceneGlobalsSceneIndex::SetActiveRenderPassPrimPath(
    const SdfPath &path)
{
    if (_activeRenderPassPrimPath == path) {
        return;
    }

    // A scene index downstream will invalidate and update the
    // sceneGlobals.activeRenderSettingsPrim locator (if the render pass points
    // to a valid render settings prim).
    // We keep things simple in this scene index.
    _activeRenderPassPrimPath = path;

    if (_IsObserved()) {
        _SendPrimsDirtied({{
            HdSceneGlobalsSchema::GetDefaultPrimPath(),
            HdSceneGlobalsSchema::GetActiveRenderPassPrimLocator()}});
    }
}

void
HdsiSceneGlobalsSceneIndex::SetActiveRenderSettingsPrimPath(
    const SdfPath &path)
{
    if (_activeRenderSettingsPrimPath == path) {
        return;
    }

    _activeRenderSettingsPrimPath = path;

    if (_IsObserved()) {
        _SendPrimsDirtied({{
            HdSceneGlobalsSchema::GetDefaultPrimPath(),
            HdSceneGlobalsSchema::GetActiveRenderSettingsPrimLocator()}});
    }
}


HdSceneIndexPrim
HdsiSceneGlobalsSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Overlay a data source at the scene globals locator for the default prim.
    if (primPath == HdSceneGlobalsSchema::GetDefaultPrimPath()) {
        HdContainerDataSourceHandle sceneGlobalsContainerDS =
            HdRetainedContainerDataSource::New(
                HdSceneGlobalsSchemaTokens->sceneGlobals,
                _SceneGlobalsDataSource::New(this));

        if (prim.dataSource) {
            prim.dataSource = HdOverlayContainerDataSource::New(
                sceneGlobalsContainerDS, prim.dataSource);
        } else {
            prim.dataSource = sceneGlobalsContainerDS;
        }
    }

    return prim;
}

SdfPathVector
HdsiSceneGlobalsSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

HdsiSceneGlobalsSceneIndex::HdsiSceneGlobalsSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{}

void
HdsiSceneGlobalsSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
HdsiSceneGlobalsSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    // XXX Since this is now a filtering scene index, handle removals of
    //     the active render settings prim.
    _SendPrimsRemoved(entries);
}

void
HdsiSceneGlobalsSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

// XXX Handle renames by sending a dirty notice that the active render settings
//     prim has changed.

PXR_NAMESPACE_CLOSE_SCOPE
