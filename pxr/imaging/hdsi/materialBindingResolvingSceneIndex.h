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

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdsiMaterialBindingResolvingSceneIndex);

/// Scene Index that resolves materialBindings that have multiple purposes into
/// a single purpose.  The first binding encountered in \p purposePriorityOrder
/// will be provided as \p dstPurpose.
class HdsiMaterialBindingResolvingSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiMaterialBindingResolvingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const TfTokenVector& purposePriorityOrder,
        const TfToken& dstPurpose);

public: // HdSceneIndex overrides
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

protected: // HdSingleInputFilteringSceneIndexBase overrides
    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

protected:
    HdsiMaterialBindingResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const TfTokenVector& purposePriorityOrder,
        const TfToken& dstPurpose);
    ~HdsiMaterialBindingResolvingSceneIndex() override;

private:
    TfTokenVector _purposePriorityOrder;
    TfToken _dstPurpose;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
