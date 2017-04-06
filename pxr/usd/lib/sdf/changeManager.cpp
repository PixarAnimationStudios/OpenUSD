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

#include "pxr/pxr.h"
#include "pxr/usd/sdf/changeManager.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/stackTrace.h"

#include <tbb/atomic.h>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(Sdf_ChangeManager);

Sdf_ChangeManager::_Data::_Data()
    : changeBlockDepth(0)
{
}

Sdf_ChangeManager::Sdf_ChangeManager()
{
}

Sdf_ChangeManager::~Sdf_ChangeManager()
{
}

void
Sdf_ChangeManager::_SendNoticesForChangeList( const SdfLayerHandle & layer,
                                              const SdfChangeList & changeList )
{
    // Notice which is only sent if a layer changes it's 'dirtiness'
    // This is useful in cases where I am just interested if a layer
    // has been dirtied or un-dirtied (changes undone)
    if (layer->_UpdateLastDirtinessState())
        SdfNotice::LayerDirtinessChanged().Send(layer);

    const SdfChangeList::EntryList & entryList = changeList.GetEntryList();
    TF_FOR_ALL(pathChanges, entryList) {
        const SdfPath & path = pathChanges->first;
        const SdfChangeList::Entry & entry = pathChanges->second;

        TF_FOR_ALL(it, entry.infoChanged) {
            if (path == SdfPath::AbsoluteRootPath())
                SdfNotice::LayerInfoDidChange(it->first).Send(layer);
        }

        if (entry.flags.didChangeIdentifier) {
            SdfNotice::LayerIdentifierDidChange(
                entry.oldIdentifier, layer->GetIdentifier()).Send(layer);
        }
        if (entry.flags.didReplaceContent)
            SdfNotice::LayerDidReplaceContent().Send(layer);
        if (entry.flags.didReloadContent)
            SdfNotice::LayerDidReloadContent().Send(layer);
    }
}

void
Sdf_ChangeManager::OpenChangeBlock()
{
    ++_data.local().changeBlockDepth;
}

void
Sdf_ChangeManager::CloseChangeBlock()
{
    int &changeBlockDepth = _data.local().changeBlockDepth;
    if (changeBlockDepth == 1) {
        // Closing outermost (last) change block.  Process removes while
        // the change block is still open.
        _ProcessRemoveIfInert();

        // Send notices with no change block open.
        --changeBlockDepth;
        TF_VERIFY(changeBlockDepth == 0);
        _SendNotices();
    }
    else {
        // Not outermost.
        TF_VERIFY(changeBlockDepth > 0);
        --changeBlockDepth;
    }
}

void
Sdf_ChangeManager::RemoveSpecIfInert(const SdfSpec& spec)
{
    // Add spec.   Process remove if we're not in a change block.
    OpenChangeBlock();
    _data.local().removeIfInert.push_back(spec);
    CloseChangeBlock();
}

void
Sdf_ChangeManager::_ProcessRemoveIfInert()
{
    _Data& data = _data.local();

    // We expect to be in an outermost change block here.
    TF_VERIFY(data.changeBlockDepth == 1);

    // Swap pending removes into a local variable.
    vector<SdfSpec> remove;
    remove.swap(data.removeIfInert);

    // Remove inert stuff.
    TF_FOR_ALL(i, remove) {
        i->GetLayer()->_RemoveIfInert(*i);
    }

    // We don't expect any deferred removes to have been added.
    TF_VERIFY(data.removeIfInert.empty());

    // We should still be in an outermost change block.
    TF_VERIFY(data.changeBlockDepth == 1);
}

static tbb::atomic<size_t> &
_InitChangeSerialNumber() {
    static tbb::atomic<size_t> value;
    value = 1;
    return value;
}

