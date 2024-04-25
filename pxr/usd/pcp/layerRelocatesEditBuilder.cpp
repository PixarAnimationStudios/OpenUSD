//
// Copyright 2024 Pixar
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

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerRelocatesEditBuilder.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/sdf/layer.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

PcpLayerRelocatesEditBuilder::PcpLayerRelocatesEditBuilder(
    const PcpLayerStackPtr &layerStack,
    const SdfLayerHandle &addNewRelocatesLayer)
{
    if (!layerStack) {
        TF_CODING_ERROR("No layer stack provided to relocates edit builder.");
        return;
    }

    // If a layer for adding new relocates is not specified we use the layer 
    // stack's root layer for any new relocates.
    const SdfLayerHandle newRelocatesLayer =  addNewRelocatesLayer ? 
        addNewRelocatesLayer : layerStack->GetIdentifier().rootLayer;

    if (!layerStack->HasLayer(newRelocatesLayer)) {
        TF_CODING_ERROR("The layer for adding new relocates does not belong to "
            "the layer stack.");
        return;
    }

    // Get the authored layer relocates for each layer in the layer stack so we
    // can keep track of all the layer metadata edits that would need to be 
    // performed to udpate the layer stack's relocates.
    for (const auto &layer: layerStack->GetLayers()) {

        SdfRelocates layerRelocates;
        const bool layerHasRelocates = layer->HasField(
            SdfPath::AbsoluteRootPath(), SdfFieldKeys->LayerRelocates, 
            &layerRelocates);
 
        // We skip layers that dont have any relocates unless the layer is
        // the layer specified for adding new relocates.
        if (layer == newRelocatesLayer) {
            // If this layer is where new relocates will be added, store 
            // what its index will be so we can find it later.
            _editForNewRelocatesIndex = _layerRelocatesEdits.size();
        } else if (!layerHasRelocates) {
            continue;
        }

        _layerRelocatesEdits.emplace_back(layer, std::move(layerRelocates));
    }

    if (!TF_VERIFY(_editForNewRelocatesIndex < _layerRelocatesEdits.size())) {
        // Clear layer relocates if we fail this verify to simplify other 
        // checks.
        _layerRelocatesEdits.clear();
    }

    // Build and cache the relocates map immediately from layer stack layers
    // so we can collect any relocates errors in the current layer stack. We
    // update the layers edits to remove the error causing relocates here so 
    // that even if no new relocates are added, applying the current edits 
    // will produce a layer stack with no relocates errors.
    SdfRelocatesMap relocatesMap;
    PcpErrorVector errors;
    Pcp_BuildRelocateMap(_layerRelocatesEdits, &relocatesMap, &errors);
    _relocatesMap = std::move(relocatesMap);

    // Note that this only has to be done once in the constructor as all calls
    // to Relocate will maintain relocates edits that produce no errors.
    _RemoveRelocatesWithErrors(errors);
}

