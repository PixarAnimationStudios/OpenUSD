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
#include "pxr/usd/pcp/namespaceEdits.h" 
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/dependencies.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/base/tracelite/trace.h"

#include <algorithm>
#include <utility>
#include <vector>

using std::make_pair;
using std::pair;
using std::vector;

using boost::dynamic_pointer_cast;

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(PcpNamespaceEdits::EditPath);
    TF_ADD_ENUM_NAME(PcpNamespaceEdits::EditInherit);
    TF_ADD_ENUM_NAME(PcpNamespaceEdits::EditReference);
    TF_ADD_ENUM_NAME(PcpNamespaceEdits::EditPayload);
    TF_ADD_ENUM_NAME(PcpNamespaceEdits::EditRelocate);
}

static bool
_IsInvalidEdit(const SdfPath& oldPath, const SdfPath& newPath)
{
    // Can't reparent an object to be a descendant of itself.
    // See testPcpRegressionBugs_bug109700 for more details
    // on how this can happen.
    return newPath.HasPrefix(oldPath);
}

static PcpNamespaceEdits::LayerStackSites &
_GetLayerStackSitesForEdit(
    PcpNamespaceEdits* result,
    const SdfPath& oldPath,
    const SdfPath& newPath)
{
    // Catch any invalid fixups that have been requested here and
    // store them away in invalidLayerStackSites so that consumers
    // can be informed about them.
    return _IsInvalidEdit(oldPath, newPath) ?
        result->invalidLayerStackSites : result->layerStackSites;
}

static bool
_RelocatesMapContainsPrimOrDescendant(
    const SdfRelocatesMapProxy& reloMap,
    const SdfPath& primPath)
{
    TF_FOR_ALL(it, reloMap) {
        if (it->first.HasPrefix(primPath) ||
            it->second.HasPrefix(primPath)) {
            return true;
        }
    }

    return false;
}

static void
_AddRelocateEditsForLayerStack(
    PcpNamespaceEdits* result,
    const PcpLayerStackPtr& layerStack,
    size_t cacheIndex,
    const SdfPath& oldRelocatePath,
    const SdfPath& newRelocatePath)
{
    if (!result) {
        return;
    }

    // Record a relocates edit for each layer stack site if any prim spec
    // at that site has a relocates statement that contains oldRelocatePath.
    // 
    // XXX: If this is a performance issue, PcpLayerStack could keep track 
    //      of a finer-grained table to avoid scanning through every prim 
    //      with relocates here.
    const SdfPathVector& relocatePrimPaths = 
        layerStack->GetPathsToPrimsWithRelocates();
    TF_FOR_ALL(pathIt, relocatePrimPaths) {
        TF_FOR_ALL(layerIt, layerStack->GetLayers()) {
            const SdfPrimSpecHandle prim = (*layerIt)->GetPrimAtPath(*pathIt);
            // The relocate we discovered in the layerStack at this path
            // doesn't necessarily mean there is a spec with a relocate
            // in every layer.  Skip layers that don't have a spec with
            // a relocate.
            if (!prim || !prim->HasRelocates()) {
                continue;
            }

            if (_RelocatesMapContainsPrimOrDescendant(
                    prim->GetRelocates(), oldRelocatePath)) {

                PcpNamespaceEdits::LayerStackSites& layerStackSites = 
                    _GetLayerStackSitesForEdit(
                        result, oldRelocatePath, newRelocatePath);

                layerStackSites.resize(layerStackSites.size() + 1);
                PcpNamespaceEdits::LayerStackSite& site =
                    layerStackSites.back();
                site.cacheIndex = cacheIndex;
                site.type       = PcpNamespaceEdits::EditRelocate;
                site.sitePath   = *pathIt;
                site.oldPath    = oldRelocatePath;
                site.newPath    = newRelocatePath;
                site.layerStack = layerStack;

                // Since we record edits at the granularity of layer stacks,
                // we can bail out once we've determined that at least one
                // prim in this layer stack needs relocates edits.
                break;
            }
        }
    }
}

static SdfPath
_TranslatePathAndTargetPaths(
    const PcpNodeRef& node,
    const SdfPath& pathIn)
{
    SdfPath path = node.GetMapToParent().MapSourceToTarget(pathIn);

    if (path == pathIn) {
        // We don't want to map paths that aren't explicitly allowed.  The
        // </> -> </> mapping should not be attempted.
        const SdfPath absRoot = SdfPath::AbsoluteRootPath();
        if (node.GetMapToParent().MapSourceToTarget(absRoot) == absRoot) {
            return SdfPath();
        }
    }

    SdfPathVector targetPaths;
    path.GetAllTargetPathsRecursively(&targetPaths);
    for (const auto& targetPath : targetPaths) {
        // Do allow translation via </> -> </> in target paths.
        const SdfPath translatedTargetPath =
            node.GetMapToParent().MapSourceToTarget(targetPath);
        if (translatedTargetPath.IsEmpty()) {
            return SdfPath();
        }
        path = path.ReplacePrefix(targetPath, translatedTargetPath);
    }

    return path;
}