void
Sdf_ChangeManager::_SendNotices()
{
    // Swap out the list of events to deliver so that notice listeners
    // can safely queue up more changes. We also need to filter out any
    // changes from layers that have since been destroyed, as the change
    // manager should only send notifications for existing layers.
    SdfLayerChangeListMap changes;
    changes.swap(_data.local().changes);

    SdfLayerChangeListMap::iterator mapIter = changes.begin();
    while (mapIter != changes.end()) {
        SdfLayerChangeListMap::iterator currIter = (mapIter++);
        if (!currIter->first) {
            changes.erase(currIter);
        }
    }

    if (changes.empty())
        return;

    TF_FOR_ALL(it, changes) {

        // Send layer-specific notices.
        _SendNoticesForChangeList(it->first, it->second);

        if (TfDebug::IsEnabled(SDF_CHANGES)) {
            TF_DEBUG(SDF_CHANGES).Msg("Changes to layer %s:\n%s",
                                     it->first->GetIdentifier().c_str(),
                                     TfStringify(it->second).c_str());
        }
    }

    // Obtain a serial number for this round of change processing.
    static tbb::atomic<size_t> &changeSerialNumber = _InitChangeSerialNumber();
    size_t serialNumber = changeSerialNumber.fetch_and_increment();

    // Send global notice.
    SdfNotice::LayersDidChange(changes, serialNumber).Send();

    // Send per-layer notices with change round number.  This is so clients
    // don't have to be invoked on every round of change processing if they are
    // only interested in a subset of layers.
    SdfNotice::LayersDidChangeSentPerLayer n(changes, serialNumber);
    TF_FOR_ALL(it, changes)
        n.Send(it->first);
}

void
Sdf_ChangeManager::DidReplaceLayerContent(const SdfLayerHandle &layer)
{
    if (!layer->_ShouldNotify())
        return;
    _data.local().changes[layer].DidReplaceLayerContent();
}

void
Sdf_ChangeManager::DidReloadLayerContent(const SdfLayerHandle &layer)
{
    if (!layer->_ShouldNotify())
        return;
    _data.local().changes[layer].DidReloadLayerContent();
}

void 
Sdf_ChangeManager::DidChangeLayerIdentifier(const SdfLayerHandle &layer,
                                          const std::string &oldIdentifier)
{
    if (!layer->_ShouldNotify())
        return;
    _data.local().changes[layer].DidChangeLayerIdentifier(oldIdentifier);
}

void 
Sdf_ChangeManager::DidChangeLayerResolvedPath(const SdfLayerHandle &layer)
{
    if (!layer->_ShouldNotify())
        return;
    _data.local().changes[layer].DidChangeLayerResolvedPath();
}

static bool
_IsOrderChangeOnly(const VtValue & oldVal, const VtValue & newVal )
{
    // Note: As an optimization, we assume here that the caller has
    // already guaranteed that oldVal != newVal.
    if (!oldVal.IsEmpty() && !newVal.IsEmpty()) {
        const TfTokenVector & oldNames = oldVal.Get<TfTokenVector>();
        const TfTokenVector & newNames = newVal.Get<TfTokenVector>();
        if (oldNames.size() == newNames.size()) {
            TRACE_SCOPE("Sdf_ChangeManager::DidChangeField - "
                        "Comparing old/new PrimChildren order");
            // XXX:optimization: This may turn out to be too slow,
            // meriting a more sophisticated approach.
            std::set<TfToken, TfTokenFastArbitraryLessThan>
                oldNamesSet(oldNames.begin(), oldNames.end()),
                newNamesSet(newNames.begin(), newNames.end());
            return oldNamesSet == newNamesSet;
        }
    }
    return false;
}

