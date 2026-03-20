//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexUtil.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/trace/trace.h"

#include <iterator>
#include <map>
#include <ostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_SCENE_INDEX_PLUGIN_ORDERING_POLICY_DEFAULT,
    "CppRegistrationOnly",
    "Default policy for ordering scene index plugins. Options are: "
    "{CppRegistrationOnly, JsonMetadataOnly, Hybrid}. "
    "The default is CppRegistrationOnly, which means the order of plugins is "
    "determined solely by the insertion phase/order arguments of the C++ "
    "registration API."
);

TF_DEFINE_PUBLIC_TOKENS(HdSceneIndexPluginRegistryTokens,
    HDSCENEINDEXPLUGINREGISTRY_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _jsonTokens,
    (loadWithRenderer)
    (loadWithApps)
    (tags)
    (ordering)
        (before)
        (after)
    (position)
        (firstAfter)
        (lastBefore)
        (doesNotMatter)
);

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(
        HdSceneIndexPluginRegistry::PluginOrderingPolicy::CppRegistrationOnly,
        "CppRegistrationOnly");
    TF_ADD_ENUM_NAME(
        HdSceneIndexPluginRegistry::PluginOrderingPolicy::JsonMetadataOnly,
        "JsonMetadataOnly");
    TF_ADD_ENUM_NAME(
        HdSceneIndexPluginRegistry::PluginOrderingPolicy::Hybrid,
        "Hybrid");
}

TF_INSTANTIATE_SINGLETON(HdSceneIndexPluginRegistry);

// Types and aliases used in the implementation of HdSceneIndexPluginRegistry.
namespace
{
using InsertionPhase = HdSceneIndexPluginRegistry::InsertionPhase;
using InsertionOrder = HdSceneIndexPluginRegistry::InsertionOrder;
using SceneIndexAppendCallback =
    HdSceneIndexPluginRegistry::SceneIndexAppendCallback;
using PluginInsertionMetadata =
    HdSceneIndexPluginRegistry::PluginInsertionMetadata;
using PluginOrderingPolicy = HdSceneIndexPluginRegistry::PluginOrderingPolicy;

PluginOrderingPolicy
_GetDefaultOrderingPolicyFromEnv()
{
    bool found = false;
    auto orderingPolicy =
        TfEnum::GetValueFromName<PluginOrderingPolicy>(
            TfGetEnvSetting(HD_SCENE_INDEX_PLUGIN_ORDERING_POLICY_DEFAULT),
            &found);

    if (found) {
        return orderingPolicy;
    }

    TF_CODING_ERROR(
        "Invalid value '%s' for environment variable "
        "HD_SCENE_INDEX_PLUGIN_ORDERING_POLICY_DEFAULT."
        "Expected one of: {CppRegistrationOnly, JsonMetadataOnly, Hybrid}. "
        "Using default policy CppRegistrationOnly.",
        TfGetEnvSetting(HD_SCENE_INDEX_PLUGIN_ORDERING_POLICY_DEFAULT).c_str());
    return PluginOrderingPolicy::CppRegistrationOnly;
}

// Entry for a plugin/callback-scene-index that's registered via the C++
// registration API.
struct _RegEntry
{
    _RegEntry(
        const TfToken &sceneIndexPluginId,
        const HdContainerDataSourceHandle &args,
        InsertionPhase phase,
        InsertionOrder order)
    : sceneIndexPluginId(sceneIndexPluginId)
    , args(args)
    , phase(phase)
    , order(order)
    {}

    _RegEntry(
        SceneIndexAppendCallback callback,
        const HdContainerDataSourceHandle &args,
        InsertionPhase phase,
        InsertionOrder order)
    : args(args)
    , callback(callback)
    , phase(phase)
    , order(order)
    {}

    TfToken sceneIndexPluginId;
    HdContainerDataSourceHandle args;
    SceneIndexAppendCallback callback;
    InsertionPhase phase;
    InsertionOrder order;
};

using _RegEntryList = std::vector<_RegEntry>;
using _RegEntriesByRendererMap = std::map<std::string, _RegEntryList>;

// Entry corresponding to the JSON metadata for a plugin discovered via Plug.
// The renderer string is used as the key in the map of entries by renderer.
// (see _JsonEntriesByRendererMap below).
struct _JsonEntry
{
    enum class _InsertionPosition
    {
        FirstAfter,
        DoesNotMatter,
        LastBefore
    };
    struct _Ordering
    {
        // Tags for other plugins that this plugin should be ordered after.
        TfTokenVector afterTags;
        // Tags for other plugins that this plugin should be ordered before.
        TfTokenVector beforeTags;

        // The tags above partition the plugins into three groups (indicated
        // by [] below):
        // [afterTags] -> [... -> this plugin -> ...] -> [beforeTags]
        // The insertion position specifies the ordering of this plugin 
        // within the middle group of plugins.
        _InsertionPosition position = _InsertionPosition::DoesNotMatter;
    };

    static TfTokenVector
    _PopulateTags(const TfTokenVector &tags, const TfToken &pluginId)
    {
        if (std::find(tags.begin(), tags.end(), pluginId) == tags.end()) {
            // Add pluginId as a "default" tag.
            TfTokenVector result(tags.begin(), tags.end());
            result.push_back(pluginId);
            return result;
        }
        return tags;
    }

    _JsonEntry(
        const TfToken &pluginId,
        const TfTokenVector &tags,
        const _Ordering &ordering,
        bool addDefaultTag = false)
    : sceneIndexPluginId(pluginId)
    , tags(addDefaultTag ? _PopulateTags(tags, pluginId) : tags)
    , ordering(ordering)
    {}

