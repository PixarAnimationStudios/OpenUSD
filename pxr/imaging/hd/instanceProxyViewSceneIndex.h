//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_INSTANCE_PROXY_VIEW_SCENE_INDEX_H
#define PXR_IMAGING_HD_INSTANCE_PROXY_VIEW_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdInstanceProxyViewSceneIndex);

///
/// \class HdInstanceProxyViewSceneIndex
///
/// A scene index that provides a topological view of the scene as though
/// instancing were not being used.
///
/// Specifically, this scene index "unrolls" leaf instance prims (i.e. prims
/// with an `instance` data source that aren't under an instancer prim) by
/// providing "instance proxy" prims as descendants. These instance proxy prims 
/// mirror the structure of the corresponding instance prim's prototype subtree.
/// Each instance proxy prim mirrors the corresponding prototype's prim type
/// and prim container with the addition of an `instanceProxy` data source that
/// provides the path to the mirrored prototype prim.
///
/// The resulting scene index view is useful for path expression-based
/// collection membership evaluation, and for UI tools like the Hydra Scene
/// Debugger.
///
/// \note This scene index does not currently send *any* notices for instance
/// proxy prims. It is not intended for use in a scene index pipeline chain.
///
/// \note This scene index unrolls implicit instancing (e.g. USD native
/// instancing). It does not unroll explicit instancing (e.g. point instancers).
///
class HdInstanceProxyViewSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    HD_API
    static HdInstanceProxyViewSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    // ------------------------------------------------------------------------
    // Non-virtual API.
    //
    /// Returns true if the given prim path is an instance proxy prim path
    /// matching the USD definition of an instance proxy prim.
    /// 
    HD_API
    bool IsInstanceProxy(const SdfPath &primPath) const;

    /// Returns true if the given prim path is a leaf instance prim path that
    /// isn't under an instancer prim.
    /// In USD parlance, this is the outermost UsdPrim for which
    /// prim.IsInstance() is true and prim.IsInstanceProxy() is false.
    HD_API
    bool IsOutermostInstance(const SdfPath &primPath) const;

    // ------------------------------------------------------------------------
    // HdSceneIndex overrides.
    //
    /// Adds support for instance proxy prim paths. Forwards other queries
    /// to the input scene index.
    HD_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    /// Adds support for queries on outer (leaf) instance prim paths and
    /// instance proxy prim paths. Forwards other queries to the input scene
    /// index.
    HD_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:

    HD_API
    HdInstanceProxyViewSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

    // ------------------------------------------------------------------------
    // HdSingleInputFilteringSceneIndexBase overrides.
    //
    HD_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    HD_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    HD_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    struct _Impl;
    std::unique_ptr<_Impl> _impl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
