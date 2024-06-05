//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/dependencies.h"

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/changes.h"
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/iterator.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/pathTable.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stl.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

Pcp_Dependencies::
ConcurrentPopulationContext::ConcurrentPopulationContext(Pcp_Dependencies &deps)
    : _deps(deps)
{
    TF_AXIOM(!_deps._concurrentPopulationContext);
    _deps._concurrentPopulationContext = this;
}

Pcp_Dependencies::ConcurrentPopulationContext::~ConcurrentPopulationContext()
{
    _deps._concurrentPopulationContext = nullptr;
}

Pcp_Dependencies::Pcp_Dependencies()
    : _layerStacksRevision(0)
    , _concurrentPopulationContext(nullptr)
{
    // Do nothing
}

Pcp_Dependencies::~Pcp_Dependencies()
{
    // Do nothing
}

// Determine if Pcp_Dependencies should store an entry
// for the arc represented by the given node.
//
// As a space optimization, Pcp_Dependencies does not store entries
// for arcs that are implied by nearby structure and which can
// be easily synthesized. Specifically, it does not store arcs
// introduced purely ancestrally, nor does it store arcs for root nodes
// (PcpDependencyTypeRoot).
inline static bool
_ShouldStoreDependency(PcpDependencyFlags depFlags)
{
    return depFlags & PcpDependencyTypeDirect;
}