    TfToken sceneIndexPluginId;
    TfTokenVector tags;
    _Ordering ordering;
};

std::ostream &
operator<<(std::ostream &out, const TfTokenVector &tokens)
{
    out << "{";
    for (size_t i = 0; i < tokens.size(); ++i) {
        out << tokens[i];
        if (i < tokens.size() - 1) {
            out << ", ";
        }
    }
    out << "}";
    return out;
}

std::ostream &
operator<<(std::ostream &out, const _JsonEntry::_InsertionPosition &position)
{
    out << "position: ";
    switch (position) {
        case _JsonEntry::_InsertionPosition::FirstAfter:
            out << "FirstAfter";
            break;
        case _JsonEntry::_InsertionPosition::DoesNotMatter:
            out << "DoesNotMatter";
            break;
        case _JsonEntry::_InsertionPosition::LastBefore:
            out << "LastBefore";
            break;
    }
    return out;
}

std::ostream &
operator<<(std::ostream &out, const _JsonEntry &entry)
{
    out << "plugin: " << entry.sceneIndexPluginId
        << "\n\t tags " << entry.tags
        << "\n\t insert after " << entry.ordering.afterTags
        << "\n\t insert before " << entry.ordering.beforeTags
        << "\n\t insertion position" << entry.ordering.position
        << "\n";
    return out;
}

using _JsonEntryList = std::vector<_JsonEntry>;
using _JsonEntriesByRendererMap = std::map<std::string, _JsonEntryList>;

// An entry composed of fields from both _RegEntry and _JsonEntry. This provides
// a unified entry type that can be returned for all ordering policies, and is
// the return type of ComputeOrderedEntriesForRenderer.
//
struct _ResolvedEntry
{
    explicit _ResolvedEntry(const _RegEntry &regEntry)
    : sceneIndexPluginId(regEntry.sceneIndexPluginId)
    , args(regEntry.args)
    , callback(regEntry.callback)
    , phase(regEntry.phase)
    , order(regEntry.order)
    {}

    explicit _ResolvedEntry(const _JsonEntry &jsonEntry)
    : sceneIndexPluginId(jsonEntry.sceneIndexPluginId)
    , tags(jsonEntry.tags)
    , ordering(jsonEntry.ordering)
    {}

    TfToken sceneIndexPluginId;

    // Registration entry fields.
    HdContainerDataSourceHandle args;
    SceneIndexAppendCallback callback;
    InsertionPhase phase;
    InsertionOrder order;

    // JSON metadata entry fields.
    TfTokenVector tags;
    _JsonEntry::_Ordering ordering;

    // A hybrid entry may have both sets of fields.
};

std::ostream &
operator<<(std::ostream &out, const _ResolvedEntry &entry)
{
    out << "plugin: " << entry.sceneIndexPluginId
        << "\n\t phase: " << entry.phase
        << "\n\t order: " << entry.order
        << "\n\t tags " << entry.tags
        << "\n\t insert after " << entry.ordering.afterTags
        << "\n\t insert before " << entry.ordering.beforeTags
        << "\n\t insertion position" << entry.ordering.position
        << "\n";
    return out;
}

using _ResolvedEntryList = std::vector<_ResolvedEntry>;

// Used to track plugins whose plugInfo entries contain "loadWithRenderer"
// values to load when the specified renderer or renderers are used.
// Loading the plug-in allows for further registration code to run when
// a plug-in wouldn't be loaded elsewhere.
using _PreloadMap = std::map<std::string, TfTokenVector>;

// Used to track app-name-based filtering for plugin loading. If a plugin
// declares "loadWithApps" in its plugInfo, the plugin will appear in this
// map. When a plugin is in this map, its library will only be loaded if
// the appName provided to AppendSceneIndexes is in the list of
// preloadInApps for the plugin.
using _EnabledAppsMap = std::map<TfToken, std::set<std::string>>;

using _MetadataMap = std::map<
    TfWeakPtr<HdSceneIndexBase>, PluginInsertionMetadata>;

using _VisitedSet = std::set<HdSceneIndexBaseRefPtr>;

using _StringPair = std::pair<std::string, std::string>;

} // anon

struct HdSceneIndexPluginRegistry::_Impl
{
    _Impl()
    : orderingPolicy(_GetDefaultOrderingPolicyFromEnv()) {}

    // -------------------------- Public API ----------------------------------

    // Filters in entries for the specified renderer and app and orders them
    // based on the current ordering policy.
    _ResolvedEntryList
    ComputeOrderedEntriesForRenderer(const _StringPair &rendererAndApp) const;
    
    void
    PopulateMetadata(
        const HdSceneIndexBaseRefPtr& sceneIndex,
        const PluginInsertionMetadata&& metadata,
        _VisitedSet&& visited);

    bool IsPluginRelevantForApp(
        const TfToken& pluginId, const std::string& appName) const;

    // ----------------------------- Data -------------------------------------

    // Updated via RegisterSceneIndexForRenderer calls.
    _RegEntriesByRendererMap regEntriesForRenderers;

    // Updated on plugin discovery.
    _PreloadMap preloadsForRenderers;
    _EnabledAppsMap preloadAppsForPlugins;
    _JsonEntriesByRendererMap jsonEntriesForRenderers;

    // Updated on scene index instantiation.
    _MetadataMap metadataMap;

    // Ordering policy.
    PluginOrderingPolicy orderingPolicy;

private:
    _RegEntryList
    _ComputeOrderedEntriesFromRegistration(
        const _StringPair &rendererAndApp) const;
    
    _ResolvedEntryList
    _ComputeOrderedEntries(
        const _ResolvedEntryList &entries,
        const _StringPair &rendererAndApp /* for debug logging*/) const;

    _ResolvedEntryList
    _ComposeRegistrationAndJsonEntries(
        const _StringPair &rendererAndApp) const;

    _ResolvedEntryList
    _GenerateTagOrderedEntriesFromRegistrationEntries(
        const _StringPair &rendererAndApp) const;

    _RegEntryList
    _GatherRegistrationEntries(
        const _StringPair &rendererAndApp,
        size_t *numEntriesForAllRenderers = nullptr) const;

    _JsonEntryList
    _GatherJsonEntries(
        const _StringPair &rendererAndApp,
        size_t *numEntriesForAllRenderers = nullptr) const;
};

