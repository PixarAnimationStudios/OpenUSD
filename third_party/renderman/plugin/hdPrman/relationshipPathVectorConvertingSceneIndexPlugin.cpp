//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/relationshipPathVectorConvertingSceneIndexPlugin.h"

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/volumeSchema.h"

#include "pxr/usdImaging/usdImaging/renderSettingsAdapter.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/pxr.h"

#include <prmanapi.h>

#include <oneapi/tbb/concurrent_unordered_map.h>

#include <iterator>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (filters)
    (isLight)
    ((outputsRiDisplayFilters, "outputs:ri:displayFilters"))
    ((outputsRiIntegrator, "outputs:ri:integrator"))
    ((outputsRiSampleFilters, "outputs:ri:sampleFilters"))
    (portals)
    ((riDisplayFilters, "ri:displayFilters"))
    ((riEnergyFilters, "ri:energyFilters"))
    ((riIntegrator, "ri:integrator"))
    ((riSampleFilters, "ri:sampleFilters"))
    ((sceneIndexPluginName, "HdPrmanRelationshipPathVectorConvertingSceneIndexPlugin"))
);

namespace {

void _WarnDeprecated(const TfToken& oldName, const TfToken& newName)
{
#if _PRMANAPI_VERSION_MAJOR >= 27
    static const bool warn = TfGetEnvSetting<bool>(
        LEGACY_PXR_RENDER_TERMINALS_API_ALLOWED_AND_WARN);
    if (warn) {
        static tbb::concurrent_unordered_map<
            TfToken, std::once_flag, TfToken::HashFunctor> flags;
        std::call_once(flags[oldName], [&oldName, &newName]() {
            TF_WARN("%s on RenderSettings is deprecated in favor of %s.",
                oldName.GetText(), newName.GetText());
        });
    }
#endif
}

bool
_IsMeshLight(
    const HdSceneIndexPrim& prim)
{
    if ((prim.primType == HdPrimTypeTokens->mesh) ||
        (prim.primType == HdPrimTypeTokens->points) ||
        (prim.primType == HdPrimTypeTokens->volume)) {
        if (auto lightSchema = HdLightSchema::GetFromParent(prim.dataSource)) {
            if (auto ds = HdBoolDataSource::Cast(
#if PXR_VERSION <= 2211
                    lightSchema.GetContainer()->Get(_tokens->isLight))) {
#else
                    lightSchema.GetContainer()->Get(HdTokens->isLight))) {
#endif
                return ds->GetTypedValue(0.0f);
            }
        }
    }
    return false;
}

class _VectorConvertingDataSource final
  : public HdTypedSampledDataSource<VtArray<SdfPath>>
{
public:
    HD_DECLARE_DATASOURCE(_VectorConvertingDataSource);
    _VectorConvertingDataSource() = default;
    using Time = HdSampledDataSource::Time;

    VtValue
    GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtArray<SdfPath>
    GetTypedValue(Time shutterOffset) override
    {
        if (!_sourceDs) { return { }; }
        VtValue val = _sourceDs->GetValue(shutterOffset);
        if (val.IsHolding<VtArray<SdfPath>>()) {
            return val.UncheckedGet<VtArray<SdfPath>>();
        }
        if (val.IsHolding<SdfPathVector>()) {
            const auto& paths = val.UncheckedGet<SdfPathVector>();
            return {
                std::make_move_iterator(paths.begin()),
                std::make_move_iterator(paths.end()) };
        }
        if (val.IsHolding<SdfPath>()) {
            return { val.UncheckedGet<SdfPath>() };
        }
        return { };
    }

    bool
    GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        if (!_sourceDs) { return false; }
        return _sourceDs->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _VectorConvertingDataSource(const HdSampledDataSourceHandle& sourceDs)
      : _sourceDs(sourceDs)
    { }

    HdSampledDataSourceHandle _sourceDs;
};

