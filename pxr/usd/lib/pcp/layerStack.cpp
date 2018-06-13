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
// \file LayerStack.cpp

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/changes.h"
#include "pxr/usd/pcp/layerStackRegistry.h"
#include "pxr/usd/pcp/layerPrefetchRequest.h"
#include "pxr/usd/pcp/utils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/mallocTag.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <unordered_set>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// Computing layer stacks

// XXX Parallel layer prefetch is disabled until Sd thread-safety issues
// can be fixed, specifically plugin loading:
// - FileFormat plugins
// - value type plugins for parsing AnimSplines
TF_DEFINE_ENV_SETTING(
    PCP_ENABLE_PARALLEL_LAYER_PREFETCH, false,
    "Enables parallel, threaded pre-fetch of sublayers.");

struct Pcp_SublayerInfo {
    Pcp_SublayerInfo(const SdfLayerRefPtr& layer_, const SdfLayerOffset& offset_):
        layer(layer_), offset(offset_) { }
    SdfLayerRefPtr layer;
    SdfLayerOffset offset;
};
typedef std::vector<Pcp_SublayerInfo> Pcp_SublayerInfoVector;

// Desired strict weak ordering.
class Pcp_SublayerOrdering {
public:
    Pcp_SublayerOrdering(const std::string& sessionOwner) :
        _sessionOwner(sessionOwner)
    {
        // Do nothing
    }

    // Returns true if a's layer has an owner equal to _sessionOwner.
    bool IsOwned(const Pcp_SublayerInfo& a) const
    {
        return a.layer->HasOwner() && 
               a.layer->GetOwner() == _sessionOwner;
    }

    // If one layer has the owner and the other does not then the one with
    // the owner is less than the other.  Otherwise the layers are equivalent.
    bool operator()(const Pcp_SublayerInfo& a,
                    const Pcp_SublayerInfo& b) const
    {
        return IsOwned(a) && !IsOwned(b);
    }

private:
    std::string _sessionOwner;
};

static void
_ApplyOwnedSublayerOrder(
    const PcpLayerStackIdentifier &identifier,
    const SdfLayerHandle & layer,
    const std::string& sessionOwner,
    Pcp_SublayerInfoVector* subtrees,
    PcpErrorVector *errors)
{
    // Reorder the given sublayers to give (opinion) priority to the sublayer
    // belonging to the session owner, if any.
    //
    // When the following conditions are met:
    //
    //     1. The session layer specifies a session owner
    //        (this should always be the case in an interactive session)
    //
    //     2. A layer specifies that its sublayers can be "owned"
    //        (e.g., the "anim" layer of a shot)
    //
    //     3. A sublayer of that layer belongs to the session owner
    //        (e.g., an animator's personal sublayer in a shared shot)
    //
    // ... then that sublayer will be moved to the front of the list of
    // sublayers, guaranteeing that it will have the strongest opinions among
    // its sibling layers.
    //
    // Note that this means the effective order of these sublayers will be
    // different between interactive sessions run by different users, which is
    // the intended result.

    // Sort if conditions 1 and 2 are met.
    if (!sessionOwner.empty() && layer->GetHasOwnedSubLayers()) {
        // Stable sort against owned layer.
        Pcp_SublayerOrdering ordering(sessionOwner);
        std::stable_sort(subtrees->begin(), subtrees->end(), ordering);

        // Complain if there was more than one owned layer.  This is not
        // a problem for our algorithm but, for now, it's cause for
        // concern to the user.
        if (!subtrees->empty() && ordering.IsOwned(subtrees->front())) {
            // The first layer is owned.  Get the range of layers that are
            // owned.
            Pcp_SublayerInfoVector::iterator first = subtrees->begin();
            Pcp_SublayerInfoVector::iterator last  =
                std::upper_bound(first, subtrees->end(), *first, ordering);

            // Report an error if more than one layer is owned.
            if (std::distance(first, last) > 1) {
                PcpErrorInvalidSublayerOwnershipPtr error =
                    PcpErrorInvalidSublayerOwnership::New();
                error->rootSite = PcpSite(identifier,
                                          SdfPath::AbsoluteRootPath());
                error->owner = sessionOwner;
                error->layer = layer;
                for (; first != last; ++first) {
                    error->sublayers.push_back(first->layer);
                }
                errors->push_back(error);
            }
        }
    }
}

