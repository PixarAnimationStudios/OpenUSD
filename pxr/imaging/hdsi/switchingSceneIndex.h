//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_SWITCHING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_SWITCHING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hdsi/computeSceneIndexDiff.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdsiSwitchingSceneIndex;
TF_DECLARE_REF_PTRS(HdsiSwitchingSceneIndex);

/// This is a scene index that can switch between multiple inputs (which are
/// fixed at construction time).
///
/// By default, this scene index will use `HdsiComputeSceneIndexDelta` to
/// compute notices to send when the index is changed.  If you know more about
/// the input scenes, you can likely provide a more efficient/specialized one.
class HdsiSwitchingSceneIndex : public HdFilteringSceneIndexBase
{
public:
    using ComputeDiffFn = HdsiComputeSceneIndexDiff;

    static HdsiSwitchingSceneIndexRefPtr
    New(const std::vector<HdSceneIndexBaseRefPtr>& inputs,
        size_t initialIndex = 0,
        ComputeDiffFn computeDiffFn = HdsiComputeSceneIndexDiffDelta)
    {
        return TfCreateRefPtr(new HdsiSwitchingSceneIndex(
            inputs, initialIndex, std::move(computeDiffFn)));
    }

protected:
    HDSI_API
    HdsiSwitchingSceneIndex(
        const std::vector<HdSceneIndexBaseRefPtr>& inputs,
        size_t initialIndex,
        ComputeDiffFn computeDiffFn);

public:
    /// Returns the current index.
    HDSI_API
    size_t GetIndex() const;

    /// Sets the current index.
    ///
    /// Index must be in `[0, GetInputScenes().size())`.
    HDSI_API
    void SetIndex(size_t index);

public: // satisfying HdFilteringSceneIndexBase
    HDSI_API
    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const override;

public: // satisfying HdSceneIndexBase
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

private:
    void _UpdateCurrentSceneIndex(size_t index);

    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries);

    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries);

    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries);

    void _PrimsRenamed(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RenamedPrimEntries& entries);

    friend class _Observer;
    class _Observer final : public HdSceneIndexObserver
    {
    public:
        _Observer(HdsiSwitchingSceneIndex* owner)
            : _owner(owner)
        {
        }

    public: // satisfying HdSceneIndexObserver
        void PrimsAdded(
            const HdSceneIndexBase& sender,
            const AddedPrimEntries& entries) override;
        void PrimsRemoved(
            const HdSceneIndexBase& sender,
            const RemovedPrimEntries& entries) override;
        void PrimsDirtied(
            const HdSceneIndexBase& sender,
            const DirtiedPrimEntries& entries) override;
        void PrimsRenamed(
            const HdSceneIndexBase& sender,
            const RenamedPrimEntries& entries) override;

    private:
        HdsiSwitchingSceneIndex* _owner;
    };

    _Observer _observer;

    std::vector<HdSceneIndexBaseRefPtr> _inputs;
    size_t _index = 0;
    HdSceneIndexBaseRefPtr _currentSceneIndex;

    ComputeDiffFn _computeDiffFn;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