void
Sdf_ChangeManager::DidChangeField(const SdfLayerHandle &layer,
                                const SdfPath & path, const TfToken &field,
                                const VtValue & oldVal, const VtValue & newVal )
{
    if (!layer->_ShouldNotify())
        return;

    SdfLayerChangeListMap &changes = _data.local().changes;

    // Note:  We intend to change the SdfChangeList protocol to provide a
    // sequence of (layer, path, field, oldValue, newValue) tuples.
    // For now, this function adapts field-based changes into the
    // existing protocol.

    if (field == SdfFieldKeys->PrimOrder) {
        changes[layer].DidReorderPrims(path);
    }
    else if (field == SdfChildrenKeys->PrimChildren) {
        // XXX:OrderNotification:
        // Sdf's change protocol does not have a "children changed"
        // message; instead it relies on a combination of "order changed"
        // and "child added/removed" messages.  To avoid sending a
        // potentially misleading "order changed" message when in fact
        // children have been added and removed, we compare the old
        // and new children lists and only send an "order changed"
        // message if this is a pure order change.
        if (_IsOrderChangeOnly(oldVal, newVal)) {
            changes[layer].DidReorderPrims(path);
        }
    }
    else if (field == SdfFieldKeys->PropertyOrder) {
        changes[layer].DidReorderProperties(path);
    }
    else if (field == SdfChildrenKeys->PropertyChildren) {
        // XXX:OrderNotification: See above.
        if (_IsOrderChangeOnly(oldVal, newVal)) {
            changes[layer].DidReorderProperties(path);
        }
    }
    else if (field == SdfFieldKeys->VariantSetNames ||
             field == SdfChildrenKeys->VariantSetChildren) {
        changes[layer].DidChangePrimVariantSets(path);
    }
    else if (field == SdfFieldKeys->InheritPaths) {
        changes[layer].DidChangePrimInheritPaths(path);
    }
    else if (field == SdfFieldKeys->Specializes) {
        changes[layer].DidChangePrimSpecializes(path);
    }
    else if (field == SdfFieldKeys->References) {
        changes[layer].DidChangePrimReferences(path);
    }
    else if (field == SdfFieldKeys->TimeSamples) {
        changes[layer].DidChangeAttributeTimeSamples(path);
    }
    else if (field == SdfFieldKeys->ConnectionPaths) {
        changes[layer].DidChangeAttributeConnection(path);
    }
    else if (field == SdfFieldKeys->MapperArgValue) {
        changes[layer].DidChangeMapperArgument(path.GetParentPath());
    }
    else if (field == SdfChildrenKeys->MapperChildren) {
        changes[layer].DidChangeAttributeConnection(path);
    }
    else if (field == SdfChildrenKeys->MapperArgChildren) {
        changes[layer].DidChangeMapperArgument(path);
    }
    else if (field == SdfFieldKeys->TargetPaths) {
        changes[layer].DidChangeRelationshipTargets(path);
    }
    else if (field == SdfFieldKeys->Marker) {
        const SdfSpecType specType = layer->GetSpecType(path);

        if (specType == SdfSpecTypeConnection) {
            changes[layer].DidChangeAttributeConnection(path.GetParentPath());
        }
        else if (specType == SdfSpecTypeRelationshipTarget) {
            changes[layer].DidChangeRelationshipTargets(path.GetParentPath());
        }
        else {
            TF_CODING_ERROR("Unknown spec type for marker value change at "
                            "path <%s>", path.GetText());
        }
    }
    else if (field == SdfFieldKeys->SubLayers) {
        std::vector<std::string> addedLayers, removedLayers;
        {        
            const vector<string> oldSubLayers = 
                oldVal.GetWithDefault<vector<string> >();
            const vector<string> newSubLayers = 
                newVal.GetWithDefault<vector<string> >();
        
            const std::set<std::string> oldSet(oldSubLayers.begin(), 
                                               oldSubLayers.end());
            const std::set<std::string> newSet(newSubLayers.begin(), 
                                               newSubLayers.end());
            std::set_difference(oldSet.begin(), oldSet.end(),
                                newSet.begin(), newSet.end(), 
                                std::back_inserter(removedLayers));
            std::set_difference(newSet.begin(), newSet.end(),
                                oldSet.begin(), oldSet.end(), 
                                std::back_inserter(addedLayers));

            // If the old and new sets are the same, the order is all
            // that has changed. The changelist protocol does not have
            // a precise way to describe this, so we represent this as
            // the removal and re-addition of all layers. (We could make
            // the changelist protocol more descriptive for this case, but
            // there isn't any actual speed win to be realized today.)
            if (addedLayers.empty() && removedLayers.empty()) {
                removedLayers.insert( removedLayers.end(),
                                      oldSet.begin(), oldSet.end() );
                addedLayers.insert( addedLayers.end(),
                                    newSet.begin(), newSet.end() );
            }
        }

        TF_FOR_ALL(it, addedLayers) {
            changes[layer]
                .DidChangeSublayerPaths(*it, SdfChangeList::SubLayerAdded);
        }

        TF_FOR_ALL(it, removedLayers) {
            changes[layer]
                .DidChangeSublayerPaths(*it, SdfChangeList::SubLayerRemoved);
        }
    }
    else if (field == SdfFieldKeys->SubLayerOffsets) {
        const SdfLayerOffsetVector oldOffsets = 
            oldVal.GetWithDefault<SdfLayerOffsetVector>();
        const SdfLayerOffsetVector newOffsets = 
            newVal.GetWithDefault<SdfLayerOffsetVector>();

        // Only add changelist entries if the number of sublayer offsets hasn't
        // changed. If the number of offsets has changed, it means sublayers
        // have been added or removed. A changelist entry would have already
        // been registered for that, so we don't need to add another one here.
        if (oldOffsets.size() == newOffsets.size()) {
            const SdfSubLayerProxy subLayers = layer->GetSubLayerPaths();
            if (TF_VERIFY(newOffsets.size() == subLayers.size())) {
                for (size_t i = 0; i < newOffsets.size(); ++i) {
                    if (oldOffsets[i] != newOffsets[i]) {
                        changes[layer].DidChangeSublayerPaths(subLayers[i],
                                               SdfChangeList::SubLayerOffset);
                    }
                }
            }
        }
    }
    else if (field == SdfFieldKeys->TypeName) {
        if (path.IsMapperPath() || path.IsExpressionPath()) {
            // Mapper and expression typename changes are treated as changes on
            // the owning attribute connection.
            changes[layer].DidChangeAttributeConnection(path.GetParentPath());
        }
        else if (path.IsPrimPath()) {
            // Prim typename changes are tricky because typename isn't
            // marked as a required field, but can be set during prim spec 
            // construction. In this case, we don't want to send notification
            // as the spec addition notice should suffice. We can identify
            // this situation by the fact that the c'tor will have created a
            // non-inert prim spec. 
            //
            // If we're *not* in this case, we need to let the world know the
            // typename has changed.
            const SdfChangeList::Entry& entry = changes[layer].GetEntry(path);
            if (!entry.flags.didAddNonInertPrim) {
                changes[layer].DidChangeInfo(path, field, oldVal, newVal);
            }
        }
        else {
            // Otherwise, this is a typename change on an attribute. Since
            // typename is a required field in this case, the only time
            // the old or new value will be empty is during the spec c'tor;
            // during all other times, we need to send notification.
            if (!oldVal.IsEmpty() && !newVal.IsEmpty() &&
                !oldVal.Get<TfToken>().IsEmpty() &&
                !newVal.Get<TfToken>().IsEmpty()) {
                changes[layer].DidChangeInfo(path, field, oldVal, newVal);
            }
        }
    }
    else if (field == SdfFieldKeys->Script) {
        changes[layer].DidChangeAttributeConnection(path.GetParentPath());
    }
    else if (field == SdfFieldKeys->Variability ||
             field == SdfFieldKeys->Custom ||
             field == SdfFieldKeys->Specifier) {
        
        // These are all required fields. We only want to send notification
        // that they are changing when both the old and new value are not
        // empty. Otherwise, the change indicates that the spec is being
        // created or removed, which will be handled through the Add/Remove
        // change notification API.
        if (!oldVal.IsEmpty() && !newVal.IsEmpty()) {
            changes[layer].DidChangeInfo(path, field, oldVal, newVal);
        }
    } 
    else if (field == SdfChildrenKeys->ConnectionChildren       ||
        field == SdfChildrenKeys->ExpressionChildren            ||
        field == SdfChildrenKeys->MapperChildren                ||
        field == SdfChildrenKeys->RelationshipTargetChildren    ||
        field == SdfChildrenKeys->VariantChildren               ||
        field == SdfChildrenKeys->VariantSetChildren) {
        // These children fields are internal. We send notification that the
        // child spec was created/deleted, not that the children field
        // changed.
    }
    else {
        // Handle any other field as a generic metadata key change.
        //
        // This is a bit of a lazy hodge.  There's no good definition of what
        // an "info key" is, but they are clearly a subset of the fields.  It
        // should be safe for now to simply report all field names as info keys.
        // If this is problematic, we'll need to filter them down to the known
        // set.
        changes[layer].DidChangeInfo(path, field, oldVal, newVal);
    }
}

