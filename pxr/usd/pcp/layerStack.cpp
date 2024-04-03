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
#include "pxr/usd/pcp/expressionVariables.h"
#include "pxr/usd/pcp/layerStackRegistry.h"
#include "pxr/usd/pcp/utils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/withScopedParallelism.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <unordered_set>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// Computing layer stacks

// XXX Parallel layer prefetch is only available in usd-mode, until Sd
// thread-safety issues can be fixed, specifically plugin loading:
// - FileFormat plugins
// - value type plugins for parsing AnimSplines
TF_DEFINE_ENV_SETTING(
    PCP_ENABLE_PARALLEL_LAYER_PREFETCH, true,
    "Enables parallel, threaded pre-fetch of sublayers.");

TF_DEFINE_ENV_SETTING(
    PCP_DISABLE_TIME_SCALING_BY_LAYER_TCPS, false,
    "Disables automatic layer offset scaling from time codes per second "
    "metadata in layers.");

bool
PcpIsTimeScalingForLayerTimeCodesPerSecondDisabled()
{
    return TfGetEnvSetting(PCP_DISABLE_TIME_SCALING_BY_LAYER_TCPS);
}

struct Pcp_SublayerInfo {
    Pcp_SublayerInfo() = default;
    Pcp_SublayerInfo(const SdfLayerRefPtr& layer_, const SdfLayerOffset& offset_,
                     double timeCodesPerSecond_)
        : layer(layer_)
        , offset(offset_)
        , timeCodesPerSecond(timeCodesPerSecond_) {}
    SdfLayerRefPtr layer;
    SdfLayerOffset offset;
    double timeCodesPerSecond;
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
                error->rootSite = 
                    PcpSite(identifier, SdfPath::AbsoluteRootPath());
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

static bool
_IsValidRelocatesPath(SdfPath const& path)
{
    return path.IsPrimPath() && !path.ContainsPrimVariantSelection();
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
        if (!(*layer)->GetHints().mightHaveRelocates) {
            continue;
        }

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

                // The SdfSchema should already enforce that these
                // are valid paths for relocates, however we still
                // double-check here to avoid problematic results
                // under composition.
                //
                // XXX As with the case below, high-level code emits a
                // warning in this case.  If we introduce a path to
                // emit PcpErrors in this code, we'd use it here too.
                if (!_IsValidRelocatesPath(source)) {
                    TF_WARN("Ignoring invalid relocate source "
                            "path <%s> in layer @%s@",
                            source.GetText(),
                            (*layer)->GetIdentifier().c_str());
                    continue;
                }
                if (!_IsValidRelocatesPath(target)) {
                    TF_WARN("Ignoring invalid relocate target "
                            "path <%s> in layer @%s@",
                            target.GetText(),
                            (*layer)->GetIdentifier().c_str());
                    continue;
                }

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

// Helper for determining whether the session layer's computed TCPS should 
// be used instead of the root layer's computed TCPS as the overall TCPS of 
// layer stack. This is according to the strength order of:
// 1. Authored session timeCodesPerSecond
// 2. Authored root timeCodesPerSecond
// 3. Authored session framesPerSecond
// 4. Authored root framesPerSecond
// 5. SdfSchema fallback.
static 
bool 
_ShouldUseSessionTcps(const SdfLayerHandle &sessionLyr,
                      const SdfLayerHandle &rootLyr)
{
    return sessionLyr && (
        sessionLyr->HasTimeCodesPerSecond() ||
        (!rootLyr->HasTimeCodesPerSecond() && sessionLyr->HasFramesPerSecond())
    );
}

bool
Pcp_NeedToRecomputeLayerStackTimeCodesPerSecond(
    const PcpLayerStackPtr& layerStack, const SdfLayerHandle &changedLayer)
{
    const SdfLayerHandle &sessionLayer = 
        layerStack->GetIdentifier().sessionLayer;
    const SdfLayerHandle &rootLayer = 
        layerStack->GetIdentifier().rootLayer;

    // The changed layer is only relevant to the overall layer stack TCPS if
    // it's the stack's root or session layer.
    if (changedLayer != sessionLayer && changedLayer != rootLayer) {
        return false;
    }

    // The new layer stack TCPS, when its computed, will come
    // from either the session or root layer depending on what's
    // authored. We use the same logic here as we do in 
    // PcpLayerStack::_Compute.
    const double newLayerStackTcps = 
        _ShouldUseSessionTcps(sessionLayer, rootLayer) ? 
            sessionLayer->GetTimeCodesPerSecond() : 
            rootLayer->GetTimeCodesPerSecond();

    // The layer stack's overall TCPS is cached so if it doesn't match, we
    // need to recompute the layer stack.
    return newLayerStackTcps != layerStack->GetTimeCodesPerSecond();
}

////////////////////////////////////////////////////////////////////////
// PcpLayerStack

PcpLayerStack::PcpLayerStack(
    const PcpLayerStackIdentifier& identifier,
    const Pcp_LayerStackRegistry& registry)
    : _identifier(identifier)
    , _expressionVariables(
        [&identifier, &registry]() {
            const PcpLayerStackIdentifier& selfId = identifier;
            const PcpLayerStackIdentifier& rootLayerStackId =
                registry._GetRootLayerStackIdentifier();

            // Optimization: If the layer stack providing expression variable
            // overrides has already been computed, use its expression variables
            // to compose this layer stack's expression variables, This is the
            // common case that happens during prim indexing.
            //
            // Otherwise, we need to take a slower code path that computes
            // the full chain of overrides. 
            const PcpLayerStackIdentifier& overrideLayerStackId =
                selfId.expressionVariablesOverrideSource
                .ResolveLayerStackIdentifier(rootLayerStackId);

            const PcpLayerStackPtr overrideLayerStack =
                overrideLayerStackId != selfId ? 
                registry.Find(overrideLayerStackId) : TfNullPtr;

            PcpExpressionVariables composedExpressionVars;

            if (overrideLayerStack) {
                composedExpressionVars = PcpExpressionVariables::Compute(
                    selfId, rootLayerStackId, 
                    &overrideLayerStack->GetExpressionVariables());

                // Optimization: If the composed expression variables for this
                // layer stack are the same as those in the overriding layer
                // stack, just share their PcpExpressionVariables object.
                if (composedExpressionVars == 
                    overrideLayerStack->GetExpressionVariables()) {
                    return overrideLayerStack->_expressionVariables;
                }
            }
            else {
                composedExpressionVars = PcpExpressionVariables::Compute(
                    selfId, rootLayerStackId);
            }

            return std::make_shared<PcpExpressionVariables>(
                std::move(composedExpressionVars));

        }())
    , _isUsd(registry._IsUsd())

    // Note that we do not set the _registry member here. This will be
    // done by Pcp_LayerStackRegistry itself when it decides to register
    // this layer stack.
    // , _registry(TfCreateWeakPtr(&registry))
{
    TfAutoMallocTag2 tag("Pcp", "PcpLayerStack::PcpLayerStack");
    TRACE_FUNCTION();

    if (!TF_VERIFY(_identifier)) {
        return;
    }

    _Compute(registry._GetFileFormatTarget(), registry._GetMutedLayers());

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
    _BlowLayers();
    if (_registry) {
        _registry->_SetLayersAndRemove(_identifier, this);
    }
}

void
PcpLayerStack::Apply(const PcpLayerStackChanges& changes, PcpLifeboat* lifeboat)
{
    // Invalidate the layer stack as necessary, recomputing immediately.
    // Recomputing immediately assists optimal change processing --
    // e.g. it lets us examine the before/after change to relocations.

    // Update expression variables if necessary. This needs to be done up front
    // since they may be needed when computing the full layer stack.
    if (changes.didChangeSignificantly || 
        changes.didChangeExpressionVariables || 
        changes._didChangeExpressionVariablesSource) {

        const auto updateExpressionVariables = 
            [&](const VtDictionary& newExprVars, 
                const PcpExpressionVariablesSource& newSource)
            {
                const PcpLayerStackIdentifier& newSourceId =
                    newSource.ResolveLayerStackIdentifier(
                        _registry->_GetRootLayerStackIdentifier());

                if (newSourceId == GetIdentifier()) {
                    // If this layer stack is the new source for its expression
                    // vars, either update _expressionVariables or create a new
                    // one based on whether it's already sourced from this layer
                    // stack.
                    if (_expressionVariables->GetSource() == newSource) {
                        _expressionVariables->SetVariables(newExprVars);
                    }
                    else {
                        _expressionVariables = 
                            std::make_shared<PcpExpressionVariables>(
                                newSource, newExprVars);
                    }
                }
                else {
                    // Optimization: If some other layer stack is the source for
                    // this layer stack's expression vars, grab that other layer
                    // stack's _expressionVariables. See matching optimization
                    // in c'tor.
                    const PcpLayerStackPtr overrideLayerStack = 
                        _registry->Find(newSourceId);
                    if (overrideLayerStack) {
                        _expressionVariables =
                            overrideLayerStack->_expressionVariables;

                        // Update _expressionVariables if it doesn't have the
                        // newly-computed expression variables. This is okay
                        // even if _expressionVariables is shared by other layer
                        // stacks, since we expect those layer stacks would have
                        // been updated in the same way.
                        if (newExprVars !=
                            _expressionVariables->GetVariables()) {
                            _expressionVariables->SetVariables(newExprVars);
                        }
                    }
                    else {
                        _expressionVariables =
                            std::make_shared<PcpExpressionVariables>(
                                newSource, newExprVars);
                    }

                }
            };

        if (changes.didChangeSignificantly) {
            const PcpExpressionVariables newExprVars =
                PcpExpressionVariables::Compute(
                    GetIdentifier(), _registry->_GetRootLayerStackIdentifier());
            updateExpressionVariables(
                newExprVars.GetVariables(), newExprVars.GetSource());
        }
        else {
            updateExpressionVariables(
                changes.didChangeExpressionVariables ?
                    changes.newExpressionVariables :
                    _expressionVariables->GetVariables(),
                changes._didChangeExpressionVariablesSource ?
                    changes._newExpressionVariablesSource : 
                    _expressionVariables->GetSource());
        }
    }
    
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
        _Compute(_registry->_GetFileFormatTarget(), _registry->_GetMutedLayers());
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
PcpLayerStack::GetMutedLayers() const
{
    return _mutedAssetPaths;
}

bool 
PcpLayerStack::HasLayer(const SdfLayerHandle& layer) const
{
    // Avoid doing refcount operations here.
    SdfLayer const *layerPtr = get_pointer(layer);
    for (SdfLayerRefPtr const &layerRefPtr: _layers) {
        if (get_pointer(layerRefPtr) == layerPtr) {
            return true;
        }
    }
    return false;
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
    const PcpMapExpression::Variable *var = nullptr;
    {
        tbb::spin_mutex::scoped_lock lock{_relocatesVariablesMutex};
        _RelocatesVarMap::const_iterator i = _relocatesVariables.find(path);
        if (i != _relocatesVariables.end()) {
            var = i->second.get();
        }
    }

    if (var) {
        return var->GetExpression();
    }

    // Create a Variable representing the relocations that affect this path.
    PcpMapExpression::VariableUniquePtr newVar =
        PcpMapExpression::NewVariable(_FilterRelocationsForPath(*this, path));

    {
        // Retain the variable so that we can update it if relocations change.
        tbb::spin_mutex::scoped_lock lock{_relocatesVariablesMutex};
        _RelocatesVarMap::const_iterator i =
            _relocatesVariables.emplace(path, std::move(newVar)).first;
        var = i->second.get();
    }

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
    _mutedAssetPaths.clear();
    _expressionVariableDependencies.clear();
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
PcpLayerStack::_Compute(const std::string &fileFormatTarget,
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
        Pcp_GetArgumentsForFileFormatTarget(fileFormatTarget);

    // The session owner.  This will be empty if there is no session owner
    // in the session layer.
    std::string sessionOwner;

    PcpErrorVector errors;

    // Build the layer stack.
    std::set<SdfLayerHandle> seenLayers;

    // Env setting for disabling TCPS scaling.
    const bool scaleLayerOffsetByTcps = 
        !PcpIsTimeScalingForLayerTimeCodesPerSecondDisabled();

    const double rootTcps = _identifier.rootLayer->GetTimeCodesPerSecond();
    SdfLayerOffset rootLayerOffset;

    // The layer stack's time codes per second initially comes from the root 
    // layer. An opinion in the session layer may override it below.
    _timeCodesPerSecond = rootTcps;

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
            // The session layer has its own time codes per second.
            const double sessionTcps = 
                _identifier.sessionLayer->GetTimeCodesPerSecond();
            SdfLayerOffset sessionLayerOffset;

            // The time codes per second of the entire layer stack may come 
            // from the session layer or the root layer depending on which 
            // metadata is authored where. We'll use the session layer's TCPS
            // only if the session layer has an authored timeCodesPerSecond or
            // if the root layer has no timeCodesPerSecond opinion but the 
            // session layer has a framesPerSecond opinion.
            //
            // Note that both the session and root layers still have their own
            // computed TCPS for just the layer itself, so either layer may end
            // up with a layer offset scale in its map function to map from the 
            // layer stack TCPS to the layer.
            if (_ShouldUseSessionTcps(_identifier.sessionLayer, 
                                      _identifier.rootLayer)) {
                _timeCodesPerSecond = sessionTcps;
                if (scaleLayerOffsetByTcps) {
                    rootLayerOffset.SetScale(_timeCodesPerSecond / rootTcps);
                }
            } else {
                if (scaleLayerOffsetByTcps) {
                    sessionLayerOffset.SetScale(_timeCodesPerSecond / sessionTcps);
                }
            }

            SdfLayerTreeHandle sessionLayerTree = 
                _BuildLayerStack(_identifier.sessionLayer, sessionLayerOffset,
                                 sessionTcps,
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
        _BuildLayerStack(_identifier.rootLayer, rootLayerOffset, rootTcps,
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
    double layerTcps,
    const ArResolverContext & pathResolverContext,
    const SdfLayer::FileFormatArguments & defaultLayerArgs,
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
    vector<string> sublayers = layer->GetSubLayerPaths();
    const SdfLayerOffsetVector &sublayerOffsets = layer->GetSubLayerOffsets();
    const size_t numSublayers = sublayers.size();

    // Evaluate expressions and compute mutedness first.
    for(size_t i=0; i != numSublayers; ++i) {
        if (Pcp_IsVariableExpression(sublayers[i])) {
            sublayers[i] = Pcp_EvaluateVariableExpression(
                sublayers[i], *_expressionVariables,
                "sublayer", layer, SdfPath::AbsoluteRootPath(),
                &_expressionVariableDependencies, errors);

            if (sublayers[i].empty()) {
                continue;
            }
        }

        string canonicalMutedPath;
        if (mutedLayers.IsLayerMuted(layer, sublayers[i], 
                                     &canonicalMutedPath)) {
            _mutedAssetPaths.insert(canonicalMutedPath);
            sublayers[i].clear();
        }
    }

    std::vector<SdfLayerRefPtr> sublayerRefPtrs(numSublayers);
    std::vector<std::string> errCommentary(numSublayers);
    std::vector<_SublayerSourceInfo> localSourceInfo(numSublayers);

    auto loadSublayer = [&](size_t i) {
        // Resolve and open sublayer.
        TfErrorMark m;
                
        SdfLayer::FileFormatArguments localArgs;
        const SdfLayer::FileFormatArguments& layerArgs = 
            Pcp_GetArgumentsForFileFormatTarget(
                sublayers[i], &defaultLayerArgs, &localArgs);
                
        // This is equivalent to SdfLayer::FindOrOpenRelativeToLayer,
        // but we want to keep track of the final sublayer path after
        // anchoring it to the layer.
        string sublayerPath = SdfComputeAssetPathRelativeToLayer(
            layer, sublayers[i]);
        sublayerRefPtrs[i] = SdfLayer::FindOrOpen(sublayerPath, layerArgs);
                
        localSourceInfo[i] =
            _SublayerSourceInfo(layer, sublayers[i], sublayerPath);
                
        // Produce commentary for eventual PcpError created below.
        if (!m.IsClean()) {
            vector<string> commentary;
            for (auto const &err: m) {
                commentary.push_back(err.GetCommentary());
            }
            m.Clear();
            errCommentary[i] = TfStringJoin(commentary.begin(),
                                            commentary.end(), "; ");
        }
    };
    
    // Open all the layers in parallel.
    WorkWithScopedDispatcher([&](WorkDispatcher &wd) {
        // Cannot use parallelism for non-USD clients due to thread-safety
        // issues in file format plugins & value readers.
        const bool goParallel = _isUsd && numSublayers > 1 &&
            TfGetEnvSetting(PCP_ENABLE_PARALLEL_LAYER_PREFETCH);

        for (size_t i=0; i != numSublayers; ++i) {
            if (sublayers[i].empty()) {
                continue;
            }

            if (goParallel) {
                wd.Run(
                    [i, &loadSublayer, &pathResolverContext]() { 
                        // Context binding is thread-specific, so we need to
                        // bind the context here.
                        ArResolverContextBinder binder(pathResolverContext);
                        loadSublayer(i);
                    });
            }
            else {
                loadSublayer(i);
            }
        }
    });

    Pcp_SublayerInfoVector sublayerInfo;
    for (size_t i=0; i != numSublayers; ++i) {
        if (sublayers[i].empty()) {
            continue;
        }
        if (!sublayerRefPtrs[i]) {
            PcpErrorInvalidSublayerPathPtr err = 
                PcpErrorInvalidSublayerPath::New();
            err->rootSite = PcpSite(_identifier, SdfPath::AbsoluteRootPath());
            err->layer = layer;
            err->sublayerPath = localSourceInfo[i].computedSublayerPath;
            err->messages = std::move(errCommentary[i]);
            errors->push_back(std::move(err));
            continue;
        }

        // Check for cycles.
        if (seenLayers->count(sublayerRefPtrs[i])) {
            PcpErrorSublayerCyclePtr err = PcpErrorSublayerCycle::New();
            err->rootSite = PcpSite(_identifier, SdfPath::AbsoluteRootPath());
            err->layer = layer;
            err->sublayer = sublayerRefPtrs[i];
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
            err->sublayer    = sublayerRefPtrs[i];
            err->offset      = sublayerOffset;
            errors->push_back(err);
            sublayerOffset = SdfLayerOffset();
        }

        // Apply the scale from computed layer TCPS to sublayer TCPS to sublayer
        // layer offset.
        const double sublayerTcps = sublayerRefPtrs[i]->GetTimeCodesPerSecond();
        if (!PcpIsTimeScalingForLayerTimeCodesPerSecondDisabled() &&
            layerTcps != sublayerTcps) {
            sublayerOffset.SetScale(sublayerOffset.GetScale() * 
                                    layerTcps / sublayerTcps);
        }

        // Combine the sublayerOffset with the cumulative offset
        // to find the absolute offset of this layer.
        sublayerOffset = offset * sublayerOffset;

        // Store the info for later recursion.
        sublayerInfo.emplace_back(
            sublayerRefPtrs[i], sublayerOffset, sublayerTcps);
    }

    // Append localSourceInfo items into _sublayerSourceInfo, skipping any for
    // which don't have a layer (these entries correspond to layers that were
    // muted).
    _sublayerSourceInfo.reserve(
        _sublayerSourceInfo.size() + localSourceInfo.size());
    for (_SublayerSourceInfo &localInfo: localSourceInfo) {
        if (localInfo.layer) {
            _sublayerSourceInfo.push_back(std::move(localInfo));
        }
    }
    localSourceInfo.clear();

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
            _BuildLayerStack(i->layer, i->offset, i->timeCodesPerSecond,
                             pathResolverContext,
                             defaultLayerArgs, sessionOwner, 
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
