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

#include "pxr/imaging/hdsi/legacyDisplayStyleOverrideSceneIndex.h"

#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiLegacyDisplayStyleSceneIndex_Impl
{

using OptionalInt = HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt;

struct _StyleInfo
{
    OptionalInt refineLevel;
    /// Retained data source storing refineLevel (or null ptr if empty optional
    /// value) to avoid allocating a data source for every prim.
    HdDataSourceBaseHandle refineLevelDs;
};

/// Data source for locator displayStyle.
class _LegacyDisplayStyleDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LegacyDisplayStyleDataSource);

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdLegacyDisplayStyleSchemaTokens->refineLevel) {
            return _styleInfo->refineLevelDs;
        }
        return nullptr;
    }

    TfTokenVector GetNames() override
    {
        static const TfTokenVector names = {
            HdLegacyDisplayStyleSchemaTokens->refineLevel
        };

        return names;
    }

private:
    _LegacyDisplayStyleDataSource(_StyleInfoSharedPtr const &styleInfo)
      : _styleInfo(styleInfo)
    {
    }

    _StyleInfoSharedPtr _styleInfo;
};

} // namespace HdsiLegacyDisplayStyleSceneIndex_Impl

using namespace HdsiLegacyDisplayStyleSceneIndex_Impl;

HdsiLegacyDisplayStyleOverrideSceneIndexRefPtr
HdsiLegacyDisplayStyleOverrideSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdsiLegacyDisplayStyleOverrideSceneIndex(
            inputSceneIndex));
}

HdsiLegacyDisplayStyleOverrideSceneIndex::
HdsiLegacyDisplayStyleOverrideSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _styleInfo(std::make_shared<_StyleInfo>())
  , _overlayDs(
      HdRetainedContainerDataSource::New(
          HdLegacyDisplayStyleSchemaTokens->displayStyle,
          _LegacyDisplayStyleDataSource::New(_styleInfo)))
{
}

HdSceneIndexPrim
HdsiLegacyDisplayStyleOverrideSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.dataSource) {
        prim.dataSource =
            HdOverlayContainerDataSource::New(
                _overlayDs, prim.dataSource);
    }
    return prim;
}

SdfPathVector
HdsiLegacyDisplayStyleOverrideSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::SetRefineLevel(
    const OptionalInt &refineLevel)
{
    if (refineLevel == _styleInfo->refineLevel) {
        return;
    }

    _styleInfo->refineLevel = refineLevel;
    _styleInfo->refineLevelDs =
        refineLevel
            ? HdRetainedTypedSampledDataSource<int>::New(*refineLevel)
            : nullptr;

    static const HdDataSourceLocatorSet locators(
        HdLegacyDisplayStyleSchema::GetDefaultLocator()
            .Append(HdLegacyDisplayStyleSchemaTokens->refineLevel));

    _DirtyAllPrims(locators);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::_DirtyAllPrims(
    const HdDataSourceLocatorSet &locators)
{
    if (!_IsObserved()) {
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries entries;
    for (const SdfPath &path : HdSceneIndexPrimView(_GetInputSceneIndex())) {
        entries.push_back({path, locators});
    }

    _SendPrimsDirtied(entries);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsAdded(entries);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsRemoved(entries);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsDirtied(entries);
}

bool operator==(
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &a,
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &b)
{
    if (a.hasValue == false && b.hasValue == false) {
        return true;
    }

    return a.hasValue == b.hasValue && a.value == b.value;
}

bool operator!=(
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &a,
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &b)
{
    return !(a == b);
}


PXR_NAMESPACE_CLOSE_SCOPE