void
Sdf_ChangeManager::DidChangeAttributeTimeSamples(const SdfLayerHandle &layer,
                                                 const SdfPath &attrPath)
{
    _data.local().changes[layer].DidChangeAttributeTimeSamples(attrPath);
}

void
Sdf_ChangeManager::DidMoveSpec(const SdfLayerHandle &layer,
                               const SdfPath & oldPath, const SdfPath & newPath)
{
    if (!layer->_ShouldNotify())
        return;

    SdfLayerChangeListMap &changes = _data.local().changes;

    if (oldPath.GetParentPath() == newPath.GetParentPath()) {
        // Rename
        if (oldPath.IsPrimPath()) {
            changes[layer].DidChangePrimName(oldPath, newPath);
        } else if (oldPath.IsPropertyPath()) {
            changes[layer].DidChangePropertyName(oldPath, newPath);
        }
    } else {
        // Reparent
        if (oldPath.IsPrimPath()) {
            changes[layer].DidRemovePrim(oldPath, /* inert = */ false);
            changes[layer].DidAddPrim(newPath, /* inert = */ false);
        } else if (oldPath.IsPropertyPath()) {
            changes[layer].DidRemoveProperty(oldPath, 
                /* hasOnlyRequiredFields = */ false);
            changes[layer].DidAddProperty(newPath, 
                /* hasOnlyRequiredFields = */ false);
        }
    }
}

