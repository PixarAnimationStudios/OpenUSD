//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_PINNED_CURVE_EXPANDING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_PINNED_CURVE_EXPANDING_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiPinnedCurveExpandingSceneIndex);

/// \class HdsiPinnedCurveExpandingSceneIndex
///
/// Pinned curves are a special case of non-periodic cubic curves (relevant only
/// for BSpline and CatmullRom basis) where the authored intent is for each
/// curve to begin and end at its first and last control points respectively.
/// This is done by setting the 'wrap' mode on the topology to 'pinned'.
///
/// Renderers that don't provide built-in support for pinned curves can use this
/// scene index to "expand" or "unpack" the authored data. This involves the
/// following:
/// - Adding "phantom" points at the ends of each curve. For BSpline basis, each
///   end point is repeated twice (so we have 3 consecutive identical points on
///   each end) while for CatmullRom basis, each end point is repeated once.
///   The topology is modified to reflect this.
///
/// - Expanding vertex primvars to account for the additional control points and
///   varying primvars to account for the additional segments.
///
/// \note This scene index does not convert indexed curves (i.e., with authored
///       curve indices) into non-indexed curves.
///
class HdsiPinnedCurveExpandingSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiPinnedCurveExpandingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override final;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override final;

protected:
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override final;

    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override final;

    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override final;

    HDSI_API
    HdsiPinnedCurveExpandingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_PINNED_CURVE_EXPANDING_SCENE_INDEX_H