// Translate *oldNodePath and *newNodePath to node's parent's namespace.
// Request any necessary edits to relocations used for the translation.
static void
_TranslatePathsAndEditRelocates(
    PcpNamespaceEdits* result,
    const PcpNodeRef& node,
    size_t cacheIndex,
    SdfPath* oldNodePath,
    SdfPath* newNodePath)
{
    // Map the path in the node namespace to the parent's namespace.  Note
    // that these are not the namespace parent paths, they're the paths in
    // the *node's parent's* namespace.
    SdfPath oldParentPath = _TranslatePathAndTargetPaths(node, *oldNodePath);
    SdfPath newParentPath = _TranslatePathAndTargetPaths(node, *newNodePath);

    // Check if there are relocations that need fixing.
    //
    // At this point, if oldParentPath and newParentPath refer to a 
    // relocated prim or a descendant of a relocated prim, they will have 
    // already had the relevant relocations from the parent's layerStack 
    // applied.
    // Check if one of these relocations affected oldParentPath.
    //
    const PcpLayerStackPtr& layerStack = 
        node.GetParentNode().GetLayerStack();

    // Since oldParentPath and newParentPath are post-relocation,
    // we'll use the targetToSource table to check for applicable
    // relocations.
    const SdfRelocatesMap& relocates = layerStack->GetRelocatesTargetToSource();

    // Find the relocation.
    SdfRelocatesMap::const_iterator i = relocates.lower_bound(oldParentPath);
    if (i != relocates.end() && oldParentPath.HasPrefix(i->first)) {
        const SdfPath & reloTargetPath = i->first;
        const SdfPath & reloSourcePath = i->second;

        // Un-relocate oldParentPath and newParentPath.
        // We will use these paths to decide how to fix the
        // relocation that applied to them.
        const SdfPath unrelocatedOldParentPath =
            oldParentPath.ReplacePrefix(reloTargetPath, reloSourcePath);
        const SdfPath unrelocatedNewParentPath =
            newParentPath.ReplacePrefix(reloTargetPath, reloSourcePath);

        bool reloTargetNeedsEdit = true;

        // Old path is relocated in this layer stack.  We might need to
        // change the relocation as part of the namespace edit.  If so
        // we'll relocate the new path differently from the old path.
        if (oldParentPath == reloTargetPath) {
            // Relocating the new parent path depends on various stuff.
            if (newParentPath.IsEmpty()) {
                // Removing the object or can't map across the arc.
                // Nothing to translate, but need to add a relocation edit
                // to indicate that relocations that involve prims at and
                // below oldParentPath need to be fixed.
                _AddRelocateEditsForLayerStack(
                    result, layerStack, cacheIndex,
                    reloTargetPath, newParentPath);
            }
            else if (oldNodePath->GetParentPath() !=
                     newNodePath->GetParentPath()) {
                // Reparenting within the arc's root.  We'll fix the
                // relocation source but not the target.
                _AddRelocateEditsForLayerStack(
                    result, layerStack, cacheIndex, 
                    unrelocatedOldParentPath, unrelocatedNewParentPath);

                reloTargetNeedsEdit = false;
            }
            else {
                // Renaming.  We must fix the relocation source path,
                // and potentially also the relocation target path
                // (if the relocation keeps the prim name).
                _AddRelocateEditsForLayerStack(
                    result, layerStack, cacheIndex,
                    unrelocatedOldParentPath, unrelocatedNewParentPath);

                // If the relocation keeps the prim name then
                // we'll fix the relocation by changing the final prim
                // name in both the source and target.  So the new parent
                // path is the old parent path with the name changed.
                if (reloSourcePath.GetNameToken()
                    == reloTargetPath.GetNameToken()) {
                    // Relocate the new path.
                    newParentPath = reloTargetPath
                        .ReplaceName(newNodePath->GetNameToken());

                    _AddRelocateEditsForLayerStack(
                        result, layerStack, cacheIndex,
                        reloTargetPath, newParentPath);
                }
                else {
                    // The relocation changes the prim name.  We've no
                    // reason to try to adjust the target's name but we
                    // should change the source name.
                    reloTargetNeedsEdit = false;
                }
            }

            if (!reloTargetNeedsEdit) {
                // Since the relocation target isn't changing, this node
                // 'absorbs' the namespace edit -- no layer stacks that 
                // reference this layer stack need to be updated.  So, 
                // we can stop traversing the graph looking for things
                // that need to be fixed.  Indicate that to consumers by 
                // setting newParentPath = oldParentPath.
                newParentPath = oldParentPath;
            }
        }
        else {
            // We don't need to fix the relocation.
        }
    }
    else {
        // In this case, oldParentPath and newParentPath do not refer to a
        // relocated prim.  However, there may be descendants of oldParentPath
        // that have been relocated, requiring relocates to be fixed up.
        _AddRelocateEditsForLayerStack(
            result, layerStack, cacheIndex,
            oldParentPath, newParentPath);
    }

    *oldNodePath = oldParentPath;
    *newNodePath = newParentPath;
}

