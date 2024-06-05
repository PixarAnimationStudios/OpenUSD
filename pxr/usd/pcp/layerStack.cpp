//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "pxr/usd/sdf/site.h"
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

TF_DEFINE_ENV_SETTING(
    PCP_ENABLE_LEGACY_RELOCATES_BEHAVIOR, true,
    "Enables the legacy behavior of ignoring composition errors that would "
    "cause us to reject conflicting relocates that are invalid within the "
    "context of all other relocates on the layer stack. This only applies to "
    "non-USD caches/layer stacks; the legacy behavior cannot be enabled in USD "
    "mode");

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

bool
Pcp_IsValidRelocatesEntry(
    const SdfPath &source, const SdfPath &target, std::string *errorMessage) 
{
    auto isValidPathFn = [&](SdfPath const& path) {
        // The SdfSchema should already enforce that these are valid paths for 
        // relocates, however we still double-check here to avoid problematic 
        // results under composition.
        if (!path.IsAbsolutePath()) {
            *errorMessage = "Relocates must use absolute paths.";
            return false;
        }

        if (!path.IsPrimPath()) {
            // Prim variant selection paths are not prim paths, but it's more
            // important to report the that the variant selection is the issue
            // in this case.
            if (path.IsPrimVariantSelectionPath()) {
                *errorMessage = "Relocates cannot have any variant selections.";
            } else {
                *errorMessage = "Only prims can be relocated.";
            }
            return false;
        }

        if (path.ContainsPrimVariantSelection()) {
            *errorMessage = "Relocates cannot have any variant selections.";
            return false;
        }

        // This is not enforced by the Sdf Schema but is still not allowed
        if (path.IsRootPrimPath()) {
            *errorMessage = 
                "Root prims cannot be the source or target of a relocate.";
            return false;
        }

        return true;
    };

    // The source and target must be valid relocates paths.
    if (!isValidPathFn(source) || !isValidPathFn(target)) {
        return false;
    }

    if (source == target) {
        *errorMessage = 
            "The target of a relocate cannot be the same as its source.";
        return false;
    }

    if (target.HasPrefix(source)) {
        *errorMessage = 
            "The target of a relocate cannot be a descendant of its source.";
        return false;
    }

    if (source.HasPrefix(target)) {
        *errorMessage = 
            "The target of a relocate cannot be an ancestor of its source.";
        return false;
    }

    if (source.GetCommonPrefix(target).IsAbsoluteRootPath()) {
        *errorMessage = 
            "Prims cannot be relocated to be a descendant of a different "
            "root prim.";
        return false;
    }

    return true;
}

namespace {

// Helper class for Pcp_ComputeRelocationsForLayerStack for gathering and 
// validating relocates for a layer stack.
class Pcp_ComputeRelocationsForLayerStackWorkspace {
    
public:

    Pcp_ComputeRelocationsForLayerStackWorkspace() = default;
    
    // Public members; running Compute on the workspace populates these
    
    // Value type for the map of processed relocates, which will map authored
    // source paths to this.
    struct ProcessedRelocateInfo {
        // Target path of the authored relocate.
        SdfPath targetPath;

        // Site where the relocate is authored (for error reporting purposes)
        SdfSite owningSite;

        // Full origin source path for the target of this relocate. This is 
        // computed using ancestral relocates to find the path of the prim spec
        // that is "moved" to the target path after all relocates are applied.
        SdfPath computedSourceOrigin;

        // Move constructor. computedSourceOrigin is initialized empty to 
        // indicate it wasn't computed.
        ProcessedRelocateInfo(SdfPath &&targetPath_, SdfSite &&owningSite_):
            targetPath(targetPath_), owningSite(owningSite_) {}
    };

    // Map of all processed relocates. This maps authored source path to the 
    // info struct above.
    using ProcessedRelocatesMap = 
        std::unordered_map<SdfPath, ProcessedRelocateInfo, TfHash>;
    ProcessedRelocatesMap processedRelocates;

