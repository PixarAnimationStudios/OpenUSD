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
#ifndef PXR_IMAGING_HD_NOTICE_BATCHING_SCENE_INDEX_H
#define PXR_IMAGING_HD_NOTICE_BATCHING_SCENE_INDEX_H

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdNoticeBatchingSceneIndex;
TF_DECLARE_REF_PTRS(HdNoticeBatchingSceneIndex);

class HdNoticeBatchingSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// Creates a new notice batching scene index.
    ///
    static HdNoticeBatchingSceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene) {
        return TfCreateRefPtr(new HdNoticeBatchingSceneIndex(inputScene));
    }

    HD_API
    ~HdNoticeBatchingSceneIndex() override;

    // satisfying HdSceneIndexBase
    HD_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HD_API 
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    bool IsBatchingEnabled() const;

    /// Once batching is enabled, observed notices are queued in contiguious
    /// blocks by notice type. Disabling batching immediately forwards and
    /// flushes any queued batches. Batching state is not currently tracked in
    /// a nested manner.
    void SetBatchingEnabled(bool enabled);
    
    /// Forwards any queued notices accumuated while batching state is enabled.
    /// This does not itself disable batching.
    void Flush();

protected:

    HD_API
    HdNoticeBatchingSceneIndex(const HdSceneIndexBaseRefPtr &inputScene);

    // satisfying HdSingleInputFilteringSceneIndexBase
    void _PrimsAdded(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    struct _BatchEntry
    {
        virtual ~_BatchEntry();
    };

    struct _PrimsAddedBatchEntry : public _BatchEntry
    {
        HdSceneIndexObserver::AddedPrimEntries entries;
    };

    struct _PrimsRemovedBatchEntry : public _BatchEntry
    {
        HdSceneIndexObserver::RemovedPrimEntries entries;
    };

    struct _PrimsDirtiedBatchEntry : public _BatchEntry
    {
        HdSceneIndexObserver::DirtiedPrimEntries entries;
    };

    bool _batchingEnabled;
    std::vector<std::unique_ptr<_BatchEntry>> _batches;

};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