_ResolvedEntryList
HdSceneIndexPluginRegistry::_Impl::ComputeOrderedEntriesForRenderer(
    const _StringPair& rendererAndApp) const
{
    switch (orderingPolicy) {

        case PluginOrderingPolicy::JsonMetadataOnly:
        {
            const auto jsonEntries = _GatherJsonEntries(rendererAndApp);
            return _ComputeOrderedEntries(
                _ResolvedEntryList(jsonEntries.begin(), jsonEntries.end()),
                rendererAndApp);
        }

        case PluginOrderingPolicy::Hybrid:
        {
            return _ComputeOrderedEntries(
                _ComposeRegistrationAndJsonEntries(rendererAndApp),
                rendererAndApp);
        }

        case PluginOrderingPolicy::CppRegistrationOnly:
        default:
            const auto regEntries =
                _ComputeOrderedEntriesFromRegistration(rendererAndApp);
            return _ResolvedEntryList(regEntries.begin(), regEntries.end());
    }
}

_RegEntryList
HdSceneIndexPluginRegistry::_Impl::_ComputeOrderedEntriesFromRegistration(
    const _StringPair& rendererAndApp) const
{
    auto sortFn = [](const _RegEntry& a, const _RegEntry& b) {
        if (a.phase != b.phase) {
            return a.phase < b.phase;
        }
        if (a.order != b.order) {
            return a.order < b.order;
        }
        // Sort by plugin id to get a stable order that does not depend on the
        // order in which the plugins are discovered/loaded.
        // Note that callback-registered entries will have an empty plugin id, 
        // so they will be sorted before any entries with a non-empty plugin id.
        return a.sceneIndexPluginId < b.sceneIndexPluginId;
    };

    // Ideally, we honor the insertion order more strictly by merging entries
    // for "all" renderers and the specified renderer together and then sorting
    // that merged list together.
    // But this requires changes to various scene index plugins that have
    // worked-around the existing adhoc-ordering where "all renderers" plugins
    // always run before "specific renderer" plugins for a given phase,
    // regardless of insertion order.
    //
    constexpr bool useIdealImpl = false;
    if constexpr (useIdealImpl) {
        _RegEntryList mergedEntries =
            _GatherRegistrationEntries(rendererAndApp);

        std::sort(mergedEntries.begin(), mergedEntries.end(), sortFn);
        return mergedEntries;
    }

    // Non-ideal ordering implementation:
    size_t numEntriesForAllRenderers = 0;
    _RegEntryList filteredEntries =
        _GatherRegistrationEntries(rendererAndApp, &numEntriesForAllRenderers);
    
    const auto allItBegin = filteredEntries.begin();
    const auto allItEnd = std::next(allItBegin, numEntriesForAllRenderers);
    std::sort(allItBegin, allItEnd, sortFn);

    const auto rendererItBegin = allItEnd;
    const auto rendererItEnd = filteredEntries.end();
    std::sort(rendererItBegin, rendererItEnd, sortFn);

    _RegEntryList mergedEntries;
    mergedEntries.reserve(filteredEntries.size());
    // Merge the sorted lists together, ensuring that entries with the same
    // phase are grouped together with "all renderers" entries coming before
    // "specific renderer" entries (i.e. not strictly honoring insertion 
    // order across the two lists).
    auto it1 = allItBegin;
    auto it2 = rendererItBegin;
    while (it1 != allItEnd && it2 != rendererItEnd) {
        if (it1->phase < it2->phase) {
            mergedEntries.push_back(*it1);
            ++it1;
        } else if (it1->phase > it2->phase) {
            mergedEntries.push_back(*it2);
            ++it2;
        } else {
            mergedEntries.push_back(*it1);
            ++it1;
        }
    }
    mergedEntries.insert(mergedEntries.end(), it1, allItEnd);
    mergedEntries.insert(mergedEntries.end(), it2, rendererItEnd);
    
    return mergedEntries;
}