void
Pcp_ComputeRelocationsForLayerStack(
    const SdfLayerRefPtrVector & layers,
    SdfRelocatesMap *relocatesSourceToTarget,
    SdfRelocatesMap *relocatesTargetToSource,
    SdfRelocatesMap *incrementalRelocatesSourceToTarget,
    SdfRelocatesMap *incrementalRelocatesTargetToSource,
    SdfPathVector *relocatesPrimPaths)
{
    TRACE_FUNCTION();

    // Compose authored relocation arcs per prim path.
    std::map<SdfPath, SdfRelocatesMap> relocatesPerPrim;
    static const TfToken field = SdfFieldKeys->Relocates;
    TF_REVERSE_FOR_ALL(layer, layers) {
        // Check for relocation arcs in this layer.
        SdfPrimSpecHandleVector stack;
        stack.push_back( (*layer)->GetPseudoRoot() );
        while (!stack.empty()) {
            SdfPrimSpecHandle prim = stack.back();
            stack.pop_back();
            // Push back any children.
            TF_FOR_ALL(child, prim->GetNameChildren()) {
                stack.push_back(*child);
            }
            // Check for relocations.
            if (!prim->HasField(field)) {
                // No opinion in this layer.
                continue;
            }
            const VtValue& fieldValue = prim->GetField(field);
            if (!fieldValue.IsHolding<SdfRelocatesMap>()) {
                TF_CODING_ERROR("Field '%s' in <%s> in layer @%s@"
                                "does not contain an SdfRelocatesMap", 
                                field.GetText(), prim->GetPath().GetText(),
                                (*layer)->GetIdentifier().c_str());
                continue;
            }
            const SdfPath & primPath = prim->GetPath();
            const SdfRelocatesMap & relocMap =
                fieldValue.UncheckedGet<SdfRelocatesMap>();
            TF_FOR_ALL(reloc, relocMap) {
                // Absolutize source/target paths.
                SdfPath source = reloc->first .MakeAbsolutePath(primPath);
                SdfPath target = reloc->second.MakeAbsolutePath(primPath);
                if (source == target || source.HasPrefix(target)) {
                    // Skip relocations from a path P back to itself and
                    // relocations from a path P to an ancestor of P.
                    // (The authoring code in Csd should never create these,
                    // but they can be introduced by hand-editing.)
                    //
                    // Including them in the composed table would complicate
                    // life downstream, since all consumers of this table
                    // would have to be aware of this weird edge-case
                    // scenario.
                    //
                    // XXX: Although Csd already throws a warning
                    //      when this happens, we should also add a
                    //      formal PcpError for this case.  Perhaps
                    //      we can do this when removing the
                    //      non-Pcp-mode composition code from Csd.
                }
                else {
                    relocatesPerPrim[primPath][source] = target;
                }
            }

            relocatesPrimPaths->push_back(prim->GetPath());
        }
    }

    // Compose the final set of relocation arcs for this layer stack,
    // taking into account the cumulative effect of relocations down
    // namespace.
    TF_FOR_ALL(relocatesForPath, relocatesPerPrim) {
        TF_FOR_ALL(reloc, relocatesForPath->second) {
            SdfPath source = reloc->first;
            const SdfPath & target = reloc->second;

            (*incrementalRelocatesTargetToSource)[target] = source;
            (*incrementalRelocatesSourceToTarget)[source] = target;

            // Check for ancestral relocations.  The source path may have
            // ancestors that were themselves the target of an ancestral
            // relocate.
            for (SdfPath p = source; !p.IsEmpty(); p = p.GetParentPath()) {
                // We rely on the fact that relocatesPerPrim is stored
                // and traversed in namespace order to ensure that we
                // have already incoporated ancestral arcs into
                // relocatesTargetToSource.
                SdfRelocatesMap::const_iterator i =
                    relocatesTargetToSource->find(p);
                if (i != relocatesTargetToSource->end()) {
                    // Ancestral source path p was itself a relocation
                    // target.  Follow back to the ancestral source.
                    source = source.ReplacePrefix(i->first, i->second);
                    // Continue the traversal at the ancestral source.
                    p = i->second;
                }
            }

            // Establish a bi-directional mapping: source <-> target.
            (*relocatesTargetToSource)[target] = source;
            (*relocatesSourceToTarget)[source] = target;
        }
    }
}