void
Sdf_ChangeManager::DidAddSpec(const SdfLayerHandle &layer, const SdfPath &path,
    bool inert)
{
    if (!layer->_ShouldNotify())
        return;

    SdfLayerChangeListMap &changes = _data.local().changes;

    if (path.IsPrimPath() || path.IsPrimVariantSelectionPath()) {
        changes[layer].DidAddPrim(path, /* inert = */ inert);
    } 
    else if (path.IsPropertyPath()) {
        changes[layer].DidAddProperty(path, 
                                       /* hasOnlyRequiredFields = */ inert);
    } 
    else if (path.IsTargetPath()) {
        changes[layer].DidAddTarget(path);
    }
    else if (path.IsMapperPath() || path.IsMapperArgPath()) {
        // This is handled when the field on the parent changes
    } 
    else if (path.IsExpressionPath()) {
        changes[layer].DidChangeAttributeConnection(path.GetParentPath());
    } 
    else {
        TF_CODING_ERROR("Unsupported Spec Type for <" + path.GetString() + ">");
    }
}

void
Sdf_ChangeManager::DidRemoveSpec(const SdfLayerHandle &layer, const SdfPath &path,
    bool inert)
{
    if (!layer->_ShouldNotify())
        return;

    SdfLayerChangeListMap &changes = _data.local().changes;

    if (path.IsPrimPath() || path.IsPrimVariantSelectionPath()) {
        changes[layer].DidRemovePrim(path, /* inert = */ inert);
    } 
    else if (path.IsPropertyPath()) {
        changes[layer].DidRemoveProperty(path, 
                                          /* hasOnlyRequiredFields = */ inert);
    } 
    else if (path.IsTargetPath()) {
        changes[layer].DidRemoveTarget(path);
    }
    else if (path.IsMapperPath() || path.IsMapperArgPath()) {
        // This is handled when the field on the parent changes
    } 
    else if (path.IsExpressionPath()) {
        changes[layer].DidChangeAttributeConnection(path.GetParentPath());
    } 
    else {
        TF_CODING_ERROR("Unsupported Spec Type for <" + path.GetString() + ">");
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
