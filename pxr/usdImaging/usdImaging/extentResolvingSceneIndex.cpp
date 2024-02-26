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

#include "pxr/usdImaging/usdImaging/extentsHintSchema.h"
#include "pxr/usdImaging/usdImaging/modelSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vectorSchema.h"
#include "pxr/base/gf/range3d.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdImagingExtentResolvingSceneIndexTokens,
                        USDIMAGINGEXTENTRESOLVINGSCENEINDEX_TOKENS);

namespace UsdImagingExtentResolvingSceneIndex_Impl
{

TfToken::HashSet
_GetPurposes(HdContainerDataSourceHandle const &inputArgs)
{
    static const TfToken defaultTokens[] = { HdTokens->geometry };
    static const TfToken::HashSet defaultSet{ std::begin(defaultTokens),
                                              std::end(defaultTokens) };

    if (!inputArgs) {
        return defaultSet;
    }

    HdTypedVectorSchema<HdTokenDataSource> vecSchema(
        HdVectorDataSource::Cast(
            inputArgs->Get(
                UsdImagingExtentResolvingSceneIndexTokens->purposes)));
    if (!vecSchema) {
        return defaultSet;
    }

    TfToken::HashSet result;

    const size_t n = vecSchema.GetNumElements();
    for (size_t i = 0; i < n; i++) {
        if (HdTokenDataSourceHandle const ds = vecSchema.GetElement(i)) {
            result.insert(ds->GetTypedValue(0.0f));
        }
    }
    return result;
}

struct _Info
{
    _Info(HdContainerDataSourceHandle const &inputArgs)
      : purposes(_GetPurposes(inputArgs))
    {
    }

    /// When computing the bounding box, we only consider geometry
    /// with purposes being in this set.
    const TfToken::HashSet purposes;
};

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
    return
        entry.dirtyLocators.Intersects(
            UsdImagingExtentsHintSchema::GetDefaultLocator()) &&
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
        if (_GetExtentsHints()) {
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

        // Use extentsHint if extent is not given.
        if (name == HdExtentSchema::GetSchemaToken()) {
            return _GetExtentFromExtentsHint();
        }

        return nullptr;
    }

private:
    _PrimSource(HdContainerDataSourceHandle const &primSource,
                _InfoSharedPtr const &info)
      : _primSource(primSource)
      , _info(info)
    {
    }

    UsdImagingExtentsHintSchema _GetExtentsHints() const {
        return
            UsdImagingExtentsHintSchema::GetFromParent(_primSource);
    }

    HdDataSourceBaseHandle _GetExtentFromExtentsHint() const {
        if (_info->purposes.empty()) {
            return nullptr;
        }

        UsdImagingExtentsHintSchema extentsHintSchema = _GetExtentsHints();
        if (!extentsHintSchema) {
            return nullptr;
        }
        
        if (_info->purposes.size() == 1) {
            return
                extentsHintSchema
                    .GetExtent(*_info->purposes.begin())
                    .GetContainer();
        }

        GfRange3d bbox;
        for (const TfToken &purpose : _info->purposes) {
            HdExtentSchema extentSchema = extentsHintSchema.GetExtent(purpose);
            HdVec3dDataSourceHandle const minDs = extentSchema.GetMin();
            HdVec3dDataSourceHandle const maxDs = extentSchema.GetMax();
            if (minDs && maxDs) {
                bbox.UnionWith(
                    GfRange3d(
                        minDs->GetTypedValue(0.0f),
                        maxDs->GetTypedValue(0.0f)));
            }
        }

        return
            HdExtentSchema::Builder()
                .SetMin(
                    HdRetainedTypedSampledDataSource<GfVec3d>::New(
                        bbox.GetMin()))
                .SetMax(
                    HdRetainedTypedSampledDataSource<GfVec3d>::New(
                        bbox.GetMax()))
                .Build();
    }
    
    HdContainerDataSourceHandle const _primSource;
    _InfoSharedPtr const _info;
};

}

using namespace UsdImagingExtentResolvingSceneIndex_Impl;

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
  , _info(std::make_shared<_Info>(inputArgs))
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
        prim.dataSource = _PrimSource::New(prim.dataSource, _info);
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