    // Mapping of authored target path to the entries in the processed relocates
    // map.
    using PathToProcessedRelocateMap = std::unordered_map<
        SdfPath, ProcessedRelocatesMap::value_type *, TfHash>;
    PathToProcessedRelocateMap targetPathToProcessedRelocateMap;

    // Set of all prims that authored relocates in any layer of the layer stack.
    SdfPathSet allPrimPathsWithAuthoredRelocates;

    // All encountered errors.
    PcpErrorVector errors;

    // Computes all the relocates populating the public members of this 
    // workspace.
    void 
    Compute(const PcpLayerStack &layerStack) 
    {
        TRACE_FUNCTION();

        _layerStack = &layerStack;
        _isUsd = layerStack.IsUsd();

        const SdfLayerRefPtrVector & layers = layerStack.GetLayers();

        // Compose the authored relocations from each layer.
        for (const SdfLayerRefPtr &layer : layers) {
            _CollectRelocatesForLayer(layer);
        }

        _ValidateAndRemoveConflictingRelocates();

        // Compute the source origin for each valid relocate. This function may
        // recurse for ancestral opinions so this will only compute each if 
        // necessary.
        for (auto &mapEntry : processedRelocates) {
            _ComputeSourceOriginForTargetIfNeeded(&mapEntry);
        }

        _ConformLegacyRelocates();
    }

    void 
    Compute(const std::vector<std::pair<SdfLayerHandle, SdfRelocates>> &layerRelocates) 
    {
        TRACE_FUNCTION();

        _layerStack = nullptr;
        _isUsd = true;

        // Compose the authored relocations from each layer.
        for (const auto &[layer, relocates] : layerRelocates) {
            _CollectRelocates(layer, SdfPath::AbsoluteRootPath(), relocates);
        }

        _ValidateAndRemoveConflictingRelocates();

        // Compute the source origin for each valid relocate. This function may
        // recurse for ancestral opinions so this will only compute each if 
        // necessary.
        for (auto &mapEntry : processedRelocates) {
            _ComputeSourceOriginForTargetIfNeeded(&mapEntry);
        }
    }

private:

    // Collects all the relocates authored on the layer.
    void
    _CollectRelocatesForLayer(
        const SdfLayerRefPtr &layer)
    {
        TRACE_FUNCTION();

        if (!layer->GetHints().mightHaveRelocates) {
            return;
        }

        // Collect relocates from the layer metadata first. In USD mode, this
        // is the only place we collect relocates from. In non-USD mode, 
        // layer metadata relocates usurp any relocates otherwise authored on
        // prims so we skip the full traversal of namespace for relocates if 
        // we found layer metadata or are in USD mode.
        if (_CollectLayerRelocates(layer) || _isUsd) {
            return;
        }

        // Check for relocation arcs in this layer.
        SdfPathVector pathStack;

        auto addChildrenToPathStack = [&](const SdfPath &primPath) {
            TfTokenVector primChildrenNames;
            if (layer->HasField(
                    primPath, SdfChildrenKeys->PrimChildren, &primChildrenNames)) {
                for (const TfToken &childName : primChildrenNames) {
                    pathStack.push_back(primPath.AppendChild(childName));
                }
            }
        };

        addChildrenToPathStack(SdfPath::AbsoluteRootPath());

        while (!pathStack.empty()) {

            SdfPath primPath = pathStack.back();
            pathStack.pop_back();

            _CollectPrimRelocates(layer, primPath);

            // Push back any children.
            addChildrenToPathStack(primPath);
        }
    }