_ResolvedEntryList
HdSceneIndexPluginRegistry::_Impl::_ComputeOrderedEntries(
    const _ResolvedEntryList& entries,
    const _StringPair& rendererAndApp) const
{
    TRACE_FUNCTION();

    // Build data structures to aid with topological sorting:
    // (1) tag -> list of entry indices with that tag
    std::map<TfToken, std::vector<size_t>> tagToIndices;
    for (size_t i = 0; i < entries.size(); ++i) {
        for (const TfToken& tag : entries[i].tags) {
            tagToIndices[tag].push_back(i);
        }
    }

    // (2) Adjacency list and in-degree count for topological sort.
    // In this scheme, an edge A -> B means "A should come before B".
    // So, in our ordering scheme:
    // * for an entry i with afterTags {A, B}, any entry j tagged A or B
    //   should come before i => edge j -> i
    // * for an entry i with beforeTags {C, D}, any entry j tagged C or D
    //   should come after i => edge i -> j
    //
    std::vector<std::set<size_t>> successors(entries.size());
    std::vector<size_t> inDegree(entries.size(), 0);

    auto addEdge = [&](size_t fromIndex, size_t toIndex) {
        if (fromIndex == toIndex) {
            return;
        }
        if (successors[fromIndex].insert(toIndex).second) {
            inDegree[toIndex]++;
        }
    };

    for (size_t i = 0; i < entries.size(); ++i) {
        const _ResolvedEntry& entry = entries[i];

        for (const TfToken& afterTag : entry.ordering.afterTags) {
            const auto it = tagToIndices.find(afterTag);
            // It could be that there are no entries with the afterTag...
            if (it == tagToIndices.end()) {
                continue;
            }
            for (size_t j : it->second) {
                addEdge(j, i); // j should come before i
            }
        }

        for (const TfToken& beforeTag : entry.ordering.beforeTags) {
            const auto it = tagToIndices.find(beforeTag);
            if (it == tagToIndices.end()) {
                continue;
            }
            for (size_t j : it->second) {
                addEdge(i, j); // i should come before j
            }
        }
    }

    // Topological sort using Kahn's algorithm.
    // First, partition entries into those with zero in-degree that are 
    // connected to other entries (i.e. have non-empty successor sets) and those
    // that are not connected to any other entries (i.e. "islands" with empty 
    // successor sets).
    // Insert the former into a priority queue where the insertion position and
    // plugin id are used to order entries with the same in-degree.
    // After performing the topological sort on the connected entries, add the
    // sorted islands to the end of the sorted list.
    //

    // Use insertion position and if necessary, plugin id, as a tie-breaker to
    // order entries in the zeroInDegreeIndices list.
    static_assert(_JsonEntry::_InsertionPosition::FirstAfter <
                  _JsonEntry::_InsertionPosition::DoesNotMatter &&
                  _JsonEntry::_InsertionPosition::DoesNotMatter <
                  _JsonEntry::_InsertionPosition::LastBefore,
        "The relative ordering of the _InsertionPosition enum values is used as"
        "a tie-breaker in the plugin ordering, so the enum values should be "
        "ordered accordingly."
    );

    auto cmp = [&](size_t a, size_t b) {
        const _ResolvedEntry& entryA = entries[a];
        const _ResolvedEntry& entryB = entries[b];

        if (entryA.ordering.position != entryB.ordering.position) {
            return entryA.ordering.position < entryB.ordering.position;
        }
        return entryA.sceneIndexPluginId < entryB.sceneIndexPluginId;
    };

    // Because the priority queue is a max-heap by default, we invert the 
    // comparison to get the desired ordering.
    auto pqCmp = [&](size_t a, size_t b) {
        return !cmp(a, b);
    };

    std::vector<size_t> islandIndices;
    std::priority_queue<size_t, std::vector<size_t>, decltype(pqCmp)>
        zeroInDegreeQueue(pqCmp);

    for (size_t i = 0; i < entries.size(); ++i) {
        if (inDegree[i] == 0) {
            if (successors[i].empty()) {
                // This entry is not connected to any other entries, so we can
                // put it aside and add it to the end of the sorted list at the
                // end (after all connected entries have been sorted).
                islandIndices.push_back(i);
            } else {
                zeroInDegreeQueue.push(i);
            }
        }
    }

    _ResolvedEntryList sortedEntries;
    sortedEntries.reserve(entries.size());

    while (!zeroInDegreeQueue.empty()) {
        size_t index = zeroInDegreeQueue.top();
        zeroInDegreeQueue.pop();
        sortedEntries.push_back(entries[index]);

        for (size_t successor : successors[index]) {
            inDegree[successor]--;
            if (inDegree[successor] == 0) {
                zeroInDegreeQueue.push(successor);
            }
        }
    }

    // Add sorted island entries to the end of the sorted list.
    std::sort(islandIndices.begin(), islandIndices.end(), cmp);
    for (size_t index : islandIndices) {
        sortedEntries.push_back(entries[index]);
    }

    if (sortedEntries.size() != entries.size()) {
        const auto &[rendererDisplayName, appName] = rendererAndApp;

        TF_WARN(
            "Detected a cycle in the scene index plugin ordering constraints "
            "for renderer '%s' & app '%s'. The ordering constraints will be "
            "ignored, and the plugins will be run in an arbitrary order.\n"
            "Please use the debug flag HD_SCENE_INDEX_PLUGIN_REGISTRY to "
            "get more information.",
            rendererDisplayName.c_str(), appName.c_str());
        
        if (TfDebug::IsEnabled(HD_SCENE_INDEX_PLUGIN_REGISTRY)) {
            std::stringstream ss;
            ss << "Partial ordering generated before detecting the cycle:\n";
            for (const _ResolvedEntry& entry : sortedEntries) {
                ss << entry;
            }
            ss << "\nRemaining entries that were not sorted:\n";
            for (const _ResolvedEntry& entry : entries) {
                if (std::find_if(sortedEntries.begin(), sortedEntries.end(),
                        [&](const _ResolvedEntry& sortedEntry) {
                            return sortedEntry.sceneIndexPluginId ==
                                entry.sceneIndexPluginId;
                        }) == sortedEntries.end()) {
                    ss << entry;
                }
            }

            TfDebug::Helper().Msg("%s", ss.str().c_str());
        }

        // Could consider returning the partially sorted list here instead of 
        // ignoring the ordering constraints entirely...
        return entries;
    }

    return sortedEntries;
}

static TfToken
_CreateTagFromPhase(InsertionPhase phase)
{
    return TfToken(TfStringPrintf("phase%d", phase));
}

_ResolvedEntryList
HdSceneIndexPluginRegistry::_Impl::_ComposeRegistrationAndJsonEntries(
    const _StringPair &rendererAndApp) const
{
    // (1)
    // To realize a hybrid ordering scheme where we attempt to honor *both*
    // the insertion phase/order specified via the C++ registration API and the
    // tags and ordering specified in the JSON metadata, we first create a
    // _ResolvedEntryList from the _RegEntryList by treating the insertion phase
    // as a tag and providing ordering constraints based on the registered
    // insertion phases.
    const _ResolvedEntryList resolvedRegEntries =
        _GenerateTagOrderedEntriesFromRegistrationEntries(rendererAndApp);

    // (2)
    // Gather the (actual) JSON entries.
    const _JsonEntryList jsonEntries = _GatherJsonEntries(rendererAndApp);

    // (3)
    // Compose (1) over (2) to use for topological ordering.
    // The only entries in (1) that won't be in (2) are the callback-based
    // entries. Otherwise, we expect that for any plugin registered via the 
    // C++ API, there should be a corresponding JSON entry.
    // 
    _ResolvedEntryList composedEntries(jsonEntries.begin(), jsonEntries.end());

    // Build index table to quickly get to registration entries by plugin id.
    // While doing so, push any callback-based entries to the composed list
    // since they won't have corresponding JSON entries.
    using _TokenToIndexMap = std::map<TfToken, size_t>;
    _TokenToIndexMap idToRegIndexMap;
    const size_t jsonEntriesCount = jsonEntries.size();
    for (size_t i = 0; i < resolvedRegEntries.size(); ++i) {
        const auto &id = resolvedRegEntries[i].sceneIndexPluginId;
        if (!id.IsEmpty()) {
            idToRegIndexMap[resolvedRegEntries[i].sceneIndexPluginId] = i;
        } else {
            composedEntries.push_back(resolvedRegEntries[i]);
        }
    }
    
    for (size_t i = 0; i < jsonEntriesCount; ++i) {
        _ResolvedEntry &composedEntry = composedEntries[i];

        const auto it = idToRegIndexMap.find(composedEntry.sceneIndexPluginId);
        // This can happen if the plugin doesn't use the C++ registration API.
        if (it == idToRegIndexMap.end()) {
            continue;
        }

        // Merge the manufactured phase tag...
        const auto &regEntry = resolvedRegEntries[it->second];
        if (!TF_VERIFY(regEntry.tags.size() == 1)) {
            continue;
        }
        composedEntry.tags.push_back(regEntry.tags[0]);
        if (!TF_VERIFY(regEntry.ordering.afterTags.size() <= 3)) {
            continue;
        }

        // and order after tag (which may be absent for the first insertion
        // phase) ...
        composedEntry.ordering.afterTags.insert(
            composedEntry.ordering.afterTags.end(),
            regEntry.ordering.afterTags.begin(),
            regEntry.ordering.afterTags.end());
        
        // ... and resolve the insertion position such that the reg entry
        // wins when the JSON entry has a position of DoesNotMatter.
        if (composedEntry.ordering.position ==
            _JsonEntry::_InsertionPosition::DoesNotMatter) {
            composedEntry.ordering.position =
                regEntry.ordering.position;
        }
    }

    return composedEntries;
}

