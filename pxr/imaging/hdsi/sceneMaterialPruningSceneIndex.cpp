//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdsi/sceneMaterialPruningSceneIndex.h"

#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdsiSceneMaterialPruningSceneIndexTokens,
                        HDSI_SCENE_MATERIAL_PRUNING_SCENE_INDEX_TOKENS);

namespace HdsiSceneMaterialPruningSceneIndex_Impl
{

struct _Info
{
    _Info(const HdDataSourceLocator &builtinMaterialLocator)
     : enabled(false)
     , builtinMaterialLocator(builtinMaterialLocator)
    {
    }
   
    bool enabled;
    const HdDataSourceLocator builtinMaterialLocator;
};

bool
_PruneMaterialBinding(
    HdContainerDataSourceHandle const &primSource)
{
    const HdLegacyDisplayStyleSchema schema =
        HdLegacyDisplayStyleSchema::GetFromParent(primSource);
    HdBoolDataSourceHandle const ds =
        schema.GetMaterialIsFinal();
    const bool materialIsFinal = ds && ds->GetTypedValue(0.0f);

    return !materialIsFinal;
}

// This _PrimDataSource filters out bindings at binding token
class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(HdContainerDataSourceHandle const &input,
                    _InfoSharedPtr const &info)
      : _input(input), _info(info)
    {
    }

    TfTokenVector GetNames() override
    {
        return _input->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        TRACE_FUNCTION();

        if (name == HdMaterialBindingsSchema::GetSchemaToken() &&
            _info->enabled &&
            _PruneMaterialBinding(_input)) {
            return nullptr;
        }
        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle const _input;
    _InfoSharedPtr const _info;
};

bool
_PruneMaterial(
    HdContainerDataSourceHandle const &primSource,
    _InfoSharedPtr const &info)
{
    auto const ds =
        HdBoolDataSource::Cast(
            HdContainerDataSource::Get(
                primSource, info->builtinMaterialLocator));
    const bool isBuiltinMaterial = ds && ds->GetTypedValue(0.0f);

    return !isBuiltinMaterial;
}

HdDataSourceLocator
_GetBuiltinLocator(HdContainerDataSourceHandle const &inputArgs)
{
    if (!inputArgs) {
        return {};
    }
        
    auto const ds =
        HdLocatorDataSource::Cast(
            inputArgs->Get(
                HdsiSceneMaterialPruningSceneIndexTokens
                    ->builtinMaterialLocator));
    if (!ds) {
        return {};
    }

    return ds->GetTypedValue(0.0f);
}
    
}
    
using namespace HdsiSceneMaterialPruningSceneIndex_Impl;

// static
HdsiSceneMaterialPruningSceneIndexRefPtr
HdsiSceneMaterialPruningSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
{
    return TfCreateRefPtr(new HdsiSceneMaterialPruningSceneIndex(
        inputSceneIndex, inputArgs));
}

bool
HdsiSceneMaterialPruningSceneIndex::GetEnabled() const
{
    return _info->enabled;
}

void
HdsiSceneMaterialPruningSceneIndex::SetEnabled(const bool enabled)
{
    TRACE_FUNCTION();
    
    if (_info->enabled == enabled) {
        return;
    }

    _info->enabled = enabled;

    if (!_IsObserved()) {
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;

    for (const SdfPath &primPath: HdSceneIndexPrimView(_GetInputSceneIndex())) {
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

        if (prim.primType == HdPrimTypeTokens->material) {
            if (_PruneMaterial(prim.dataSource, _info)) {
                if (_info->enabled) {
                    addedEntries.push_back({ primPath, TfToken() });
                } else {
                    addedEntries.push_back({ primPath, prim.primType });
                }
            }
        } else {
            if (_PruneMaterialBinding(prim.dataSource)) {
                static const HdDataSourceLocatorSet locators{
                    HdMaterialBindingsSchema::GetDefaultLocator()};
                dirtiedEntries.push_back({ primPath, locators });
            }
        }
    }
        
    if (!addedEntries.empty()) {
        _SendPrimsAdded(addedEntries);
    }
    if (!dirtiedEntries.empty()) {
        _SendPrimsDirtied(dirtiedEntries);
    }
}

HdSceneIndexPrim
HdsiSceneMaterialPruningSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (!prim.dataSource) {
        return prim;
    }

    if (prim.primType == HdPrimTypeTokens->material) {
        if (_info->enabled && _PruneMaterial(prim.dataSource, _info)) {
            prim.primType = TfToken();
        }
    } else {
        if (prim.dataSource) {
            prim.dataSource = _PrimDataSource::New(prim.dataSource, _info);
        }
    }
    return prim;
}

