//
// Copyright 2021 Pixar
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
        const HdContainerDataSourceHandle &input,
        const SdfPath &primPath,
        const HdMaterialFilteringSceneIndexBase::FilteringFnc &fnc)
    : _input(input)
    , _primPath(primPath)
    , _fnc(fnc)
    {}

    TfTokenVector
    GetNames() override
    {
        if (_input) {
            return _input->GetNames();
        }
        return {};
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (_input) {
            HdDataSourceBaseHandle result = _input->Get(name);
            if (HdContainerDataSourceHandle networkContainer =
                    HdContainerDataSource::Cast(result)) {

                HdDataSourceMaterialNetworkInterface networkInterface(
                    _primPath, networkContainer);
                _fnc(&networkInterface);
                return networkInterface.Finish();
            }
        }

        return nullptr;
    }


private:
    HdContainerDataSourceHandle _input;
    SdfPath _primPath;
    HdMaterialFilteringSceneIndexBase::FilteringFnc _fnc;
};


class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdMaterialFilteringSceneIndexBase* base,
        const HdContainerDataSourceHandle &input,
        const SdfPath &primPath)
    : _base(base)
    , _input(input)
    , _primPath(primPath)
    {}

    TfTokenVector
    GetNames() override
    {
        if (_input) {
            return _input->GetNames();
        }
        return {};
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (_input) {
            HdDataSourceBaseHandle result = _input->Get(name);
            if (result && name == HdMaterialSchemaTokens->material) {
                if (HdContainerDataSourceHandle materialContainer =
                        HdContainerDataSource::Cast(result)) {
                    return _MaterialDataSource::New(
                        materialContainer, _primPath, _base->GetFilteringFunction());
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
    HdContainerDataSourceHandle _input;
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
