//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

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

template <typename... Args>
static void _PrintToWhyNot(
    std::string *whyNot,
    const SdfPath &source, const SdfPath &target,
    const char *format, Args... args)
{
    if (!whyNot) {
        return;
    }
    *whyNot = 
        TfStringPrintf("Cannot relocate <%s> to <%s>: ",
            source.GetText(), target.GetText()) +
        TfStringPrintf(format, args...);
}

static bool 
_ValidateAgainstExistingRelocate(
    const SdfPath &newSource, const SdfPath &newTarget,
    const SdfPath &existingSource, const SdfPath &existingTarget,
    std::string *whyNot)
{
    // Cannot relocate a descendant of a path that is already the source 
    // of an existing relocate.
    if (newSource.HasPrefix(existingSource)) {
        _PrintToWhyNot(whyNot, newSource, newTarget,
            "A relocate from <%s> to <%s> already exists; neither the "
            "source <%s> nor any of its descendants can be relocated again "
            "using their original paths.", 
            existingSource.GetText(),
            existingTarget.GetText(),
            existingSource.GetText());
        return false;
    }

    // If the target is empty, we're good after validating the source path.
    if (newTarget.IsEmpty()) {
        return true;
    }

    // Cannot relocate to an existing relocate's target again. E.g. if a 
    // relocate from <A> -> <B> already exists, we cannot add a relocate 
    // from <C> -> <B>.
    if (newTarget == existingTarget) {
        _PrintToWhyNot(whyNot, newSource, newTarget,
            "A relocate from <%s> to <%s> already exists and the same "
            "target cannot be relocated to again.", 
            existingSource.GetText(),
            existingTarget.GetText());
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
            _PrintToWhyNot(whyNot, newSource, newTarget,
                "Cannot relocate a prim to be a descendant of <%s> which "
                "is already relocated to <%s>.", 
                existingSource.GetText(),
                existingTarget.GetText());
            return false;
        } 

        if (newSource != existingTarget) {
            _PrintToWhyNot(whyNot, newSource, newTarget,
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

void 
PcpLayerRelocatesEditBuilder::_UpdateExistingRelocates(
    const SdfPath &source, const SdfPath &target)
{
    // For each layer with relocates entries update all of them that need to 
    // have their source or target paths ancestrally relocated by the new 
    // relocate.
    for (auto &[layer, relocates] : _layerRelocatesEdits ) {
        if (Pcp_ModifyRelocates(&relocates, source, target)) {
            _layersWithRelocatesChanges.insert(layer);
        }
    }
}

bool
Pcp_ModifyRelocates(SdfRelocates *relocates,
    const SdfPath &oldPath, const SdfPath &newPath)
{
    bool modified = false;

    for (auto &[existingSource, existingTarget] : *relocates) {
        // If the existing relocate source would be ancestrally relocated by
        // the new relocate, apply the relocate to it.
        if (existingSource.HasPrefix(oldPath)) {
            existingSource = 
                existingSource.ReplacePrefix(oldPath, newPath);
            modified = true;
        }
        // If the existing target source would be ancestrally relocated by
        // the new relocate, apply the relocate to it.
        if (existingTarget.HasPrefix(oldPath)) {
            existingTarget = 
                existingTarget.ReplacePrefix(oldPath, newPath);
            modified = true;
        }
    }

    // Applying the new relocate to the existing relocates can cause any 
    // number of them to map a source path to itself, making them redundant
    // no-ops. These cases are effectively a relocate delete so we remove
    // these relocates from the layer's relocates list.
    if (modified) {
        relocates->erase(
            std::remove_if(relocates->begin(), relocates->end(),
                [](const auto &relocate) { 
                    return relocate.first.IsEmpty() || 
                        relocate.first == relocate.second;
                }),
            relocates->end());
    }
    return modified;
}

bool
PcpLayerRelocatesEditBuilder::Relocate(
    const SdfPath &newSource, 
    const SdfPath &newTarget, 
    std::string *whyNot)
{
    if (_layerRelocatesEdits.empty()) {
        TF_CODING_ERROR("Relocates edit builder is invalid");
        return false;
    }

    // Validate the that this source and target pair is a valid relocate period.
    std::string reason;
    if (!Pcp_IsValidRelocatesEntry(newSource, newTarget, &reason)) {
        _PrintToWhyNot(whyNot, newSource, newTarget, "%s", reason.c_str());
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

    // One last validation: if this would result in adding a new relocate entry
    // we have to make sure the source is not a root prim as that is invalid in
    // the layer stack. We can't filter out root prim sources before this point
    // as we allow root prims that are already targets of relocates to be 
    // re-relocated through this method.
    if (addNewRelocate && newSource.IsRootPrimPath()) {
        _PrintToWhyNot(whyNot, newSource, newTarget,
            "Adding a relocate from <%s> would result in a root prim "
            "being relocated.", newSource.GetText());
        return false;
    }

    // Update existing relocates to account for how this new relocation will 
    // change their paths.
    _UpdateExistingRelocates(newSource, newTarget);

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

bool 
PcpLayerRelocatesEditBuilder::RemoveRelocate(
    const SdfPath &sourcePath,
    std::string *whyNot)
{
    const auto &relocatesMap = GetEditedRelocatesMap();
    auto it = relocatesMap.find(sourcePath);
    if (it == relocatesMap.end()) {
        if (whyNot) {
            *whyNot = TfStringPrintf("Cannot remove relocate for source path "
                "<%s>: No relocate with the source path found.",
                sourcePath.GetText());
            return false;
        }
    }

    const SdfPath &targetPath = it->second;
    if (targetPath.IsEmpty()) {
        // If this the target path of the existing relocate is empty, we had a
        // "deletion" relocate. To remove it we just have to delete any 
        // relocates entries in any layer that use the source path.
        for (auto &[layer, relocates] : _layerRelocatesEdits ) {
            auto removeIt = std::remove_if(relocates.begin(), relocates.end(),
                [&](const auto &relocate) { 
                    return relocate.first == sourcePath;
                });
            if (removeIt != relocates.end()) {
                _layersWithRelocatesChanges.insert(layer);
                relocates.erase(removeIt, relocates.end());
            }
        }
    } else {
        // Update existing relocates as if we have relocated the target path 
        // back to the source path in order to account for how removing this
        // relocation will change their paths. Note that this call will handle
        // removing the existing relocate itself. Also note that we do not have
        // to do any validation of the source and target paths as their presence
        // in the relocates map already assures the validity of this call.
        _UpdateExistingRelocates(targetPath, sourcePath);
    }

    // The relocates were updated so the relocates map will need to be
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