    template <class RelocatesType>
    void
    _CollectRelocates(
        const SdfLayerRefPtr &layer, const SdfPath &primPath, 
        const RelocatesType &relocates) 
    {
        for(const auto &[sourcePath, targetPath] : relocates) {
            // Absolutize source/target paths.
            // XXX: This shouldn't be necessary as the paths are typically
            // abolutized on layer read. But this is here to make sure for
            // now. Eventually all relocates will be authored only in layer
            // metadata so paths will have to be absolute to begin with so
            // this will be able to be safely removed.
            SdfPath source = sourcePath.MakeAbsolutePath(primPath);
            SdfPath target = targetPath.MakeAbsolutePath(primPath);

            // Validate the relocate in context of just itself and add to
            // the processed relocates or log an error.
            std::string errorMessage;
            if (Pcp_IsValidRelocatesEntry(source, target, &errorMessage)) {
                // It's not an error for this to fail to be added; it just 
                // means a stronger relocate for the source path as been 
                // added already.
                processedRelocates.try_emplace(
                    std::move(source), std::move(target), 
                    SdfSite(layer, primPath));
            } else {
                PcpErrorInvalidAuthoredRelocationPtr err = 
                    PcpErrorInvalidAuthoredRelocation::New();
                err->rootSite = _GetErrorRootSite();
                err->layer = layer;
                err->owningPath = primPath;
                err->sourcePath = std::move(source);
                err->targetPath = std::move(target);
                err->messages = std::move(errorMessage);
                errors.push_back(std::move(err));
            }
        }
    }

    bool
    _CollectLayerRelocates(const SdfLayerRefPtr &layer)
    {
        // Check for layer metadata relocates.
        SdfRelocates relocates;
        if (!layer->HasField(SdfPath::AbsoluteRootPath(), 
                SdfFieldKeys->LayerRelocates, &relocates)) {
            return false;
        }

        _CollectRelocates(layer, SdfPath::AbsoluteRootPath(), relocates);

        return true;
    }

    bool
    _CollectPrimRelocates(const SdfLayerRefPtr &layer, const SdfPath &primPath)
    {
        // Check for relocations on this prim.
        SdfRelocatesMap relocates;
        if (!layer->HasField(primPath, SdfFieldKeys->Relocates, &relocates)) {
            return false;
        }

        _CollectRelocates(layer, primPath, relocates);

        allPrimPathsWithAuthoredRelocates.insert(primPath);
        return true;
    }

