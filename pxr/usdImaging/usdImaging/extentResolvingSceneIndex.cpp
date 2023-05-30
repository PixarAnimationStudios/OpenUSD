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

#include "pxr/usdImaging/usdImaging/extentResolvingSceneIndex.h"

#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/modelSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

using _DirtyEntryPredicate =
    bool(*)(const HdSceneIndexObserver::DirtiedPrimEntry&);
using _DirtyEntryTransform =
    HdSceneIndexObserver::DirtiedPrimEntry(*)(
        const HdSceneIndexObserver::DirtiedPrimEntry&);

template<_DirtyEntryPredicate Predicate,
         _DirtyEntryTransform Transform>
class _TransformedEntries
{
public:
    _TransformedEntries(
        const HdSceneIndexObserver::DirtiedPrimEntries &entries)
      : _entries(entries)
    {
        size_t i = 0;
        for (; i < _entries.size(); ++i) {
            if (Predicate(_entries[i])) {
                break;
            }
        }

        if (i == _entries.size()) {
            return;
        }

        _newEntries.reserve(_entries.size());
        _newEntries.insert(
            _newEntries.end(), _entries.begin(), _entries.begin() + i);

        _newEntries.push_back(Transform(_entries[i]));
        ++i;
        
        for (; i < _entries.size(); ++i) {
            if (Predicate(_entries[i])) {
                _newEntries.push_back(Transform(_entries[i]));
            } else {
                _newEntries.push_back(_entries[i]);
            }
        }
    }

    const HdSceneIndexObserver::DirtiedPrimEntries &
    GetEntries() const {
        if (_newEntries.empty()) {
            return _entries;
        } else {
            return _newEntries;
        }
    }

private:

    const HdSceneIndexObserver::DirtiedPrimEntries &_entries;
    HdSceneIndexObserver::DirtiedPrimEntries _newEntries;
};

bool
_ContainsExtentsHintWithoutExtent(
    const HdSceneIndexObserver::DirtiedPrimEntry &entry)
{
    static const HdDataSourceLocator extentsHintLocator(
        UsdImagingTokens->extentsHint);

    return
        entry.dirtyLocators.Intersects(extentsHintLocator) &&
        !entry.dirtyLocators.Contains(HdExtentSchema::GetDefaultLocator());
}

HdSceneIndexObserver::DirtiedPrimEntry
_ExtentAdded(
    const HdSceneIndexObserver::DirtiedPrimEntry &entry)
{
    HdDataSourceLocatorSet locators = entry.dirtyLocators;
    locators.insert(HdExtentSchema::GetDefaultLocator());
    return { entry.primPath, locators };
}

bool
_Contains(const TfTokenVector &v, const TfToken &t)
{
    return std::find(v.begin(), v.end(), t) != v.end();
}

class _PrimSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimSource);

    TfTokenVector GetNames() override {
        TfTokenVector result = _primSource->GetNames();
        if (HdVectorDataSource::Cast(
                _primSource->Get(UsdImagingTokens->extentsHint))) {
            if (!_Contains(result, HdExtentSchema::GetSchemaToken())) {
                result.push_back(HdExtentSchema::GetSchemaToken());
            }
        }
        return result;
    }
    
    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (HdDataSourceBaseHandle const result = _primSource->Get(name)) {
            return result;
        }

        if (name != HdExtentSchema::GetSchemaToken()) {
            return nullptr;
        }

        HdVectorDataSourceHandle const extentsHintDs =
            HdVectorDataSource::Cast(
                _primSource->Get(
                    UsdImagingTokens->extentsHint));
        if (!extentsHintDs) {
            return nullptr;
        }
        if (const HdDataSourceBaseHandle ds =
                extentsHintDs->GetElement(_extentsHintIndex)) {
            return ds;
        }
        return extentsHintDs->GetElement(0);
    }
    
private:
    _PrimSource(HdContainerDataSourceHandle const &primSource,
                const size_t extentsHintIndex)
      : _primSource(primSource)
      , _extentsHintIndex(extentsHintIndex)
    {
    }

    HdContainerDataSourceHandle const _primSource;
    const size_t _extentsHintIndex;
};

size_t
_GetExtentsHintIndex(HdContainerDataSourceHandle const &inputArgs)
{
    if (!inputArgs) {
        return 0;
    }
    HdTokenDataSourceHandle const ds = HdTokenDataSource::Cast(
        inputArgs->Get(UsdGeomTokens->purpose));
    if (!ds) {
        return 0;
    }
    const TfToken purpose = ds->GetTypedValue(0.0f);
    const TfTokenVector &purposes =
        UsdGeomImageable::GetOrderedPurposeTokens();
    for (size_t i = 0; i < purposes.size(); ++i) {
        if (purpose == purposes[i]) {
            return i;
        }
    }
    return 0;
}

}

UsdImagingExtentResolvingSceneIndexRefPtr
UsdImagingExtentResolvingSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
{
    return TfCreateRefPtr(
        new UsdImagingExtentResolvingSceneIndex(
            inputSceneIndex, inputArgs));
}

UsdImagingExtentResolvingSceneIndex::UsdImagingExtentResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _extentsHintIndex(_GetExtentsHintIndex(inputArgs))
{
}

UsdImagingExtentResolvingSceneIndex::
~UsdImagingExtentResolvingSceneIndex() = default;

HdSceneIndexPrim
UsdImagingExtentResolvingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.dataSource) {
        prim.dataSource = _PrimSource::New(prim.dataSource, _extentsHintIndex);
    }

    return prim;
}

SdfPathVector
UsdImagingExtentResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImagingExtentResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
UsdImagingExtentResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _TransformedEntries<
        /* Predicate = */ _ContainsExtentsHintWithoutExtent,
        /* Transform = */ _ExtentAdded> newEntries(entries);

    _SendPrimsDirtied(newEntries.GetEntries());
}

void
UsdImagingExtentResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