void 
PcpLayerRelocatesEditBuilder::_RemoveRelocatesWithErrors(
    const PcpErrorVector &errors)
{
    using PathSet = std::unordered_set<SdfPath, TfHash>;
    using RelocateSet = std::unordered_set<SdfRelocate, TfHash>;
    using LayerToRelocateSetMap = std::map<SdfLayerHandle, RelocateSet>;

    if (errors.empty()) {
        return;
    }

    PathSet relocateSourcePathsToDelete;
    LayerToRelocateSetMap relocatesToDeletePerLayer;

    // There are a few different types of relocation errors. The type determines
    // how we handle fixing the error.
    for (const auto &error : errors) {
        if (error->errorType == PcpErrorType_InvalidAuthoredRelocation) {
            // Authored relocation errors are for relocate entries that will
            // always be invalid in any context. These relocates are marked
            // to be deleted from their layers.
            const auto *err =
                static_cast<const PcpErrorInvalidAuthoredRelocation *>(
                    error.get());
            relocatesToDeletePerLayer[err->layer].emplace(
                err->sourcePath, err->targetPath);
        } else if (error->errorType == PcpErrorType_InvalidConflictingRelocation) {
            // A conflicting relocate is invalid in the context of other 
            // relocates. To clear these we have to remove any relocate that 
            // uses source path from any layer. This is to ensure that deleting
            // the invalid relocate from one layer will not make a relocate with
            // same source from different layer (that could potentially be 
            // valid) now pop through, changing the value of the computed 
            // relocates map.
            const auto *err =
                static_cast<const PcpErrorInvalidConflictingRelocation *>(
                    error.get());
            relocateSourcePathsToDelete.insert(err->sourcePath);
        } else if (error->errorType == PcpErrorType_InvalidSameTargetRelocations) {
            // Invalid same target relocate errors are similar to the 
            // conflicting relocate error, except it instead holds multiple 
            // source paths. We have to remove all relocates using all sources
            // in the error for the same reason as the conflicting relocate
            // error case.
            const PcpErrorInvalidSameTargetRelocations *err =
                static_cast<const PcpErrorInvalidSameTargetRelocations *>(
                    error.get());
            for (const auto &source : err->sources) {
                relocateSourcePathsToDelete.insert(source.sourcePath);
            }
        } else {
            TF_CODING_ERROR("Unexpected error type: %s", 
            TfStringify(TfEnum(error->errorType)).c_str());
        };
    }

    // If there are any relocates to delete from individual layers, we do that 
    // first.
    if (!relocatesToDeletePerLayer.empty()) {
        for (auto &[layer, relocates] : _layerRelocatesEdits ) {
            // Skip if there are no relocates to delete for this layer
            const RelocateSet *relocatesToDelete = 
                TfMapLookupPtr(relocatesToDeletePerLayer, layer);
            if (!relocatesToDelete) {
                continue;
            }

            // Otherwise mark that this layer has relocates changes and remove
            // the invalid relocates from the edited value.
            _layersWithRelocatesChanges.insert(layer);
            relocates.erase(
                std::remove_if(relocates.begin(), relocates.end(),
                    [&](const auto &relocate) { 
                        return relocatesToDelete->count(relocate);
                    }),
                relocates.end());
        }
    }

    // If there any relocate sources to delete, do those now too.
    if (!relocateSourcePathsToDelete.empty()) {
        for (auto &[layer, relocates] : _layerRelocatesEdits ) {
            // Remove any relocate entries in relocates value that use as source
            // path that needs to be deleted.
            const auto eraseIt = 
                std::remove_if(relocates.begin(), relocates.end(),
                    [&](const auto &relocate) { 
                        return relocateSourcePathsToDelete.count(relocate.first);
                    });
            // If we removed entries, erase them and mark the layer as having
            // changes.
            if (eraseIt != relocates.end()) {
                _layersWithRelocatesChanges.insert(layer);
                relocates.erase(eraseIt, relocates.end());
            }
        }
    }
}

const SdfRelocatesMap &
PcpLayerRelocatesEditBuilder::GetEditedRelocatesMap() const {
    // Only rebuild the map if needed.
    if (!_relocatesMap) {
        SdfRelocatesMap relocatesMap;
        PcpErrorVector errors;
        Pcp_BuildRelocateMap(_layerRelocatesEdits, &relocatesMap, &errors);
        // The layer relocates edits are maintained such that they never produce
        // errors when used to build a relocates map for the layer stack. 
        // Verify that here to catch any possible mistakes in maintaining this
        // invariant.
        TF_VERIFY(errors.empty());
        _relocatesMap = std::move(relocatesMap);       
    }
    return *_relocatesMap;
}