class _VectorConvertingContainerDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_VectorConvertingContainerDataSource);

    std::vector<TfToken>
    GetNames() override
    {
        if (!_sourceDs) { return { }; }
        std::vector<TfToken> names = _sourceDs->GetNames();
        if (_remap.empty()) { return names; }
        TfToken::Set namesSet {
            std::make_move_iterator(names.begin()),
            std::make_move_iterator(names.end()) };
        for (const auto& pair : _remap) {
            if (namesSet.count(pair.first) > 0) {
                namesSet.insert(pair.second);
                namesSet.erase(pair.first);
            }
        }
        return {
            std::make_move_iterator(namesSet.begin()),
            std::make_move_iterator(namesSet.end()) };
    }

    HdDataSourceBaseHandle
    Get(const TfToken& name) override
    {
        if (!_sourceDs) { return nullptr; }
        HdDataSourceBaseHandle result = _sourceDs->Get(name);
        if (!result) {
            auto it = _pamer.find(name);
            if (it != _pamer.end()) {
                _WarnDeprecated(name, it->second);
                result = _sourceDs->Get(it->second);
            }
        }
        if (_names.count(name) > 0) {
            if (auto ds = HdSampledDataSource::Cast(result)) {
                result = _VectorConvertingDataSource::New(ds);
            }
        }
        return result;
    }

private:
    _VectorConvertingContainerDataSource(
        const HdContainerDataSourceHandle& sourceDs,
        const std::vector<TfToken>& names = { },
        const std::vector<std::pair<TfToken, TfToken>>& remap = { })
      : _sourceDs(sourceDs)
      , _names(names.begin(), names.end())
      , _remap(remap.begin(), remap.end())
    {
        for (const std::pair<TfToken, TfToken>& pair : remap) {
            _pamer.insert({ pair.second, pair.first });
        }
    }

    HdContainerDataSourceHandle _sourceDs;
    TfToken::Set _names;
    std::unordered_map<TfToken, TfToken, TfToken::HashFunctor> _remap;
    std::unordered_map<TfToken, TfToken, TfToken::HashFunctor> _pamer;
};

class _RenderSettingsContainerDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderSettingsContainerDataSource);

    std::vector<TfToken>
    GetNames() override
    {
        if (!_sourceDs) { return { }; }
        return _sourceDs->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken& name) override
    {
        if (!_sourceDs) { return nullptr; }
        HdDataSourceBaseHandle result = _sourceDs->Get(name);
        if (name == HdRenderSettingsSchemaTokens->namespacedSettings) {
            static const std::vector<TfToken> names {
                _tokens->riDisplayFilters,
                _tokens->riEnergyFilters,
                _tokens->riIntegrator,
                _tokens->riSampleFilters };
            static const bool schemaHasRelationships =
                _PRMANAPI_VERSION_MAJOR_ >= 27;
            static const bool enableRemap = TfGetEnvSetting<bool>(
                LEGACY_PXR_RENDER_TERMINALS_API_ALLOWED_AND_WARN);
            if (!schemaHasRelationships || enableRemap) {
                static const std::vector<std::pair<TfToken, TfToken>> remap {
                    {   _tokens->outputsRiDisplayFilters,
                        _tokens->riDisplayFilters },
                    {   _tokens->outputsRiIntegrator,
                        _tokens->riIntegrator },
                    {   _tokens->outputsRiSampleFilters,
                        _tokens->riSampleFilters } };
                if (auto ds = HdContainerDataSource::Cast(result)) {
                    result = _VectorConvertingContainerDataSource::New(
                        ds, names, remap);
                }
            } else if (auto ds = HdContainerDataSource::Cast(result)) {
                result = _VectorConvertingContainerDataSource::New(ds, names);
            }
        }
        return result;
    }

private:
    _RenderSettingsContainerDataSource(
        const HdContainerDataSourceHandle& sourceDs)
      : _sourceDs(sourceDs)
    { }

    HdContainerDataSourceHandle _sourceDs;
};

class _PrimContainerDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimContainerDataSource);

    std::vector<TfToken>
    GetNames() override
    {
        if (!_sourceDs) { return { }; }
        return _sourceDs->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken& name) override
    {
        if (!_sourceDs) { return nullptr; }
        HdDataSourceBaseHandle result = _sourceDs->Get(name);
        if (auto ds = HdContainerDataSource::Cast(result)) {
            if (name == HdRenderSettingsSchema::GetSchemaToken()) {
                result = _RenderSettingsContainerDataSource::New(ds);
            } else if (name == HdLightSchema::GetSchemaToken()) {
                static const std::vector<TfToken> names {
                    _tokens->filters,
                    _tokens->portals };
                result = _VectorConvertingContainerDataSource::New(ds, names);
            } else if (name == HdVolumeSchema::GetSchemaToken()) {
                static const std::vector<TfToken> names {
                    _tokens->filters };
                result = _VectorConvertingContainerDataSource::New(ds, names);
            }
        }
        return result;
    }

