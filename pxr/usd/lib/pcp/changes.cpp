//
// Copyright 2016 Pixar
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
/// \file Changes.cpp


#include "pxr/usd/pcp/changes.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/dependencies.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/layerStackRegistry.h"
#include "pxr/usd/pcp/pathTranslation.h"
#include "pxr/usd/pcp/payloadDecorator.h"
#include "pxr/usd/pcp/utils.h"
#include "pxr/usd/sdf/changeList.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/base/tracelite/trace.h"

static
void
Pcp_SubsumeDescendants(SdfPathSet* pathSet)
{
    SdfPathSet::iterator prefixIt = pathSet->begin(), end = pathSet->end();
    while (prefixIt != end) {
        // Find the range of paths under path *prefixIt.
        SdfPathSet::iterator first = prefixIt;
        SdfPathSet::iterator last  = ++first;
        while (last != end and last->HasPrefix(*prefixIt)) {
            ++last;
        }

        // Remove the range.
        pathSet->erase(first, last);

        // Next path is not under previous path.
        prefixIt = last;
    }
}

void
Pcp_SubsumeDescendants(SdfPathSet* pathSet, const SdfPath& prefix)
{
    // Start at first path in pathSet that is prefix or greater.
    SdfPathSet::iterator first = pathSet->lower_bound(prefix); 

    // Scan for next path in pathSet that does not have prefix as a prefix.
    SdfPathSet::iterator last = first;
    SdfPathSet::iterator end = pathSet->end();
    while (last != end and last->HasPrefix(prefix)) {
        ++last;
    }

    // Erase the paths in the range.
    pathSet->erase(first, last);
}

PcpLifeboat::PcpLifeboat()
{
    // Do nothing
}

PcpLifeboat::~PcpLifeboat()
{
    // Do nothing
}

void
PcpLifeboat::Retain(const SdfLayerRefPtr& layer)
{
    _layers.insert(layer);
}

void
PcpLifeboat::Retain(const PcpLayerStackRefPtr& layerStack)
{
    _layerStacks.insert(layerStack);
}

const std::set<PcpLayerStackRefPtr>& 
PcpLifeboat::GetLayerStacks() const
{
    return _layerStacks;
}

void
PcpLifeboat::Swap(PcpLifeboat& other)
{
    std::swap(_layers, other._layers);
    std::swap(_layerStacks, other._layerStacks);
}

PcpChanges::PcpChanges()
{
    // Do nothing
}

PcpChanges::~PcpChanges()
{
    // Do nothing
}

#define PCP_APPEND_DEBUG(...)                       \
    if (not debugSummary) ; else                    \
        *debugSummary += TfStringPrintf(__VA_ARGS__)

enum Pcp_ChangesLayerStackChange {
    Pcp_ChangesLayerStackChangeNone,
    Pcp_ChangesLayerStackChangeSignificant,
    Pcp_ChangesLayerStackChangeMaybeSignificant
};

static
Pcp_ChangesLayerStackChange
Pcp_EntryRequiresLayerStackChange(const SdfChangeList::Entry& entry)
{
    // XXX: This only requires blowing the layer stacks using this
    //      identifier that haven't also been updated to use the new
    //      identifier.
    if (entry.flags.didChangeIdentifier) {
        return Pcp_ChangesLayerStackChangeSignificant;
    }

    // Order of layers in layer stack probably changed.
    // XXX: Don't return true if these changes don't affect the
    //      layer tree order.
    if (entry.infoChanged.count(SdfFieldKeys->Owner) or
        entry.infoChanged.count(SdfFieldKeys->SessionOwner) or
        entry.infoChanged.count(SdfFieldKeys->HasOwnedSubLayers)) {
        return Pcp_ChangesLayerStackChangeSignificant;
    }

    // Layer was added or removed.
    TF_FOR_ALL(i, entry.subLayerChanges) {
        if (i->second == SdfChangeList::SubLayerAdded or
            i->second == SdfChangeList::SubLayerRemoved) {
            // Whether the change is significant depends on whether any
            // added/removed layer is significant.  To check that we need
            // the help of each cache using this layer.
            return Pcp_ChangesLayerStackChangeMaybeSignificant;
        }
    }

    return Pcp_ChangesLayerStackChangeNone;
}

static
bool
Pcp_EntryRequiresLayerStackOffsetsChange(const SdfChangeList::Entry& entry)
{
    TF_FOR_ALL(i, entry.subLayerChanges) {
        if (i->second == SdfChangeList::SubLayerOffset) {
            return true;
        }
    }

    return false;
}

static
bool
Pcp_EntryRequiresPrimIndexChange(const SdfChangeList::Entry& entry)
{
    // Inherits, specializes, reference or variants changed.
    if (entry.flags.didChangePrimInheritPaths or
        entry.flags.didChangePrimSpecializes or
        entry.flags.didChangePrimReferences or
        entry.flags.didChangePrimVariantSets) {
        return true;
    }

    // Payload, permission or variant selection changed.
    // XXX: We don't require a prim graph change if:
    //        we add/remove an unrequested payload;
    //        permissions change doesn't add/remove any specs
    //            that themselves require prim graph changes;
    //        variant selection was invalid and is still invalid.
    if (entry.infoChanged.count(SdfFieldKeys->Payload) or
        entry.infoChanged.count(SdfFieldKeys->Permission) or
        entry.infoChanged.count(SdfFieldKeys->VariantSelection) or
        entry.infoChanged.count(SdfFieldKeys->Instanceable)) {
        return true;
    }

    return false;
}

enum {
    Pcp_EntryChangeSpecsAddInert        = 1,
    Pcp_EntryChangeSpecsRemoveInert     = 2,
    Pcp_EntryChangeSpecsAddNonInert     = 4,
    Pcp_EntryChangeSpecsRemoveNonInert  = 8,
    Pcp_EntryChangeSpecsTargets         = 16,
    Pcp_EntryChangeSpecsConnections     = 32,
    Pcp_EntryChangeSpecsAdd =
        Pcp_EntryChangeSpecsAddInert |
        Pcp_EntryChangeSpecsAddNonInert,
    Pcp_EntryChangeSpecsRemove =
        Pcp_EntryChangeSpecsRemoveInert |
        Pcp_EntryChangeSpecsRemoveNonInert,
    Pcp_EntryChangeSpecsInert =
        Pcp_EntryChangeSpecsAddInert |
        Pcp_EntryChangeSpecsRemoveInert,
    Pcp_EntryChangeSpecsNonInert =
        Pcp_EntryChangeSpecsAddNonInert |
        Pcp_EntryChangeSpecsRemoveNonInert
};

static int
Pcp_EntryRequiresPrimSpecsChange(const SdfChangeList::Entry& entry)
{
    int result = 0;

    result |= entry.flags.didAddInertPrim
              ? Pcp_EntryChangeSpecsAddInert      : 0;
    result |= entry.flags.didRemoveInertPrim
             ? Pcp_EntryChangeSpecsRemoveInert    : 0;
    result |= entry.flags.didAddNonInertPrim
             ? Pcp_EntryChangeSpecsAddNonInert    : 0;
    result |= entry.flags.didRemoveNonInertPrim
             ? Pcp_EntryChangeSpecsRemoveNonInert : 0;

    return result;
}

