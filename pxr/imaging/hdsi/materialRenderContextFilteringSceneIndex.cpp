//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdsi/materialRenderContextFilteringSceneIndex.h"

#include "pxr/imaging/hd/materialSchema.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{    
using _TypePredicateFn =
    HdsiMaterialRenderContextFilteringSceneIndex::TypePredicateFn;

class _MaterialDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialDataSource);

    _MaterialDataSource(
        const HdContainerDataSourceHandle& materialContainer,
        const TfTokenVector& renderContextPriorityOrder)
        : _materialContainer(materialContainer)
        , _winningContext(_FindWinningContext(renderContextPriorityOrder))
    {
    }

    TfTokenVector GetNames() override
    {
        if (_winningContext) {
            return {*_winningContext};
        }
        return {};
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (name == _winningContext) {
            return _materialContainer->Get(name);
        }
        return nullptr;
    }

private:
    std::optional<TfToken> _FindWinningContext(
        const TfTokenVector& renderContextPriorityOrder) const
    {
        const HdMaterialSchema materialSchema(_materialContainer);
        for (const TfToken& context : renderContextPriorityOrder) {
            if (materialSchema.GetMaterialNetwork(context)) {
                return context;
            }
        }
        return {};
    }

    const HdContainerDataSourceHandle _materialContainer;
    const std::optional<TfToken> _winningContext;
};

class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdContainerDataSourceHandle& primContainer,
        const TfTokenVector& renderContextPriorityOrder)
        : _primContainer(primContainer)
        , _renderContextPriorityOrder(renderContextPriorityOrder)
    {
    }

    TfTokenVector GetNames() override
    {
        return _primContainer->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (name == HdMaterialSchema::GetSchemaToken()) {
            if (auto matSchema =
                    HdMaterialSchema::GetFromParent(_primContainer)) {
                return _MaterialDataSource::New(
                    matSchema.GetContainer(), _renderContextPriorityOrder);
            }
        }
        return _primContainer->Get(name);
    }

private:
    const HdContainerDataSourceHandle _primContainer;
    const TfTokenVector &_renderContextPriorityOrder;
};

};

// static
HdsiMaterialRenderContextFilteringSceneIndexRefPtr
HdsiMaterialRenderContextFilteringSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    TfTokenVector renderContextPriorityOrder,
    _TypePredicateFn typePredicateFn)
{
    return TfCreateRefPtr(new HdsiMaterialRenderContextFilteringSceneIndex(
        inputSceneIndex,
        std::move(renderContextPriorityOrder),
        std::move(typePredicateFn)));
}

HdSceneIndexPrim
HdsiMaterialRenderContextFilteringSceneIndex::GetPrim(
    const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim && (!_typePredicateFn || _typePredicateFn(prim.primType))) {
        prim.dataSource = _PrimDataSource::New(
            prim.dataSource, _renderContextPriorityOrder);
    }
    return prim;
}

SdfPathVector
HdsiMaterialRenderContextFilteringSceneIndex::GetChildPrimPaths(
    const SdfPath& primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiMaterialRenderContextFilteringSceneIndex::_PrimsAdded(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    _SendPrimsAdded(entries);
}

void
HdsiMaterialRenderContextFilteringSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiMaterialRenderContextFilteringSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    _SendPrimsDirtied(entries);
}

HdsiMaterialRenderContextFilteringSceneIndex::
    HdsiMaterialRenderContextFilteringSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    TfTokenVector renderContextPriorityOrder,
    _TypePredicateFn typePredicateFn)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    , _renderContextPriorityOrder(std::move(renderContextPriorityOrder))
    , _typePredicateFn(std::move(typePredicateFn))
{
}

HdsiMaterialRenderContextFilteringSceneIndex::
    ~HdsiMaterialRenderContextFilteringSceneIndex()
    = default;

PXR_NAMESPACE_CLOSE_SCOPE