_ResolvedEntryList
HdSceneIndexPluginRegistry::_Impl::
_GenerateTagOrderedEntriesFromRegistrationEntries(
    const _StringPair &rendererAndApp) const
{
    size_t numEntriesForAllRenderers = 0;
    const _RegEntryList regEntries =
        _GatherRegistrationEntries(rendererAndApp, &numEntriesForAllRenderers);
    
    std::set<InsertionPhase> phases;
    for (const _RegEntry& entry : regEntries) {
        phases.insert(entry.phase);
    }

    _ResolvedEntryList resolvedEntries(regEntries.begin(), regEntries.end());

    for (_ResolvedEntry& entry : resolvedEntries) {
        _JsonEntry::_Ordering ordering;

        // An entry with insertion phase P should come after any entry with
        // insertion phase less than P and before any entry with insertion phase
        // greater than P. So, we add "after" constraints for the tag
        // corresponding to the first phase less than P. We don't need to add 
        // "after" constraints for all other phases less than P because the 
        // transitive ordering constraints take care of that.
        // We don't need to add "before" constraints for the next higher
        // insertion phase because the "after" constraints for that phase will 
        // ensure the entry comes before all entries with phases greater than P.
        //
        auto phaseIt = phases.find(entry.phase);
        if (phaseIt != phases.begin()) {
            --phaseIt;
            ordering.afterTags.push_back(_CreateTagFromPhase(*phaseIt));
        }

        // HdSceneIndexPluginRegistry::InsertionOrder does not
        // have a DoesNotMatter value.
        ordering.position =
            entry.order == InsertionOrder::InsertionOrderAtStart
            ? _JsonEntry::_InsertionPosition::FirstAfter
            : _JsonEntry::_InsertionPosition::LastBefore;

        entry.tags = {_CreateTagFromPhase(entry.phase)};
        entry.ordering = std::move(ordering);
    }

    // See relevant note in _ComputeOrderedEntriesFromRegistration.
    constexpr bool idealImpl = false;
    if constexpr (idealImpl) {
        return resolvedEntries;
    }

    if (numEntriesForAllRenderers == 0 ||
        numEntriesForAllRenderers == resolvedEntries.size()) {
        return resolvedEntries;
    }

    // To provide the adhoc ordering where entries for "all renderers" come
    // before entries for the specified renderer,
    // - for "all renderers" entries that have an ordering tag like "phaseN",
    //   add an additional tag "phaseN_".
    // - for "specific renderer" entries, update a tag like "phaseX" to
    //   "phaseX_" and add an ordering constaint to come after "phaseX".
    //
    auto adjustAllRenderersEntry = [](_ResolvedEntry &entry) {
        if (entry.ordering.afterTags.empty()) {
            return;
        }
        TF_VERIFY(entry.ordering.afterTags.size() == 1);
        TfToken &tag = entry.ordering.afterTags[0];
        entry.ordering.afterTags.push_back(TfToken(tag.GetString() + "_"));
    };

    auto adjustSpecificRendererEntry = [](_ResolvedEntry &entry) {
        TF_VERIFY(entry.tags.size() == 1);
        const TfToken oldTag = entry.tags[0];
        entry.tags[0] = TfToken(oldTag.GetString() + "_");
        auto &afterTags = entry.ordering.afterTags;
        if (!afterTags.empty()) {
            afterTags.push_back(TfToken(afterTags[0].GetString() + "_"));
        }
        afterTags.push_back(oldTag);
    };

    for (size_t i = 0; i < numEntriesForAllRenderers; ++i) {
        adjustAllRenderersEntry(resolvedEntries[i]);
    }
    for (size_t i = numEntriesForAllRenderers;
            i < resolvedEntries.size(); ++i) {
        adjustSpecificRendererEntry(resolvedEntries[i]);
    }
    return resolvedEntries;
}

_RegEntryList
HdSceneIndexPluginRegistry::_Impl::_GatherRegistrationEntries(
    const _StringPair &rendererAndApp,
    size_t *numEntriesForAllRenderers /* = nullptr */) const
{
    const auto &[rendererDisplayName, appName] = rendererAndApp;

    _RegEntriesByRendererMap::const_iterator allRenderersIt =
        regEntriesForRenderers.find(
            HdSceneIndexPluginRegistryTokens->allRenderers.GetString());
            
    _RegEntriesByRendererMap::const_iterator rendererIt =
        rendererDisplayName.empty()
        ? regEntriesForRenderers.end()
        : regEntriesForRenderers.find(rendererDisplayName);

    _RegEntryList entries;
    if (allRenderersIt != regEntriesForRenderers.end()) {
        std::copy_if(
            allRenderersIt->second.begin(), allRenderersIt->second.end(),
            std::back_inserter(entries),
            [&](const _RegEntry &entry) {
                return IsPluginRelevantForApp(
                    entry.sceneIndexPluginId, appName);
            }
        );
    }

    if (numEntriesForAllRenderers) {
        *numEntriesForAllRenderers = entries.size();
    }

    if (rendererIt != regEntriesForRenderers.end()) {
        std::copy_if(
            rendererIt->second.begin(), rendererIt->second.end(),
            std::back_inserter(entries),
            [&](const _RegEntry &entry) {
                return IsPluginRelevantForApp(
                    entry.sceneIndexPluginId, appName);
            }
        );
    }
    return entries;
}

