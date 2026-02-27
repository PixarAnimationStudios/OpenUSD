//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdsi/sceneMaterialPruningSceneIndex.h"

#include "pxr/imaging/hd/builtinMaterialSchema.h"
#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiSceneMaterialPruningSceneIndex_Impl
{

struct _Info
{
    _Info() : enabled(false)
    {}

    bool enabled;
};

bool
_MaterialIsFinal(HdContainerDataSourceHandle const &primSource)
{
    const HdLegacyDisplayStyleSchema schema =
        HdLegacyDisplayStyleSchema::GetFromParent(primSource);
    HdBoolDataSourceHandle const ds = schema.GetMaterialIsFinal();
    return ds && ds->GetTypedValue(0.0f);
}

bool
_PruneMaterialBinding(HdContainerDataSourceHandle const &primSource)
{
    // Prune the Material only if there is material binding, and the material 
    // is not marked as final.
    const HdMaterialBindingsSchema materialBindingSchema =
        HdMaterialBindingsSchema::GetFromParent(primSource);
    
    if (!materialBindingSchema) {
        return false;
    }

    return !_MaterialIsFinal(primSource);
}

bool
_PruneMaterial(HdContainerDataSourceHandle const &primSource)
{
    const HdBuiltinMaterialSchema schema =
        HdBuiltinMaterialSchema::GetFromParent(primSource);
    HdBoolDataSourceHandle const ds = schema.GetBuiltinMaterial();
    const bool isBuiltinMaterial = ds && ds->GetTypedValue(0.0f);

    return !isBuiltinMaterial;
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
            _info->enabled && !_MaterialIsFinal(_input)) {
            return nullptr;
        }
        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle const _input;
    _InfoSharedPtr const _info;
};

}
    
using namespace HdsiSceneMaterialPruningSceneIndex_Impl;

// static
HdsiSceneMaterialPruningSceneIndexRefPtr
HdsiSceneMaterialPruningSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdsiSceneMaterialPruningSceneIndex(inputSceneIndex));
}

bool
HdsiSceneMaterialPruningSceneIndex::GetEnabled() const
{
    return _info->enabled;
}

void
HdsiSceneMaterialPruningSceneIndex::SetEnabled(const bool enabled)
{    
    if (_info->enabled == enabled) {
        return;
    }

    TRACE_FUNCTION();

    _info->enabled = enabled;

    if (!_IsObserved()) {
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;

    for (const SdfPath &primPath: HdSceneIndexPrimView(_GetInputSceneIndex())) {
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

        if (prim.primType == HdPrimTypeTokens->material) {
            if (_PruneMaterial(prim.dataSource)) {
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
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (!prim.dataSource) {
        return prim;
    }

    if (prim.primType == HdPrimTypeTokens->material) {
        if (_info->enabled && _PruneMaterial(prim.dataSource)) {
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
    if (!_IsObserved()) {
        return;
    }
    
    if (!_info->enabled) {
        _SendPrimsAdded(entries);
        return;
    }
    
    TRACE_FUNCTION();

    size_t i = 0;

    while (i < entries.size()) {
        const HdSceneIndexObserver::AddedPrimEntry &entry = entries[i];
        if (entry.primType == HdPrimTypeTokens->material) {
            if (_PruneMaterial(
                    _GetInputSceneIndex()->GetPrim(entry.primPath).dataSource)) {
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
                    _GetInputSceneIndex()->GetPrim(entry.primPath).dataSource)) {
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
    // Add/Prune the material if it changed whether it is builtin or not
    if (!entry.dirtyLocators.Contains(
            HdBuiltinMaterialSchema::GetBuiltinMaterialLocator())) {
        return;
    }

    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(entry.primPath);
    if (prim.primType != HdPrimTypeTokens->material) {
        return;
    }
    if (_PruneMaterial(prim.dataSource)) {
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
    if (!_IsObserved()) {
        return;
    }

    if (!_info->enabled) {
        _SendPrimsDirtied(entries);
        return;
    }

    TRACE_FUNCTION();

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
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
 : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
 , _info(std::make_shared<_Info>())
{
}

HdsiSceneMaterialPruningSceneIndex::
~HdsiSceneMaterialPruningSceneIndex() = default;

PXR_NAMESPACE_CLOSE_SCOPE