SdfPathVector
HdsiSceneMaterialPruningSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiSceneMaterialPruningSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (!_IsObserved()) {
        return;
    }

    if (!_info->enabled) {
        _SendPrimsAdded(entries);
        return;
    }
    
    size_t i = 0;

    while (i < entries.size()) {
        const HdSceneIndexObserver::AddedPrimEntry &entry = entries[i];
        if (entry.primType == HdPrimTypeTokens->material) {
            if (_PruneMaterial(
                    _GetInputSceneIndex()->GetPrim(entry.primPath).dataSource,
                    _info)) {
                break;
            }
        }
        i++;
    }

    if (i == entries.size()) {
        _SendPrimsAdded(entries);
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries newEntries(entries);

    {
        HdSceneIndexObserver::AddedPrimEntry &entry = newEntries[i];
        entry.primType = TfToken();
    }
    i++;

    while (i < entries.size()) {
        HdSceneIndexObserver::AddedPrimEntry &entry = newEntries[i];
        if (entry.primType == HdPrimTypeTokens->material) {
            if (_PruneMaterial(
                    _GetInputSceneIndex()->GetPrim(entry.primPath).dataSource,
                    _info)) {
                entry.primType = TfToken();
            }
        }
        i++;
    }

    _SendPrimsAdded(newEntries);
}

void
HdsiSceneMaterialPruningSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiSceneMaterialPruningSceneIndex::_ProcessDirtiedEntryHelper(
    const HdSceneIndexObserver::DirtiedPrimEntry &entry,
    HdSceneIndexObserver::AddedPrimEntries * const addedEntries)
{
    if (!entry.dirtyLocators.Contains(_info->builtinMaterialLocator)) {
        return;
    }
    const HdSceneIndexPrim prim =
        _GetInputSceneIndex()->GetPrim(entry.primPath);
    if (prim.primType != HdPrimTypeTokens->material) {
        return;
    }
    if (_PruneMaterial(prim.dataSource, _info)) {
        addedEntries->push_back({entry.primPath, TfToken()});
    } else {
        addedEntries->push_back({entry.primPath, prim.primType});
    }
}


void
HdsiSceneMaterialPruningSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (!_IsObserved()) {
        return;
    }

    if (!_info->enabled) {
        _SendPrimsDirtied(entries);
        return;
    }

    static const HdDataSourceLocator materialIsFinalLocator =
        HdLegacyDisplayStyleSchema::GetDefaultLocator()
            .Append(HdLegacyDisplayStyleSchemaTokens->materialIsFinal);
    
    HdSceneIndexObserver::AddedPrimEntries addedEntries;

    size_t i = 0;

    while (i < entries.size()) {
        const HdSceneIndexObserver::DirtiedPrimEntry &entry = entries[i];
        _ProcessDirtiedEntryHelper(entry, &addedEntries);
        if (entry.dirtyLocators.Intersects(materialIsFinalLocator)) {
            break;
        }
        i++;
    }
    if (i == entries.size()) {
        if (!addedEntries.empty()) {
            _SendPrimsAdded(addedEntries);
        }
        _SendPrimsDirtied(entries);
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries newEntries(entries);
    {
        HdSceneIndexObserver::DirtiedPrimEntry &entry = newEntries[i];
        entry.dirtyLocators.insert(
            HdMaterialBindingsSchema::GetDefaultLocator());
    }
    i++;

    while (i < entries.size()) {
        HdSceneIndexObserver::DirtiedPrimEntry &entry = newEntries[i];
        _ProcessDirtiedEntryHelper(entry, &addedEntries);
        if (entry.dirtyLocators.Intersects(materialIsFinalLocator)) {
            entry.dirtyLocators.insert(
                HdMaterialBindingsSchema::GetDefaultLocator());
        }
        i++;
    }
    
    if (!addedEntries.empty()) {
        _SendPrimsAdded(addedEntries);
    }
    _SendPrimsDirtied(newEntries);
}

HdsiSceneMaterialPruningSceneIndex::HdsiSceneMaterialPruningSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
 : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
 , _info(std::make_shared<_Info>(_GetBuiltinLocator(inputArgs)))
{
}

HdsiSceneMaterialPruningSceneIndex::
~HdsiSceneMaterialPruningSceneIndex() = default;

PXR_NAMESPACE_CLOSE_SCOPE