void
Pcp_Dependencies::Add(
    const PcpPrimIndex &primIndex,
    PcpCulledDependencyVector &&culledDependencies,
    PcpDynamicFileFormatDependencyData &&fileFormatDependencyData,
    PcpExpressionVariablesDependencyData &&exprVarDependencyData)
{
    TfAutoMallocTag2 tag("Pcp", "Pcp_Dependencies::Add");
    if (!primIndex.GetRootNode()) {
        return;
    }
    const SdfPath& primIndexPath = primIndex.GetRootNode().GetPath();
    TF_DEBUG(PCP_DEPENDENCIES)
        .Msg("Pcp_Dependencies: Adding deps for index <%s>:\n",
             primIndexPath.GetText());

    auto addDependency = [this, &primIndexPath](
        const PcpLayerStackRefPtr& layerStack,
        const SdfPath& path)
    {
        auto iresult = _deps.emplace(layerStack, _SiteDepMap());
        _SiteDepMap &siteDepMap = iresult.first->second;
        if (iresult.second) {
            // If we inserted a new entry, bump the revision count.
            ++_layerStacksRevision;
        }
        std::vector<SdfPath> &deps = siteDepMap[path];
        deps.push_back(primIndexPath);
    };

    int nodeIndex=0, count=0;
    for (const PcpNodeRef &n: primIndex.GetNodeRange()) {
        const int curNodeIndex = nodeIndex++;
        const PcpDependencyFlags depFlags = PcpClassifyNodeDependency(n);
        if (_ShouldStoreDependency(depFlags)) {
            ++count;
            {
                tbb::spin_mutex::scoped_lock lock;
                if (_concurrentPopulationContext) {
                    lock.acquire(_concurrentPopulationContext->_mutex);
                }
                addDependency(n.GetLayerStack(), n.GetPath());
            }

            TF_DEBUG(PCP_DEPENDENCIES)
                .Msg(" - Node %i (%s %s): <%s> %s\n",
                     curNodeIndex,
                     PcpDependencyFlagsToString(depFlags).c_str(),
                     TfEnum::GetDisplayName(n.GetArcType()).c_str(),
                     n.GetPath().GetText(),
                     TfStringify(n.GetLayerStack()->GetIdentifier()).c_str());
        }
    }

    if (!culledDependencies.empty()) {
        const PcpCulledDependencyVector* insertedDeps = nullptr;
        {
            tbb::spin_mutex::scoped_lock lock;
            if (_concurrentPopulationContext) {
                lock.acquire(_concurrentPopulationContext->_mutex);
            }

            for (const PcpCulledDependency& dep : culledDependencies) {
                addDependency(dep.layerStack, dep.sitePath);
            }

            count += culledDependencies.size();

            PcpCulledDependencyVector& deps =
                _culledDependenciesMap[primIndexPath];
            if (deps.empty()) {
                deps = std::move(culledDependencies);
            }
            else {
                deps.insert(deps.begin(),
                    std::make_move_iterator(culledDependencies.begin()),
                    std::make_move_iterator(culledDependencies.end()));
            }

            insertedDeps = &deps;
        }

        if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
            for (const PcpCulledDependency& dep : *insertedDeps) {
                TF_DEBUG(PCP_DEPENDENCIES)
                    .Msg(" - Node (culled) (%s): <%s> %s\n",
                        PcpDependencyFlagsToString(dep.flags).c_str(),
                        dep.sitePath.GetText(),
                        TfStringify(dep.layerStack->GetIdentifier()).c_str());
            }
        }
    }

    // Store the prim index's dynamic file format dependency of the prim index
    // if possible
    if (!fileFormatDependencyData.IsEmpty()) {
        // Update the caches of field names and attribute names that are are 
        // possible dynamic file format argument dependencies by incrementing 
        // their reference counts, adding them to the appropriate cache if not
        // already there.
        tbb::spin_mutex::scoped_lock lock;
        if (_concurrentPopulationContext) {
            lock.acquire(_concurrentPopulationContext->_mutex);
        }

        auto addNamesToDepMapFn = [](
            _FileFormatArgumentFieldDepMap &depMap, const TfToken::Set &names)
        {
            for (const TfToken &name : names) {
                auto it = depMap.emplace(name, 0);
                it.first->second++;
            }
        };
        addNamesToDepMapFn(_possibleDynamicFileFormatArgumentFields,
            fileFormatDependencyData.GetRelevantFieldNames());
        addNamesToDepMapFn(_possibleDynamicFileFormatArgumentAttributes,
            fileFormatDependencyData.GetRelevantAttributeNames());
       
        // Take and store the dependency data.
        _fileFormatArgumentDependencyMap[primIndexPath] = 
            std::move(fileFormatDependencyData);
    }

    if (!exprVarDependencyData.IsEmpty()) {
        tbb::spin_mutex::scoped_lock lock;
        if (_concurrentPopulationContext) {
            lock.acquire(_concurrentPopulationContext->_mutex);
        }

        exprVarDependencyData.ForEachDependency(
            [&, this](
                const PcpLayerStackPtr& layerStack,
                const std::unordered_set<std::string>&)
            {
                _layerStackExprVarsMap[layerStack].push_back(primIndexPath);
            });

        _exprVarsDependencyMap[primIndexPath] = 
            std::move(exprVarDependencyData);
    }

    if (count == 0) {
        TF_DEBUG(PCP_DEPENDENCIES).Msg("    None\n");
    }
}

