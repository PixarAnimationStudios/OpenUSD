//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/renderTerminalOutputInvalidatingSceneIndexPlugin.h"

#include "pxr/pxr.h"

#if PXR_VERSION >= 2308

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/displayFilterSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/integratorSchema.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sampleFilterSchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/work/loops.h"

#include <cstddef>
#include <iterator>
#include <oneapi/tbb/concurrent_map.h>
#include <oneapi/tbb/concurrent_unordered_map.h>
#include <oneapi/tbb/concurrent_unordered_set.h>
#include <unordered_map>
#include <vector>

#if PXR_VERSION >= 2607
#include "pxr/imaging/hd/energyFilterSchema.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin"))
    ((riIntegrator, "ri:integrator"))
    ((riSampleFilters, "ri:sampleFilters"))
    ((riDisplayFilters, "ri:displayFilters"))
    ((riEnergyFilters, "ri:energyFilters"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 1000;

    for( auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        pluginDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
    }
}

HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin::
HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin() = default;


namespace {

// Gets the given locator from the given data source under the assumption that
// the locator points to a data source for a VtArray<SdfPath>.
VtArray<SdfPath>
_GetPaths(
    const HdContainerDataSourceHandle& rsPrimDs,
    const HdDataSourceLocator& locator)
{
    const auto ds = HdPathArrayDataSource::Cast(
        HdContainerDataSource::Get(rsPrimDs, locator));
    if (!ds) { return { }; }
    // These need to be read during notice processing when no shutterOffset
    // is available. This is okay because these values are never time-varying.
    return ds->GetTypedValue(0.f);
}

TF_DECLARE_REF_PTRS(_HdPrmanRenderTerminalOutputInvalidatingSceneIndex);

////////////////////////////////////////////////////////////////////////////////
/// \class _HdPrmanRenderTerminalOutputInvalidatingSceneIndex
///
/// The scene index feeding into HdDependencyForwardingSceneIndex and
/// constructed by the HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin.
///

class _HdPrmanRenderTerminalOutputInvalidatingSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _HdPrmanRenderTerminalOutputInvalidatingSceneIndexRefPtr
        New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _HdPrmanRenderTerminalOutputInvalidatingSceneIndex(
                inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetPrim(primPath);
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

private:
    using _DirtiedEntryMap = std::unordered_map<
        SdfPath, HdDataSourceLocatorSet, SdfPath::Hash>;

    struct _TargetType
    {
        TfToken primType;
        HdDataSourceLocator terminalLocator;
        HdDataSourceLocator targetLocator;
    };

    static const std::vector<_TargetType>&
    _GetTargetTypes()
    {
        static const std::vector<_TargetType> targetTypes {
            {   HdPrimTypeTokens->displayFilter,
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->riDisplayFilters),
                HdDisplayFilterSchema::GetDefaultLocator() },
#if PXR_VERSION >= 2607
            {   HdPrimTypeTokens->energyFilter,
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->riEnergyFilters),
                HdEnergyFilterSchema::GetDefaultLocator() },
#endif
            {   HdPrimTypeTokens->integrator,
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->riIntegrator),
                HdIntegratorSchema::GetDefaultLocator() },
            {   HdPrimTypeTokens->sampleFilter,
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->riSampleFilters),
                HdSampleFilterSchema::GetDefaultLocator() } };
        return targetTypes;
    }