static int
Pcp_EntryRequiresPropertySpecsChange(const SdfChangeList::Entry& entry)
{
    int result = 0;

    result |= entry.flags.didAddPropertyWithOnlyRequiredFields
              ? Pcp_EntryChangeSpecsAddInert      : 0;
    result |= entry.flags.didRemovePropertyWithOnlyRequiredFields
             ? Pcp_EntryChangeSpecsRemoveInert    : 0;
    result |= entry.flags.didAddProperty
             ? Pcp_EntryChangeSpecsAddNonInert    : 0;
    result |= entry.flags.didRemoveProperty
             ? Pcp_EntryChangeSpecsRemoveNonInert : 0;

    if (entry.flags.didChangeRelationshipTargets) {
        result |= Pcp_EntryChangeSpecsTargets;
    }
    if (entry.flags.didChangeAttributeConnection) {
        result |= Pcp_EntryChangeSpecsConnections;
    }

    return result;
}

static bool
Pcp_EntryRequiresPropertyIndexChange(const SdfChangeList::Entry& entry)
{
    return entry.infoChanged.count(SdfFieldKeys->Permission) != 0;
}

static bool
Pcp_DecoratorRequiresPrimIndexChange(PcpPayloadDecorator* decorator,
                                     const SdfLayerHandle& layer,
                                     const SdfPath& path,
                                     const SdfChangeList::Entry& entry)
{
    if (not decorator) {
        return false;
    }

    TF_FOR_ALL(it, entry.infoChanged) {
        if (decorator->IsFieldRelevantForDecoration(layer, path, it->first)) {
            return true;
        }
    }
    return false;
}

