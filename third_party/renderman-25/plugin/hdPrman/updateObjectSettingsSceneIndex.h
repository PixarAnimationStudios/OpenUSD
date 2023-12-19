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
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_UPDATE_OBJECT_SETTINGS_SCENE_INDEX_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_UPDATE_OBJECT_SETTINGS_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdPrman_UpdateObjectSettingsSceneIndex);

/// This scene index is similar to PRManUpdateObjectSettingsOp in
/// RenderMan-for-Katana: it migrates object settings to track
/// changes in conventions between RenderMan releases.
///
class HdPrman_UpdateObjectSettingsSceneIndex : 
    public  HdSingleInputFilteringSceneIndexBase
{
public:
    static HdPrman_UpdateObjectSettingsSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HdPrman_UpdateObjectSettingsSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    ~HdPrman_UpdateObjectSettingsSceneIndex();

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_UPDATE_OBJECT_SETTINGS_SCENE_INDEX_H
