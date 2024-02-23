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
#ifndef PXR_USD_IMAGING_USD_IMAGING_DRAW_MODE_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_DRAW_MODE_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

using UsdImaging_DrawModeStandinSharedPtr =
    std::shared_ptr<class UsdImaging_DrawModeStandin>;

TF_DECLARE_REF_PTRS(UsdImagingDrawModeSceneIndex);

/// A scene index replacing geometry based on the draw mode.
///
/// Inspects a prim's values for drawMode and applyDrawMode (see
/// UsdImagingGeomModelSchema).
/// If the drawMode is valid and not the default and applyDrawMode is true,
/// the prim and all its descendents are replaced by stand-in geometry
/// specified by the draw mode.
///
/// Note that the material that ensures the correct texture is used on each
/// face is using glslfx nodes and thus only works properly in Storm.
///
/// Using a UsdPreviewSurface instead (so that it works accross different
/// renderers) probably requires breaking up the geometry into several pieces.
///
class UsdImagingDrawModeSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// inputArgs unused for now. In the future, we might use it to say that
    /// we want to break up the geometry and use UsdPreviewSurface to work
    /// across different renderers.
    /// 
    USDIMAGING_API
    static UsdImagingDrawModeSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    USDIMAGING_API
    ~UsdImagingDrawModeSceneIndex() override;
    
    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    UsdImagingDrawModeSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    // Delete path and all descendents from _prims.
    void _DeleteSubtree(const SdfPath &path);
    // Pull prim at path and recursively its descendants from input
    // scene index - stopping the recursion when a prim with
    // non-default drawmode is hit. When a prim has non-trivial drawmode,
    // the DrawModeStandin object is instantiated instead.
    void _RecursePrims(
        const TfToken &drawMode,
        const SdfPath &path,
        const HdSceneIndexPrim &prim,
        HdSceneIndexObserver::AddedPrimEntries *entries);

    // Finds prim or ancestor of prim with non-default drawmode in _prims map.
    // relPathLen indicates whether the found entry is for the prim itself (0),
    // an immediate parent (1) or further ancestor (2 or larger).
    UsdImaging_DrawModeStandinSharedPtr
    _FindStandinForPrimOrAncestor(
        const SdfPath &path, size_t * const relPathLen) const;

    // For prims with non-default drawmode, store a DrawModeStandin object
    // that can be queried for the stand-in geometry.
    // No path in the map is a prefix of any other path in the map.
    std::map<SdfPath, UsdImaging_DrawModeStandinSharedPtr> _prims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_GL_DRAW_MODE_SCENE_INDEX_H