void
PcpChanges::DidChange(const std::vector<PcpCache*>& caches,
                      const SdfLayerChangeListMap& changes)
{
    // LayerStack changes
    static const int LayerStackLayersChange      = 1;
    static const int LayerStackOffsetsChange     = 2;
    static const int LayerStackRelocatesChange   = 4;
    static const int LayerStackSignificantChange = 8;
    typedef int LayerStackChangeBitmask;
    typedef std::map<PcpLayerStackPtr, LayerStackChangeBitmask>
        LayerStackChangeMap;

    // Path changes
    static const int PathChangeSimple      = 1;
    static const int PathChangeTargets     = 2;
    static const int PathChangeConnections = 4;
    typedef int PathChangeBitmask;
    typedef std::map<SdfPath, PathChangeBitmask,
                     SdfPath::FastLessThan> PathChangeMap;
    typedef PathChangeMap::value_type PathChangeValue;

    // Spec changes
    typedef int SpecChangeBitmask;
    typedef std::map<SdfPath, SpecChangeBitmask,
                     SdfPath::FastLessThan> SpecChangesTypes;

    TRACE_FUNCTION();

    SdfPathSet pathsWithSignificantChanges;
    PathChangeMap pathsWithSpecChanges;
    SpecChangesTypes pathsWithSpecChangesTypes;
    SdfPathSet pathsWithRelocatesChanges;
    SdfPathVector oldPaths, newPaths;
    SdfPathSet fallbackToAncestorPaths;

    typedef std::pair<PcpCache*, SdfPath> CacheAndLayerPathPair;
    typedef std::vector<CacheAndLayerPathPair> CacheAndLayerPathPairVector;
    CacheAndLayerPathPairVector pathsWithSignificantChangesByCache;

    // As we process each layer below, we'll look for changes that
    // affect entire layer stacks, then process those in one pass
    // at the end.
    LayerStackChangeMap layerStackChangesMap;

    // Change debugging.
    std::string summary;
    std::string* debugSummary = TfDebug::IsEnabled(PCP_CHANGES) ? &summary : 0;

    PCP_APPEND_DEBUG("  Caches:\n");
    for (PcpCache* cache: caches) {
        PCP_APPEND_DEBUG("    %s\n",
                 TfStringify(cache->GetLayerStack()->GetIdentifier()).c_str());
    }

    // Process all changes, first looping over all layers.
    TF_FOR_ALL(i, changes) {
        const SdfLayerHandle& layer     = TfDynamic_cast<SdfLayerHandle>(i->first);
        const SdfChangeList& changeList = i->second;

        // Find every layer stack in every cache that includes 'layer'.
        // If there aren't any such layer stacks, we can ignore this change.
        typedef std::pair<PcpCache*, PcpLayerStackPtrVector> CacheLayerStacks;
        typedef std::vector<CacheLayerStacks> CacheLayerStacksVector;

        CacheLayerStacksVector cacheLayerStacks;
        for (auto cache : caches) {
            PcpLayerStackPtrVector stacks =
                cache->FindAllLayerStacksUsingLayer(layer);
            if (not stacks.empty()) {
                cacheLayerStacks.push_back(std::make_pair( cache, stacks) );
            }
        }
        if (cacheLayerStacks.empty()) {
            PCP_APPEND_DEBUG("  Layer @%s@ changed:  unused\n",
                             layer->GetIdentifier().c_str());
            continue;
        }

        PCP_APPEND_DEBUG("  Changes to layer %s:\n%s",
                         layer->GetIdentifier().c_str(),
                         TfStringify(changeList).c_str());

        // Reset state.
        LayerStackChangeBitmask layerStackChangeMask = 0;
        pathsWithSignificantChanges.clear();
        pathsWithSpecChanges.clear();
        pathsWithSpecChangesTypes.clear();
        pathsWithRelocatesChanges.clear();
        oldPaths.clear();
        newPaths.clear();
        fallbackToAncestorPaths.clear();
        pathsWithSignificantChangesByCache.clear();

        // Loop over each entry on the layer.
        TF_FOR_ALL(j, changeList.GetEntryList()) {
            const SdfPath& path = j->first;
            const SdfChangeList::Entry& entry = j->second;

            // Figure out for which paths we must fallback to an ancestor.
            // These are the paths where a prim/property was added or
            // removed and any descendant.
            //
            // When adding the first spec for a prim or property, there
            // won't be any dependencies for that object yet, but we still
            // need to figure out the locations that will be affected by
            // the addition of this new object. Hence the need to fallback
            // to an ancestor to synthesize dependencies.
            //
            // When removing a prim or property spec, the fallback ancestor
            // is usually not needed because there should already be
            // dependencies registered for that object. However, in the case
            // where an object is renamed then removed in a single change
            // block, we will need the fallback ancestor because the
            // dependencies at the renamed path will not have been registered
            // yet. The fallback ancestor code won't be run in the usual
            // case anyway, so it's safe to just always set up the fallback
            // ancestor path.
            const bool fallbackToParent = 
                entry.flags.didAddInertPrim or
                entry.flags.didRemoveInertPrim or
                entry.flags.didAddNonInertPrim or
                entry.flags.didRemoveNonInertPrim or
                entry.flags.didAddProperty or
                entry.flags.didRemoveProperty or
                entry.flags.didAddPropertyWithOnlyRequiredFields or
                entry.flags.didRemovePropertyWithOnlyRequiredFields;

            if (fallbackToParent) {
                fallbackToAncestorPaths.insert(path);
            }

            if (path == SdfPath::AbsoluteRootPath()) {
                if (entry.flags.didReplaceContent) {
                    pathsWithSignificantChanges.insert(path);
                }

                // Treat a change to DefaultPrim as a resync
                // of that root prim path.
                SdfChangeList::Entry::InfoChangeMap::const_iterator i =
                    entry.infoChanged.find(
                        SdfFieldKeys->DefaultPrim);
                if (i != entry.infoChanged.end()) {
                    // old value.
                    TfToken token = i->second.first.GetWithDefault<TfToken>();
                    pathsWithSignificantChanges.insert(
                        SdfPath::IsValidIdentifier(token)
                        ? SdfPath::AbsoluteRootPath().AppendChild(token)
                        : SdfPath::AbsoluteRootPath());
                    // new value.
                    token = i->second.second.GetWithDefault<TfToken>();
                    pathsWithSignificantChanges.insert(
                        SdfPath::IsValidIdentifier(token)
                        ? SdfPath::AbsoluteRootPath().AppendChild(token)
                        : SdfPath::AbsoluteRootPath());
                }

                // Handle changes that require blowing the layer stack.
                switch (Pcp_EntryRequiresLayerStackChange(entry)) {
                case Pcp_ChangesLayerStackChangeMaybeSignificant:
                    layerStackChangeMask |= LayerStackLayersChange;
                    TF_FOR_ALL(k, entry.subLayerChanges) {
                        if (k->second == SdfChangeList::SubLayerAdded or
                            k->second == SdfChangeList::SubLayerRemoved) {
                            const std::string& sublayerPath = k->first;
                            const _SublayerChangeType sublayerChange = 
                                k->second == SdfChangeList::SubLayerAdded ? 
                                _SublayerAdded : _SublayerRemoved;

                            TF_FOR_ALL(i, cacheLayerStacks) {
                                bool significant = false;
                                const SdfLayerRefPtr sublayer = 
                                    _LoadSublayerForChange(i->first,
                                                           layer,
                                                           sublayerPath,
                                                           sublayerChange);
                                
                                PCP_APPEND_DEBUG(
                                    "  Layer @%s@ changed sublayers\n",
                                    layer ? 
                                    layer->GetIdentifier().c_str() : "invalid");

                                _DidChangeSublayer(i->first /* cache */,
                                                   i->second /* stack */,
                                                   sublayer,
                                                   sublayerChange,
                                                   debugSummary,
                                                   &significant);
                                if (significant) {
                                    layerStackChangeMask |=
                                        LayerStackSignificantChange;
                                }
                            }
                        }
                    }
                    break;

                case Pcp_ChangesLayerStackChangeSignificant:
                    // Must blow everything
                    layerStackChangeMask |= LayerStackLayersChange |
                                            LayerStackSignificantChange;
                    pathsWithSignificantChanges.insert(path);
                    PCP_APPEND_DEBUG("  Layer @%s@ changed:  significant\n",
                                     layer->GetIdentifier().c_str());
                    break;

                case Pcp_ChangesLayerStackChangeNone:
                    // Layer stack is okay.   Handle changes that require
                    // blowing the layer stack offsets.
                    if (Pcp_EntryRequiresLayerStackOffsetsChange(entry)) {
                        layerStackChangeMask |= LayerStackOffsetsChange;

                        // Layer offsets are folded into the map functions
                        // for arcs that originate from a layer. So, when 
                        // offsets authored in a layer change, all indexes
                        // that depend on that layer must be significantly
                        // resync'd to regenerate those functions.
                        //
                        // XXX: If this becomes a performance issue, we could
                        //      potentially apply the same incremental updating
                        //      that's currently done for relocates.
                        pathsWithSignificantChanges.insert(path);
                        PCP_APPEND_DEBUG("  Layer @%s@ changed:  "
                                         "layer offsets (significant)\n",
                                         layer->GetIdentifier().c_str());
                    }
                    break;
                }
            }

            // Handle changes that require a prim graph change.
            else if (path.IsPrimOrPrimVariantSelectionPath()) {
                if (entry.flags.didRename) {
                    // XXX: We don't have enough info to determine if
                    //      the changes so far are a rename in layer
                    //      stack space.  Renames in Sd are only renames
                    //      in a Pcp layer stack if all specs in the
                    //      layer stack were renamed the same way (for
                    //      and given old path, new path pair).  But we
                    //      don't know which layer stacks to check and
                    //      which caches they belong to.  For example,
                    //      if we rename in a referenced layer stack we
                    //      don't know here what caches are referencing
                    //      that layer stack.
                    //
                    //      In the future we'll probably avoid this
                    //      problem altogether by requiring clients to
                    //      do namespace edits through Csd if they want
                    //      efficient handling.  Csd will be able to
                    //      tell its PcpChanges object about the
                    //      renames directly and we won't do *any*
                    //      *handling of renames in this method.
                    //
                    //      For now we'll just treat renames as resyncs.
                    oldPaths.push_back(entry.oldPath);
                    newPaths.push_back(path);
                    PCP_APPEND_DEBUG("  Renamed @%s@<%s> to <%s>\n",
                                     layer->GetIdentifier().c_str(),
                                     entry.oldPath.GetText(), path.GetText());
                }
                if (int specChanges = Pcp_EntryRequiresPrimSpecsChange(entry)) {
                    pathsWithSpecChangesTypes[path] |= specChanges;
                }
                if (Pcp_EntryRequiresPrimIndexChange(entry)) {
                    pathsWithSignificantChanges.insert(path);
                }
                else {
                    for (const auto& c : cacheLayerStacks) {
                        PcpCache* cache = c.first;
                        if (Pcp_DecoratorRequiresPrimIndexChange(
                                cache->GetPayloadDecorator(), 
                                layer, path, entry)) {

                            pathsWithSignificantChangesByCache.push_back(
                                CacheAndLayerPathPair(cache, path));
                        }
                    }
                }

                if (entry.infoChanged.count(SdfFieldKeys->Relocates)) {
                    layerStackChangeMask |= LayerStackRelocatesChange;
                }
            }

            else if (path.IsPropertyPath()) {
                if (entry.flags.didRename) {
                    // XXX: See the comment above regarding renaming
                    //      prims.
                    oldPaths.push_back(entry.oldPath);
                    newPaths.push_back(path);
                    PCP_APPEND_DEBUG("  Renamed @%s@<%s> to <%s>\n",
                                     layer->GetIdentifier().c_str(),
                                     entry.oldPath.GetText(), path.GetText());
                }
                if (int specChanges =
                        Pcp_EntryRequiresPropertySpecsChange(entry)) {
                    pathsWithSpecChangesTypes[path] |= specChanges;
                }
                if (Pcp_EntryRequiresPropertyIndexChange(entry)) {
                    pathsWithSignificantChanges.insert(path);
                }
            }
            else if (path.IsTargetPath()) {
                if (entry.flags.didAddTarget) {
                    pathsWithSpecChangesTypes[path] |=
                        Pcp_EntryChangeSpecsAddInert;
                }
                if (entry.flags.didRemoveTarget) {
                    pathsWithSpecChangesTypes[path] |=
                        Pcp_EntryChangeSpecsRemoveInert;
                }
            }
        }

        // Push layer stack changes to all layer stacks using this layer.
        if (layerStackChangeMask != 0) {
            TF_FOR_ALL(i, cacheLayerStacks) {
                TF_FOR_ALL(layerStack, i->second) {
                    layerStackChangesMap[*layerStack] |= layerStackChangeMask;
                }
            }
        }

        // Handle spec changes.  What we do depends on what changes happened
        // and the cache at each path.
        //
        //  Prim:
        //     Add first spec   -- prim graph change (1)
        //     Remove last spec -- prim graph change (2)
        //     Add non-inert    -- prim graph change
        //     Remove non-inert -- prim graph change
        //     Add/remove inert -- update specs
        //
        //  Property:
        //     Add/remove inert     -- significant change
        //     Add/remove non-inert -- insignificant change
        //
        // 1) We can't tell here if we're adding the first prim spec because
        // these results are independent of the Pcp caches/layer stacks.  So
        // when adding we assume we might be adding the first spec.  Later
        // we'll check more carefully.
        //
        // 2) We can't tell if we're removing the last prim spec because we
        // don't cache prim stacks.  So we'll just ignore the remove last
        // spec case;  non-inert removes are prim graph changes anyway and
        // inert removes will cause PcpCache::Apply() to check if any
        // specs remain and, if not, blow the prim graph.
        //
        // Note that in the below code, the order of the if statements does
        // matter, as a spec could be added, then removed (for example) within 
        // the same change.
        for (const auto& value : pathsWithSpecChangesTypes) {
            const SdfPath& path = value.first;
            if (path.IsPrimOrPrimVariantSelectionPath()) {
                if (value.second & Pcp_EntryChangeSpecsNonInert) {
                    pathsWithSignificantChanges.insert(path);
                }
                else if (value.second & Pcp_EntryChangeSpecsInert) {
                    pathsWithSpecChanges[path] |= PathChangeSimple;
                }
            }
            else {
                if (value.second & Pcp_EntryChangeSpecsNonInert) {
                    pathsWithSignificantChanges.insert(path);
                }
                else if (value.second & Pcp_EntryChangeSpecsInert) {
                    pathsWithSpecChanges[path] |= PathChangeSimple;
                }

                if (value.second & Pcp_EntryChangeSpecsTargets) {
                    pathsWithSpecChanges[path] |= PathChangeTargets;
                }
                if (value.second & Pcp_EntryChangeSpecsConnections) {
                    pathsWithSpecChanges[path] |= PathChangeConnections;
                }
            }
        }

        // For every path we've found on this layer that has a
        // significant change, find all paths in the cache that use the
        // spec at (layer, path) and mark them as affected.
        for (const auto& path : pathsWithSignificantChanges) {
            const bool onlyExistingDependentPaths =
                fallbackToAncestorPaths.count(path) == 0;
            for (auto cache : caches) {
                _DidChangeDependents(
                    _ChangeTypeSignificant, cache, layer,
                    path, onlyExistingDependentPaths, debugSummary);
            }
        }

        // For every path we've found that has a significant change in
        // a specific cache, use the same logic as above to mark those paths
        // as having a significant change, but only in the associated cache.
        for (const auto& p : pathsWithSignificantChangesByCache) {
            const bool onlyExistingDependentPaths =
                fallbackToAncestorPaths.count(p.second) == 0;
            _DidChangeDependents(
                _ChangeTypeSignificant, p.first, layer,
                p.second, onlyExistingDependentPaths, debugSummary);
        }

        // For every path we've found that has a significant change,
        // check layer stacks that have discovered relocations that
        // could be affected by that change.
        if (not pathsWithSignificantChanges.empty()) {
            // If this scope turns out to be expensive, we should look
            // at switching PcpLayerStack's _relocatesPrimPaths from
            // a std::vector to a path set.  _AddRelocateEditsForLayerStack
            // also does a traversal and might see a similar benefit.
            TRACE_SCOPE("PcpChanges::DidChange -- Checking layer stack "
                        "relocations against significant prim resyncs");

            for(const CacheLayerStacks &i: cacheLayerStacks) {
                if (i.first->_usd) {
                    // No relocations in usd mode
                    continue;
                }
                for(const PcpLayerStackPtr &layerStack: i.second) {
                    const SdfPathVector& reloPaths =
                        layerStack->GetPathsToPrimsWithRelocates();
                    if (reloPaths.empty()) {
                        continue;
                    }
                    for(const SdfPath &changedPath:
                        pathsWithSignificantChanges) {
                        for(const SdfPath &reloPath: reloPaths) {
                            if (reloPath.HasPrefix(changedPath)) {
                                layerStackChangesMap[layerStack]
                                    |= LayerStackRelocatesChange;
                                goto doneWithLayerStack;
                            }
                        }
                    }
                    doneWithLayerStack:
                    ;
                }
            }
        }

        // For every path we've found on this layer that maybe requires
        // rebuilding the property stack based on parent dependencies, find
        // all paths in the cache that use the spec at (layer,path).  If
        // there aren't any then find all paths in the cache that use the
        // parent.  In either case mark the found paths as needing their
        // property spec stacks blown.
        for (const auto& value : pathsWithSpecChanges) {
            const SdfPath& path        = value.first;
            PathChangeBitmask changes = value.second;

            int changeType = 0;
            if (changes & PathChangeTargets) {
                changeType |= _ChangeTypeTargets;
            }
            if (changes & PathChangeConnections) {
                changeType |= _ChangeTypeConnections;
            }

            // If the changes for this path include something other than
            // target changes, they must be spec changes.
            if (changes & ~(PathChangeTargets | PathChangeConnections)) {
                changeType |= _ChangeTypeSpecs;
            }

            for (auto cache : caches) {
                _DidChangeDependents(
                    changeType, cache, layer,
                    path, /* filter */ false, debugSummary);
            }
        }

        // For every path we've found on this layer that was namespace
        // edited, find all paths in the cache that map to the path and
        // the corresponding new path.  Save these internally for later
        // comparison to edits added through DidChangePaths().
        if (not oldPaths.empty()) {
            SdfPathVector depPaths;

            for (auto cache : caches) {
                PcpCacheChanges::PathEditMap& renameChanges =
                    _GetRenameChanges(cache);

                // Do every path.
                for (size_t i = 0, n = oldPaths.size(); i != n; ++i) {
                    const SdfPath& oldPath = oldPaths[i];
                    const SdfPath& newPath = newPaths[i];
                    // Do every path dependent on the new path.  We might
                    // have an object at the new path and we're replacing
                    // it with the object at the old path.  So we must
                    // act as if we're deleting the object at the new path.
                    if (not newPath.IsEmpty()) {
                        PcpDependencyVector deps =
                            cache->FindDependentPaths(
                                layer, newPath,
                                PcpDependencyTypeAnyNonVirtual,
                                /* recurseOnSite */ false,
                                /* recurseOnIndex */ false,
                                /* filter */ true
                            );
                        for (const auto &dep: deps) {
                            renameChanges[dep.indexPath] = SdfPath();
                        }
                    }

                    // Do every path dependent on the old path.
                    PcpDependencyVector deps =
                        cache->FindDependentPaths(
                            layer, oldPath,
                            PcpDependencyTypeAnyNonVirtual,
                            /* recurseOnSite */ false,
                            /* recurseOnIndex */ false,
                            /* filter */ true
                        );
                    for (const auto &dep: deps) {
                        SdfPath newIndexPath;
                        // If this isn't a delete then translate newPath
                        if (not newPath.IsEmpty()) {
                            newIndexPath =
                                dep.mapFunc.MapSourceToTarget(newPath);
                        }
                        renameChanges[dep.indexPath] = newIndexPath;
                        PCP_APPEND_DEBUG("  renameChanges <%s> to <%s>\n",
                             dep.indexPath.GetText(),
                             newIndexPath.GetText());
                    }
                }
            }
        }
    }

    // Process layer stack changes.  This will handle both blowing
    // caches (as needed) for the layer stack contents and offsets,
    // as well as  analyzing relocation changes in the layer stack.
    TF_FOR_ALL(layerStackChange, layerStackChangesMap) {
        _DidChangeLayerStack(
            layerStackChange->first,
            layerStackChange->second & LayerStackLayersChange,
            layerStackChange->second & LayerStackOffsetsChange,
            layerStackChange->second & LayerStackSignificantChange);
        if (layerStackChange->second & LayerStackRelocatesChange) {
            _DidChangeLayerStackRelocations(caches, layerStackChange->first,
                                            debugSummary);
        }
    }

    if (debugSummary and not debugSummary->empty()) {
        TfDebug::Helper().Msg("PcpChanges::DidChange\n%s",
                              debugSummary->c_str());
    }
}