void
Pcp_Dependencies::Remove(const PcpPrimIndex &primIndex, PcpLifeboat *lifeboat)
{
    if (!primIndex.GetRootNode()) {
        return;
    }
    const SdfPath& primIndexPath = primIndex.GetRootNode().GetPath();
    TF_DEBUG(PCP_DEPENDENCIES)
        .Msg("Pcp_Dependencies: Removing deps for index <%s>\n",
             primIndexPath.GetText());

    auto removeDependency = [this, &primIndexPath, &lifeboat](
        const PcpLayerStackRefPtr& layerStack,
        const SdfPath& path)
    {
        _SiteDepMap &siteDepMap = _deps[layerStack];
        std::vector<SdfPath> &deps = siteDepMap[path];

        // Swap with last element, then remove that.
        // We are using the vector as an unordered set.
        std::vector<SdfPath>::iterator i =
            std::find(deps.begin(), deps.end(), primIndexPath);
        if (!TF_VERIFY(i != deps.end())) {
            return;
        }
        std::vector<SdfPath>::iterator last = --deps.end();
        std::swap(*i, *last);
        deps.erase(last);

        // Reap container entries when no deps are left.
        // This is slightly tricky with SdfPathTable since we need
        // to examine subtrees and parents.
        if (deps.empty()) {
            TF_DEBUG(PCP_DEPENDENCIES).Msg("      Removed last dep on site\n");

            // Scan children to see if we can remove this subtree.
            _SiteDepMap::iterator i, iBegin, iEnd;
            std::tie(iBegin, iEnd) = siteDepMap.FindSubtreeRange(path);
            for (i = iBegin; i != iEnd && i->second.empty(); ++i) {}
            bool subtreeIsEmpty = i == iEnd;
            if (subtreeIsEmpty) {
                siteDepMap.erase(iBegin);
                TF_DEBUG(PCP_DEPENDENCIES).Msg("      No subtree deps\n");

                // Now scan upwards to reap parent entries.
                for (SdfPath p = path.GetParentPath();
                     !p.IsEmpty(); p = p.GetParentPath()) {
                    std::tie(iBegin, iEnd) = siteDepMap.FindSubtreeRange(p);
                    if (iBegin != iEnd
                        && std::next(iBegin) == iEnd
                        && iBegin->second.empty()) {
                    TF_DEBUG(PCP_DEPENDENCIES)
                        .Msg("    Removing empty parent entry <%s>\n",
                             p.GetText());
                        siteDepMap.erase(iBegin);
                    } else {
                        break;
                    }
                }

                // Check if the entire table is empty.
                if (siteDepMap.empty()) {
                    if (lifeboat) {
                        lifeboat->Retain(layerStack);
                    }
                    _deps.erase(layerStack);
                    ++_layerStacksRevision;

                    TF_DEBUG(PCP_DEPENDENCIES)
                        .Msg("    Removed last dep on %s\n",
                             TfStringify(layerStack
                                         ->GetIdentifier()).c_str());
                }
            }
        }

    };

    int nodeIndex=0;
    for (const PcpNodeRef &n: primIndex.GetNodeRange()) {
        const int curNodeIndex = nodeIndex++;
        const PcpDependencyFlags depFlags = PcpClassifyNodeDependency(n);
        if (!_ShouldStoreDependency(depFlags)) {
            continue;
        }

        TF_DEBUG(PCP_DEPENDENCIES)
            .Msg(" - Node %i (%s %s): <%s> %s\n",
                 curNodeIndex,
                 PcpDependencyFlagsToString(depFlags).c_str(),
                 TfEnum::GetDisplayName(n.GetArcType()).c_str(),
                 n.GetPath().GetText(),
                 TfStringify(n.GetLayerStack()->GetIdentifier()).c_str());

        removeDependency(n.GetLayerStack(), n.GetPath());
    }

    auto culledDepIt = _culledDependenciesMap.find(primIndexPath);
    if (culledDepIt != _culledDependenciesMap.end()) {
        for (const PcpCulledDependency& dep : culledDepIt->second) {
            TF_DEBUG(PCP_DEPENDENCIES)
                .Msg(" - Node (culled) (%s): <%s> %s\n",
                    PcpDependencyFlagsToString(dep.flags).c_str(),
                    dep.sitePath.GetText(),
                    TfStringify(dep.layerStack->GetIdentifier()).c_str());

            removeDependency(dep.layerStack, dep.sitePath);
        }

        _culledDependenciesMap.erase(culledDepIt);
    }

    // We need to remove prim index's dynamic format dependency object
    // if there is one.
    auto it = _fileFormatArgumentDependencyMap.find(primIndexPath);
    if (it != _fileFormatArgumentDependencyMap.end()) {
        if (TF_VERIFY(!it->second.IsEmpty())) {

            auto removeNamesFromDepMapFn = [](
                _FileFormatArgumentFieldDepMap &depMap, const TfToken::Set &names)
            {
                for (const auto &name : names) {
                    auto depMapIt = depMap.find(name);
                    if (TF_VERIFY(depMapIt != depMap.end())) {
                        // If the reference count will drop to 0, we need to 
                        // remove it completely as the 
                        // IsPossibleDynamicFileFormatArgument... functions only
                        // test for existence of the name in the map.
                        if (depMapIt->second <= 1) {
                            depMap.erase(depMapIt);
                        } else {
                            depMapIt->second--;
                        }
                    }
                }
            };

            // We need to also update the reference counts for the 
            // dependency's relevant fields and attributes in their respective
            // name caches.
            removeNamesFromDepMapFn(_possibleDynamicFileFormatArgumentFields,
                it->second.GetRelevantFieldNames());
            removeNamesFromDepMapFn(_possibleDynamicFileFormatArgumentAttributes,
                it->second.GetRelevantAttributeNames());
        }
        // Remove the dependency data.
        _fileFormatArgumentDependencyMap.erase(it);
    }

    auto exprVarIt = _exprVarsDependencyMap.find(primIndexPath);
    if (exprVarIt != _exprVarsDependencyMap.end()) {
        exprVarIt->second.ForEachDependency(
            [&, this](
                const PcpLayerStackPtr& layerStack,
                const std::unordered_set<std::string>&)
            {
                auto layerStackIt = _layerStackExprVarsMap.find(layerStack);
                if (TF_VERIFY(layerStackIt != _layerStackExprVarsMap.end())) {
                    SdfPathVector& primIndexPaths = layerStackIt->second;
                    primIndexPaths.erase(
                        std::remove(
                            primIndexPaths.begin(), primIndexPaths.end(),
                            primIndexPath),
                        primIndexPaths.end());
                    if (primIndexPaths.empty()) {
                        _layerStackExprVarsMap.erase(layerStackIt);
                    }
                }
            });

        _exprVarsDependencyMap.erase(exprVarIt);
    }
}