    // XXX: There are non-USD use cases that rely on the fact that we 
    // allowed the source of relocates statement to be expressed as either
    // a fully unrelocated path or a partially or fully relocated path (due
    // to ancestral relocates). For example if a relocate from 
    // /Prim/Foo -> /Prim/Bar exists, a rename of Foo's child prim A could
    // expressed as either 
    //      /Prim/Foo/A -> /Prim/Bar/B
    //   or /Prim/Bar/A -> /Prim/Bar/B
    //
    // In USD relocates, this would only be expressable using the latter 
    // /Prim/Bar/A -> /Prim/Bar/B. Furthermore, an upcoming change will 
    // actually cause /Prim/Foo/A -> /Prim/Bar/B to have a completely 
    // different meaning and composition result than relocating from 
    // /Prim/Bar/A. So in order to maintain legacy behavior when these 
    // changes come online, we need to to convert unrelocated source paths
    // to be instead the "most ancestrally relocated" source path here.
    //
    // This behavior is not meant to be long-term; this a placeholder 
    // solution until we can update existing assets to conform to USD
    // relocates requirements. At that point we'll remove this legacy 
    // behavior.
    void
    _ConformLegacyRelocates()
    {
        if (_isUsd || !TfGetEnvSetting(PCP_ENABLE_LEGACY_RELOCATES_BEHAVIOR)) {
            return;
        }

        TRACE_FUNCTION();

        // We're building a list of relocation source path that need be updated
        // which means they will moved in the map.
        std::vector<std::pair<SdfPath, SdfPath>> relocateSourcesToMove;

        for (const auto &[source, reloInfo] : processedRelocates) {

            // We're looking at the computed source origin of each relocate as 
            // that will be consistent regardless of the how the authored 
            // relocate is represented. And we start with the parent path of
            // the origin source as we want closest relocate that moves our 
            // origin source path that isn't this relocate itself.
            const SdfPath sourceOriginParent = 
                reloInfo.computedSourceOrigin.GetParentPath();

            // Find the best match relocate by looking for another relocate with 
            // the longest computed origin source path that is a prefix of this
            // relocate's computed origin source path
            auto bestMatchIt = processedRelocates.end();
            size_t bestMatchElementCount = 0;
            for (auto otherRelocateIt = processedRelocates.begin();
                    otherRelocateIt != processedRelocates.end();
                    ++otherRelocateIt) {
                const SdfPath &anotherSourceOriginPath = 
                    otherRelocateIt->second.computedSourceOrigin;
                const size_t elementCount = 
                    anotherSourceOriginPath.GetPathElementCount();
                if (elementCount > bestMatchElementCount &&
                        sourceOriginParent.HasPrefix(anotherSourceOriginPath)) {
                    bestMatchIt = otherRelocateIt;
                    bestMatchElementCount = elementCount;
                }
            }
            if (bestMatchIt == processedRelocates.end()) {
                continue;
            }

            // Apply the best match relocate to the computed origin source path
            // to get the most relocated source path. If this doesn't match
            // the actual source path then this relocate needs to be updated.
            SdfPath mostRelocatedSource = 
                reloInfo.computedSourceOrigin.ReplacePrefix(
                    bestMatchIt->second.computedSourceOrigin, 
                    bestMatchIt->second.targetPath);
            if (mostRelocatedSource != source) {
                relocateSourcesToMove.emplace_back(
                    source, std::move(mostRelocatedSource));
            }
        }

        // With all relocates processed we can update the necessary source paths
        // to conform to "most relocated".
        for (auto &[oldSource, newSource] : relocateSourcesToMove) {
            auto oldIt = processedRelocates.find(oldSource);
            if (auto [it, success] = processedRelocates.emplace(
                    newSource, std::move(oldIt->second));
                    !success && it->second.targetPath != oldIt->second.targetPath) {
                // It's possible that this could fail since different authored 
                // relocate sources can represent the same "most relocated" 
                // source path. Legacy relocates didn't use to correct for this
                // at all meaning it was possible to relocate a prim to multiple
                // locations if authored incorrectly. This was never intended 
                // and is impossible in the updated layer relocates. So while 
                // we're still supporting legacy relocates, we'll just warn when
                // this occurs (instead of a proper error) and use the first
                // relocate to have claimed this source path.
                TF_WARN("Could confrom relocate from %s to %s to use the "
                    "correct source path %s because a relocate from %s to %s "
                    "already exists. This relocate will be ignored.",
                    oldIt->first.GetText(),
                    oldIt->second.targetPath.GetText(),
                    newSource.GetText(),
                    it->first.GetText(),
                    it->second.targetPath.GetText());
            }
            processedRelocates.erase(oldIt);
        }       
    }