void 
PcpChanges::DidMuteLayer(
    PcpCache* cache, 
    const std::string& layerId)
{
    // Change debugging.
    std::string summary;
    std::string* debugSummary = TfDebug::IsEnabled(PCP_CHANGES) ? &summary : 0;

    const SdfLayerRefPtr mutedLayer = 
        _LoadSublayerForChange(cache, layerId, _SublayerRemoved);
    const PcpLayerStackPtrVector& layerStacks = 
        cache->FindAllLayerStacksUsingLayer(mutedLayer);

    PCP_APPEND_DEBUG("  Did mute layer @%s@\n", layerId.c_str());

    if (not layerStacks.empty()) {
        _DidChangeSublayerAndLayerStacks(cache, layerStacks, mutedLayer,
                                         _SublayerRemoved, debugSummary);
    }

    if (debugSummary and not debugSummary->empty()) {
        TfDebug::Helper().Msg("PcpChanges::DidMuteLayer\n%s",
                              debugSummary->c_str());
    }
}

void 
PcpChanges::DidUnmuteLayer(
    PcpCache* cache, 
    const std::string& layerId)
{
    // Change debugging.
    std::string summary;
    std::string* debugSummary = TfDebug::IsEnabled(PCP_CHANGES) ? &summary : 0;

    const SdfLayerRefPtr unmutedLayer = 
        _LoadSublayerForChange(cache, layerId, _SublayerAdded);
    const PcpLayerStackPtrVector& layerStacks = 
        cache->_layerStackCache->FindAllUsingMutedLayer(layerId);

    PCP_APPEND_DEBUG("  Did unmute layer @%s@\n", layerId.c_str());

    if (not layerStacks.empty()) {
        _DidChangeSublayerAndLayerStacks(cache, layerStacks, unmutedLayer,
                                         _SublayerAdded, debugSummary);
    }

    if (debugSummary and not debugSummary->empty()) {
        TfDebug::Helper().Msg("PcpChanges::DidUnmuteLayer\n%s",
                              debugSummary->c_str());
    }
}

