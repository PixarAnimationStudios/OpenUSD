//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_DEBUGGING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_DEBUGGING_SCENE_INDEX_H

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/pxr.h"

#include <optional>
#include <map>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiDebuggingSceneIndex);

namespace HdsiDebuggingSceneIndex_Impl
{

struct _PrimInfo;
using _PrimInfoSharedPtr = std::shared_ptr<_PrimInfo>;
using _PrimMap = std::map<SdfPath, _PrimInfo>;

}

/// \class HdsiDebuggingSceneIndex
///
/// A filtering scene index that checks for certain inconsistencies (without
/// transforming the scene) in its input scene.
/// For example, it will report if the input scene's GetPrim(/foo) returns a
/// prim type different from a previous call to GetPrim(/foo) even though the
/// input scene sent no related prims added or removed notice.
///
/// The easiest way to invoke this scene index is by setting the env var
/// HDSI_DEBUGGING_SCENE_INDEX_INSERTION_PHASE. Also see
/// HdsiDebuggingSceneIndexPlugin.
///
class HdsiDebuggingSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiDebuggingSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);

    HDSI_API
    ~HdsiDebuggingSceneIndex() override;

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
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

    HDSI_API
    void _PrimsRenamed(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RenamedPrimEntries &entries) override;

private:
    HdsiDebuggingSceneIndex(HdSceneIndexBaseRefPtr const &inputSceneIndex,
                            HdContainerDataSourceHandle const &inputArgs);

    mutable std::mutex _primsMutex;
    mutable HdsiDebuggingSceneIndex_Impl::_PrimMap _prims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_DEBUGGING_SCENE_INDEX_H
