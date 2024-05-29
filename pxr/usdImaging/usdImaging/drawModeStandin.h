//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_USD_IMAGING_USD_IMAGING_DRAW_MODE_STANDIN_H
#define PXR_USD_IMAGING_USD_IMAGING_DRAW_MODE_STANDIN_H

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImaging_DrawModeStandin
///
/// Provides stand-in geometry for a prim with non-default draw mode.
///
class UsdImaging_DrawModeStandin
{
public:
    virtual ~UsdImaging_DrawModeStandin();

    virtual TfToken GetDrawMode() const = 0;

    /// Get prim replacing the original prim.
    ///
    /// For now, this is just a typeless container prim without data source.
    const HdSceneIndexPrim &GetPrim() const;

    /// Get paths of all descendants of the typeless container.
    SdfPathVector GetDescendantPrimPaths() const;
    
    /// GetDescendant prim by absolute or relative path.
    HdSceneIndexPrim GetDescendantPrim(const SdfPath& path) const;

    /// Compute added entries for the stand-in geometry
    void ComputePrimAddedEntries(
        HdSceneIndexObserver::AddedPrimEntries * entries) const;
    
    /// Compute removed entries for the stand-in geometry
    void ComputePrimRemovedEntries(
        HdSceneIndexObserver::RemovedPrimEntries* entries) const;

    /// Given dirty data source locators for the original prim, invalidate
    /// cached data and emit dirty entries for the stand-in geometry.
    virtual void ProcessDirtyLocators(
        const HdDataSourceLocatorSet &dirtyLocator,
        HdSceneIndexObserver::DirtiedPrimEntries * entries,
        bool * needsRefresh) = 0;

protected:
    /// returns paths relative to the typeless container.
    virtual const SdfPathVector _GetDescendantPaths() const = 0;
    /// accepts paths relative to the typeless container.
    virtual TfToken _GetDescendantPrimType(const SdfPath& path) const = 0;
    /// accepts paths relative to the typeless container.
    virtual HdContainerDataSourceHandle _GetDescendantPrimSource(
        const SdfPath& path) const = 0;

    UsdImaging_DrawModeStandin(
        const SdfPath &path,
        const HdContainerDataSourceHandle &primSource)
      : _path(path)
      , _primSource(primSource)
    {
    }

    // Path of original prim and prim replacing it.
    const SdfPath _path;
    HdContainerDataSourceHandle const _primSource;
};

using UsdImaging_DrawModeStandinSharedPtr =
    std::shared_ptr<UsdImaging_DrawModeStandin>;

/// Given a draw mode and the path and data source for a prim (from the input scene index
/// to the UsdImagingDrawModeSceneIndex), return the stand-in geometry or nullptr
/// (if draw mode is default or invalid).
///
UsdImaging_DrawModeStandinSharedPtr
UsdImaging_GetDrawModeStandin(const TfToken &drawMode,
                                const SdfPath &path,
                                const HdContainerDataSourceHandle &primSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DRAW_MODE_STANDIN_H