    // Run after all authored relocates are collected from all layers; validates
    // that relocates are valid in the context of all other relocates in the
    // layer stack and removes any that are not. This also populates the target
    // path to processed relocate map for all of the valid relocates.
    void 
    _ValidateAndRemoveConflictingRelocates()
    {
        using ConflictReason = 
            PcpErrorInvalidConflictingRelocation::ConflictReason;

        TRACE_FUNCTION();

        for (auto &relocatesEntry : processedRelocates) {
            const SdfPath &sourcePath = relocatesEntry.first;                        
            const SdfPath &targetPath = relocatesEntry.second.targetPath;                        

            std::string whyNot;
            // If we can't add this relocate to the "by target" map, we have
            // a duplicate target error.
            if (auto [it, success] = targetPathToProcessedRelocateMap.emplace(
                    targetPath, &relocatesEntry); !success) {
                auto *&existingAuthoredRelocatesEntry = it->second;

                // Always add this relocate entry as an error. If this function
                // returns true, it's adding the error for this target for the
                // first time so add the existing relocate entry to the error
                // as well in that case.
                if (_LogInvalidSameTargetRelocates(relocatesEntry)) {
                    _LogInvalidSameTargetRelocates(
                        *existingAuthoredRelocatesEntry);
                }
            }

            // XXX: There are some non-USD use cases that rely on the fact that
            // we validate and reject these conflicting relocates in Pcp. We 
            // will update these cases to conform in the future, but the work to
            // do so is non-trivial, so for now we need to allow these cases to 
            // still work.
            if (!_isUsd && TfGetEnvSetting(PCP_ENABLE_LEGACY_RELOCATES_BEHAVIOR)) {
                continue;
            }

            // If the target can be found as a source path of any of our 
            // relocates, then both relocates are invalid.
            if (auto it = processedRelocates.find(targetPath); 
                    it != processedRelocates.end()) {
                _LogInvalidConflictingRelocate(
                    relocatesEntry,
                    *it,
                    ConflictReason::TargetIsConflictSource);
                _LogInvalidConflictingRelocate(
                    *it,
                    relocatesEntry,
                    ConflictReason::SourceIsConflictTarget);           
            }

            // The target of a relocate must be a fully relocated path which we
            // enforce by making sure that it cannot itself be ancestrally 
            // relocated by any other relocates in the layer stack.
            for (SdfPath pathToCheck = targetPath.GetParentPath();
                    !pathToCheck.IsRootPrimPath(); 
                    pathToCheck = pathToCheck.GetParentPath()) {
                if (auto it = processedRelocates.find(pathToCheck); 
                        it != processedRelocates.end()) {
                    _LogInvalidConflictingRelocate(relocatesEntry, *it,
                        ConflictReason::TargetIsConflictSourceDescendant);
                }
            }

            // The source of a relocate must be fully relocated with respect to
            // all the other relocates (except itself). We enforce this by 
            // making sure the source path cannot be ancestrally relocated by
            // any other relocates in the layer stack.
            for (SdfPath pathToCheck = sourcePath.GetParentPath();
                    !pathToCheck.IsRootPrimPath(); 
                    pathToCheck = pathToCheck.GetParentPath()) {
                if (auto it = processedRelocates.find(pathToCheck); 
                        it != processedRelocates.end()) {
                    _LogInvalidConflictingRelocate(relocatesEntry, *it,
                        ConflictReason::SourceIsConflictSourceDescendant);
                }
            }
        }

        // After we have found all invalid relocates, we go ahead and remove
        // them from the relocates list. We do this after to make sure we're 
        // always validating each relocate against all the other relocates.

        // Process the same target errors first. Note these errors are ordered
        // by target path already since we store them as a map.
        for (auto &[targetPath, err] : _invalidSameTargetRelocates) { 
            // Errors are generated in an arbitrary and inconstistent order 
            // because we process the relocates from an unordered map. So
            // sort the error's sources for error consistency between runs and
            // across platforms. 
            std::sort(err->sources.begin(), err->sources.end(), 
                [](const auto &lhs, const auto &rhs){ 
                    return lhs.sourcePath < rhs.sourcePath;
                });

            // Delete all these errored relocates from the source and target
            // maps
            targetPathToProcessedRelocateMap.erase(err->targetPath);
            for (const auto &source : err->sources) {
                processedRelocates.erase(source.sourcePath);
            }

            // Move the error to the full error list.
            errors.push_back(std::move(err));
        }

        // Now process the other conflicting relocates errors. We sort them 
        // first to keep the error order consistent between runs across all 
        // platforms.
        std::sort(
            _invalidConflictingRelocates.begin(), 
            _invalidConflictingRelocates.end(), 
            [](const auto &lhs, const auto &rhs) {
                return 
                    std::tie(
                        lhs->sourcePath, 
                        lhs->conflictReason, 
                        lhs->conflictSourcePath) <
                    std::tie(
                        rhs->sourcePath, 
                        rhs->conflictReason, 
                        rhs->conflictSourcePath);
            });
        
        // Delete all these errored relocates from both the source and target 
        // maps
        for (const auto &err : _invalidConflictingRelocates) {
            targetPathToProcessedRelocateMap.erase(err->targetPath);
            processedRelocates.erase(err->sourcePath);

            // Move the error to the full error list.
            errors.push_back(std::move(err));
        }
    }