void
PcpChanges::DidMaybeFixSublayer(
    PcpCache* cache,
    const SdfLayerHandle& layer,
    const std::string& sublayerPath)
{
    // Change debugging.
    std::string summary;
    std::string* debugSummary = TfDebug::IsEnabled(PCP_CHANGES) ? &summary : 0;

    // See if the sublayer is now readable.  If so mark the layer stacks
    // using the sublayer's parent (and thus the sublayer) as dirty, and also
    // all of the prims in cache that are using any prim from any of those
    // layer stacks.
    const SdfLayerRefPtr sublayer = 
        _LoadSublayerForChange(cache, layer, sublayerPath, _SublayerAdded);
    const PcpLayerStackPtrVector& layerStacks =
        cache->FindAllLayerStacksUsingLayer(layer);

    PCP_APPEND_DEBUG(
        "  Layer @%s@ changed sublayer @%s@\n",
        layer ? layer->GetIdentifier().c_str() : "invalid",
        sublayerPath.c_str());

    _DidChangeSublayerAndLayerStacks(cache, layerStacks, sublayer,
                                     _SublayerAdded, debugSummary);

    if (debugSummary and not debugSummary->empty()) {
        TfDebug::Helper().Msg("PcpChanges::DidMaybeFixSublayer\n%s",
                              debugSummary->c_str());
    }
}

void 
PcpChanges::_DidChangeSublayerAndLayerStacks(
    PcpCache* cache,
    const PcpLayerStackPtrVector& layerStacks,
    const SdfLayerHandle& sublayer,
    _SublayerChangeType sublayerChange,
    std::string* debugSummary)
{
    static const bool requiresLayerStackChange        = true;
    static const bool requiresLayerStackOffsetsChange = false;
    bool requiresSignificantChange = false;

    bool loaded = _DidChangeSublayer(cache, layerStacks,
                                     sublayer,
                                     sublayerChange,
                                     debugSummary,
                                     &requiresSignificantChange);
    if (loaded) {
        // Layer was loaded.  The layer stacks are changed.
        TF_FOR_ALL(layerStack, layerStacks) {
            _DidChangeLayerStack(*layerStack,
                                 requiresLayerStackChange,
                                 requiresLayerStackOffsetsChange,
                                 requiresSignificantChange );
        }
    }
}

void
PcpChanges::DidMaybeFixAsset(
    PcpCache* cache,
    const PcpSite& site,
    const SdfLayerHandle& srcLayer,
    const std::string& assetPath)
{
    // Get the site's layer stack and make sure it's valid.
    PcpLayerStackPtr layerStack = 
        cache->FindLayerStack(site.layerStackIdentifier);
    if (not layerStack) {
        return;
    }

    // Change debugging.
    std::string summary;
    std::string* debugSummary = TfDebug::IsEnabled(PCP_CHANGES) ? &summary : 0;

    // Load the layer.
    std::string resolvedAssetPath(assetPath);
    SdfLayerRefPtr layer = SdfFindOrOpenRelativeToLayer(
        srcLayer, &resolvedAssetPath);

    PCP_APPEND_DEBUG("  Asset @%s@ %s\n",
                     assetPath.c_str(),
                     layer ? (layer->IsEmpty() ? "insignificant"
                                               : "significant")
                           : "invalid");

    if (layer) {
        // Hold layer to avoid reparsing.
        _lifeboat.Retain(layer);

        // Mark prims using site as changed.
        PCP_APPEND_DEBUG(
            "Resync following in @%s@ significantly due to "
            "loading asset used by @%s@<%s>:\n",
            cache->GetLayerStackIdentifier().rootLayer->
                 GetIdentifier().c_str(),
            layerStack->GetIdentifier().rootLayer->
                 GetIdentifier().c_str(), site.path.GetText());
        if (layerStack == cache->GetLayerStack()) {
            PCP_APPEND_DEBUG("    <%s>\n", site.path.GetText());
            DidChangeSignificantly(cache, site.path);
        }
        PcpDependencyVector deps =
            cache->FindDependentPaths(layerStack, site.path,
                                      PcpDependencyTypeAnyIncludingVirtual,
                                      /* recurseOnSite */ true,
                                      /* recurseOnIndex */ true,
                                      /* filter */ true);
        for(const auto &dep: deps) {
            PCP_APPEND_DEBUG("    <%s>\n", dep.indexPath.GetText());
            DidChangeSignificantly(cache, dep.indexPath);
        }
    }

    if (debugSummary and not debugSummary->empty()) {
        TfDebug::Helper().Msg("PcpChanges::DidMaybeFixAsset\n%s",
                              debugSummary->c_str());
    }
}

void
PcpChanges::DidChangeLayers(PcpCache* cache)
{
    TF_DEBUG(PCP_CHANGES).Msg("PcpChanges::DidChangeLayers: @%s@\n",
                              cache->GetLayerStackIdentifier().rootLayer->
                                  GetIdentifier().c_str());

    PcpLayerStackChanges& changes = _GetLayerStackChanges(cache);
    if (not changes.didChangeLayers) {
        changes.didChangeLayers       = true;
        changes.didChangeLayerOffsets = false;
    }
}

void
PcpChanges::DidChangeLayerOffsets(PcpCache* cache)
{
    PcpLayerStackChanges& changes = _GetLayerStackChanges(cache);
    if (not changes.didChangeLayers) {
        changes.didChangeLayerOffsets = true;
    }
}

void
PcpChanges::DidChangeSignificantly(PcpCache* cache, const SdfPath& path)
{
    _GetCacheChanges(cache).didChangeSignificantly.insert(path);
}