static bool
_AddLayerStackSite(
    PcpNamespaceEdits* result,
    const PcpNodeRef& node,
    size_t cacheIndex,
    SdfPath* oldNodePath,
    SdfPath* newNodePath)
{
    bool final = false;

    // Save the old paths.
    SdfPath oldPath = *oldNodePath, newPath = *newNodePath;

    // Translate the paths to the parent.
    _TranslatePathsAndEditRelocates(result, node, cacheIndex,
                                    oldNodePath, newNodePath);

    // The site is the parent's path.
    SdfPath sitePath = *oldNodePath;

    // Compute the type of edit.
    PcpNamespaceEdits::EditType type;
    if (node.GetArcType() == PcpArcTypeRelocate) {
        // Ignore.
        *oldNodePath = oldPath;
        *newNodePath = newPath;
        TF_DEBUG(PCP_NAMESPACE_EDIT)
            .Msg("  - not final. skipping relocate\n");
        return final;
    }
    else if (*oldNodePath == *newNodePath) {
        // The edit is absorbed by this layer stack, so there's
        // no need to propagate the edit any further.
        TF_DEBUG(PCP_NAMESPACE_EDIT)
            .Msg("  - final.  stopping at node where path is unaffected\n");
        final = true;
        return final;
    }
    else if (oldNodePath->IsPrimPath() && !node.IsDueToAncestor()) {
        final = true;
        TF_DEBUG(PCP_NAMESPACE_EDIT)
            .Msg("  - final.  direct arc fixup\n");
        switch (node.GetArcType()) {
        case PcpArcTypeLocalInherit:
        case PcpArcTypeGlobalInherit:
            type = PcpNamespaceEdits::EditInherit;
            break;

        case PcpArcTypeReference:
            type = PcpNamespaceEdits::EditReference;
            break;

        case PcpArcTypePayload:
            type = PcpNamespaceEdits::EditPayload;
            break;

        case PcpArcTypeVariant:
            // Do nothing.  The variant prim has no name (and therefore
            // nothing referring to the name) so there's nothing to do.
            return final;

        default:
            TF_VERIFY(false, "Unexpected arc type %d", node.GetArcType());
            return final;
        }
    }
    else {
        // NamespaceEditPath the parent.
        type    = PcpNamespaceEdits::EditPath;
        oldPath = *oldNodePath;
        newPath = *newNodePath;
    }

    // Add a new layer stack site element at the end.
    PcpNamespaceEdits::LayerStackSites& layerStackSites = 
        _GetLayerStackSitesForEdit(result, oldPath, newPath);

    layerStackSites.resize(layerStackSites.size() + 1);
    PcpNamespaceEdits::LayerStackSite& site = layerStackSites.back();

    // Fill in the site.
    site.cacheIndex = cacheIndex;
    site.type       = type;
    site.sitePath   = sitePath;
    site.oldPath    = oldPath;
    site.newPath    = newPath;
    site.layerStack = node.GetParentNode().GetLayerStack();

    TF_DEBUG(PCP_NAMESPACE_EDIT)
        .Msg("  - adding layer stack edit <%s> -> <%s>\n",
            site.oldPath.GetText(),
            site.newPath.GetText());

    return final;
}

