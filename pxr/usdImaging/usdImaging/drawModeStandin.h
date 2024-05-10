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
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.

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
