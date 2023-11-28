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
#ifndef PXR_USD_PCP_DEPENDENCIES_H
#define PXR_USD_PCP_DEPENDENCIES_H

/// \file pcp/dependencies.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/dependency.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"

#include <tbb/spin_mutex.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class PcpLifeboat;
class PcpPrimIndexDependencies;

TF_DECLARE_WEAK_PTRS(PcpLayerStack);

/// \class Pcp_Dependencies
///
/// Tracks the dependencies of PcpPrimIndex entries in a PcpCache.
/// This is an internal class only meant for use by PcpCache.
///
class Pcp_Dependencies {
public:
    /// Construct with no dependencies.
    Pcp_Dependencies();
    ~Pcp_Dependencies();

    Pcp_Dependencies(Pcp_Dependencies const &) = delete;
    Pcp_Dependencies &operator=(Pcp_Dependencies const &) = delete;

    /// \name Registration
    /// @{

    /// Add dependency information for the given PcpPrimIndex along with a
    /// dynamic file format dependency data object if the prim index has
    /// any arcs that depend on a dynamic file format.
    ///
    /// Assumptions:
    /// - A computed prim index will be added exactly once
    /// - Parent indices will be added before children
    void Add(const PcpPrimIndex &primIndex,
        PcpCulledDependencyVector &&culledDependencies,
        PcpDynamicFileFormatDependencyData &&fileFormatDependencyData,
        PcpExpressionVariablesDependencyData &&exprVarDependencyData);

    /// Remove dependency information for the given PcpPrimIndex.
    /// Any layer stacks in use by any site are added to \p lifeboat,
    /// if not \c NULL.
    ///
    /// Assumptions:
    /// - The prim index has previously been added exactly once
    void Remove(const PcpPrimIndex &primIndex, PcpLifeboat *lifeboat);

    /// Remove all dependencies.  Any layer stacks in use by any site are
    /// added to \p lifeboat, if not \c NULL.
    void RemoveAll(PcpLifeboat* lifeboat);

    /// Inform this dependencies object that layer stacks it observes may have
    /// changed.
    void LayerStacksChanged() {
        ++_layerStacksRevision;
    }

    /// \struct ConcurrentPopulationContext
    ///
    /// Structure for enabling cache population via concurrent calls to Add().
    /// Protects member data with a mutex during its lifetime.
    /// \sa Add().
    struct ConcurrentPopulationContext
    {
        explicit ConcurrentPopulationContext(Pcp_Dependencies &deps);
        ~ConcurrentPopulationContext();
        Pcp_Dependencies &_deps;
        tbb::spin_mutex _mutex;
    };

    /// @}
    /// \name Queries
    /// @{

    /// Invokes \p fn for every \c PcpPrimIndex that uses
    /// the site represented by (siteLayerStack, sitePath).
    ///
    /// The arguments to \p fn are: (depIndexPath, depSitePath).
    ///
    /// If \p includeAncestral is \c true, this will also walk up
    /// ancestral dependencies introduced by parent prims.
    /// 
    /// If \p recurseBelowSite is \c true, then also runs the callback
    /// of every \c PcpSite that uses any descendant of \p path.
    /// depSitePath provides the descendent dependency path.
    ///
    /// If \p recurseBelowSite is \c false, depSitePath is always
    /// the sitePath supplied and can be ignored.
    template <typename FN>
    void
    ForEachDependencyOnSite( const PcpLayerStackPtr &siteLayerStack,
                             const SdfPath &sitePath,
                             bool includeAncestral,
                             bool recurseBelowSite,
                             const FN &fn ) const
    {
        _LayerStackDepMap::const_iterator i = _deps.find(siteLayerStack); 
        if (i == _deps.end()) {
            return;
        }
        const _SiteDepMap & siteDepMap = i->second;
        if (recurseBelowSite) {
            auto range = siteDepMap.FindSubtreeRange(sitePath);
            for (auto iter = range.first; iter != range.second; ++iter) {
                for(const SdfPath &primIndexPath: iter->second) {
                    fn(primIndexPath, iter->first);
                }
            }
        } else {
            _SiteDepMap::const_iterator j = siteDepMap.find(sitePath);
            if (j != siteDepMap.end()) {
                for(const SdfPath &primIndexPath: j->second) {
                    fn(primIndexPath, sitePath);
                }
            }
        }
        if (includeAncestral) {
            for (SdfPath ancestorSitePath = sitePath.GetParentPath();
                 !ancestorSitePath.IsEmpty();
                 ancestorSitePath = ancestorSitePath.GetParentPath())
            {
                _SiteDepMap::const_iterator j =
                    siteDepMap.find(ancestorSitePath);
                if (j != siteDepMap.end()) {
                    for(const SdfPath &ancestorPrimIndexPath: j->second) {
                        fn(ancestorPrimIndexPath, ancestorSitePath);
                    }
                }
            }
        }
    }

