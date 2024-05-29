//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdsi/materialBindingResolvingSceneIndex.h"

#include "pxr/imaging/hd/materialBindingsSchema.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

class _MaterialBindingsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialBindingsDataSource);

    _MaterialBindingsDataSource(
        const HdContainerDataSourceHandle& input,
        const TfTokenVector& purposePriorityOrder,
        const TfToken& dstPurpose)
        : _input(input)
        , _purposePriorityOrder(purposePriorityOrder)
        , _dstPurpose(dstPurpose)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_input) {
            return {};
        }

        if (_HasAny()) {
            return {
                _dstPurpose,
            };
        }
        return {};
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (!_input) {
            return nullptr;
        }

        if (name != _dstPurpose) {
            return nullptr;
        }

        for (const TfToken& purpose : _purposePriorityOrder) {
            if (auto data = _input->Get(purpose)) {
                return data;
            }
        }
        return nullptr;
    }

private:
    bool _HasAny() const
    {
        TfTokenVector names = _input->GetNames();
        for (const TfToken& purpose : _purposePriorityOrder) {
            if (std::find(names.begin(), names.end(), purpose) != names.end()) {
                return true;
            }
        }
        return false;
    }

    HdContainerDataSourceHandle _input;
    TfTokenVector _purposePriorityOrder;
    TfToken _dstPurpose;
};

class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdContainerDataSourceHandle& input,
        const TfTokenVector& purposePriorityOrder,
        const TfToken& dstPurpose)
        : _input(input)
        , _purposePriorityOrder(purposePriorityOrder)
        , _dstPurpose(dstPurpose)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_input) {
            return {};
        }
        return _input->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (!_input) {
            return nullptr;
        }

        auto data = _input->Get(name);
        if (name == HdMaterialBindingsSchema::GetSchemaToken()) {
            if (auto materialBindingContainer
                = HdContainerDataSource::Cast(data)) {
                return _MaterialBindingsDataSource::New(
                    materialBindingContainer, _purposePriorityOrder, _dstPurpose);
            }
        }
        return data;
    }

private:
    HdContainerDataSourceHandle _input;
    TfTokenVector _purposePriorityOrder;
    TfToken _dstPurpose;
};

};

// static
HdsiMaterialBindingResolvingSceneIndexRefPtr
HdsiMaterialBindingResolvingSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const TfTokenVector& purposePriorityOrder,
    const TfToken& dstPurpose)
{
    return TfCreateRefPtr(new HdsiMaterialBindingResolvingSceneIndex(
        inputSceneIndex, purposePriorityOrder, dstPurpose));
}

HdSceneIndexPrim
HdsiMaterialBindingResolvingSceneIndex::GetPrim(const SdfPath& primPath) const
{
    if (auto input = _GetInputSceneIndex()) {
        HdSceneIndexPrim prim = input->GetPrim(primPath);
        if (prim.dataSource) {
            prim.dataSource = _PrimDataSource::New(
                prim.dataSource, _purposePriorityOrder, _dstPurpose);
        }
        return prim;
    }
    return { TfToken(), nullptr };
}

SdfPathVector
HdsiMaterialBindingResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath& primPath) const
{
    if (auto input = _GetInputSceneIndex()) {
        return input->GetChildPrimPaths(primPath);
    }
    return {};
}

void
HdsiMaterialBindingResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    _SendPrimsAdded(entries);
}

void
HdsiMaterialBindingResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiMaterialBindingResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    _SendPrimsDirtied(entries);
}

HdsiMaterialBindingResolvingSceneIndex::HdsiMaterialBindingResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const TfTokenVector& purposePriorityOrder,
    const TfToken& dstPurpose)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    , _purposePriorityOrder(purposePriorityOrder)
    , _dstPurpose(dstPurpose)
{
}

HdsiMaterialBindingResolvingSceneIndex::
    ~HdsiMaterialBindingResolvingSceneIndex()
    = default;

PXR_NAMESPACE_CLOSE_SCOPE