private:
    _PrimContainerDataSource(
        const HdContainerDataSourceHandle& sourceDs)
      : _sourceDs(sourceDs)
    { }

    HdContainerDataSourceHandle _sourceDs;
};

TF_DECLARE_REF_PTRS(_RelationshipPathVectorConvertingSceneIndex);
class _RelationshipPathVectorConvertingSceneIndex final
  : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _RelationshipPathVectorConvertingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _RelationshipPathVectorConvertingSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim
    GetPrim(const SdfPath& primPath) const override
    {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (prim.primType == HdPrimTypeTokens->renderSettings ||
            prim.primType == HdPrimTypeTokens->volume ||
            HdPrimTypeIsLight(prim.primType) ||
            _IsMeshLight(prim)) {
            prim.dataSource = _PrimContainerDataSource::New(prim.dataSource);
        }
        return prim;
    }

    SdfPathVector
    GetChildPrimPaths(const SdfPath& primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _RelationshipPathVectorConvertingSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    { }

    void _PrimsAdded(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override
    {
        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override
    {
        _SendPrimsRemoved(entries);
    }


    void _PrimsDirtied(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override
    {
        static const std::vector<
            std::pair<HdDataSourceLocator, HdDataSourceLocator>> mappings {
            {
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->outputsRiDisplayFilters),
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->riDisplayFilters) },
            {
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->outputsRiIntegrator),
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->riIntegrator) },
            {
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->outputsRiSampleFilters),
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->riSampleFilters) } };
        HdSceneIndexObserver::DirtiedPrimEntries dirtied;
        for (const auto *it = entries.begin(); it != entries.end(); ++it) {
            HdSceneIndexObserver::DirtiedPrimEntry tempEntry = *it;
            for (const auto& mapping : mappings) {
                if (tempEntry.dirtyLocators.Contains(mapping.first) &&
                    !tempEntry.dirtyLocators.Contains(mapping.second)) {
                    tempEntry.dirtyLocators.insert(mapping.second);
                }
            }
            if (tempEntry.dirtyLocators != it->dirtyLocators) {
                if (dirtied.empty()) {
                    dirtied = { entries.begin(), it };
                }
                dirtied.push_back(tempEntry);
            } else if (!dirtied.empty()) {
                dirtied.push_back(*it);
            }
        }
        if (dirtied.empty()) {
            _SendPrimsDirtied(entries);
        } else {
            _SendPrimsDirtied(dirtied);
        }
    }
};

} // anonymous namespace

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrmanRelationshipPathVectorConvertingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;
    for (const auto& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            /* inputArgs = */ nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
    }
}

HdPrmanRelationshipPathVectorConvertingSceneIndexPlugin::
HdPrmanRelationshipPathVectorConvertingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrmanRelationshipPathVectorConvertingSceneIndexPlugin::
_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& /* inputArgs */)
{
    HdSceneIndexBaseRefPtr si = inputScene;
    si = _RelationshipPathVectorConvertingSceneIndex::New(si);
    return si;
}

// XXX: IsEnabled() introduced in HD_API_VERSION 94 (PXR_VERSION 2605),
// so for anything before that, this plugin is always enabled. This is
// correct. The plugin should only be disabled when both are true:
//
// PXR_VERSION >= 2702
// The true USD discriminator is USDIMAGING_API_VERSION >= 28, but
// hdPrman should not link usdImaging directly. It still does, but
// we're working towards removing that direct link, so we won't add
// to it for this.
//
// _PRMANAPI_VERSION_MAJOR_ >= 27
// We still need this plugin for rman < 27 since it also handles
// converting requests for the namespaced parameters to the older
// outputs:ri: form found in the schemas for earlier rman versions.
#if HD_API_VERSION >= 94
bool
HdPrmanRelationshipPathVectorConvertingSceneIndexPlugin::
_IsEnabled(const HdContainerDataSourceHandle& /* inputArgs */) const
{

#if PXR_VERSION < 2702 || _PRMANAPI_VERSION_MAJOR_ < 27
    return true;
#else
    return false;
#endif
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE
