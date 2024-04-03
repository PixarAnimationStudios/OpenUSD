//
// Copyright 2023 Pixar
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
#ifndef PXR_IMAGING_HDSI_LEGACY_DISPLAY_STYLE_OVERRIDE_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_LEGACY_DISPLAY_STYLE_OVERRIDE_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiLegacyDisplayStyleSceneIndex_Impl
{
struct _StyleInfo;
using _StyleInfoSharedPtr = std::shared_ptr<_StyleInfo>;
}

TF_DECLARE_REF_PTRS(HdsiLegacyDisplayStyleOverrideSceneIndex);

///
/// \class HdsiLegacyDisplayStyleOverrideSceneIndex
///
/// A scene index overriding the legacy display style for each prim.
/// So far, it only supports the refine level.
///
class HdsiLegacyDisplayStyleOverrideSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiLegacyDisplayStyleOverrideSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    /// A replacement for std::optional<int> that is not available until C++17.
    struct OptionalInt
    {
        bool hasValue = false;
        int value = 0;

        operator bool() const { return hasValue; }
        int operator*() const { return value; }
    };

    /// Sets the refine level (at data source locator displayStyle:refineLevel)
    /// for every prim in the input scene inedx.
    ///
    /// If an empty optional value is provided, a null data source will be
    /// returned for the data source locator.
    ///
    HDSI_API
    void SetRefineLevel(const OptionalInt &refineLevel);

protected:
    HdsiLegacyDisplayStyleOverrideSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

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
    void _DirtyAllPrims(const HdDataSourceLocatorSet &locators);

    HdsiLegacyDisplayStyleSceneIndex_Impl::
    _StyleInfoSharedPtr const _styleInfo;

    /// Prim overlay data source.
    HdContainerDataSourceHandle const _overlayDs;
};

HDSI_API
bool operator==(
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &a,
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &b);

HDSI_API
bool operator!=(
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &a,
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &b);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_LEGACY_DISPLAY_STYLE_OVERRIDE_SCENE_INDEX_H