void
PcpChanges::DidChangeSpecs(
    PcpCache* cache, const SdfPath& path,
    const SdfLayerHandle& changedLayer, const SdfPath& changedPath)
{
    // If we're adding an inert prim spec, it may correspond to a node that
    // was culled in the prim index at path. If so, we need to rebuild that
    // index to pick up the new node. We don't need to rebuild the indexes
    // for namespace descendants because those should not be affected.
    //
    // XXX: We could also rebuild the index if we removed the last prim spec 
    //      from a layer stack, to cull the corresponding node. The cost for
    //      determining whether this is the case may outweigh the benefit,
    //      though.
    if (path.IsPrimPath()) {
        TF_VERIFY(changedPath.IsPrimOrPrimVariantSelectionPath());

        bool shouldRebuildIndex = false;

        const PcpNodeRef nodeForChangedSpec = 
            cache->_GetNodeProvidingSpec(path, changedLayer, changedPath);
        if (not nodeForChangedSpec) {
            const bool primWasAdded = changedLayer->HasSpec(changedPath);
            if (primWasAdded) {
                shouldRebuildIndex = true;
            }
        }

        if (shouldRebuildIndex) {
            _GetCacheChanges(cache).didChangePrims.insert(path);
            return;
        }
    }

    DidChangeSpecStack(cache, path);
}

void 
PcpChanges::DidChangeSpecStack(PcpCache* cache, const SdfPath& path)
{
    _GetCacheChanges(cache).didChangeSpecs.insert(path);
}

void
PcpChanges::DidChangeTargets(PcpCache* cache, const SdfPath& path,
                             PcpCacheChanges::TargetType targetType)
{
    _GetCacheChanges(cache).didChangeTargets[path] |= targetType;
}

void
PcpChanges::DidChangeRelocates(PcpCache* cache, const SdfPath& path)
{
    // XXX For now we resync the prim entirely.  This is both because
    // we do not yet have a way to incrementally update the mappings,
    // as well as to ensure that we provide a change entry that will
    // cause Csd to pull on the cache and keep its contents alive.
    _GetCacheChanges(cache).didChangeSignificantly.insert(path);
}

void
PcpChanges::DidChangePaths(
    PcpCache* cache,
    const SdfPath& oldPath,
    const SdfPath& newPath)
{
    // XXX: Do we need to handle rename chains?  I.e. A renamed to B
    //      then renamed to C.  If so then we may need to handle one
    //      oldPath appearing multiple times, e.g. A -> B -> C and
    //      D -> B -> E, where B appears in two chains.

    TF_DEBUG(PCP_CHANGES).Msg("PcpChanges::DidChangePaths: @%s@<%s> to <%s>\n",
                              cache->GetLayerStackIdentifier().rootLayer->
                                  GetIdentifier().c_str(),
                              oldPath.GetText(), newPath.GetText());

    _GetCacheChanges(cache).didChangePath[oldPath] = newPath;
}

void
PcpChanges::DidDestroyCache(PcpCache* cache)
{
    _cacheChanges.erase(cache);
    _renameChanges.erase(cache);

    // Note that a layer stack in _layerStackChanges may be expired.  We
    // just leave it there and let clients and Apply() check for expired
    // layer stacks.
}

void
PcpChanges::Swap(PcpChanges& other)
{
    std::swap(_layerStackChanges, other._layerStackChanges);
    std::swap(_cacheChanges, other._cacheChanges);
    std::swap(_renameChanges, other._renameChanges);
    _lifeboat.Swap(other._lifeboat);
}

bool
PcpChanges::IsEmpty() const
{
    return _layerStackChanges.empty() and
           _cacheChanges.empty() and
           _renameChanges.empty();
}

const PcpChanges::LayerStackChanges&
PcpChanges::GetLayerStackChanges() const
{
    return _layerStackChanges;
}

const PcpChanges::CacheChanges&
PcpChanges::GetCacheChanges() const
{
    // NOTE: This is potentially expensive even if we've already done
    //       it.  In the expected use pattern we only call this method
    //       once, so it shouldn't be a problem.
    _Optimize();

    return _cacheChanges;
}

const PcpLifeboat&
PcpChanges::GetLifeboat() const
{
    return _lifeboat;
}

void
PcpChanges::Apply() const
{
    // NOTE: This is potentially expensive even if we've already done
    //       it.  In the expected use pattern we only call this method
    //       once, so it shouldn't be a problem.
    _Optimize();

    // Apply layer changes first.
    TF_FOR_ALL(i, _layerStackChanges) {
        if (i->first) {
            i->first->Apply(i->second, &_lifeboat);
        }
    }

    // Now apply cache changes.
    TF_FOR_ALL(i, _cacheChanges) {
        i->first->Apply(i->second, &_lifeboat);
    }
}

PcpLayerStackChanges&
PcpChanges::_GetLayerStackChanges(PcpCache* cache)
{
    return _layerStackChanges[cache->GetLayerStack()];
}

PcpLayerStackChanges&
PcpChanges::_GetLayerStackChanges(const PcpLayerStackPtr& layerStack)
{
    return _layerStackChanges[layerStack];
}

PcpCacheChanges&
PcpChanges::_GetCacheChanges(PcpCache* cache)
{
    return _cacheChanges[cache];
}

PcpCacheChanges::PathEditMap&
PcpChanges::_GetRenameChanges(PcpCache* cache)
{
    return _renameChanges[cache];
}

void
PcpChanges::_Optimize() const
{
    const_cast<PcpChanges*>(this)->_Optimize();
}

void
PcpChanges::_Optimize()
{
    TF_FOR_ALL(i, _renameChanges) {
        _OptimizePathChanges(i->first, &_cacheChanges[i->first], &i->second);
    }

    // This must be called after _OptimizePathChanges().
    TF_FOR_ALL(i, _cacheChanges) {
        _Optimize(&i->second);
    }
}

void
PcpChanges::_Optimize(PcpCacheChanges* changes)
{
    // Subsume changes implied by ancestors.
    Pcp_SubsumeDescendants(&changes->didChangeSignificantly);

    // Subsume changes implied by prim graph changes.
    TF_FOR_ALL(i, changes->didChangeSignificantly) {
        Pcp_SubsumeDescendants(&changes->didChangePrims, *i);
        Pcp_SubsumeDescendants(&changes->didChangeSpecs, *i);
    }

    // Subsume spec changes for prims whose indexes will be rebuilt.
    TF_FOR_ALL(i, changes->didChangePrims) {
        changes->didChangeSpecs.erase(*i);
    }

    // XXX: Do we subsume name changes?
}

void
PcpChanges::_OptimizePathChanges(
    const PcpCache* cache,
    PcpCacheChanges* changes,
    PcpCacheChanges::PathEditMap* pathChanges)
{
    // Discard any path change that's also in changes->didChangePath.
    typedef std::pair<SdfPath, SdfPath> PathPair;
    std::vector<PathPair> sdOnly;
    std::set_difference(pathChanges->begin(), pathChanges->end(),
                        changes->didChangePath.begin(),
                        changes->didChangePath.end(),
                        std::back_inserter(sdOnly));

    std::string summary;
    std::string* debugSummary = TfDebug::IsEnabled(PCP_CHANGES) ? &summary : 0;

    // sdOnly now has the path changes that Sd told us about but
    // DidChangePaths() did not.  We must assume the worst.
    for (const auto& change : sdOnly) {
        const SdfPath& oldPath = change.first;
        const SdfPath& newPath = change.second;

        PCP_APPEND_DEBUG("  Sd only path change @%s@<%s> to <%s>\n",
                         cache->GetLayerStackIdentifier().rootLayer->
                             GetIdentifier().c_str(),
                         oldPath.GetText(), newPath.GetText());
        changes->didChangeSignificantly.insert(oldPath);
        if (not newPath.IsEmpty()) {
            changes->didChangeSignificantly.insert(newPath);
        }
    }

    if (debugSummary and not debugSummary->empty()) {
        TfDebug::Helper().Msg("PcpChanges::_Optimize:\n%s",
                              debugSummary->c_str());
    }
}