static PcpMapFunction
_FilterRelocationsForPath(const PcpLayerStack& layerStack,
                          const SdfPath& path)
{
    // Gather the relocations that affect this path.
    PcpMapFunction::PathMap siteRelocates;

    // If this layer stack has relocates nested in namespace, the combined
    // and incremental relocates map will both have an entry with the same
    // target. We cannot include both in the map function, since that would
    // make it non-invertible. In this case, we use the entry from the 
    // combined map since that's what consumers are expecting.
    std::unordered_set<SdfPath, SdfPath::Hash> seenTargets;

    const SdfRelocatesMap& relocates = layerStack.GetRelocatesSourceToTarget();
    for (SdfRelocatesMap::const_iterator
         i = relocates.lower_bound(path), n = relocates.end();
         (i != n) && (i->first.HasPrefix(path)); ++i) {
        siteRelocates.insert(*i);
        seenTargets.insert(i->second);
    }

    const SdfRelocatesMap& incrementalRelocates = 
        layerStack.GetIncrementalRelocatesSourceToTarget();
    for (SdfRelocatesMap::const_iterator
         i = incrementalRelocates.lower_bound(path), 
         n = incrementalRelocates.end();
         (i != n) && (i->first.HasPrefix(path)); ++i) {

        if (seenTargets.find(i->second) == seenTargets.end()) {
            siteRelocates.insert(*i);
            seenTargets.insert(i->second);
        }
    }

    siteRelocates[SdfPath::AbsoluteRootPath()] = SdfPath::AbsoluteRootPath();

    // Return a map function representing the relocates.
    return PcpMapFunction::Create(siteRelocates, SdfLayerOffset());
}

////////////////////////////////////////////////////////////////////////

