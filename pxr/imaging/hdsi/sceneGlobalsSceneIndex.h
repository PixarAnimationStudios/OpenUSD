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
//     http://www.apache.org/licenses/LICEN SE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_IMAGING_HDSI_SCENE_GLOBALS_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_SCENE_GLOBALS_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiSceneGlobalsSceneIndex);

// undefined -> 2 : Make SGSI a filtering scene index.
#define HDSI_SGSI_API_VERSION 2

/// \class HdsiSceneGlobalsSceneIndex
///
/// Scene index that populates a "sceneGlobals" data source as modeled
/// by HdSceneGlobalsSchema and provides public API to mutate it.
///
class HdsiSceneGlobalsSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiSceneGlobalsSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    // ------------------------------------------------------------------------
    // Public (non-virtual) API
    // ------------------------------------------------------------------------
    
    /// Caches the provided path and notifies any observers when the active 
    /// render settings prim path is modified.
    ///
    HDSI_API
    void SetActiveRenderSettingsPrimPath(const SdfPath &);

    // ------------------------------------------------------------------------
    // Satisfying HdSceneIndexBase
    // ------------------------------------------------------------------------
    HDSI_API
    HdSceneIndexPrim
    GetPrim(const SdfPath &primPath) const override final;

    HDSI_API
    SdfPathVector
    GetChildPrimPaths(const SdfPath &primPath) const override final;

protected:
    HDSI_API
    HdsiSceneGlobalsSceneIndex(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    // ------------------------------------------------------------------------
    // Satisfying HdSingleInputFilteringSceneIndexBase
    // ------------------------------------------------------------------------
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    friend class _SceneGlobalsDataSource;

    SdfPath _activeRenderSettingsPrimPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_SCENE_GLOBALS_SCENE_INDEX_H