_JsonEntryList
HdSceneIndexPluginRegistry::_Impl::_GatherJsonEntries(
    const _StringPair &rendererAndApp,
    size_t *numEntriesForAllRenderers /* = nullptr */) const
{
    const auto &[rendererDisplayName, appName] = rendererAndApp;

    _JsonEntriesByRendererMap::const_iterator allRenderersIt =
        jsonEntriesForRenderers.find(
            HdSceneIndexPluginRegistryTokens->allRenderers.GetString());
    
    _JsonEntriesByRendererMap::const_iterator rendererIt =
        rendererDisplayName.empty()
        ? jsonEntriesForRenderers.end()
        : jsonEntriesForRenderers.find(rendererDisplayName);
    
    // Merge entries for all renderers and the specified renderer.
    _JsonEntryList entries;
    if (allRenderersIt != jsonEntriesForRenderers.end()) {
        std::copy_if(
            allRenderersIt->second.begin(), allRenderersIt->second.end(),
            std::back_inserter(entries),
            [&](const _JsonEntry &entry) {
                return IsPluginRelevantForApp(
                    entry.sceneIndexPluginId, appName);
            }
        );
    }

    if (numEntriesForAllRenderers) {
        *numEntriesForAllRenderers = entries.size();
    }

    if (rendererIt != jsonEntriesForRenderers.end()) {
        std::copy_if(
            rendererIt->second.begin(), rendererIt->second.end(),
            std::back_inserter(entries),
            [&](const _JsonEntry &entry) {
                return IsPluginRelevantForApp(
                    entry.sceneIndexPluginId, appName);
            }
        );
    }
    return entries;
}

void
HdSceneIndexPluginRegistry::_Impl::PopulateMetadata(
    const HdSceneIndexBaseRefPtr& sceneIndex,
    const PluginInsertionMetadata&& metadata,
    _VisitedSet&& visited)
{
    if (visited.count(sceneIndex)) {
        return;
    }
    TfWeakPtr<HdSceneIndexBase> weakSI = TfCreateWeakPtr(&(*sceneIndex));
    metadataMap[weakSI] = metadata;
    visited.insert(sceneIndex);

    if (auto *const encapsulatingSI =
        HdEncapsulatingSceneIndexBase::Cast(sceneIndex)) {
        for (const auto& scene : encapsulatingSI->GetEncapsulatedScenes()) {
            PopulateMetadata(
                scene,
                std::forward<const PluginInsertionMetadata>(metadata),
                std::forward<_VisitedSet>(visited));
        }
    }

    if (const auto filteringSI =
        TfDynamic_cast<HdFilteringSceneIndexBasePtr>(sceneIndex)) {
        for (const auto& scene : filteringSI->GetInputScenes()) {
            PopulateMetadata(
                scene,
                std::forward<const PluginInsertionMetadata>(metadata),
                std::forward<_VisitedSet>(visited));
        }
    }
}

bool
HdSceneIndexPluginRegistry::_Impl::IsPluginRelevantForApp(
    const TfToken& pluginId, const std::string& appName) const
{
    const auto preloadAppsIt = preloadAppsForPlugins.find(pluginId);
    if (preloadAppsIt == preloadAppsForPlugins.end()) {
        // No loadWithApps entry => the plugin is to be loaded for all apps.
        return true;
    }
    const std::set<std::string>& apps = preloadAppsIt->second;
    return apps.empty() || apps.count(appName) > 0;
}

//------------------------------------------------------------------------------

HdSceneIndexPluginRegistry &
HdSceneIndexPluginRegistry::GetInstance()
{
    return TfSingleton<HdSceneIndexPluginRegistry>::GetInstance();
}

HdSceneIndexPluginRegistry::HdSceneIndexPluginRegistry()
: HfPluginRegistry(TfType::Find<HdSceneIndexPlugin>())
, _impl(std::make_unique<_Impl>())
{
    TfSingleton<HdSceneIndexPluginRegistry>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<HdSceneIndexPlugin>();

    // Force discovery at instantiation time
    std::vector<HfPluginDesc> descs;
    HdSceneIndexPluginRegistry::GetInstance().GetPluginDescs(&descs);
}

HdSceneIndexPluginRegistry::~HdSceneIndexPluginRegistry() = default;

HdSceneIndexPlugin *
HdSceneIndexPluginRegistry::_GetSceneIndexPlugin(const TfToken &pluginId)
{
    return static_cast<HdSceneIndexPlugin*>(GetPlugin(pluginId));
}

static
std::string
_GetRendererName(const HdContainerDataSourceHandle& inputArgs)
{
    std::string rendererName;
    if (!inputArgs) {
        return rendererName;
    }
    if (const auto& nameDS = HdStringDataSource::Cast(inputArgs->Get(
        HdSceneIndexPluginRegistryTokens->rendererDisplayName))) {
        rendererName = nameDS->GetTypedValue(0.f);
    }
    return rendererName;
}