void
Pcp_Dependencies::RemoveAll(PcpLifeboat* lifeboat)
{
    TF_DEBUG(PCP_DEPENDENCIES).Msg(
        "Pcp_Dependencies::RemoveAll: Clearing all dependencies\n");

    // Retain all layerstacks in the lifeboat.
    if (lifeboat) {
        TF_FOR_ALL(i, _deps) {
            lifeboat->Retain(i->first);
        }
    }

    _deps.clear();
    ++_layerStacksRevision;
    _possibleDynamicFileFormatArgumentFields.clear();
    _possibleDynamicFileFormatArgumentAttributes.clear();
    _culledDependenciesMap.clear();
    _fileFormatArgumentDependencyMap.clear();
    _exprVarsDependencyMap.clear();
    _layerStackExprVarsMap.clear();
}

SdfLayerHandleSet 
Pcp_Dependencies::GetUsedLayers() const
{
    SdfLayerHandleSet reachedLayers;

    TF_FOR_ALL(layerStack, _deps) {
        const SdfLayerRefPtrVector& layers = layerStack->first->GetLayers();
        reachedLayers.insert(layers.begin(), layers.end());
    }

    return reachedLayers;
}

SdfLayerHandleSet 
Pcp_Dependencies::GetUsedRootLayers() const
{
    SdfLayerHandleSet reachedRootLayers;

    TF_FOR_ALL(i, _deps) {
        const PcpLayerStackPtr& layerStack = i->first;
        reachedRootLayers.insert(layerStack->GetIdentifier().rootLayer );
    }

    return reachedRootLayers;
}

bool 
Pcp_Dependencies::UsesLayerStack(const PcpLayerStackPtr& layerStack) const
{
    return _deps.find(layerStack) != _deps.end();
}