bool
PcpLayerRelocatesEditBuilder::Relocate(
    const SdfPath &source, 
    const SdfPath &target, 
    std::string *whyNot)
{
    // When building relocates, we'll allow the source path to be an unrelocated
    // path (or even partially relocated if there are mutltiple ancestral 
    // relocates that affect a path). But these source paths need to mapped to 
    // their fully relocated path before applying as that is how they must be
    // expressed in the final relocates map.
    // 
    // Note that we do not do the same for target paths as these must only be 
    // expressed in their final relocated paths as we cannot always determine 
    // the intention otherwise.

    // We'll apply relocates starting from the root most ancestor, Get all the
    // prefixes so we can do this cumulatively in order
    SdfPathVector ancestorPaths = source.GetPrefixes();
    for (auto ancestorPathsIt = ancestorPaths.begin(); 
            ancestorPathsIt != ancestorPaths.end(); 
            ++ancestorPathsIt) {

        // Find an existing relocate that moves this ancestor path and if we do
        // apply the source to target mapping to it and ALL of its descendant 
        // paths in the ancestor paths vector.
        const SdfRelocatesMap &relocatesMap = GetEditedRelocatesMap();
        const auto foundRelocateIt = relocatesMap.find(*ancestorPathsIt);
        if (foundRelocateIt == relocatesMap.end()) {
            continue;
        }

        const SdfPath &relocateSource = foundRelocateIt->first;
        const SdfPath &relocateTarget = foundRelocateIt->second;

        for (auto pathsToRelocateIt = ancestorPathsIt; 
                pathsToRelocateIt != ancestorPaths.end(); 
                ++pathsToRelocateIt) {
            *pathsToRelocateIt = pathsToRelocateIt->ReplacePrefix(
                relocateSource, relocateTarget);
        }
    }

    // The last path in ancestorPaths started as the source path itself, so now
    // the last path is the original source path with all ancestral relocates 
    // applied.
    const SdfPath &relocatedSource = ancestorPaths.back();

    // Attempt to add the relocate with the updated source path.
    std::string reason;
    if (_AddAndUpdateRelocates(relocatedSource, target, &reason)) {
        // Success!
        return true;
    }

    // Relocate failed if we get here, so report the reason if desired.
    if (whyNot) {
        if (relocatedSource != source) {
            *whyNot = TfStringPrintf("Cannot relocate <%s> (relocated from "
                "original source <%s>) to <%s>: %s",
                relocatedSource.GetText(),
                source.GetText(),
                target.GetText(),
                reason.c_str());
        } else {
            *whyNot = 
                TfStringPrintf("Cannot relocate <%s> to <%s>: %s",
                    relocatedSource.GetText(),
                    target.GetText(),
                    reason.c_str());
        }
    }
    return false;
}

static bool 
_ValidateAgainstExistingRelocate(
    const SdfPath &newSource, const SdfPath &newTarget,
    const SdfPath &existingSource, const SdfPath &existingTarget,
    std::string *whyNot)
{
    // Cannot relocate to an existing relocate's target again. E.g. if a 
    // relocate from <A> -> <B> already exists, we cannot add a relocate 
    // from <C> -> <B>.
    if (newTarget == existingTarget) {
        *whyNot = TfStringPrintf(
            "A relocate from <%s> to <%s> already exists and the same "
            "target cannot be relocated to again.", 
            existingSource.GetText(),
            existingTarget.GetText());
        return false;
    }

    // Cannot relocate a descendant of a path that is already the source 
    // of an existing relocate.
    //
    // This case should be impossible as all source paths are updated to
    // their fully ancestrally relocated paths before they get passed to 
    // this function. E.g. if a relocate from <A> -> <B> already existed 
    // and we tried to add a relocate from <A/C> -> <C>, the source path 
    // <A/C> would have already been converted to <B/C> before we got here,
    // therefore avoiding this error condition.
    //
    // We still check it and issue a coding error just in case.
    if (ARCH_UNLIKELY(newSource.HasPrefix(existingSource))) {
        TF_CODING_ERROR(
            "A relocate from <%s> to <%s> already exists; neither the "
            "source <%s> nor any of its descendants can be relocated again "
            "using their original paths.", 
            existingSource.GetText(),
            existingTarget.GetText(),
            existingSource.GetText());
            return false;
    }

    // The target of a relocate cannot be a prim, or a descendant of a prim,
    // that has been itself relocated with one notable exception: a 
    // directly relocated prim can be relocated back to its immediate 
    // source effectively deleting the relocate.
    //
    // So, for example, if /A/B is relocated to /A/C, no other prim except 
    // /A/C can be relocated to /A/B or any descendant path of /A/B as the 
    // namespace hierarchy starting at /A/B is a tombstone. But /A/C itself
    // can be relocated back to /A/B which has the effect of "unrelocating"
    // /A/B.
    if (newTarget.HasPrefix(existingSource)) {
        if (newTarget != existingSource) {
            *whyNot = TfStringPrintf(
                "Cannot relocate a prim to be a descendant of <%s> which "
                "is already relocated to <%s>.", 
                existingSource.GetText(),
                existingTarget.GetText());
            return false;
        } 

        if (newSource != existingTarget) {
            *whyNot = TfStringPrintf(
                "The target of the relocate is the same as the source of "
                "an existing relocate from <%s> to <%s>; the only prim "
                "that can be relocated to <%s> is the existing relocate's "
                "target <%s>, which will remove the relocate.", 
                existingSource.GetText(),
                existingTarget.GetText(),
                existingSource.GetText(),
                existingTarget.GetText());
            return false;
        }
    }

    return true;
}

