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
#ifndef PXR_IMAGING_HDSI_MATERIAL_BINDING_RESOLVING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_MATERIAL_BINDING_RESOLVING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/usd/sdf/pathTable.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiMaterialPruningSceneIndex);

/// Scene Index that prunes materials and material bindings.
class HdsiMaterialPruningSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiMaterialPruningSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HDSI_API
    bool GetSceneMaterialsEnabled() const;
    HDSI_API
    void SetSceneMaterialsEnabled(bool);

public: // HdSceneIndex overrides
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

protected: // HdSingleInputFilteringSceneIndexBase overrides
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;
    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;
    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

protected:
    HDSI_API
    HdsiMaterialPruningSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    HDSI_API
    ~HdsiMaterialPruningSceneIndex() override;

private:
    // Track pruned materials in a SdfPathTable.  A value of true
    // indicates a material was filtered at that path.
    using _PruneMap = SdfPathTable<bool>;
    _PruneMap _pruneMap;

    bool _materialsEnabled;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