    // Computes the origin source path for the processed relocates map entry
    void
    _ComputeSourceOriginForTargetIfNeeded(
        ProcessedRelocatesMap::value_type *processedRelocationEntry) 
    {
        if (!TF_VERIFY(processedRelocationEntry)) {
            return;
        }

        // We'll be editing the processed relocation entry in place to add
        // the computed source origin.
        const SdfPath &sourcePath = processedRelocationEntry->first;
        ProcessedRelocateInfo &relocationInfo = processedRelocationEntry->second;

        // If the computed source origin is not empty we've computed it already
        // and can just return it.
        if (!relocationInfo.computedSourceOrigin.IsEmpty()) {
            return;
        }

        // Set the source origin to source path to start. This will typically 
        // be correct in the first place and it prevents recursion cycles if
        // we re-enter this function for the same relocate.
        relocationInfo.computedSourceOrigin = sourcePath;

        // Search for the nearest relocation entry whose target to source 
        // transformation would affect the source of our relocation. This 
        // relocation will be applied to our source path to the get the true
        // source origin.
        ProcessedRelocatesMap::value_type *nearestAncestralSourceRelocation = 
            [&]()->decltype(nearestAncestralSourceRelocation) {
                // Walk up the hierarchy looking for the first ancestor path 
                // that is the target path of another relocate and return that
                // relocate entry.
                // XXX: Note that we should be able to start the loop with 
                // sourcePath.GetPath() since A -> B, B -> C relocate chains 
                // are invalid, but we aren't yet guaranteeing those types of
                // invalid relocates are always removed yet. Once we do 
                // guarantee their removal in all cases, we can start this loop
                // with the source parent.
                for (SdfPath ancestorPath = sourcePath; 
                        !ancestorPath.IsAbsoluteRootPath(); 
                        ancestorPath = ancestorPath.GetParentPath()) {
                    PathToProcessedRelocateMap::iterator ancestorRelocateIt = 
                        targetPathToProcessedRelocateMap.find(ancestorPath);
                    if (ancestorRelocateIt != 
                            targetPathToProcessedRelocateMap.end()) {
                        return ancestorRelocateIt->second;
                    }
                }
                return nullptr;
            }();

        // If we found an ancestral relocate for the source, make sure its 
        // source origin is computed and apply its full target to source origin
        // transformation to our relocate's source path to get and store its 
        // source origin.
        if (nearestAncestralSourceRelocation) {
            _ComputeSourceOriginForTargetIfNeeded(
                nearestAncestralSourceRelocation);
            relocationInfo.computedSourceOrigin = 
                relocationInfo.computedSourceOrigin.ReplacePrefix(
                    nearestAncestralSourceRelocation->second.targetPath, 
                    nearestAncestralSourceRelocation->second.computedSourceOrigin);
        }
    }

    // Logs an invalid conflicting relocate by adding an error and logging that 
    // the entry needs to be deleted after all relocates are validated.
    void
    _LogInvalidConflictingRelocate(
        const ProcessedRelocatesMap::value_type &entry,
        const ProcessedRelocatesMap::value_type &conflictEntry,
        PcpErrorInvalidConflictingRelocation::ConflictReason conflictReason) 
    {
        // Add the error for this relocate
        PcpErrorInvalidConflictingRelocationPtr err = 
            PcpErrorInvalidConflictingRelocation::New();
        err->rootSite = _GetErrorRootSite();

        err->layer = entry.second.owningSite.layer;
        err->owningPath = entry.second.owningSite.path;
        err->sourcePath = entry.first;
        err->targetPath = entry.second.targetPath;

        err->conflictLayer = conflictEntry.second.owningSite.layer;
        err->conflictOwningPath = conflictEntry.second.owningSite.path;
        err->conflictSourcePath = conflictEntry.first;
        err->conflictTargetPath = conflictEntry.second.targetPath;

        err->conflictReason = std::move(conflictReason);

        _invalidConflictingRelocates.push_back(std::move(err));
    }

