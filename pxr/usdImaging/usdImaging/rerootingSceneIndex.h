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

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImagingRerootingSceneIndex);

/// \class UsdImagingRerootingSceneIndex
///
/// Drops all prims not under srcPrefix and moves those under srcPrefix to
/// dstPrefix.
///
/// Data sources containing paths will be updated accordingly. That is, if it
/// contains a path with srcPrefix as prefix, the prefix will be replaced by
/// dstPrefix.
///
/// Note that this can be used as prefixing scene index by setting srcPrefix
/// to the root path. It can also use to isolate part of the namespace by
/// setting the srcPrefix and dstPrefix to be equal.
///
class UsdImagingRerootingSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static UsdImagingRerootingSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputScene,
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix)
    {
        return TfCreateRefPtr(
            new UsdImagingRerootingSceneIndex(
                inputScene, srcPrefix, dstPrefix));
    }

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    USDIMAGING_API
    UsdImagingRerootingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene,
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix);

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
    SdfPath _SrcPathToDstPath(const SdfPath &primPath) const;
    SdfPath _DstPathToSrcPath(const SdfPath &primPath) const;

    const SdfPath _srcPrefix;
    const SdfPath _dstPrefix;
    // Prefixes of _dstPrefix
    const SdfPathVector _dstPrefixes;
    // Is _srcPrefix equal to _dstPrefix?
    const bool _srcEqualsDst;
    // Is _srcPrefix == / ?
    const bool _srcPrefixIsRoot;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