    /// Returns all layers from all layer stacks with dependencies recorded
    /// against them.
    SdfLayerHandleSet GetUsedLayers() const;

    /// Return a number that can be used to determine whether or not layer
    /// stacks that are observed by this set of dependencies may have changed or
    /// not.  For example, if one calls GetUsedLayers() and saves the
    /// GetLayerStacksRevision(), and then later calls GetLayerStacksRevision()
    /// again, if the number is unchanged, then GetUsedLayers() is guaranteed to
    /// be unchanged as well.
    size_t GetLayerStacksRevision() const {
        return _layerStacksRevision;
    }

    /// Returns the root layers of all layer stacks with dependencies
    /// recorded against them.
    SdfLayerHandleSet GetUsedRootLayers() const;

    /// Returns true if there are dependencies recorded against the given
    /// layer stack.
    bool UsesLayerStack(const PcpLayerStackPtr& layerStack) const;

    /// Returns list of culled dependencies for the prim index with the given
    /// \p primIndexPath.
    const PcpCulledDependencyVector&
    GetCulledDependencies(const SdfPath &primIndexPath) const;

    /// Returns list of culled dependencies for the prim index in \p cache
    /// with the given \p primIndexPath.
    static const PcpCulledDependencyVector&
    GetCulledDependencies(const PcpCache& cache, const SdfPath &primIndexPath);

    /// Returns true if there are any dynamic file format argument field
    /// dependencies in this dependencies object. 
    bool HasAnyDynamicFileFormatArgumentFieldDependencies() const;

    /// Returns true if there are any dynamic file format argument attribute
    /// default value dependencies in this dependencies object. 
    bool HasAnyDynamicFileFormatArgumentAttributeDependencies() const;

    /// Returns true if the given \p field name is a field that was 
    /// composed while generating dynamic file format arguments for any prim 
    /// index that was added to this dependencies object. 
    bool IsPossibleDynamicFileFormatArgumentField(
        const TfToken &field) const;

    /// Returns true if the given \p attribute name is an attribute whose 
    /// default field was composed while generating dynamic file format
    /// arguments for any prim index that was added to this dependencies object. 
    bool IsPossibleDynamicFileFormatArgumentAttribute(
        const TfToken &attributeName) const;

    /// Returns the dynamic file format dependency data object for the prim
    /// index with the given \p primIndexPath. This will return an empty 
    /// dependency data if either there is no cache prim index for the path or 
    /// if the prim index has no dynamic file formats that it depends on.
    const PcpDynamicFileFormatDependencyData &
    GetDynamicFileFormatArgumentDependencyData(
        const SdfPath &primIndexPath) const;

    /// Returns the list of prim index paths that depend on one or more
    /// expression variables from \p layerStack.
    const SdfPathVector&
    GetPrimsUsingExpressionVariablesFromLayerStack(
        const PcpLayerStackPtr &layerStack) const;