void
PcpChanges::_DidChangeDependents(
    int changeType,
    PcpCache* cache,
    const SdfLayerHandle& layer,
    const SdfPath& path,
    bool onlyExistingDependentPaths,
    std::string* debugSummary)
{
    // Don't want to put a trace here, as this function can get called many
    // times during change processing.
    // TRACE_FUNCTION();

    const bool isSignificantPrimChange = 
        (changeType & _ChangeTypeSignificant) and 
        (path == SdfPath::AbsoluteRootPath() or
         path.IsPrimOrPrimVariantSelectionPath());

    // Set up table of functions to call for each dependency, based
    // on the type of change we're trying to propagate.
    struct _ChangeFunctions {
        _ChangeFunctions(PcpChanges* changes, int changeType)
            : _changes(changes)
            , _changeType(changeType)
        {
        }

        void RunFunctionsOnDependency(
            PcpCache* cache, 
            const SdfPath& depPath,
            const SdfLayerHandle& changedLayer,
            const SdfPath& changedPath) const
        {
            if (_changeType & _ChangeTypeSignificant) {
                _changes->DidChangeSignificantly(cache, depPath);
            }
            else {
                if (_changeType & _ChangeTypeSpecs) {
                    _changes->DidChangeSpecs(
                        cache, depPath, changedLayer, changedPath);
                }
                if (_changeType & _ChangeTypeTargets) {
                    _changes->DidChangeTargets(cache, depPath,
                        PcpCacheChanges::TargetTypeRelationshipTarget);
                }
                if (_changeType & _ChangeTypeConnections) {
                    _changes->DidChangeTargets(cache, depPath,
                        PcpCacheChanges::TargetTypeConnection);
                }
            }
        }

    private:
        PcpChanges* _changes;
        int _changeType;
    };

    const _ChangeFunctions changeFuncs(this, changeType);

    // For significant changes to an Sd prim, we need to process its
    // dependencies as well as dependencies on descendants of that prim.
    //
    // This is needed to accommodate relocates, specifically the case where
    // a descendant of the changed prim was relocated out from beneath it.
    // In this case, dependencies on that descendant will be in a different
    // branch of namespace than the dependencies on the changed prim. We
    // need to mark both sets of dependencies as being changed.
    //
    // We don't need to do this for significant property changes as properties
    // can't be individually relocated.
    PcpDependencyVector deps = cache->FindDependentPaths(
        layer, path, PcpDependencyTypeAnyIncludingVirtual,
        /* recurseOnSite */ isSignificantPrimChange,
        /* recurseOnIndex */ false,
        /* filter */ onlyExistingDependentPaths);

    PCP_APPEND_DEBUG(
        "   Resync following in @%s@ %s due to Sd site @%s@<%s>%s:\n",
        cache->GetLayerStackIdentifier()
        .rootLayer->GetIdentifier().c_str(),
        (changeType & _ChangeTypeSignificant) ?
        "significant" :
        "insignificant",
        layer->GetIdentifier().c_str(), path.GetText(),
        onlyExistingDependentPaths ?
        " (restricted to existing caches)" :
        " (not restricted to existing caches)");
    for (const auto& dep: deps) {
        PCP_APPEND_DEBUG(
            "    <%s> depends on <%s>\n",
            dep.indexPath.GetText(),
            dep.sitePath.GetText());
        changeFuncs.RunFunctionsOnDependency(cache, dep.indexPath,
                                             layer, dep.sitePath);
    }
    PCP_APPEND_DEBUG("   Resync end\n");
}

SdfLayerRefPtr 
PcpChanges::_LoadSublayerForChange(
    PcpCache* cache,
    const std::string& sublayerPath,
    _SublayerChangeType sublayerChange) const
{
    // Bind the resolver context.
    const ArResolverContextBinder binder(
        cache->GetLayerStackIdentifier().pathResolverContext);

    // Load the layer.
    SdfLayerRefPtr sublayer;

    const SdfLayer::FileFormatArguments sublayerArgs = 
        Pcp_GetArgumentsForTargetSchema(cache->GetTargetSchema());

    if (sublayerChange == _SublayerAdded) {
        sublayer = SdfLayer::FindOrOpen(sublayerPath, sublayerArgs);
    }
    else {
        sublayer = SdfLayer::Find(sublayerPath, sublayerArgs);
    }

    return sublayer;
}

SdfLayerRefPtr
PcpChanges::_LoadSublayerForChange(
    PcpCache* cache,
    const SdfLayerHandle& layer,
    const std::string& sublayerPath,
    _SublayerChangeType sublayerChange) const
{
    if (not layer) {
        return SdfLayerRefPtr();
    }

    // Bind the resolver context.
    const ArResolverContextBinder binder(
        cache->GetLayerStackIdentifier().pathResolverContext);

    // Load the layer.
    std::string resolvedAssetPath(sublayerPath);
    SdfLayerRefPtr sublayer;

    const SdfLayer::FileFormatArguments sublayerArgs = 
        Pcp_GetArgumentsForTargetSchema(cache->GetTargetSchema());

    // Note the possible conversions from SdfLayerHandle to SdfLayerRefPtr below.
    if (SdfLayer::IsAnonymousLayerIdentifier(resolvedAssetPath)) {
        sublayer = SdfLayer::Find(resolvedAssetPath, sublayerArgs);
    }
    else {
        // Don't bother trying to open a sublayer if we're removing it;
        // either it's already opened in the system and we'll find it, or
        // it's invalid, which we'll deal with below.
        if (sublayerChange == _SublayerAdded) {
            sublayer = SdfFindOrOpenRelativeToLayer(
                layer, &resolvedAssetPath, sublayerArgs);
        }
        else {
            sublayer = SdfLayer::FindRelativeToLayer(
                layer, sublayerPath, sublayerArgs);
        }
    }
    
    return sublayer;
}