    // Logs an invalid relocate where its target is the same as another relocate
    // with a different source. Only one error is logged for each target which 
    // holds all of its sources. 
    // 
    // This returns true if a new error is created for the target and false if
    // there's already an existing error that we can just add the source info to.
    bool 
    _LogInvalidSameTargetRelocates(
        const ProcessedRelocatesMap::value_type &entry)
    {
        // See if we can add a new error
        const auto [it, createNewError] = 
            _invalidSameTargetRelocates.try_emplace(
                entry.second.targetPath, nullptr);

        PcpErrorInvalidSameTargetRelocationsPtr &err = it->second;

        // Create the error if we have to.
        if (createNewError) {
            err = PcpErrorInvalidSameTargetRelocations::New();
            err->targetPath = entry.second.targetPath;
        }

        // Always add the source info.
        err->sources.push_back({
            entry.first, 
            entry.second.owningSite.layer, 
            entry.second.owningSite.path});

        return createNewError;
    }

    PcpSite _GetErrorRootSite() const
    {
        return PcpSite(
            _layerStack ? _layerStack->GetIdentifier() : PcpLayerStackIdentifier(), 
            SdfPath::AbsoluteRootPath());
    }

    const PcpLayerStack *_layerStack = nullptr;
    bool _isUsd = true;

    std::vector<PcpErrorInvalidConflictingRelocationPtr>
        _invalidConflictingRelocates;
    std::map<SdfPath, PcpErrorInvalidSameTargetRelocationsPtr> 
        _invalidSameTargetRelocates;
};

}; // end anonymous namespace

void
Pcp_BuildRelocateMap(
    const std::vector<std::pair<SdfLayerHandle, SdfRelocates>> &layerRelocates,
    SdfRelocatesMap *relocatesMap,
    PcpErrorVector *errors)
{
    Pcp_ComputeRelocationsForLayerStackWorkspace ws;
    ws.Compute(layerRelocates);

    relocatesMap->clear();
    for (const auto &[source, reloInfo] : ws.processedRelocates) {
        (*relocatesMap)[source] = reloInfo.targetPath;
    }

    if (errors) {
        *errors = std::move(ws.errors);
    }
}

