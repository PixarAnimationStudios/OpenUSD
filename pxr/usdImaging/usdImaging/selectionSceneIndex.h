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
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_SELECTION_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_SELECTION_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingSelectionSceneIndex_Impl
{
using _SelectionInfoSharedPtr = std::shared_ptr<struct _SelectionInfo>;
}

TF_DECLARE_REF_PTRS(UsdImagingSelectionSceneIndex);

/// \class UsdImagingSelectionSceneIndex
///
/// A simple scene index adding HdSelectionsSchema to all prims selected
/// with AddSelection.
///
class UsdImagingSelectionSceneIndex final
                          : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDIMAGING_API
    static UsdImagingSelectionSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

    ~UsdImagingSelectionSceneIndex() override;

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    /// Given a path (including usd proxy path inside a native instance) of
    /// a USD prim, determine the corresponding prim in the Usd scene index
    /// (filtered by the UsdImagingNiPrototypePropagatingSceneIndex) and
    /// populate its selections data source.
    USDIMAGING_API
    void AddSelection(const SdfPath &usdPath);

    /// Reset the scene index selection state.
    USDIMAGING_API
    void ClearSelection();

protected:
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
    UsdImagingSelectionSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

    UsdImagingSelectionSceneIndex_Impl::
    _SelectionInfoSharedPtr _selectionInfo;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