bool
PcpLayerRelocatesEditBuilder::_AddAndUpdateRelocates(
    const SdfPath &newSource, 
    const SdfPath &newTarget, 
    std::string *whyNot)
{
    if (_layerRelocatesEdits.empty()) {
        TF_CODING_ERROR("Relocates edit builder is invalid");
        return false;
    }

    // Validate the that this source and target pair is a valid relocate period.
    if (!Pcp_IsValidRelocatesEntry(newSource, newTarget, whyNot)) {
        return false;
    }

    // Validate that we can add this relocate given all the current relocates on
    // the layer stack. This loop will also determine whether the new relocate 
    // entry needs to be added or if only updates to existing relocates are 
    // needed.
    bool addNewRelocate = true;
    for (auto &existingRelocate : GetEditedRelocatesMap()) {
        const SdfPath &existingSource = existingRelocate.first;
        const SdfPath &existingTarget = existingRelocate.second;

        if (!_ValidateAgainstExistingRelocate(
                newSource, newTarget, existingSource, existingTarget, whyNot)) {
            return false;
        }

        // We will add a new relocate entry unless the new relocate is moving 
        // an existing relocate's target. In that case we only want to update
        // the existing relocate to use the new target path.
        // E.g. We already already have a relocate from </Root/A> -> </Root/B> 
        // and we go to add a new relocate from </Root/B> -> </Root/C>. In this
        // case the existing relocate will be changed to </Root/A> -> </Root/C>
        // and we cannot add </Root/B> -> </Root/C> itself as that would be a
        // conflict with the existing relocate (both would have the same target)
        if (newSource == existingTarget) {
            addNewRelocate = false;
        }
    }

    // For each layer with relocates entries update all of them that need to 
    // have their source or target paths ancestrally relocated by the new 
    // relocate.
    for (auto &[layer, relocates] : _layerRelocatesEdits ) {
        
        for (auto &[existingSource, existingTarget] : relocates) {
            // If the existing relocate source would be ancestrally relocated by
            // the new relocate, apply the relocate to it.
            if (existingSource.HasPrefix(newSource)) {
                existingSource = 
                    existingSource.ReplacePrefix(newSource, newTarget);
                _layersWithRelocatesChanges.insert(layer);
            }
            // If the existing target source would be ancestrally relocated by
            // the new relocate, apply the relocate to it.
            if (existingTarget.HasPrefix(newSource)) {
                existingTarget = 
                    existingTarget.ReplacePrefix(newSource, newTarget);
                _layersWithRelocatesChanges.insert(layer);
            }
        }

        // Applying the new relocate to the existing relocates can cause any 
        // number of them to map a source path to itself, making them redundant
        // no-ops. These cases are effectively a relocate delete so we remove
        // these relocates from the layer's relocates list.
        relocates.erase(
            std::remove_if(relocates.begin(), relocates.end(),
                [](const auto &relocate) { 
                    return relocate.first == relocate.second;
                }),
            relocates.end());
    }

    // Always add the new relocate after updating existing relocates so we don't
    // end up updating it to be relocated by itself.
    if (addNewRelocate) {
        // New relocates entries added to a specified layer for for this 
        // builder.
        LayerRelocatesEdit &layerRelocatesEdit = 
            _layerRelocatesEdits[_editForNewRelocatesIndex];
        layerRelocatesEdit.second.emplace_back(newSource, newTarget);
        _layersWithRelocatesChanges.insert(layerRelocatesEdit.first);
    }

    // The relocate was added successfully so the relocates map will need to be
    // recomputed the next time it's needed.
    _relocatesMap.reset();

    return true;
}

PcpLayerRelocatesEditBuilder::LayerRelocatesEdits
PcpLayerRelocatesEditBuilder::GetEdits() const
{
    std::vector<PcpLayerRelocatesEditBuilder::LayerRelocatesEdit> result;
    result.reserve(_layersWithRelocatesChanges.size());
    for (const auto &edit : _layerRelocatesEdits) {
        // Filter out layers that won't have changes to their original 
        // relocates values.
        if (_layersWithRelocatesChanges.count(edit.first)) {
            result.push_back(edit);
        }
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