const PcpCulledDependencyVector&
Pcp_Dependencies::GetCulledDependencies(const SdfPath &primIndexPath) const
{
    static const PcpCulledDependencyVector empty;
    auto it = _culledDependenciesMap.find(primIndexPath);
    return it == _culledDependenciesMap.end() ? empty : it->second;
}

const PcpCulledDependencyVector&
Pcp_Dependencies::GetCulledDependencies(
    const PcpCache& cache, const SdfPath &primIndexPath)
{
    return cache._primDependencies->GetCulledDependencies(primIndexPath);
}

bool 
Pcp_Dependencies::HasAnyDynamicFileFormatArgumentFieldDependencies() const
{
    return !_possibleDynamicFileFormatArgumentFields.empty();
}

bool 
Pcp_Dependencies::
HasAnyDynamicFileFormatArgumentAttributeDependencies() const
{
    return !_possibleDynamicFileFormatArgumentAttributes.empty();
}

bool 
Pcp_Dependencies::IsPossibleDynamicFileFormatArgumentField(
    const TfToken &field) const
{
    // Any field in the map will have at least one prim index dependency logged
    // for it.
    return _possibleDynamicFileFormatArgumentFields.count(field) > 0;
}

bool 
Pcp_Dependencies::IsPossibleDynamicFileFormatArgumentAttribute(
    const TfToken &attributeName) const
{
    // Any attribute name in the map will have at least one prim index 
    // dependency logged for it.
    return _possibleDynamicFileFormatArgumentAttributes.count(attributeName) > 0;
}

const PcpDynamicFileFormatDependencyData &
Pcp_Dependencies::GetDynamicFileFormatArgumentDependencyData(
    const SdfPath &primIndexPath) const
{
    static const PcpDynamicFileFormatDependencyData empty;
    auto it = _fileFormatArgumentDependencyMap.find(primIndexPath);
    if (it == _fileFormatArgumentDependencyMap.end()) {
        return empty;
    }
    return it->second;
}

const SdfPathVector&
Pcp_Dependencies::GetPrimsUsingExpressionVariablesFromLayerStack(
    const PcpLayerStackPtr& layerStack) const
{
    static const SdfPathVector empty;

    const SdfPathVector* primIndexPaths = 
        TfMapLookupPtr(_layerStackExprVarsMap, layerStack);
    return primIndexPaths ? *primIndexPaths : empty;
}

const std::unordered_set<std::string>&
Pcp_Dependencies::GetExpressionVariablesFromLayerStackUsedByPrim(
    const SdfPath &primIndexPath,
    const PcpLayerStackPtr &layerStack) const
{
    static const std::unordered_set<std::string> empty;
    
    const PcpExpressionVariablesDependencyData* exprVarDeps =
        TfMapLookupPtr(_exprVarsDependencyMap, primIndexPath);
    if (!exprVarDeps) {
        return empty;
    }

    const std::unordered_set<std::string>* usedExprVars =
        exprVarDeps->GetDependenciesForLayerStack(layerStack);
    return usedExprVars ? *usedExprVars : empty;
}

void
Pcp_AddCulledDependency(
    const PcpNodeRef& node,
    PcpCulledDependencyVector* culledDeps)
{
    const PcpDependencyFlags depFlags = PcpClassifyNodeDependency(node);
    if (!_ShouldStoreDependency(depFlags)) {
        return;
    }

    PcpCulledDependency dep;
    dep.flags = depFlags;
    dep.layerStack = node.GetLayerStack();
    dep.sitePath = node.GetPath();
    dep.unrelocatedSitePath = 
        node.GetArcType() == PcpArcTypeRelocate ? 
        node.GetParentNode().GetPath() : SdfPath();
    dep.mapToRoot = node.GetMapToRoot().Evaluate();

    culledDeps->push_back(std::move(dep));
}

PXR_NAMESPACE_CLOSE_SCOPE