protected:
    _HdPrmanRenderTerminalOutputInvalidatingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override
    {
        if (!_IsObserved()) {
            return;
        }

        using EntryType = HdSceneIndexObserver::AddedPrimEntry;

        // Parallel gather of all renderSettings, integrator, displayFilter,
        // sampleFilter, and energyFilter prims in the batch
        static const TfToken::Set allTypes {
            HdPrimTypeTokens->displayFilter,
            HdPrimTypeTokens->energyFilter,
            HdPrimTypeTokens->integrator,
            HdPrimTypeTokens->renderSettings,
            HdPrimTypeTokens->sampleFilter };
        using _PathSet = tbb::concurrent_unordered_set<SdfPath, SdfPath::Hash>;
        using _TypeMap = tbb::concurrent_unordered_map<
            TfToken, _PathSet, TfToken::HashFunctor>;
        _TypeMap prims;
        WorkParallelForN(
            entries.size(),
            [&entries, &prims](size_t begin, size_t end) {
                for (size_t i = begin; i < end; ++i) {
                    const EntryType& entry = entries[i];
                    if (allTypes.count(entry.primType) > 0) {
                        prims[entry.primType].insert(entry.primPath);
                    }
                }
            },
            1024);

        // Serial creation of additional dirty notices for the targeted prims.
        // We iterate only over all renderSettings prims in the batch, so
        // we only expect a small number.
        _DirtiedEntryMap dirtied;
        const _PathSet& rsPrims = prims[HdPrimTypeTokens->renderSettings];
        const std::vector<_TargetType>& targetTypes = _GetTargetTypes();
        for (const SdfPath& rsPrimPath : rsPrims) {
            const HdSceneIndexPrim rsPrim =
                _GetInputSceneIndex()->GetPrim(rsPrimPath);
            for (const _TargetType& type : targetTypes) {
                VtArray<SdfPath> paths =
                    _GetPaths(rsPrim.dataSource, type.terminalLocator);
                for (const SdfPath& path : paths) {
                    // No need to generate a dirty notice for a prim that's
                    // already being added in this batch
                    if (!path.IsEmpty() &&
                        !prims[type.primType].contains(path)) {
                        dirtied[path].insert(type.targetLocator);
                    }
                }
            }
        }

        _SendPrimsAdded(entries);
        if (!dirtied.empty()) {
            HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;
            dirtiedEntries.reserve(dirtied.size());
            for (const auto& pair : dirtied) {
                dirtiedEntries.emplace_back(pair.first, pair.second);
            }
            _SendPrimsDirtied(dirtiedEntries);
        }
    }

    void _PrimsRemoved(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override
    {
        if (!_IsObserved()) {
            return;
        }

        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override
    {
        HD_TRACE_FUNCTION();

        if (!_IsObserved()) {
            return;
        }

        using EntryType = HdSceneIndexObserver::DirtiedPrimEntry;

        // We do this serially here because we (perhaps optimistically) expect
        // a well-behaved upstream that doesn't send universal set dirty notices
        // and a typical scene with no more than a handful of renderSettings
        // prims. If lots of prims are getting invalidated upstream with the
        // universal locator set, or if the scene has thousands of
        // renderSettings prims all being dirtied at once, this will bottleneck.
        _DirtiedEntryMap dirtied;
        const std::vector<_TargetType>& targetTypes = _GetTargetTypes();
        HdDataSourceLocatorSet filterSet;
        for (const _TargetType& type : targetTypes) {
            filterSet.insert(type.terminalLocator);
        }
        for (const EntryType& entry : entries) {
            // Only renderSettings prims are expected to have dirty locator sets
            // that intersect the terminal locators, but there is danger here.
            // Any non-renderSettings prims invalidated upstream with the
            // universal locator set will also hit on the intersection check
            // and we end up calling GetPrim() on them unnecessarily. Sending
            // dirty universal set (rather than a resyncing added notice) is not
            // standard and discouraged, but we do know that it happens anyway.
            // Only way to avoid this would be to keep and manage a prim type
            // cache via PrimsAdded() and PrimsRemoved().
            if (!entry.dirtyLocators.Intersects(filterSet)) { continue; }
            HdSceneIndexPrim prim =
                _GetInputSceneIndex()->GetPrim(entry.primPath);
            for (const _TargetType& type : targetTypes) {
                if (entry.dirtyLocators.Contains(type.terminalLocator)) {
                    VtArray<SdfPath> paths =
                        _GetPaths(prim.dataSource, type.terminalLocator);
                    for (const SdfPath& path : paths) {
                        dirtied[path].insert(type.targetLocator);
                    }
                }
            }
        }

        if (dirtied.empty()) {
            _SendPrimsDirtied(entries);
        } else {
            for (const EntryType& entry : entries) {
                dirtied[entry.primPath].insert(entry.dirtyLocators);
            }
            HdSceneIndexObserver::DirtiedPrimEntries allEntries;
            allEntries.reserve(dirtied.size());
            for (const auto& pair : dirtied) {
                allEntries.emplace_back(pair.first, pair.second);
            }
            _SendPrimsDirtied(allEntries);
        }
    }
};

}

HdSceneIndexBaseRefPtr
HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _HdPrmanRenderTerminalOutputInvalidatingSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308
