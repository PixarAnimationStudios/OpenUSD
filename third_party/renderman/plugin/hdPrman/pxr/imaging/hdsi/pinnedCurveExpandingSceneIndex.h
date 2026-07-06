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
//     http://www.apache.org/licenses/LICEN SE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_IMAGING_HDSI_PINNED_CURVE_EXPANDING_SCENE_INDEX_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_IMAGING_HDSI_PINNED_CURVE_EXPANDING_SCENE_INDEX_H

// XXX: Delete this file after hdPrman drops support for USD versions
// older than 22.11.

#include "pxr/pxr.h" // PXR_VERSION
#if PXR_VERSION >= 2211
#include <pxr/imaging/hdsi/pinnedCurveExpandingSceneIndex.h> // IWYU pragma: export
#else

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
    static HdsiPinnedCurveExpandingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override final;

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override final;

protected:
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override final;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override final;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override final;

    HdsiPinnedCurveExpandingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2211

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_IMAGING_HDSI_PINNED_CURVE_EXPANDING_SCENE_INDEX_H
