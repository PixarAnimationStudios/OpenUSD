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

#ifndef PXR_USD_IMAGING_USD_IMAGING_GL_DRAW_MODE_STANDIN_H
#define PXR_USD_IMAGING_USD_IMAGING_GL_DRAW_MODE_STANDIN_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingGL_DrawModeStandin
///
/// Provides stand-in geometry for a prim with non-default draw mode.
///
class UsdImagingGL_DrawModeStandin
{
public:
    virtual TfToken GetDrawMode() const = 0;

    /// Get prim replacing the original prim.
    ///
    /// For now, this is just a typeless prim without data source.
    const HdSceneIndexPrim &GetPrim() const;

    /// Get immediate children of the prim replacing the original prim.
    SdfPathVector GetChildPrimPaths() const;
    HdSceneIndexPrim GetChildPrim(const TfToken &name) const;

    // Compute added entries for the stand-in geometry
    void ComputePrimAddedEntries(
        HdSceneIndexObserver::AddedPrimEntries * entries) const;

    /// Given dirty data source locators for the original prim, invalidate
    /// cached data and emit dirty entries for the stand-in geometry.
    virtual void ProcessDirtyLocators(
        const HdDataSourceLocatorSet &dirtyLocator,
        HdSceneIndexObserver::DirtiedPrimEntries * entries) = 0;

protected:
    virtual const TfTokenVector &_GetChildNames() const = 0;
    virtual TfToken _GetChildPrimType(const TfToken &name) const = 0;
    virtual HdContainerDataSourceHandle _GetChildPrimSource(const TfToken &name) const = 0;

    UsdImagingGL_DrawModeStandin(
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

using UsdImagingGL_DrawModeStandinSharedPtr =
    std::shared_ptr<UsdImagingGL_DrawModeStandin>;

/// Given a draw mode and the path and data source for a prim (from the input scene index
/// to the UsdImagingGLDrawModeSceneIndex), return the stand-in geometry or nullptr
/// (if draw mode is default or invalid).
///
UsdImagingGL_DrawModeStandinSharedPtr
UsdImagingGL_GetDrawModeStandin(const TfToken &drawMode,
                                const SdfPath &path,
                                const HdContainerDataSourceHandle &primSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_GL_DRAW_MODE_STANDIN_H