bool
Pcp_NeedToRecomputeDueToAssetPathChange(const PcpLayerStackPtr& layerStack)
{
    ArResolverContextBinder binder(
        layerStack->GetIdentifier().pathResolverContext);

    // Iterate through _sublayerSourceInfo to see if recomputing the
    // asset paths used to open sublayers would result in different
    // sublayers being opened.
    for (const auto& sourceInfo : layerStack->_sublayerSourceInfo) {
        const std::string& assetPath = SdfComputeAssetPathRelativeToLayer(
            sourceInfo.layer, sourceInfo.authoredSublayerPath);
        if (assetPath != sourceInfo.computedSublayerPath) {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
// PcpLayerStack

PcpLayerStack::PcpLayerStack(
    const PcpLayerStackIdentifier& identifier,
    const std::string &targetSchema,
    const Pcp_MutedLayers &mutedLayers,
    bool isUsd) :
    _identifier(identifier),
    _isUsd(isUsd)
{
    TfAutoMallocTag2 tag("Pcp", "PcpLayerStack::PcpLayerStack");
    TRACE_FUNCTION();

    if (!TF_VERIFY(_identifier)) {
        return;
    }

    _Compute(targetSchema, mutedLayers);

    if (!_isUsd) {
        Pcp_ComputeRelocationsForLayerStack(_layers, 
                                            &_relocatesSourceToTarget,
                                            &_relocatesTargetToSource,
                                            &_incrementalRelocatesSourceToTarget,
                                            &_incrementalRelocatesTargetToSource,
                                            &_relocatesPrimPaths);
    }
}

PcpLayerStack::~PcpLayerStack()
{
    // Update layer-stack-to-layer maps in the registry.
    _layers.clear();

    if (_registry) {
        _registry->_SetLayers(this);
        _registry->_Remove(_identifier, this);
    }
}

void
PcpLayerStack::Apply(const PcpLayerStackChanges& changes, PcpLifeboat* lifeboat)
{
    // Invalidate the layer stack as necessary, recomputing immediately.
    // Recomputing immediately assists optimal change processing --
    // e.g. it lets us examine the before/after chagnge to relocations.
    
    // Blow layer tree/offsets if necessary.
    if (changes.didChangeLayers || changes.didChangeLayerOffsets) {
        // The following comment applies to didChangeLayerOffsets:
        // XXX: We should just blow the layer offsets but for now
        //      now it's easier to just blow the whole layer stack.
        //      When we blow just the offsets we won't retain layers.

        // Retain prior set of layers.
        TF_FOR_ALL(i, _layers) {
            lifeboat->Retain(*i);
        }
        _BlowLayers();
        _Compute(_registry->_GetTargetSchema(), _registry->_GetMutedLayers());
    }

    // Update relocations if necessary.
    if (!_isUsd &&
        (changes.didChangeSignificantly || changes.didChangeRelocates)) {
        // Blow the relocations if they changed specifically, or if there's been
        // a significant change.
        // A significant change means the composed opinions of the layer stack
        // has changed in arbitrary ways, so we need to recompute the relocation
        // table.
        _BlowRelocations();
        if (changes.didChangeSignificantly) {
            // Recompute relocations from scratch.
            Pcp_ComputeRelocationsForLayerStack(
                _layers, 
                &_relocatesSourceToTarget,
                &_relocatesTargetToSource,
                &_incrementalRelocatesSourceToTarget,
                &_incrementalRelocatesTargetToSource,
                &_relocatesPrimPaths);
        } else {
            // Change processing has provided a specific new set of
            // relocations to use.
            _relocatesSourceToTarget = changes.newRelocatesSourceToTarget;
            _relocatesTargetToSource = changes.newRelocatesTargetToSource;
            _incrementalRelocatesSourceToTarget = 
                changes.newIncrementalRelocatesSourceToTarget;
            _incrementalRelocatesTargetToSource = 
                changes.newIncrementalRelocatesTargetToSource;
            _relocatesPrimPaths = changes.newRelocatesPrimPaths;
        }
        
        // Recompute the derived relocation variables.
        TF_FOR_ALL(i, _relocatesVariables) {
            i->second->SetValue(_FilterRelocationsForPath(*this, i->first));
        }
    }
}

const PcpLayerStackIdentifier& 
PcpLayerStack::GetIdentifier() const
{
    return _identifier;
}

const SdfLayerRefPtrVector& 
PcpLayerStack::GetLayers() const
{
    return _layers;
}

SdfLayerHandleVector
PcpLayerStack::GetSessionLayers() const
{
    SdfLayerHandleVector sessionLayers;
    if (_identifier.sessionLayer) {
        // Session layers will always be the strongest layers in the
        // layer stack. So, we can just take all of the layers stronger
        // than the root layer.
        SdfLayerRefPtrVector::const_iterator rootLayerIt = 
            std::find(_layers.begin(), _layers.end(), _identifier.rootLayer);
        if (TF_VERIFY(rootLayerIt != _layers.end())) {
            sessionLayers.insert(
                sessionLayers.begin(), _layers.begin(), rootLayerIt);
        }
    }

    return sessionLayers;
}

const SdfLayerTreeHandle& 
PcpLayerStack::GetLayerTree() const
{
    return _layerTree;
}

// We have this version so that we can avoid weakptr/refptr conversions on the
// \p layer arg.
template <class LayerPtr>
static inline const SdfLayerOffset *
_GetLayerOffsetForLayer(
    LayerPtr const &layer,
    SdfLayerRefPtrVector const &layers,
    std::vector<PcpMapFunction> const &mapFunctions)
{
    // XXX: Optimization: store a flag if all offsets are identity
    //      and just return NULL if it's set.
    for (size_t i = 0, n = layers.size(); i != n; ++i) {
        if (layers[i] == layer) {
            const SdfLayerOffset& layerOffset = mapFunctions[i].GetTimeOffset();
            return layerOffset.IsIdentity() ? NULL : &layerOffset;
        }
    }
    return NULL;
}

const SdfLayerOffset*
PcpLayerStack::GetLayerOffsetForLayer(const SdfLayerHandle& layer) const
{
    return _GetLayerOffsetForLayer(layer, _layers, _mapFunctions);
}

const SdfLayerOffset*
PcpLayerStack::GetLayerOffsetForLayer(const SdfLayerRefPtr& layer) const
{
    return _GetLayerOffsetForLayer(layer, _layers, _mapFunctions);
}

const SdfLayerOffset* 
PcpLayerStack::GetLayerOffsetForLayer(size_t layerIdx) const
{
    // XXX: Optimization: store a flag if all offsets are identity
    //      and just return NULL if it's set.
    if (!TF_VERIFY(layerIdx < _mapFunctions.size())) {
        return NULL;
    }

    const SdfLayerOffset& layerOffset = _mapFunctions[layerIdx].GetTimeOffset();
    return layerOffset.IsIdentity() ? NULL : &layerOffset;
}

const std::set<std::string>& 
PcpLayerStack::GetResolvedAssetPaths() const
{
    return _assetPaths;
}

const std::set<std::string>& 
PcpLayerStack::GetMutedLayers() const
{
    return _mutedAssetPaths;
}

bool 
PcpLayerStack::HasLayer(const SdfLayerHandle& layer) const
{
    return std::find(_layers.begin(), _layers.end(), SdfLayerRefPtr(layer)) 
        != _layers.end();
}

bool 
PcpLayerStack::HasLayer(const SdfLayerRefPtr& layer) const
{
    return std::find(_layers.begin(), _layers.end(), layer) != _layers.end();
}

const SdfRelocatesMap& 
PcpLayerStack::GetRelocatesSourceToTarget() const
{
    return _relocatesSourceToTarget;
}

const SdfRelocatesMap& 
PcpLayerStack::GetRelocatesTargetToSource() const
{
    return _relocatesTargetToSource;
}

const SdfRelocatesMap& 
PcpLayerStack::GetIncrementalRelocatesSourceToTarget() const
{
    return _incrementalRelocatesSourceToTarget;
}

const SdfRelocatesMap& 
PcpLayerStack::GetIncrementalRelocatesTargetToSource() const
{
    return _incrementalRelocatesTargetToSource;
}

const SdfPathVector& 
PcpLayerStack::GetPathsToPrimsWithRelocates() const
{
    return _relocatesPrimPaths;
}

PcpMapExpression
PcpLayerStack::GetExpressionForRelocatesAtPath(const SdfPath &path)
{
    if (_isUsd) {
        return PcpMapExpression::Identity();
    }

    _RelocatesVarMap::iterator i = _relocatesVariables.find(path);
    if (i != _relocatesVariables.end()) {
        return i->second->GetExpression();
    }

    // Create a Variable representing the relocations that affect this path.
    PcpMapExpression::VariableRefPtr var =
        PcpMapExpression::NewVariable(_FilterRelocationsForPath(*this, path));

    // Retain the variable so that we can update it if relocations change.
    _relocatesVariables[path] = var;

    return var->GetExpression();
}

void
PcpLayerStack::_BlowLayers()
{
    // Blow all of the members that get recomputed during _Compute.
    // Note this does not include relocations, which are maintained 
    // separately for efficiency.
    _layers.clear();
    _mapFunctions.clear();
    _layerTree = TfNullPtr;
    _sublayerSourceInfo.clear();
    _assetPaths.clear();
    _mutedAssetPaths.clear();
}

void
PcpLayerStack::_BlowRelocations()
{
    _relocatesSourceToTarget.clear();
    _relocatesTargetToSource.clear();
    _incrementalRelocatesSourceToTarget.clear();
    _incrementalRelocatesTargetToSource.clear();
    _relocatesPrimPaths.clear();
}

void
PcpLayerStack::_Compute(const std::string &targetSchema,
                        const Pcp_MutedLayers &mutedLayers)
{
    // Builds the composed layer stack for \p result by recursively
    // resolving sublayer asset paths and reading in the sublayers.
    // In addition, this populates the result data with:
    //
    // - \c layerStack with a strength-ordered list of layers
    //   (as ref-pointers, to keep the layers open)
    // - \c mapFunctions with the corresponding full layer offset from
    //   the root layer to each sublayer in layerStack
    // - \c layerAssetPaths with the resolved asset path of every sublayer
    // - \c errors with a precise description of any errors encountered
    //
    TRACE_FUNCTION();

    // Bind the resolver context.
    ArResolverContextBinder binder(_identifier.pathResolverContext);

    // Get any special file format arguments we need to use when finding
    // or opening sublayers.
    const SdfLayer::FileFormatArguments layerArgs =
        Pcp_GetArgumentsForTargetSchema(targetSchema);

    // Do a parallel pre-fetch request of the shot layer stack. This
    // resolves and parses the layers, retaining them until we do a
    // serial pass below to stitch them into a layer tree. The post-pass
    // is serial in order to get deterministic ordering of errors,
    // and to keep the layer stack composition algorithm as simple as
    // possible while doing the high-latency work up front in parallel.
    PcpLayerPrefetchRequest prefetch;
    if (TfGetEnvSetting(PCP_ENABLE_PARALLEL_LAYER_PREFETCH)) {
        if (_identifier.sessionLayer) {
            prefetch.RequestSublayerStack(_identifier.sessionLayer, layerArgs);
        }
        prefetch.RequestSublayerStack(_identifier.rootLayer, layerArgs);
        prefetch.Run(mutedLayers);
    }

    // The session owner.  This will be empty if there is no session owner
    // in the session layer.
    std::string sessionOwner;

    PcpErrorVector errors;

    // Build the layer stack.
    std::set<SdfLayerHandle> seenLayers;

    // Add the layer stack due to the session layer.  We *don't* apply
    // the sessionOwner to this stack.  We also skip this if the session
    // layer has been muted; in this case, the stack will not include the
    // session layer specified in the identifier.
    if (_identifier.sessionLayer) {
        std::string canonicalMutedPath;
        if (mutedLayers.IsLayerMuted(_identifier.sessionLayer,
                                     _identifier.sessionLayer->GetIdentifier(),
                                     &canonicalMutedPath)) {
            _mutedAssetPaths.insert(canonicalMutedPath);
        }
        else {
            SdfLayerTreeHandle sessionLayerTree = 
                _BuildLayerStack(_identifier.sessionLayer, SdfLayerOffset(),
                                 _identifier.pathResolverContext, layerArgs,
                                 std::string(), mutedLayers, &seenLayers, 
                                 &errors);

            // Get the session owner.
            struct _Helper {
                static bool FindSessionOwner(const SdfLayerTreeHandle& tree,
                                             std::string* sessionOwner)
                {
                    if (tree->GetLayer()->HasField(SdfPath::AbsoluteRootPath(), 
                                                   SdfFieldKeys->SessionOwner,
                                                   sessionOwner)) {
                        return true;
                    }
                    TF_FOR_ALL(subtree, tree->GetChildTrees()) {
                        if (FindSessionOwner(*subtree, sessionOwner)) {
                            return true;
                        }
                    }
                    return false;
                }
            };

            _Helper::FindSessionOwner(sessionLayerTree, &sessionOwner);
        }
    }

    // Add the layer stack due to the root layer.  We do apply the
    // sessionOwner, if any, to this stack.  Unlike session layers, we
    // don't allow muting a layer stack's root layer since that would
    // lead to empty layer stacks.
    _layerTree =
        _BuildLayerStack(_identifier.rootLayer, SdfLayerOffset(),
                         _identifier.pathResolverContext, layerArgs, 
                         sessionOwner, mutedLayers, &seenLayers, &errors);

    // Update layer-stack-to-layer maps in the registry, if we're installed in a
    // registry.
    if (_registry)
        _registry->_SetLayers(this);

    if (errors.empty()) {
        _localErrors.reset();
    } else {
        _localErrors.reset(new PcpErrorVector);
        _localErrors->swap(errors);
    }
}

SdfLayerTreeHandle
PcpLayerStack::_BuildLayerStack(
    const SdfLayerHandle & layer,
    const SdfLayerOffset & offset,
    const ArResolverContext & pathResolverContext,
    const SdfLayer::FileFormatArguments & layerArgs,
    const std::string & sessionOwner,
    const Pcp_MutedLayers & mutedLayers, 
    SdfLayerHandleSet *seenLayers,
    PcpErrorVector *errors)
{
    seenLayers->insert(layer);

    // Accumulate layer into results.
    _layers.push_back(layer);

    const PcpMapFunction::PathMap &identity = PcpMapFunction::IdentityPathMap();
    PcpMapFunction mapFunction = PcpMapFunction::Create(identity, offset);
    _mapFunctions.push_back(mapFunction);
        
    // Recurse over sublayers to build subtrees.
    Pcp_SublayerInfoVector sublayerInfo;
    const vector<string> &sublayers = layer->GetSubLayerPaths();
    const SdfLayerOffsetVector &sublayerOffsets = layer->GetSubLayerOffsets();
    for(size_t i=0, numSublayers = sublayers.size(); i<numSublayers; i++) {
        _assetPaths.insert(sublayers[i]);

        string canonicalMutedPath;
        if (mutedLayers.IsLayerMuted(layer, sublayers[i], 
                                     &canonicalMutedPath)) {
            _mutedAssetPaths.insert(canonicalMutedPath);
            continue;
        }

        // Resolve and open sublayer.
        string sublayerPath(sublayers[i]);
        TfErrorMark m;
        SdfLayerRefPtr sublayer = SdfFindOrOpenRelativeToLayer(
            layer, &sublayerPath, layerArgs);

        _sublayerSourceInfo.emplace_back(layer, sublayers[i], sublayerPath);

        if (!sublayer) {
            PcpErrorInvalidSublayerPathPtr err = 
                PcpErrorInvalidSublayerPath::New();
            err->rootSite = PcpSite(_identifier, SdfPath::AbsoluteRootPath());
            err->layer           = layer;
            err->sublayerPath    = sublayerPath;
            if (!m.IsClean()) {
                vector<string> commentary;
                for (auto const &err: m) {
                    commentary.push_back(err.GetCommentary());
                }
                m.Clear();
                err->messages = TfStringJoin(commentary.begin(),
                                             commentary.end(), "; ");
            }
            errors->push_back(err);
            continue;
        }
        m.Clear();

        // Check for cycles.
        if (seenLayers->count(sublayer)) {
            PcpErrorSublayerCyclePtr err = PcpErrorSublayerCycle::New();
            err->rootSite = PcpSite(_identifier, SdfPath::AbsoluteRootPath());
            err->layer = layer;
            err->sublayer = sublayer;
            errors->push_back(err);
            continue;
        }

        // Check sublayer offset.
        SdfLayerOffset sublayerOffset = sublayerOffsets[i];
        if (!sublayerOffset.IsValid()
            || !sublayerOffset.GetInverse().IsValid()) {
            // Report error, but continue with an identity layer offset.
            PcpErrorInvalidSublayerOffsetPtr err =
                PcpErrorInvalidSublayerOffset::New();
            err->rootSite = PcpSite(_identifier, SdfPath::AbsoluteRootPath());
            err->layer       = layer;
            err->sublayer    = sublayer;
            err->offset      = sublayerOffset;
            errors->push_back(err);
            sublayerOffset = SdfLayerOffset();
        }

        // Combine the sublayerOffset with the cumulative offset
        // to find the absolute offset of this layer.
        sublayerOffset = offset * sublayerOffset;

        // Store the info for later recursion.
        sublayerInfo.push_back(Pcp_SublayerInfo(sublayer, sublayerOffset));
    }

    // Reorder sublayers according to sessionOwner.
    _ApplyOwnedSublayerOrder(_identifier, layer, sessionOwner, &sublayerInfo,
                             errors);

    // Recurse over sublayers to build subtrees.  We must do this after
    // applying the sublayer order, otherwise _layers and
    // _mapFunctions will not appear in the right order.
    // XXX: We might want the tree nodes themselves to own the layers.
    //      Then we can construct the subtree nodes in the loop above
    //      and reorder them afterwards.  After building the tree we
    //      can preorder traverse it to collect the layers and offsets.
    SdfLayerTreeHandleVector subtrees;
    TF_FOR_ALL(i, sublayerInfo) {
        if (SdfLayerTreeHandle subtree =
            _BuildLayerStack(i->layer, i->offset, pathResolverContext,
                             layerArgs, sessionOwner, 
                             mutedLayers, seenLayers, errors)) {
            subtrees.push_back(subtree);
        }
    }

    // Remove the layer from seenLayers.  We want to detect cycles, but
    // do not prohibit the same layer from appearing multiple times.
    seenLayers->erase(layer);

    return SdfLayerTree::New(layer, subtrees, offset);
}

std::ostream&
operator<<(std::ostream& s, const PcpLayerStackPtr& x)
{
    if (x) {
        return s << x->GetIdentifier();
    }
    else {
        return s << "@<expired>@";
    }
}

std::ostream&
operator<<(std::ostream& s, const PcpLayerStackRefPtr& x)
{
    if (x) {
        return s << x->GetIdentifier();
    }
    else {
        return s << "@NULL@";
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
