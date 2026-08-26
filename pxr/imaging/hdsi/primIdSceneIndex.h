//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_PRIM_ID_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_PRIM_ID_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiPrimIdSceneIndex_Impl
{
class _PrimIdInfo;
using _PrimIdInfoSharedPtr = std::shared_ptr<_PrimIdInfo>;
}

TF_DECLARE_REF_PTRS(HdsiPrimIdSceneIndex);

///
/// \class HdsiPrimIdSceneIndex
///
/// A filtering scene index assigning an id to each imagable prim.
///
/// It populates the HdPrimId schema and the primIdToPath map of the
/// HdSceneGlobalsSchema.
///
/// Note that the id's might not be consecutive. When a prim is removed, its
/// id is freed and later reused for a new prim. The prim id of a prim does
/// not change unless the prim is removed and re-added or re-added with a
/// different prim type.
///
/// It replaces the prim id's that were assigned in the HdRenderIndex.
///
class HdsiPrimIdSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// Creates a new prim id scene index.
    ///
    static HdsiPrimIdSceneIndexRefPtr New(
            HdSceneIndexBaseRefPtr const &inputScene) {
        return TfCreateRefPtr(
            new HdsiPrimIdSceneIndex(inputScene));
    }

    HDSI_API
    ~HdsiPrimIdSceneIndex() override;

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HDSI_API
    HdsiPrimIdSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene);

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
    // State for this scene index.
    HdsiPrimIdSceneIndex_Impl::_PrimIdInfoSharedPtr const _primIdInfo;

    // Prim-level container ({ sceneGlobals : { primId : <reverse table> } })
    // overlaid on the root prim.
    HdContainerDataSourceHandle const _rootPrimOverlayDataSource;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
