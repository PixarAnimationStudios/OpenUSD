//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_PREFIXING_SCENE_INDEX_H
#define PXR_IMAGING_HD_PREFIXING_SCENE_INDEX_H

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrefixingSceneIndex;
TF_DECLARE_REF_PTRS(HdPrefixingSceneIndex);

///
/// \class HdPrefixingSceneIndex
///
/// A prefixing scene index is one in which the input scene contains 
/// data sources whose paths are all prefixed with a given prefix.
///
class HdPrefixingSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:

    /// Creates a new prefixing scene index.
    ///
    static HdPrefixingSceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene, const SdfPath &prefix) 
    {
        return TfCreateRefPtr(new HdPrefixingSceneIndex(inputScene, prefix));
    }

    // satisfying HdSceneIndexBase
    HD_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HD_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:

    HD_API
    HdPrefixingSceneIndex(const HdSceneIndexBaseRefPtr &inputScene,
        const SdfPath &prefix);

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

private:

    SdfPath _AddPathPrefix(const SdfPath &primPath) const;

    SdfPath _RemovePathPrefix(const SdfPath &primPath) const;

    const SdfPath _prefix;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

