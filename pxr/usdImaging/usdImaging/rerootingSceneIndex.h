//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_REROOTING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_REROOTING_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/sdf/pathTable.h"
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImagingRerootingSceneIndex);

/// \class UsdImagingRerootingSceneIndex
///
/// Applies a bijective namespace mapping, as represented by a PcpMapFunction,
/// to the input namespace.
///
/// Data sources containing paths will be updated accordingly. That is, 
/// the namespace mapping function will be applied to path-valued data.
///
/// Note that this can be used as prefixing scene index by mapping a
/// path to the root path.  It can also use to isolate part of the namespace
/// by using a namespace mapping that maps a specific path to itself but
/// does not include a root identity mapping.  See PcpMapFunction for more.
/// The SdfLayerOffset component of the path mapping is not applied.
///
class UsdImagingRerootingSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// A convenience method that maps between two provided paths.
    static UsdImagingRerootingSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputScene,
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix)
    {
        const PcpMapFunction mapFn =
            PcpMapFunction::Create(
                {{srcPrefix, dstPrefix}}, SdfLayerOffset());
        return TfCreateRefPtr(
            new UsdImagingRerootingSceneIndex(inputScene, mapFn));
    }

    static UsdImagingRerootingSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputScene,
        PcpMapFunction const& mapFn)
    {
        return TfCreateRefPtr(
            new UsdImagingRerootingSceneIndex(inputScene, mapFn));
    }

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    USDIMAGING_API
    UsdImagingRerootingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene,
        PcpMapFunction const& mapFn);

    USDIMAGING_API
    ~UsdImagingRerootingSceneIndex() override;

    // satisfying HdSingleInputFilteringSceneIndexBase
    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

private:
    const PcpMapFunction _mapFn;
    SdfPathTable<std::monostate> _targetRootPathsTable;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
