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

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"

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
        HdSceneGlobalsSchemaTokens->activeRenderSettingsPrim };

    return names;
}

HdDataSourceBaseHandle
_SceneGlobalsDataSource::Get(const TfToken &name)
{
    if (name == HdSceneGlobalsSchemaTokens->activeRenderSettingsPrim) {
        SdfPath const &path = _si->_activeRenderSettingsPrimPath;
        // XXX Ideally, this should check if an opinion was set rather than use
        //     a non-empty path to imply that.
        if (!path.IsEmpty()) {
            return HdRetainedTypedSampledDataSource<SdfPath>::New(path);
        }
    }

    return nullptr;
}

// -----------------------------------------------------------------------------
// HdsiSceneGlobalsSceneIndex
// -----------------------------------------------------------------------------

/* static */
HdsiSceneGlobalsSceneIndexRefPtr
HdsiSceneGlobalsSceneIndex::New()
{
    return TfCreateRefPtr(new HdsiSceneGlobalsSceneIndex());
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
    // This scene index services only the root prim path.
    if (primPath == HdSceneGlobalsSchema::GetDefaultPrimPath()) {
        return { 
            TfToken() /*primType*/,
            HdRetainedContainerDataSource::New(
                HdSceneGlobalsSchemaTokens->sceneGlobals,
                _SceneGlobalsDataSource::New(this))};
    }

    return HdSceneIndexPrim();
}

SdfPathVector
HdsiSceneGlobalsSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return {};
}

HdsiSceneGlobalsSceneIndex::HdsiSceneGlobalsSceneIndex() = default;

PXR_NAMESPACE_CLOSE_SCOPE