HdSceneIndexBaseRefPtr
HdSceneIndexPluginRegistry::AppendSceneIndex(
    const TfToken &sceneIndexPluginId,
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs,
    const std::string &renderInstanceId)
{
    if (HdSceneIndexPlugin *plugin = _GetSceneIndexPlugin(sceneIndexPluginId)) {
        if (!plugin->IsEnabled(inputArgs)) {
            return inputScene;
        }

        HdSceneIndexBaseRefPtr result =
            plugin->AppendSceneIndex(renderInstanceId, inputScene, inputArgs);

        // NOTE: While HfPluginRegistry has a ref count mechanism for
        //       life time of plug-in instances, we don't need them to be
        //       cleaned up -- so we won't manually decrement their ref count
        //ReleasePlugin(plugin);

        const std::string rendererName = _GetRendererName(inputArgs);

        _impl->PopulateMetadata(
            result,
            { rendererName, sceneIndexPluginId, -1 },
            { inputScene });

        return result;
    } else {
        return inputScene;
    }
}

static TfTokenVector
_ToTokenVector(const std::vector<std::string>& strings)
{
    TfTokenVector result;
    result.reserve(strings.size());
    for (const std::string& s : strings) {
        result.push_back(TfToken(s));
    }
    return result;
}

static std::vector<std::string>
_BuildStringVector(const JsValue &value)
{
    if (value.GetType() == JsValue::ArrayType) {
        return value.GetArrayOf<std::string>();
    }
    if (value.GetType() == JsValue::StringType) {
        return { value.GetString() };
    }
    return {};
}

static TfTokenVector
_BuildTokenVector(const JsValue &value)
{
    return _ToTokenVector(_BuildStringVector(value));
}

static _JsonEntry::_Ordering
_GetOrderingFromJsValue(const JsValue &orderingValue, const TfToken &pluginId)
{
    _JsonEntry::_Ordering ordering;
    if (orderingValue.GetType() != JsValue::ObjectType) {
        return ordering;
    }

    const JsValue *val =
        TfMapLookupPtr(orderingValue.GetJsObject(), _jsonTokens->after);
    if (val) {
        ordering.afterTags = _BuildTokenVector(*val);
    }

    val = TfMapLookupPtr(orderingValue.GetJsObject(), _jsonTokens->before);
    if (val) {
        ordering.beforeTags = _BuildTokenVector(*val);
    }
    val = TfMapLookupPtr(
        orderingValue.GetJsObject(), _jsonTokens->position);
    if (val && val->IsString()) {
        const std::string positionStr = val->GetString();
        if (positionStr == _jsonTokens->firstAfter.GetString()) {
            ordering.position =
                _JsonEntry::_InsertionPosition::FirstAfter;
        } else if (positionStr == _jsonTokens->lastBefore.GetString()) {
            ordering.position =
                _JsonEntry::_InsertionPosition::LastBefore;
        } else if (positionStr != _jsonTokens->doesNotMatter.GetString()){
            TF_WARN("Unrecognized position value '%s' in plugInfo "
                    "metadata for scene index plugin %s. Expected '%s', '%s', "
                    "or '%s'. Defaulting to '%s'.",
                    positionStr.c_str(),
                    pluginId.GetText(),
                    _jsonTokens->firstAfter.GetText(),
                    _jsonTokens->lastBefore.GetText(),
                    _jsonTokens->doesNotMatter.GetText(),
                    _jsonTokens->doesNotMatter.GetText());
        }
    }

    return ordering;
}

// The control flow to get here is rather adhoc:
// HdSceneIndexPluginRegistry c'tor
//   > HfPluginRegistry::GetPluginDescs
//     > HfPluginRegistry::DiscoverPlugins
//       > HdSceneIndexPluginRegistry::_CollectAdditionalMetadata
void
HdSceneIndexPluginRegistry::_CollectAdditionalMetadata(
    const PlugRegistry &plugRegistry, const TfType &pluginType)
{
    const TfToken pluginTypeToken(pluginType.GetTypeName());

    // Update renderer -> pluginIds preload map.
    const JsValue &loadWithRendererValue =
        plugRegistry.GetDataFromPluginMetaData(pluginType,
            _jsonTokens->loadWithRenderer);
    const std::vector<std::string> renderers =
        _BuildStringVector(loadWithRendererValue);
    
    for (const std::string& renderer : renderers) {
        _impl->preloadsForRenderers[renderer].push_back(pluginTypeToken);
    }

    // Update pluginId -> apps preload map.
    {
        const JsValue &loadWithAppsValue =
            plugRegistry.GetDataFromPluginMetaData(
                pluginType, _jsonTokens->loadWithApps);
        const auto apps = _BuildStringVector(loadWithAppsValue);
        _impl->preloadAppsForPlugins[pluginTypeToken] =
            std::set<std::string>(apps.begin(), apps.end());
    }

    // Update renderer -> JSON entries map.
    {
        const JsValue &orderingValue =
            plugRegistry.GetDataFromPluginMetaData(pluginType,
                _jsonTokens->ordering);
        _JsonEntry::_Ordering ordering =
            _GetOrderingFromJsValue(orderingValue, pluginTypeToken);

        const JsValue &tagsValue = plugRegistry.GetDataFromPluginMetaData(
            pluginType, _jsonTokens->tags);
        TfTokenVector tags = _BuildTokenVector(tagsValue);

        for (const std::string& renderer : renderers) {
            _impl->jsonEntriesForRenderers[renderer].emplace_back(
                pluginTypeToken, tags, ordering, /*addDefaultTag=*/true);
            
            if (TfDebug::IsEnabled(HD_SCENE_INDEX_PLUGIN_REGISTRY)) {
                const auto rendererString =
                    renderer.empty() ? "all renderers" : renderer;

                std::stringstream ss;
                ss << "Adding JSON entry for "
                   << rendererString << ":\n"
                   << _impl->jsonEntriesForRenderers[renderer].back();

                TfDebug::Helper().Msg("%s", ss.str().c_str());
            }
        }
    }
}

