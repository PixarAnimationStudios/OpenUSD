//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_XFORM_RESOLVER_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_XFORM_RESOLVER_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/dataSourceLocator.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdSceneIndexBase);

/// \class UsdSkelImagingDataSourceXformResolver
///
/// Given a prim, computes transform from prim local space to a space common to
/// all descendants of a skel root.
///
/// Without instancing, the common space is simply world. But if the
/// skel root is inside a ni or pi prototype, the common space is that
/// of the prototype.
///
/// A particular difficulty is presented by the following situation:
/// Consider a prim inside a reference which is inside a skel root.
/// Now author instancable on the reference. This change the topology
/// after the ni prototype propagating scene index and the prim now
/// has an instancer and the xform authored on the reference is now
/// in the instancer's instanceTransforms primvar.
///
/// This class takes these instanceTransforms into account.
///
/// Note that it does not consider instancers instancing the skel root.
///
/// Also note that it assumes that an instancer has only instance.
///
class UsdSkelImagingDataSourceXformResolver
{
public:
    /// Construct from data source of relevant prim.
    UsdSkelImagingDataSourceXformResolver(
        HdSceneIndexBaseRefPtr const &sceneIndex,
        HdContainerDataSourceHandle const &primSource);

    /// Data source for the transform.
    HdMatrixDataSourceHandle GetPrimLocalToCommonSpace() const;

    /// Paths of instancer contributing to the transform. They
    /// need to be observed for invalidation.
    const VtArray<SdfPath> &GetInstancerPaths() const {
        return _instancerPaths;
    }

    /// If a dirty message for this prim or any instancer includes
    /// this locator, we need to reconstruct
    /// UsdSkelImagingDataSourceXformResolver.
    static const HdDataSourceLocator &GetInstancedByLocator();
    /// If a dirty message for this prim or any instancer includes
    /// this locator, we need to refetch the transform data source.
    static const HdDataSourceLocator &GetXformLocator();
    /// If a dirty message for any instancer includes this locator,
    /// we need to refetch the transform data source.
    static const HdDataSourceLocator &GetInstanceXformLocator();

private:
    HdSceneIndexBaseRefPtr const _sceneIndex;
    HdContainerDataSourceHandle const _primSource;
    const VtArray<SdfPath> _instancerPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
