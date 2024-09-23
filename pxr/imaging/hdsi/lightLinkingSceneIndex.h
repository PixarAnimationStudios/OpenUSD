//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_LIGHT_LINKING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_LIGHT_LINKING_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdsi/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// See documentation below for the use of these tokens.
///
#define HDSI_LIGHT_LINKING_SCENE_INDEX_TOKENS   \
    (lightPrimTypes)                            \
    (lightFilterPrimTypes)                      \
    (geometryPrimTypes)

TF_DECLARE_PUBLIC_TOKENS(HdsiLightLinkingSceneIndexTokens, HDSI_API,
    HDSI_LIGHT_LINKING_SCENE_INDEX_TOKENS);

namespace HdsiLightLinkingSceneIndex_Impl
{
    struct _Cache;
    using _CacheSharedPtr = std::shared_ptr<_Cache>;
}

TF_DECLARE_REF_PTRS(HdsiLightLinkingSceneIndex);

///
/// \class HdsiLightLinkingSceneIndex
///
/// Scene index that implements light linking semantics by:
/// - discovering light linking collections on lights and light filters; this
///   may be configured using the \p inputArgs c'tor argument by providing a
///   HdTokenArrayDataSourceHandle for \p lightPrimTypes and
///   \p lightFilterPrimTypes.
///
/// - assigning a category ID token to each unique collection based on
///   its membership expression; in PRMan parlance, this is the value fed to
///   the "grouping:membership" attribute on the light/light filter.
///   Trivial collections that include all prims in the scene use the empty
///   token.
///
/// - invalidating the categories locator on prims targeted (i.e. matched) by
///   the expression,
///
/// - invalidating the light/light filter prim when the category ID for its
///   linking collection has changed, and
///
/// - computing the categories that a (geometry) prim belong to; the list of
///   prim types affected by linking may be configured using the \p inputArgs
///   c'tor argument by providing a HdTokenArrayDataSourceHandle for
///   \p geometryPrimTypes.
///
/// \note Current support for instancing is limited to linking non-nested
///       instance prims and non-nested point instancer prims.
//        Linking to instance proxy prims, nested instances and
///       nested point instancers is not yet supported.
///
/// \note For legacy scene delegates that implement light linking (e.g.
///       UsdImagingDelegate) and don't transport the light linking collections,
///       this scene index should leave the category(ies) unaffected on the
///       light, geometry prims and instancers.
///
class HdsiLightLinkingSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdSceneIndexBaseRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:

    HDSI_API
    HdsiLightLinkingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

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

private:
    void _ProcessAddedLightOrFilter(
        const HdSceneIndexObserver::AddedPrimEntry &entry,
        const TfTokenVector &collectionNames,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries);

    bool _IsLight(const TfToken &primType) const;
    bool _IsLightFilter(const TfToken &primType) const;
    bool _IsGeometry(const TfToken &primType) const;

private:
    HdsiLightLinkingSceneIndex_Impl::_CacheSharedPtr const _cache;

    // Track prims with light linking collections.
    SdfPathSet _lightAndFilterPrimPaths;

    const VtArray<TfToken> _lightPrimTypes;
    const VtArray<TfToken> _lightFilterPrimTypes;
    const VtArray<TfToken> _geometryPrimTypes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