void
Pcp_ComputeRelocationsForLayerStack(
    const PcpLayerStack &layerStack,
    SdfRelocatesMap *relocatesSourceToTarget,
    SdfRelocatesMap *relocatesTargetToSource,
    SdfRelocatesMap *incrementalRelocatesSourceToTarget,
    SdfRelocatesMap *incrementalRelocatesTargetToSource,
    SdfPathVector *relocatesPrimPaths,
    PcpErrorVector *errors)
{
    TRACE_FUNCTION();

    // Use the workspace helper to compute and validate the full set of 
    // relocates on the layer stack.
    Pcp_ComputeRelocationsForLayerStackWorkspace ws;
    ws.Compute(layerStack);

    // Take any encountered errors.
    if (errors && !ws.errors.empty()) {
        if (errors->empty()) {
            *errors = std::move(ws.errors);
        } else {
            errors->insert(
                errors->end(),
                std::make_move_iterator(ws.errors.begin()),
                std::make_move_iterator(ws.errors.end()));
        }
    }

    if (ws.processedRelocates.empty()) {
        return;
    }

    // Use the processed relocates to populate the bi-directional mapping of all
    // the relocates maps.
    if (TfGetEnvSetting(PCP_ENABLE_LEGACY_RELOCATES_BEHAVIOR)) {
        for (const auto &[source, reloInfo] : ws.processedRelocates) {
            incrementalRelocatesSourceToTarget->emplace(
                source, reloInfo.targetPath);
            // XXX: With the legacy behavior you can end up with the erroneous 
            // behavior of more than one source mapping to the same target. We 
            // need to at least make this consistent by making sure we choose 
            // the lexicographically greater source when we have a target conflict.
            auto [it, inserted] = incrementalRelocatesTargetToSource->emplace(
                reloInfo.targetPath, source);
            if (!inserted && source > it->second) {
                it->second = source;
            }

            relocatesTargetToSource->emplace(
                reloInfo.targetPath, reloInfo.computedSourceOrigin);
            relocatesSourceToTarget->emplace(
                reloInfo.computedSourceOrigin, reloInfo.targetPath);
        }       
    } else {
        for (const auto &[source, reloInfo] : ws.processedRelocates) {
            incrementalRelocatesSourceToTarget->emplace(
                source, reloInfo.targetPath);
            incrementalRelocatesTargetToSource->emplace(
                reloInfo.targetPath, source);

            relocatesTargetToSource->emplace(
                reloInfo.targetPath, reloInfo.computedSourceOrigin);
            relocatesSourceToTarget->emplace(
                reloInfo.computedSourceOrigin, reloInfo.targetPath);
        }       
    }

    // Take the list of prim paths with relocates.
    relocatesPrimPaths->assign(
        std::make_move_iterator(ws.allPrimPathsWithAuthoredRelocates.begin()), 
        std::make_move_iterator(ws.allPrimPathsWithAuthoredRelocates.end()));
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
        _BlowRelocations();
        _Compute(_registry->_GetFileFormatTarget(), _registry->_GetMutedLayers());

        // Recompute the derived relocation variables.
        for (auto &[path, variable] : _relocatesVariables) {
            variable->SetValue(_FilterRelocationsForPath(*this, path));
        }
    } else if (changes.didChangeSignificantly || changes.didChangeRelocates) {

        PcpErrorVector errors;

        // We're only updating relocates in this case so if we have any current
        // local errors, copy any that aren't relocates errors over.
        if (_localErrors) {
            errors.reserve(_localErrors->size());
            for (const auto &err : *_localErrors) {
                if (!std::dynamic_pointer_cast<PcpErrorRelocationBasePtr>(err)) {
                    errors.push_back(err);
                }
            }
        }

        // Blow the relocations if they changed specifically, or if there's been
        // a significant change.
        // A significant change means the composed opinions of the layer stack
        // has changed in arbitrary ways, so we need to recompute the relocation
        // table.
        _BlowRelocations();
        if (changes.didChangeSignificantly) {
            // Recompute relocations from scratch.
            Pcp_ComputeRelocationsForLayerStack(
                *this,
                &_relocatesSourceToTarget,
                &_relocatesTargetToSource,
                &_incrementalRelocatesSourceToTarget,
                &_incrementalRelocatesTargetToSource,
                &_relocatesPrimPaths,
                &errors);
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
            errors.insert(errors.end(), 
                changes.newRelocatesErrors.begin(), 
                changes.newRelocatesErrors.end());
        }
        
        // Recompute the derived relocation variables.
        for (auto &[path, variable] : _relocatesVariables) {
            variable->SetValue(_FilterRelocationsForPath(*this, path));
        }

        if (errors.empty()) {
            _localErrors.reset();
        } else {
            _localErrors.reset(new PcpErrorVector);
            _localErrors->swap(errors);
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

const SdfLayerTreeHandle& 
PcpLayerStack::GetSessionLayerTree() const
{
    return _sessionLayerTree;
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
    // Don't waste time and memory if there are no relocates.
    if (IsUsd() && !HasRelocates()) {
        return PcpMapExpression();
    }

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

bool PcpLayerStack::HasRelocates() const
{
    // Doesn't matter which of the relocates maps we check; they'll
    // either be all empty or all non-empty.
    return !_incrementalRelocatesSourceToTarget.empty();
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
    _sessionLayerTree = TfNullPtr;
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

            _sessionLayerTree = 
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

            _Helper::FindSessionOwner(_sessionLayerTree, &sessionOwner);
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

    Pcp_ComputeRelocationsForLayerStack(*this,
                                        &_relocatesSourceToTarget,
                                        &_relocatesTargetToSource,
                                        &_incrementalRelocatesSourceToTarget,
                                        &_incrementalRelocatesTargetToSource,
                                        &_relocatesPrimPaths,
                                        &errors);

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