void
HdSceneIndexPluginRegistry::_LoadPluginsForRenderer(
    const std::string &rendererDisplayName,
    const std::string &appName)
{
    // Preload any renderer plug-ins which have been tagged (via plugInfo) to
    // be loaded along with the specified renderer (or any renderer)
    const std::string preloadKeys[] = {
        HdSceneIndexPluginRegistryTokens->allRenderers.GetString(),
        rendererDisplayName,
    };

    auto &preloadsForRenderers = _impl->preloadsForRenderers;
    for (size_t i = 0; i < TfArraySize(preloadKeys); ++i) {
        _PreloadMap::iterator plit = preloadsForRenderers.find(preloadKeys[i]);
        if (plit == preloadsForRenderers.end()) {
            continue;
        }
        TfTokenVector &rendererPlugins = plit->second;

        for (auto iter = rendererPlugins.begin();
                iter != rendererPlugins.end();) {
            const TfToken &id = *iter;

            if (!_impl->IsPluginRelevantForApp(id, appName)) {
                ++iter;
                continue;
            }

            // This only ensures that the plug-in is loaded as the plug-in
            // itself might do further registration relevant to below.
            _GetSceneIndexPlugin(id);

            // Preload only needs to happen once per process. Remove this
            // plugin so we don't try to load it again later.
            iter = rendererPlugins.erase(iter);
        }

        if (rendererPlugins.empty()) {
            // Common case: we loaded all the plugins. We can entirely
            // erase the entry for this renderer from the overall map.
            preloadsForRenderers.erase(plit);
        }
    }
}

HdSceneIndexBaseRefPtr
HdSceneIndexPluginRegistry::AppendSceneIndicesForRenderer(
    const std::string &rendererDisplayName,
    const HdSceneIndexBaseRefPtr &inputScene,
    const std::string &renderInstanceId,
    const std::string &appName)
{
    // We've only discovered scene index plugins (from their plugInfo entries)
    // and collected their metadata at this point.
    // Filter the discovered plugins based on the {renderer, app} and load
    // them. For plugins using the registration API, this will trigger their
    // registration.
    _LoadPluginsForRenderer(rendererDisplayName, appName);

    HdContainerDataSourceHandle underlayArgs =
        HdRetainedContainerDataSource::New(
            HdSceneIndexPluginRegistryTokens->rendererDisplayName,
            HdRetainedTypedSampledDataSource<std::string>::New(
                rendererDisplayName));

    const _ResolvedEntryList orderedEntries =
        _impl->ComputeOrderedEntriesForRenderer({rendererDisplayName, appName});

    HdSceneIndexBaseRefPtr result = inputScene;

    for (const auto &entry : orderedEntries) {
        HdSceneIndexBaseRefPtr input = result;
        HdContainerDataSourceHandle args =
            HdOverlayContainerDataSource::OverlayedContainerDataSources(
                entry.args, underlayArgs);
        
        if (entry.callback) {
            result = entry.callback(renderInstanceId, input, args);
        } else {
            result = AppendSceneIndex(
                entry.sceneIndexPluginId, input, args, renderInstanceId);
        }
        // XXX Update metadata to include tags.
        _impl->PopulateMetadata(
            result,
            { rendererDisplayName, entry.sceneIndexPluginId, entry.phase },
            { input });
    }

    if (TfGetEnvSetting<bool>(HD_USE_ENCAPSULATING_SCENE_INDICES)) {
        result = HdMakeEncapsulatingSceneIndex(
            { inputScene }, result);
        result->SetDisplayName("Scene index plugins");
    }
    return result;
}

void
HdSceneIndexPluginRegistry::RegisterSceneIndexForRenderer(
    const std::string &rendererDisplayName,
    const TfToken &sceneIndexPluginId,
    const HdContainerDataSourceHandle &inputArgs,
    InsertionPhase insertionPhase,
    InsertionOrder insertionOrder)
{
    // Note that we're simply appending to the list here. The ordering
    // will be resolved later.
    _impl->regEntriesForRenderers[rendererDisplayName].emplace_back(
        sceneIndexPluginId, inputArgs, insertionPhase, insertionOrder);
}

void
HdSceneIndexPluginRegistry::RegisterSceneIndexForRenderer(
    const std::string &rendererDisplayName,
    SceneIndexAppendCallback callback,
    const HdContainerDataSourceHandle &inputArgs,
    InsertionPhase insertionPhase,
    InsertionOrder insertionOrder)
{
    // Note that we're simply appending to the list here. The ordering
    // will be resolved later.
    _impl->regEntriesForRenderers[rendererDisplayName].emplace_back(
        callback, inputArgs, insertionPhase, insertionOrder);
}

std::vector<TfToken>
HdSceneIndexPluginRegistry::LoadAndGetSceneIndexPluginIds(
    const std::string& rendererDisplayName, const std::string& appName)
{
    std::vector<TfToken> ret;
    _LoadPluginsForRenderer(rendererDisplayName, appName);
    const _ResolvedEntryList entries =
        _impl->ComputeOrderedEntriesForRenderer({rendererDisplayName, appName});
    
    for (const _ResolvedEntry& entry : entries) {
        if (entry.sceneIndexPluginId.IsEmpty()) {
            // Skip callback-registered entries.
            continue;
        }
        if (HdSceneIndexPlugin *plugin =
                _GetSceneIndexPlugin(entry.sceneIndexPluginId)) {
            if (!plugin->IsEnabled(entry.args)) {
                continue;
            }
            ret.push_back(entry.sceneIndexPluginId);
        }
    }

    return ret;
}

bool
HdSceneIndexPluginRegistry::
GetPluginInsertionMetadataForSceneIndex(
    const HdSceneIndexBaseRefPtr& sceneIndex,
    PluginInsertionMetadata& metadata)
{
    auto &metadataMap = _impl->metadataMap;
    auto it =
        metadataMap.find(TfCreateWeakPtr<HdSceneIndexBase>(&(*sceneIndex)));
    if (it == metadataMap.end()) {
        return false;
    }
    if (!it->first) {
        metadataMap.erase(it);
        return false;
    }
    metadata = it->second;
    return true;
}

void
HdSceneIndexPluginRegistry::SetPluginOrderingPolicy(PluginOrderingPolicy policy)
{
    TF_DEBUG(HD_SCENE_INDEX_PLUGIN_REGISTRY).Msg(
        "Setting scene index plugin ordering policy to %s\n",
        TfEnum::GetName(policy).c_str());

    _impl->orderingPolicy = policy;
}

PXR_NAMESPACE_CLOSE_SCOPE