    /// Returns the set of expression variables in \p layerStack that are
    /// used by the prim index at \p primIndexPath.
    const std::unordered_set<std::string>&
    GetExpressionVariablesFromLayerStackUsedByPrim(
        const SdfPath &primIndexPath,
        const PcpLayerStackPtr &layerStack) const;

    /// @}

private:
    // Map of site paths to dependencies, as cache paths.  Stores cache
    // paths as an unordered vector: for our datasets this is both more
    // compact and faster than std::set.
    using _SiteDepMap = SdfPathTable<SdfPathVector>;

    // Map of layer stacks to dependencies on that layerStack.
    // Retains references to those layer stacks, which in turn
    // retain references to their constituent layers.
    using _LayerStackDepMap = 
        std::unordered_map<PcpLayerStackRefPtr, _SiteDepMap, TfHash>;
    _LayerStackDepMap _deps;

    // A revision number that's incremented when the set of layer stacks that
    // this dependencies object observes changes.  That includes both when a
    // layer stack enters or leaves this dependencies object, *and* when the
    // contents of any of the observed layer stacks changes.  This is generally
    // used to help determine whether the set of layers reached by a pcp cache
    // has changed or not.
    size_t _layerStacksRevision;

    // Map of prim index paths to the culled dependencies for the prim index.
    using _CulledDependenciesMap = std::unordered_map<
        SdfPath, PcpCulledDependencyVector, SdfPath::Hash>;
    _CulledDependenciesMap _culledDependenciesMap;

    // Map of prim index paths to the dynamic file format dependency info for 
    // the prim index.
    using _FileFormatArgumentDependencyMap = std::unordered_map<
        SdfPath, PcpDynamicFileFormatDependencyData, SdfPath::Hash>;
    _FileFormatArgumentDependencyMap _fileFormatArgumentDependencyMap;

    // Map of field name to the number of cached prim indices that depend on
    // the field for dynamic file format arguments. This for quick lookup of
    // possible file format argument relevant field changes.
    using _FileFormatArgumentFieldDepMap = 
        std::unordered_map<TfToken, int, TfToken::HashFunctor>;
    _FileFormatArgumentFieldDepMap _possibleDynamicFileFormatArgumentFields; 
    _FileFormatArgumentFieldDepMap _possibleDynamicFileFormatArgumentAttributes; 

    using _ExprVariablesDependencyMap = std::unordered_map<
        SdfPath, PcpExpressionVariablesDependencyData, SdfPath::Hash>;
    _ExprVariablesDependencyMap _exprVarsDependencyMap;

    using _LayerStackToExprVarDepMap = std::unordered_map<
        PcpLayerStackPtr, SdfPathVector, TfHash>;
    _LayerStackToExprVarDepMap _layerStackExprVarsMap;

    ConcurrentPopulationContext *_concurrentPopulationContext;
};

inline bool
Pcp_NodeUsesLayerOrLayerStack( const PcpNodeRef& node,
                               const SdfLayerHandle& layer )
{
    return node.GetLayerStack()->HasLayer(layer);
}

inline bool
Pcp_NodeUsesLayerOrLayerStack( const PcpNodeRef& node,
                               const PcpLayerStackRefPtr& layerStack )
{
    return node.GetLayerStack() == layerStack;
}

inline bool
Pcp_NodeUsesLayerOrLayerStack( const PcpNodeRef& node,
                               const PcpLayerStackPtr& layerStack )
{
    return node.GetLayerStack() == layerStack;
}