bool
PcpChanges::_DidChangeSublayer(
    PcpCache* cache,
    const PcpLayerStackPtrVector& layerStacks,
    const SdfLayerHandle& sublayer,
    _SublayerChangeType sublayerChange,
    std::string* debugSummary,
    bool *significant)
{
    *significant = (sublayer and not sublayer->IsEmpty());

    PCP_APPEND_DEBUG("  %s sublayer @%s@ %s\n",
                     sublayer ? (*significant ? "significant"
                                              : "insignificant")
                              : "invalid",
                     sublayer ? sublayer->GetIdentifier().c_str() 
                              : "invalid",
                     sublayerChange == _SublayerAdded ? "added" : "removed");

    if (not sublayer) {
        // If we're processing the removal of a sublayer and can't find the
        // sublayer in question, there are a couple of possibilities:
        //
        // 1. The sublayer was invalid to begin with; e.g., there was a bogus
        //    sublayer specified that is now being removed. 
        // 2. The sublayer was renamed; this shows up as a remove of the old
        //    sublayer and an add of the new sublayer.
        // 3. The sublayer had been opened and valid, but unexpectedly became 
        //    invalid before hitting this code.
        //
        // In cases 1 and 2, we don't want to emit an error. However, in case
        // 3 we'd ideally emit a coding error. Unfortunately, distinguishing
        // between case 3 and the others is not possible with the information
        // Sd currently provides -- we'd really need to know if a sublayer has
        // been renamed. So for now, just skip the coding error in all cases.
        if (sublayerChange == _SublayerRemoved) {
            return false;
        }

        TF_CODING_ERROR("Can't find or open sublayer");
        return false;
    }

    // Keep the layer alive to avoid reparsing.
    _lifeboat.Retain(sublayer);

    // Register change entries for affected paths.
    //
    // For significant sublayer changes, the sublayer may have introduced
    // new prims with new arcs, requiring prim and property indexes to be
    // recomputed. So, register significant changes for every prim path
    // in the cache that uses any path in any of the layer stacks that
    // included layer.  Only bother doing this for prims, since thex
    // properties will be implicitly invalidated by significant
    // prim resyncs.
    //
    // For insignificant sublayer changes, the only prim that's really 
    // affected is the pseudo-root. However, we still need to rebuild the 
    // prim stacks for every prim that uses an affected layer stack. This
    // is because PcpPrimIndex's prim stack stores indices into the layer
    // stack that may need to be adjusted due to the addition or removal of
    // a layer from that stack.
    //
    // We rely on the caller to provide the affected layer stacks for
    // us because some changes introduce new dependencies that wouldn't
    // have been registered yet using the normal means -- such as unmuting
    // a sublayer.

    bool anyFound = false;
    TF_FOR_ALL(layerStack, layerStacks) {
        PcpDependencyVector deps = cache->FindDependentPaths(
            *layerStack,
            SdfPath::AbsoluteRootPath(), 
            PcpDependencyTypeAnyIncludingVirtual,
            /* recurseOnSite */ true,
            /* recurseOnIndex */ true,
            /* filter */ true);
        for (const auto &dep: deps) {
            if (!dep.indexPath.IsAbsoluteRootOrPrimPath()) {
                // Filter to only prims; see comment above re: properties.
                continue;
            }
            if (!anyFound) {
                PCP_APPEND_DEBUG(
                    "  %s following in @%s@ due to "
                    "%s reload in sublayer @%s@:\n",
                    *significant ? "Resync" : "Spec changes",
                    cache->GetLayerStackIdentifier().rootLayer->
                         GetIdentifier().c_str(),
                    *significant ? "significant" : "insignificant",
                    sublayer->GetIdentifier().c_str());
                anyFound = true;
            }
            PCP_APPEND_DEBUG("    <%s>\n", dep.indexPath.GetText());
            if (*significant) {
                DidChangeSignificantly(cache, dep.indexPath);
            } else {
                DidChangeSpecStack(cache, dep.indexPath);
            }
        }
    }

    return true;
}

void
PcpChanges::_DidChangeLayerStack(
    const PcpLayerStackPtr& layerStack,
    bool requiresLayerStackChange,
    bool requiresLayerStackOffsetsChange,
    bool requiresSignificantChange)
{
    PcpLayerStackChanges& changes = _GetLayerStackChanges(layerStack);
    changes.didChangeLayers        |= requiresLayerStackChange;
    changes.didChangeLayerOffsets  |= requiresLayerStackOffsetsChange;
    changes.didChangeSignificantly |= requiresSignificantChange;

    // didChangeLayers subsumes didChangeLayerOffsets.
    if (changes.didChangeLayers) {
        changes.didChangeLayerOffsets = false;
    }
}

static void
_DeterminePathsAffectedByRelocationChanges( const SdfRelocatesMap & oldMap,
                                            const SdfRelocatesMap & newMap,
                                            SdfPathSet *affectedPaths )
{
    TF_FOR_ALL(path, oldMap) {
        SdfRelocatesMap::const_iterator i = newMap.find(path->first);
        if (i == newMap.end() or i->second != path->second) {
            // This entry in oldMap does not exist in newMap, or
            // newMap relocates this to a different path.
            // Record the affected paths.
            affectedPaths->insert(path->first);
            affectedPaths->insert(path->second);
            if (i != newMap.end()) {
                affectedPaths->insert(i->second);
            }
        }
    }
    TF_FOR_ALL(path, newMap) {
        SdfRelocatesMap::const_iterator i = oldMap.find(path->first);
        if (i == oldMap.end() or i->second != path->second) {
            // This entry in newMap does not exist in oldMap, or
            // oldMap relocated this to a different path.
            // Record the affected paths.
            affectedPaths->insert(path->first);
            affectedPaths->insert(path->second);
            if (i != oldMap.end()) {
                affectedPaths->insert(i->second);
            }
        }
    }
}

// Handle changes to relocations.  This requires:
// 1. rebuilding the composed relocation tables in layer stacks
// 2. blowing PrimIndex caches affected by relocations
// 3. rebuilding MapFunction values that consumed those relocations
void
PcpChanges::_DidChangeLayerStackRelocations(
    const std::vector<PcpCache*>& caches,
    const PcpLayerStackPtr & layerStack,
    std::string* debugSummary)
{
    PcpLayerStackChanges& changes = _GetLayerStackChanges(layerStack);

    if (changes.didChangeRelocates) {
        // There might be multiple relocation changes in a given
        // layer stack, but we only need to process them once.
        return;
    }

    changes.didChangeRelocates = true;

    // Rebuild this layer stack's composed relocations.
    // Store the result in the PcpLayerStackChanges so they can
    // be committed when the changes are applied.
    Pcp_ComputeRelocationsForLayerStack(
        layerStack->GetLayers(),
        &changes.newRelocatesSourceToTarget,
        &changes.newRelocatesTargetToSource,
        &changes.newRelocatesPrimPaths);

    // Compare the old and new relocations to determine which
    // paths (in this layer stack) are affected.
    _DeterminePathsAffectedByRelocationChanges(
        layerStack->GetRelocatesSourceToTarget(),
        changes.newRelocatesSourceToTarget,
        &changes.pathsAffectedByRelocationChanges);

    // Resync affected prims.
    // Use dependencies to find affected caches.
    if (not changes.pathsAffectedByRelocationChanges.empty()) {
        PCP_APPEND_DEBUG("  Relocation change in %s affects:\n",
                         TfStringify(layerStack).c_str());
    }
    for (PcpCache* cache: caches) {
        // Find the equivalent layer stack in this cache.
        PcpLayerStackPtr equivLayerStack =
            cache->FindLayerStack(layerStack->GetIdentifier());
        if (not equivLayerStack) {
            continue;
        }

        SdfPathSet depPathSet;
        for (const SdfPath& path : changes.pathsAffectedByRelocationChanges) {
            PCP_APPEND_DEBUG("    <%s>\n", path.GetText());

            PcpDependencyVector deps =
                cache->FindDependentPaths(
                    equivLayerStack, path,
                    PcpDependencyTypeAnyIncludingVirtual,
                    /* recurseOnSite */ true,
                    /* recurseOnIndex */ true,
                    /* filterForExistingCachesOnly */ false);
            for (const auto &dep: deps) {
                depPathSet.insert(dep.indexPath);
            }
        }

        if (not depPathSet.empty()) {
            PCP_APPEND_DEBUG("  and dependent paths in %s\n",
                             TfStringify(layerStack).c_str());
        }
        for (const SdfPath& depPath : depPathSet) {
            PCP_APPEND_DEBUG("      <%s>\n", depPath.GetText());
            DidChangeSignificantly(cache, depPath);
        }
    }
}
