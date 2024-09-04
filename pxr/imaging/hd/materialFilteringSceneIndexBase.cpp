//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/materialFilteringSceneIndexBase.h"
#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"

#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

class _MaterialDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialDataSource);

    _MaterialDataSource(
        const HdContainerDataSourceHandle &materialInput,
        const HdContainerDataSourceHandle &primInput,
        const HdContainerDataSourceHandle &config,
        const SdfPath &primPath,
        const HdMaterialFilteringSceneIndexBase::FilteringFnc &fnc)
    : _materialInput(materialInput)
    , _primInput(primInput)
    , _config(config)
    , _primPath(primPath)
    , _fnc(fnc)
    {}

    TfTokenVector
    GetNames() override
    {
        if (_materialInput) {
            return _materialInput->GetNames();
        }
        return {};
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (_materialInput) {
            HdDataSourceBaseHandle result = _materialInput->Get(name);
            if (HdContainerDataSourceHandle networkContainer =
                    HdContainerDataSource::Cast(result)) {

                HdDataSourceMaterialNetworkInterface networkInterface(
                    _primPath, networkContainer, _primInput);
                _fnc(&networkInterface);
                return networkInterface.Finish();
            }
        }

        return nullptr;
    }


private:
    HdContainerDataSourceHandle _materialInput;
    HdContainerDataSourceHandle _primInput;
    HdContainerDataSourceHandle _config;
    SdfPath _primPath;
    HdMaterialFilteringSceneIndexBase::FilteringFnc _fnc;
};


class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdMaterialFilteringSceneIndexBase* base,
        const HdContainerDataSourceHandle &primInput,
        const SdfPath &primPath)
    : _base(base)
    , _primInput(primInput)
    , _primPath(primPath)
    {}

    TfTokenVector
    GetNames() override
    {
        if (_primInput) {
            return _primInput->GetNames();
        }
        return {};
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (_primInput) {
            HdDataSourceBaseHandle result = _primInput->Get(name);
            if (result && name == HdMaterialSchemaTokens->material) {
                if (HdContainerDataSourceHandle materialContainer =
                        HdContainerDataSource::Cast(result)) {
                    return _MaterialDataSource::New(
                        materialContainer, _primInput, _config, _primPath,
                        _base->GetFilteringFunction());
                }
            }
            return result;
        }

        return nullptr;
    }

private:
    // pointer to HdMaterialFilteringSceneIndexBase so that we can query for the
    // filtering function.
    const HdMaterialFilteringSceneIndexBase* _base;
    HdContainerDataSourceHandle _primInput;
    HdContainerDataSourceHandle _config;
    SdfPath _primPath;
};



} // namespace anonymous

HdMaterialFilteringSceneIndexBase::HdMaterialFilteringSceneIndexBase(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim
HdMaterialFilteringSceneIndexBase::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.primType == HdPrimTypeTokens->material && prim.dataSource) {
        prim.dataSource = _PrimDataSource::New(this, prim.dataSource, primPath);
    }

    return prim;
}

SdfPathVector
HdMaterialFilteringSceneIndexBase::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

HdMaterialFilteringSceneIndexBase::FilteringFnc
HdMaterialFilteringSceneIndexBase::GetFilteringFunction() const
{
    return _GetFilteringFunction();
}

void
HdMaterialFilteringSceneIndexBase::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
HdMaterialFilteringSceneIndexBase::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdMaterialFilteringSceneIndexBase::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