PcpNamespaceEdits
PcpComputeNamespaceEdits(
    const PcpCache *primaryCache,
    const std::vector<PcpCache*>& caches,
    const SdfPath& curPath,
    const SdfPath& newPath,
    const SdfLayerHandle& relocatesLayer)
{
    TRACE_FUNCTION();

    PcpNamespaceEdits result;
    SdfPathVector depPaths;

    if (caches.empty()) {
        return result;
    }
    const PcpLayerStackRefPtr primaryLayerStack = primaryCache->GetLayerStack();

    // We find dependencies using prim paths.  Compute the closest prim
    // path to curPath.
    const SdfPath primPath = curPath.GetPrimPath();

    // Verify that a prim index at primPath exists.
    if (!primaryCache->FindPrimIndex(primPath)) {
        TF_CODING_ERROR("No prim index computed for %s<%s>\n",
                        TfStringify(primaryLayerStack->GetIdentifier()).c_str(),
                        curPath.GetText());
        return result;
    }

    // Handle trivial case.
    if (curPath == newPath) {
        return result;
    }

    // Find cache sites one cache at a time.  We can't simply check if a
    // site uses (primaryLayerStack, primPath) -- we must check if it uses any
    // site at primPath with an intersecting layer stack.  Even that's not
    // quite right -- we only care if the layer stacks intersect where a
    // spec already exists (see bug 59216).  And, unfortunately, that's
    // not right either -- if (primaryLayerStack, primPath) has no specs at all
    // (because opinions come across an ancestor arc) then we're doing a
    // relocation and only sites using primPath in a layer stack that
    // includes relocatesLayer are affected.  We special case the last
    // case.  The earlier cases we handle by looking for any site using
    // any spec at the namespace edited site.

    // Find all specs at (primaryLayerStack, primPath).
    SdfSiteVector primSites;
    PcpComposeSitePrimSites(
        PcpLayerStackSite(primaryLayerStack, primPath), &primSites);

    // Find the nodes corresponding to primPath in any relevant layer
    // stack over all caches.
    typedef std::pair<size_t, PcpNodeRef> CacheNodePair;
    std::set<CacheNodePair> nodes, descendantNodes;

    struct _CacheNodeHelper {
        static void InsertCacheNodePair(size_t cacheIdx, PcpNodeRef node,
                                        std::set<CacheNodePair>* nodes)
        {
            // If a dependency on primPath was introduced via a variant
            // node (e.g., a prim authored locally in a variant), we store
            // the node that introduced the variant as this truly represents
            // the namespace edited site.
            while (node && node.GetArcType() == PcpArcTypeVariant) {
                node = node.GetParentNode();
            }

            if (TF_VERIFY(node)) {
                nodes->insert(std::make_pair(cacheIdx, node));
            }
        }
    };

    if (primSites.empty()) {
        // This is the relocation case.
        // We'll find every site using (someLayerStack, primPath)
        // where someLayerStack is any layer stack that includes
        // relocatesLayer.
        for (size_t cacheIndex = 0, n = caches.size();
                                        cacheIndex != n; ++cacheIndex) {
            PcpCache* cache = caches[cacheIndex];

            // Store the node for each dependent site.
            PcpDependencyVector deps =
                cache->FindSiteDependencies(
                    relocatesLayer, primPath,
                    PcpDependencyTypeAnyNonVirtual,
                    /* recurseOnSite */ true,
                    /* recurseOnIndex */ true,
                    /* filter */ true);
            auto visitNodeFn = [&](const SdfPath &depIndexPath,
                                   const PcpNodeRef &node,
                                   PcpDependencyFlags flags) {
                _CacheNodeHelper::InsertCacheNodePair(cacheIndex,
                                                      node, &nodes);
            };
            for(const PcpDependency &dep: deps) {
                Pcp_ForEachDependentNode(dep.sitePath, relocatesLayer,
                                         dep.indexPath,
                                         *cache, visitNodeFn);
            }
        }
    }
    else {
        // We find dependent sites by looking for used prim specs.
        for (size_t cacheIndex = 0, n = caches.size();
                                        cacheIndex != n; ++cacheIndex) {
            PcpCache* cache = caches[cacheIndex];

            // Store the node for each dependent site.
            for(const SdfSite& primSite: primSites) {
                PcpDependencyVector deps =
                    cache->FindSiteDependencies(
                        primSite.layer, primPath,
                        PcpDependencyTypeAnyNonVirtual,
                        /* recurseOnSite */ false,
                        /* recurseOnIndex */ false,
                        /* filter */ true);
                auto visitNodeFn = [&](const SdfPath &depIndexPath,
                                       const PcpNodeRef &node,
                                       PcpDependencyFlags flags) {
                    TF_DEBUG(PCP_NAMESPACE_EDIT)
                        .Msg(" found dep node: <%s> -> <%s> %s\n",
                             depIndexPath.GetText(),
                             node.GetPath().GetText(),
                            PcpDependencyFlagsToString(flags).c_str());
                    if (flags != PcpDependencyTypeNone) {
                        _CacheNodeHelper::InsertCacheNodePair(cacheIndex,
                                                              node, &nodes);
                    }
                };
                for(const PcpDependency &dep: deps) {
                    Pcp_ForEachDependentNode(dep.sitePath, primSite.layer,
                                             dep.indexPath,
                                             *cache, visitNodeFn);
                }
            }

            // Special case for direct inherits.  An inherit can target a
            // descendant of curPath and we must fix up those inherits.
            // References and payloads can't target a non-root prim so we
            // don't have to worry about those.
            //
            // XXX: We only do this in this cache.  Inherits in this
            //      cache can definitely see the namespace edit so
            //      they must be fixed, but can't inherits outside
            //      this cache also see the namespace edit?
            if (cache == primaryCache && curPath.IsPrimPath()) {

                SdfPathSet descendentPrimPaths;

                const PcpDependencyFlags depMask =
                   PcpDependencyTypeDirect | PcpDependencyTypeNonVirtual;

                // Get all of the direct dependents on the namespace edited
                // site and anything below.
                for (const PcpDependency &dep:
                    primaryCache->FindSiteDependencies(
                        primaryLayerStack, primPath, depMask,
                        /* recurseOnSite */ true,
                        /* recurseOnIndex */ false,
                        /* filter */ true)) {
                    if (dep.indexPath.IsPrimPath()) {
                        descendentPrimPaths.insert(dep.indexPath);
                    }
                }

                // Remove the direct dependents on the site itself.
                for (const PcpDependency &dep:
                    primaryCache->FindSiteDependencies(
                        primaryLayerStack, primPath, depMask,
                        /* recurseOnSite */ false,
                        /* recurseOnIndex */ false,
                        /* filter */ true)) {
                    descendentPrimPaths.erase(dep.indexPath);
                }

                // Check each direct dependent site for inherits pointing
                // at this cache's layer stack. Make sure to skip ancestral
                // nodes, since the code that handles direct inherits below
                // needs to have the nodes where the inherits are introduced.
                for (const SdfPath& descendentPrimPath : descendentPrimPaths) {
                    // We were just told this prim index is a deependency
                    // so it certainly should exist.
                    const PcpPrimIndex *index =
                        primaryCache->FindPrimIndex(descendentPrimPath);
                    if (TF_VERIFY(index, "Reported descendent dependency "
                                  "lacks a prim index")) {
                        for (const PcpNodeRef &node:
                             index->GetNodeRange(PcpRangeTypeLocalInherit)) {
                            if (node.GetLayerStack() == primaryLayerStack &&
                                !node.IsDueToAncestor()) {
                                // Found an inherit using a descendant.
                                descendantNodes.insert(
                                   std::make_pair(cacheIndex, node));
                            }
                        }
                    }
                }
            }
        }
    }

    // We now have every node representing the namespace edited site in
    // every graph in every cache in caches that uses that site.  We now
    // need to convert them into something the client can use.  There are
    // two kinds of sites from the client's point of view:
    //
    //   1) Composed sites.  Composed sites are stored in
    //      result.cacheSites.  They represent composed namespace
    //      that is being namespace edited, i.e. the object is
    //      being renamed, reparented, renamed and reparented, or
    //      removed. They're identified by a cache (index) and path.
    //      
    //   2) Uncomposed sites.  Uncomposed sites are stored in
    //      result.layerStackSites.  Each site is a layer stack and a
    //      path (the site path) -- all Sd specs at the path in any
    //      layer in the layer stack must be fixed up the same way.
    //
    // We don't include composed sites here just because they have a
    // reference (or payload or inherit) to a site that's being namespace
    // edited.  The reference itself absorbs the namespace edit, so the
    // object with the reference doesn't need a namespace edit.  For
    // example, if </A> references </B> and we rename </B> to </C> we need
    // to fix the reference on </A> but we don't need to change the name
    // of </A>. So </B> is added to cacheSites but </A> is not.
    //
    // To find composed sites we simply include one for every node that
    // doesn't have any direct (i.e. non-ancestral) reference, inherit, or
    // payload on the traversal of the graph from node to the root.  The
    // old and new paths are found by translating the edited site's old
    // and new paths from the node to the root.
    //
    // We expect the caller (i.e. Csd) to consume cacheSites to fix up
    // connections and targets that point to anything in cacheSites.  It
    // must also fix up relocations involving old paths in cacheSites.
    //
    // Uncomposed sites are found by walking the graph from node to root
    // for each node.  Every node is an uncomposed site.  If we find a
    // a direct (i.e. non-ancestral) reference, inherit, or payload then
    // we store the referencing site and stop the traversal because the
    // reference absorbs the namespace edit.
    //
    // The trick is to do path translation correctly while traversing the
    // graph.  The easy but wrong way is to translate the path from node
    // to root once, then translate the root paths to each node in the
    // traversal.  That doesn't work for the new path if we need to fix
    // any relocation, reference, inherit, or payload along the traversal
    // because the mapping functions will not have the new mapping.  Using
    // the example above </A> references </B> and we rename </B> to </C>.
    // We can't map path </C> across the existing reference because it
    // maps </A> -> </B>.  If we try we'll just get the empty path.  So
    // we translate the path using mapToParent across each arc and we
    // account for relocations as necessary.
    //
    // If we're removing then we must also remove every object using any
    // descendant of the namespace edited prim.  (This only applies to
    // prims since non-prims can't have arcs.)  This cleans up the layers
    // in a way that users expect.  Note, however, that we remove objects
    // even if they're provided via other arcs.  That could be unexpected
    // but it doesn't normally happen.  We don't find these descendants
    // during traversal;  instead we find them separately by getting sites
    // using any prim spec that's a descendant of the namespace edited
    // object.
    //
    // Similar to removing, if we reparent a descendant of a referenced
    // (or inherited or payloaded) prim outside of the arc then it's as
    // if the object was removed as far as the referencing site is
    // concerned.
    //
    // Uncomposed sites have an edit type associated with them, along
    // with an old path and a new path.  The type may be a namespace edit
    // (in which case the site path and the old path are the same);  an
    // edit to the references, inherits, or payload where old path must
    // be replaced with new path;  or a relocation where we must replace
    // old path with new path in every relocation table on every ancestor.
    // These edits must be applied directly to the Sd objects.  If we know
    // there are no opinions to fix at a given uncomposed site we don't
    // have to record the site (but may anyway).

    // XXX: We need to report errors, too.  There are various edits
    //      that can't be represented and we should detect them and
    //      call them out.  Examples are:
    //        1) Reparent a referenced/payloaded prim (e.g. /B ->
    //           /X/B).  We can't target a non-root prim.
    //        2) Rename a prim into namespace already used upstream.
    //           E.g. </A> references </B>, </A/Y> exists and so
    //           does </B/X> -- rename </B/X> to </B/Y>.  Doing so
    //           would cause </A/Y> to pull in </B/Y>, probably
    //           unexpectedly.
    //        3) Rename a prim into namespace that's salted earth
    //           upstream.  E.g. </A> references </B>, </A> relocates
    //           </A/Y> to </A/Z> and </B/X> exists -- rename </B/X>
    //           to </B/Y>.  Doing so would cause </B/Y> to map to
    //           the salted earth </A/Y>.
    // XXX: Should we be doing (layer,path) sites?  If we have
    //      intersecting layer stacks we might do the same spec
    //      twice.  Csd is aware of this so it's okay for now.

    // Walk the graph for each node.
    typedef PcpNamespaceEdits::LayerStackSites LayerStackSites;
    typedef PcpNamespaceEdits::LayerStackSite LayerStackSite;
    typedef PcpNamespaceEdits::CacheSite CacheSite;
    std::set<PcpLayerStackSite> sites;
    for (const auto& cacheAndNode : nodes) {
        size_t cacheIndex   = cacheAndNode.first;
        PcpNodeRef node     = cacheAndNode.second;
        SdfPath oldNodePath  = curPath;
        SdfPath newNodePath  = newPath;

        TF_DEBUG(PCP_NAMESPACE_EDIT)
            .Msg("\n processing node:\n"
            "  cache:           %s\n"
            "  node.type:       %s\n"
            "  node.path:       <%s>\n"
            "  node.rootPath:   <%s>\n"
            "  node.layerStack: %s\n"
            "  curPath:         <%s>\n"
            "  newPath:         <%s>\n"
            "  oldNodePath:     <%s>\n"
            "  newNodePath:     <%s>\n",
            TfStringify( caches[cacheIndex]
                         ->GetLayerStack()->GetIdentifier()).c_str(),
            TfStringify(node.GetArcType()).c_str(),
            node.GetPath().GetText(),
            node.GetRootNode().GetPath().GetText(),
            TfStringify(node.GetLayerStack()->GetIdentifier()).c_str(),
            curPath.GetText(),
            newPath.GetText(),
            oldNodePath.GetText(),
            newNodePath.GetText()
            );

        // Handle the node itself.  Note that the node, although representing
        // the namespace edited site, can appear in different layer stacks.
        // This happens when we have two scenes sharing a layer.  If we edit
        // in the shared layer then we must do the corresponding edit in each
        // of the scene's layer stacks.
        if (sites.insert(node.GetSite()).second) {
            LayerStackSites& layerStackSites = 
                _GetLayerStackSitesForEdit(&result, oldNodePath, newNodePath);
            layerStackSites.resize(layerStackSites.size()+1);
            LayerStackSite& site = layerStackSites.back();
            site.cacheIndex      = cacheIndex;
            site.type            = PcpNamespaceEdits::EditPath;
            site.sitePath        = oldNodePath;
            site.oldPath         = oldNodePath;
            site.newPath         = newNodePath;
            site.layerStack      = node.GetLayerStack();

            _AddRelocateEditsForLayerStack(
                &result, site.layerStack, cacheIndex,
                oldNodePath, newNodePath);
        }

        // Handle each arc from node to the root.
        while (node.GetParentNode()) {
            TF_DEBUG(PCP_NAMESPACE_EDIT)
                .Msg("  - traverse to parent of <%s>.  <%s> -> <%s>\n",
                   node.GetPath().GetText(),
                   oldNodePath.GetText(),
                   newNodePath.GetText());
            if (sites.insert(node.GetParentNode().GetSite()).second) {
                // Add site and translate paths to parent node.
                if (_AddLayerStackSite(&result, node, cacheIndex,
                                       &oldNodePath, &newNodePath)) {
                    // Reached a direct arc, so we don't have to continue.
                    // The composed object will continue to exist at the
                    // same path, with the arc target updated.
                    TF_DEBUG(PCP_NAMESPACE_EDIT)
                        .Msg("  - done!  fixed direct arc.\n");
                    break;
                }
            }
            else {
                TF_DEBUG(PCP_NAMESPACE_EDIT)
                    .Msg("  - adjusted path for relocate\n");
                // Translate paths to parent node.
                // Adjust relocates as needed.
                _TranslatePathsAndEditRelocates(NULL, node, cacheIndex,
                                                &oldNodePath, &newNodePath);
            }

            // Next node.
            node = node.GetParentNode();
        }

        // If we made it all the way to the root then we have a cacheSite.
        if (!node.GetParentNode()) {
            if (!_IsInvalidEdit(oldNodePath, newNodePath)) {
                TF_DEBUG(PCP_NAMESPACE_EDIT)
                    .Msg("  - adding cacheSite for %s\n",
                         node.GetPath().GetText());
                result.cacheSites.resize(result.cacheSites.size() + 1);
                CacheSite& cacheSite = result.cacheSites.back();
                cacheSite.cacheIndex = cacheIndex;
                cacheSite.oldPath    = oldNodePath;
                cacheSite.newPath    = newNodePath;
            }
        }
    }

    // Helper functions.
    struct _Helper {
        // Returns true if sites contains site or any ancestor of site.
        static bool HasSite(const std::map<PcpLayerStackSite, size_t>& sites,
                            const PcpLayerStackSite& site)
        {
            std::map<PcpLayerStackSite, size_t>::const_iterator i =
                sites.lower_bound(site);
            if (i != sites.end() && i->first == site) {
                // Site exists in sites.
                return true;
            }
            if (i == sites.begin()) {
                // Site comes before any other site so it's not in sites.
                return false;
            }
            --i;
            if (site.layerStack != i->first.layerStack) {
                // Closest site is in a different layer stack.
                return false;
            }
            if (site.path.HasPrefix(i->first.path)) {
                // Ancestor exists.
                return true;
            }
            // Neither site nor any ancestor of it is in sites.
            return false;
        }
    };

    // If we're removing a prim then also collect every uncomposed site
    // that uses a descendant of the namespace edited site.
    if (newPath.IsEmpty() && curPath.IsPrimPath()) {
        std::map<PcpLayerStackSite, size_t> descendantSites;

        // Make a set of sites we already know have direct arcs to
        // descendants.  We don't want to remove those but we may
        // want to remove their descendants.
        std::set<PcpLayerStackSite> doNotRemoveSites;
        for (const auto& cacheAndNode : descendantNodes) {
            const PcpNodeRef& node = cacheAndNode.second;
            doNotRemoveSites.insert(node.GetParentNode().GetSite());
        }

        for (size_t cacheIndex = 0, n = caches.size();
                                        cacheIndex != n; ++cacheIndex) {
            PcpCache* cache = caches[cacheIndex];

            TF_DEBUG(PCP_NAMESPACE_EDIT)
                .Msg("- dep cache: %s\n",
                TfStringify( cache->GetLayerStack()->GetIdentifier()).c_str());
            
            std::set<PcpLayerStackPtr> layerStacks;
            for(const SdfLayerRefPtr &layer: primaryLayerStack->GetLayers()) { 
                const PcpLayerStackPtrVector& layerStackVec =
                    cache->FindAllLayerStacksUsingLayer(layer);
                layerStacks.insert(layerStackVec.begin(), layerStackVec.end());
            }

            // Get the sites in cache that use any proper descendant of the
            // namespace edited site and what each site depends on.
            std::map<SdfPath, PcpNodeRef> descendantPathsAndNodes;
            for (const PcpLayerStackPtr& layerStack: layerStacks) {
                PcpDependencyVector deps =
                    cache->FindSiteDependencies(
                        layerStack, primPath,
                        PcpDependencyTypeAnyNonVirtual,
                        /* recurseOnSite */ true,
                        /* recurseOnIndex */ true,
                        /* filter */ true);
                auto visitNodeFn = [&](const SdfPath &depIndexPath,
                                       const PcpNodeRef &node,
                                       PcpDependencyFlags flags) {
                    if (!depIndexPath.IsPrimPath() ||
                        node.GetPath() != curPath) {
                        descendantPathsAndNodes[depIndexPath] = node;
                    }
                };
                for(const PcpDependency &dep: deps) {
                    // Check that specs exist at this site.  There may
                    // not be any, because we synthesized dependent paths
                    // with recurseOnIndex, which may not actually
                    // have depended on this site (and exist for other
                    // reasons).
                    if (PcpComposeSiteHasPrimSpecs(
                            PcpLayerStackSite(layerStack, dep.sitePath))) {
                        Pcp_ForEachDependentNode(dep.sitePath, layerStack,
                                                 dep.indexPath,
                                                 *cache, visitNodeFn);
                    }
                }
            }

            // Add every uncomposed site used by each (cache,path) pair
            // if we haven't already added its parent.  We don't need to
            // add a site if we've added its parent because removing the
            // parent will remove its descendants.  The result is that we
            // add every uncomposed site that doesn't have a direct arc to
            // another uncomposed site.
            //
            // Note that we only check nodes from the namespace edited
            // site and its descendants to the root.  Other nodes are due
            // to other arcs and not affected by the namespace edit.
            TF_FOR_ALL(i, descendantPathsAndNodes) {
                PcpNodeRef node              = i->second;
                const SdfPath& descendantPath = i->first;
                SdfPath descendantPrimPath    = descendantPath.GetPrimPath();

                for (; node; node = node.GetParentNode()) {
                    SdfPath path =
                        descendantPath.ReplacePrefix(descendantPrimPath,
                                                     node.GetPath());
                    PcpLayerStackSite site(node.GetLayerStack(), path);
                    if (!_Helper::HasSite(descendantSites, site)) {
                        // We haven't seen this site or an ancestor yet.
                        if (doNotRemoveSites.count(site) == 0) {
                            // We're not blocking the addition of this site.
                            descendantSites[site] = cacheIndex;
                        }
                    }
                }
            }
        }

        // We now have all the descendant sites to remove.  Add them to
        // result.layerStackSites.
        TF_FOR_ALL(j, descendantSites) {
            result.layerStackSites.resize(result.layerStackSites.size()+1);
            LayerStackSite& site = result.layerStackSites.back();
            site.cacheIndex      = j->second;
            site.type            = PcpNamespaceEdits::EditPath;
            site.sitePath        = j->first.path;
            site.oldPath         = j->first.path;
            site.newPath         = newPath;         // This is the empty path.
            site.layerStack      = j->first.layerStack;
        }
    }

    // Fix up all direct inherits to a descendant site.
    if (!descendantNodes.empty()) {
        for (const auto& cacheAndNode : descendantNodes) {
            size_t cacheIndex   = cacheAndNode.first;
            const PcpNodeRef& node = cacheAndNode.second;
            SdfPath oldNodePath  = node.GetPath();
            SdfPath newNodePath  = oldNodePath.ReplacePrefix(curPath, newPath);
            _AddLayerStackSite(&result, node, cacheIndex,
                               &oldNodePath, &newNodePath);
        }
    }

    // Final namespace edits.

    // Diagnostics.
    // TODO: We should split this into a PcpNamespaceEdits->string function
    // in diagnostics.cpp.
    if (TfDebug::IsEnabled(PCP_NAMESPACE_EDIT)) {
        TF_DEBUG(PCP_NAMESPACE_EDIT)
            .Msg("PcpComputeNamespaceEdits():\n"
                 "  cache:   %s\n"
                 "  curPath: <%s>\n"
                 "  newPath: <%s>\n",
                TfStringify( primaryLayerStack->GetIdentifier()).c_str(),
                curPath.GetText(),
                newPath.GetText()
                );
        TF_FOR_ALL(cacheSite, result.cacheSites) {
            TF_DEBUG(PCP_NAMESPACE_EDIT)
                .Msg(" cacheSite:\n"
                "  cache:   %s\n"
                "  oldPath: <%s>\n"
                "  newPath: <%s>\n",
                TfStringify( caches[cacheSite->cacheIndex]
                             ->GetLayerStack()->GetIdentifier()).c_str(),
                cacheSite->oldPath.GetText(),
                cacheSite->newPath.GetText());
        }
        TF_FOR_ALL(layerStackSite, result.layerStackSites) {
            TF_DEBUG(PCP_NAMESPACE_EDIT)
                .Msg(" layerStackSite:\n"
                "  cache:      %s\n"
                "  type:       %s\n"
                "  layerStack: %s\n"
                "  sitePath:   <%s>\n"
                "  oldPath:    <%s>\n"
                "  newPath:    <%s>\n",
                TfStringify( caches[layerStackSite->cacheIndex]
                             ->GetLayerStack()->GetIdentifier()).c_str(),
                TfStringify(layerStackSite->type).c_str(),
                TfStringify(layerStackSite->layerStack
                            ->GetIdentifier()).c_str(),
                layerStackSite->sitePath.GetText(),
                layerStackSite->oldPath.GetText(),
                layerStackSite->newPath.GetText());
        }
        TF_FOR_ALL(layerStackSite, result.invalidLayerStackSites) {
            TF_DEBUG(PCP_NAMESPACE_EDIT)
                .Msg(" invalidLayerStackSite:\n"
                "  cache:      %s\n"
                "  type:       %s\n"
                "  layerStack: %s\n"
                "  sitePath:   <%s>\n"
                "  oldPath:    <%s>\n"
                "  newPath:    <%s>\n",
                TfStringify( caches[layerStackSite->cacheIndex]
                             ->GetLayerStack()->GetIdentifier()).c_str(),
                TfStringify(layerStackSite->type).c_str(),
                TfStringify(layerStackSite->layerStack
                            ->GetIdentifier()).c_str(),
                layerStackSite->sitePath.GetText(),
                layerStackSite->oldPath.GetText(),
                layerStackSite->newPath.GetText());
        }
    }

    return result;
}