template <class FN, class LayerOrLayerStack>
bool
Pcp_ForEachDependentNodeImpl( const SdfPath &sitePath,
                              const LayerOrLayerStack &layerOrLayerStack,
                              const SdfPath &depIndexPath,
                              const PcpCache &cache,
                              const FN &fn)
{
    bool foundDep = false;

    // Walk up as needed to find a containing prim index.
    SdfPath indexPath;
    const PcpPrimIndex *primIndex = nullptr;
    for (indexPath = depIndexPath.GetAbsoluteRootOrPrimPath();
         indexPath != SdfPath();
         indexPath = indexPath.GetParentPath())
    {
        primIndex = cache.FindPrimIndex(indexPath);
        if (primIndex) {
            break;
        }
    }
    if (primIndex) {
        // Find which node corresponds to (layerOrLayerStack, sitePath).
        for (const PcpNodeRef &node: primIndex->GetNodeRange()) {
            if (PcpNodeIntroducesDependency(node) && 
                Pcp_NodeUsesLayerOrLayerStack(node, layerOrLayerStack) &&
                sitePath.HasPrefix(node.GetPath()))
            {
                foundDep = true;
                fn(depIndexPath, node);
            }
        }
    }

    return foundDep;
}

/// Run a callback for each node in the prim index at \p depIndexPath in
/// \p cache that introduced a dependency on the prim at \p sitePath or
/// an ancestor in \p layerOrLayerStack.
/// 
/// The \p nodeFn callback will be called with \p depIndexPath and the
/// PcpNodeRef for each dependent node in the prim index.
template <class FN, class LayerOrLayerStack>
void
Pcp_ForEachDependentNode( const SdfPath &sitePath,
                          const LayerOrLayerStack &layerOrLayerStack,
                          const SdfPath &depIndexPath,
                          const PcpCache &cache,
                          const FN &nodeFn)
{
    const bool foundDep = Pcp_ForEachDependentNodeImpl(
        sitePath, layerOrLayerStack, depIndexPath, cache, nodeFn);

    TF_VERIFY(
            foundDep,
            "Unable to find node that introduced dependency on site "
            "<%s>@%s@ for prim <%s> in %s", 
            sitePath.GetText(),
            TfStringify(layerOrLayerStack->GetIdentifier()).c_str(),
            depIndexPath.GetText(),
            TfStringify(cache.GetLayerStack()->GetIdentifier()).c_str()
            );
}

/// Run a callback for each node or culled dependency from the prim index at
/// \p depIndexPath in \p cache that introduced a dependency on the prim at
/// \p sitePath or an ancestor in \p layerStack.
/// 
/// The \p nodeFn callback will be called with \p depIndexPath and the
/// PcpNodeRef for each dependent node in the prim index.
///
/// The \p culledDepFn will be called with \p depIndexPath and the
/// PcpCulledDependency for each culled dependent node in the prim index.
template <class NodeFn, class CulledDepFn>
void
Pcp_ForEachDependentNode( const SdfPath &sitePath,
                          const PcpLayerStackRefPtr &layerStack,
                          const SdfPath &depIndexPath,
                          const PcpCache &cache,
                          const NodeFn &nodeFn,
                          const CulledDepFn &culledDepFn)
{
    bool foundDep = Pcp_ForEachDependentNodeImpl(
        sitePath, layerStack, depIndexPath, cache, nodeFn);

    const PcpCulledDependencyVector& culledDeps =
        Pcp_Dependencies::GetCulledDependencies(cache, depIndexPath);
    for (const PcpCulledDependency& dep : culledDeps) {
        if (layerStack == dep.layerStack &&
            sitePath.HasPrefix(dep.sitePath)) {
            foundDep = true;
            culledDepFn(depIndexPath, dep);
        }
    }

    TF_VERIFY(
            foundDep,
            "Unable to find node that introduced dependency on site "
            "<%s>@%s@ for prim <%s> in %s", 
            sitePath.GetText(),
            TfStringify(layerStack->GetIdentifier()).c_str(),
            depIndexPath.GetText(),
            TfStringify(cache.GetLayerStack()->GetIdentifier()).c_str()
            );
}

/// Record a PcpCulledDependency for \p node in \p culledDeps if that
/// node would be recorded by Pcp_Dependencies if it remained in the
/// prim index.
void
Pcp_AddCulledDependency(
    const PcpNodeRef& node,
    PcpCulledDependencyVector* culledDeps);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_DEPENDENCIES_H
