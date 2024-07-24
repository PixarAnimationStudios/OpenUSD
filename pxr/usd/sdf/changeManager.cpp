//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/changeManager.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/stackTrace.h"

#include <atomic>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(Sdf_ChangeManager);

Sdf_ChangeManager::_Data::_Data()
    : outermostBlock(nullptr)
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
    if (layer->_UpdateLastDirtinessState()) {
        SdfNotice::LayerDirtinessChanged().Send(layer);
    }

    const SdfChangeList::const_iterator layerEntry = 
        changeList.FindEntry(SdfPath::AbsoluteRootPath());
    if (layerEntry != changeList.end()) {
        const SdfChangeList::Entry& entry = layerEntry->second;

        for (const auto& keyAndChange : entry.infoChanged) {
            SdfNotice::LayerInfoDidChange(keyAndChange.first).Send(layer);
        }

        if (entry.flags.didChangeIdentifier) {
            SdfNotice::LayerIdentifierDidChange(
                entry.oldIdentifier, layer->GetIdentifier()).Send(layer);
        }
        if (entry.flags.didReplaceContent) {
            SdfNotice::LayerDidReplaceContent().Send(layer);
        }
        if (entry.flags.didReloadContent) {
            SdfNotice::LayerDidReloadContent().Send(layer);
        }
    }
}

void const *
Sdf_ChangeManager::_OpenChangeBlock(SdfChangeBlock const *block)
{
    _Data &data = _data.local();
    if (!data.outermostBlock) {
        data.outermostBlock = block;
        return static_cast<void const *>(&data);
    }
    return nullptr;
}

void
Sdf_ChangeManager::_CloseChangeBlock(
    SdfChangeBlock const *block, void const *key)
{
    _Data &data = *static_cast<_Data *>(const_cast<void *>(key));
    TF_VERIFY(data.outermostBlock == block,
              "Improperly nested SdfChangeBlocks!");

    // Closing outermost (last) change block.  Process removes while the change
    // block is still open.
    _ProcessRemoveIfInert(&data);

    // Send notices with no change block open.
    data.outermostBlock = nullptr;
    _SendNotices(&data);
}

void
Sdf_ChangeManager::RemoveSpecIfInert(const SdfSpec& spec)
{
    // Add spec.   Process remove if we're not in a change block.
    SdfChangeBlock block;
    _data.local().removeIfInert.push_back(spec);
}

void
Sdf_ChangeManager::_ProcessRemoveIfInert(_Data *data)
{
    // Note, \p data is never allowed to be null here.
    
    if (data->removeIfInert.empty()) {
        return;
    }
    
    // Swap pending removes into a local variable.
    vector<SdfSpec> remove;
    remove.swap(data->removeIfInert);

    // Remove inert stuff.
    TF_FOR_ALL(i, remove) {
        i->GetLayer()->_RemoveIfInert(*i);
    }

    // We don't expect any deferred removes to have been added.
    TF_VERIFY(data->removeIfInert.empty());

    // We should still be in an outermost change block.
    TF_VERIFY(data->outermostBlock);
}

static std::atomic<size_t> &
_InitChangeSerialNumber() {
    static std::atomic<size_t> value;
    value = 1;
    return value;
}

void
Sdf_ChangeManager::_SendNotices(_Data *data)
{
    // Note, \p data is never allowed to be null here.

    // Move aside the list of changes to deliver and clear the TLS so that
    // notice listeners can safely queue up more changes. We also need to filter
    // out any changes from layers that have since been destroyed, as the change
    // manager should only send notifications for existing layers.
    SdfLayerChangeListVec &tlsChanges = data->changes;
    SdfLayerChangeListVec changes = std::move(tlsChanges);
    tlsChanges.clear();

    changes.erase(
        std::remove_if(changes.begin(), changes.end(),
                       [](SdfLayerChangeListVec::value_type const &p) {
                           return !p.first;
                       }),
        changes.end());

    if (changes.empty())
        return;

    for (auto const &lc: changes) {
        // Send layer-specific notices.
        _SendNoticesForChangeList(lc.first, lc.second);
        if (TfDebug::IsEnabled(SDF_CHANGES)) {
            TF_DEBUG(SDF_CHANGES).Msg("Changes to layer %s:\n%s",
                                     lc.first->GetIdentifier().c_str(),
                                     TfStringify(lc.second).c_str());
        }
    }

    // Obtain a serial number for this round of change processing.
    static std::atomic<size_t> &changeSerialNumber = _InitChangeSerialNumber();
    size_t serialNumber = changeSerialNumber.fetch_add(1);

    // Send global notice.
    SdfNotice::LayersDidChange(changes, serialNumber).Send();

    // Send per-layer notices with change round number.  This is so clients
    // don't have to be invoked on every round of change processing if they are
    // only interested in a subset of layers.
    SdfNotice::LayersDidChangeSentPerLayer n(changes, serialNumber);
    for (auto const &lc: changes) {
        n.Send(lc.first);
    }

    // If no new changes have been queued in the meantime then move the changes
    // vector back and clear it.  This is a performance optimization: it lets us
    // reuse the existing capacity in the changes vector, so we can potentially
    // avoid reallocation on the next round of changes.
    if (tlsChanges.empty()) {
        tlsChanges = std::move(changes);
        tlsChanges.clear();
    }
}

void
Sdf_ChangeManager::DidReplaceLayerContent(const SdfLayerHandle &layer)
{
    if (!layer->_ShouldNotify())
        return;
    _GetListFor(_data.local().changes, layer).DidReplaceLayerContent();
}

void
Sdf_ChangeManager::DidReloadLayerContent(const SdfLayerHandle &layer)
{
    if (!layer->_ShouldNotify())
        return;
    _GetListFor(_data.local().changes, layer).DidReloadLayerContent();
}

void 
Sdf_ChangeManager::DidChangeLayerIdentifier(const SdfLayerHandle &layer,
                                          const std::string &oldIdentifier)
{
    if (!layer->_ShouldNotify())
        return;
    _GetListFor(_data.local().changes, layer)
        .DidChangeLayerIdentifier(oldIdentifier);
}

void 
Sdf_ChangeManager::DidChangeLayerResolvedPath(const SdfLayerHandle &layer)
{
    if (!layer->_ShouldNotify())
        return;
    _GetListFor(_data.local().changes, layer).DidChangeLayerResolvedPath();
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
Sdf_ChangeManager::DidChangeField(
    const SdfLayerHandle &layer,
    const SdfPath & path, const TfToken &field,
    VtValue && oldVal, const VtValue & newVal )
{
    if (!layer->_ShouldNotify())
        return;

    auto FieldKeys = SdfFieldKeys.Get();
    auto ChildrenKeys = SdfChildrenKeys.Get();
    
    SdfLayerChangeListVec &changes = _data.local().changes;

    // Note:  We intend to change the SdfChangeList protocol to provide a
    // sequence of (layer, path, field, oldValue, newValue) tuples.
    // For now, this function adapts field-based changes into the
    // existing protocol.

    bool sendInfoChange = false;

    if (field == FieldKeys->Default) {
        // Special case default first, since it's a commonly set field.
        sendInfoChange = true;
    }
    else if (field == FieldKeys->PrimOrder) {
        _GetListFor(changes, layer).DidReorderPrims(path);
        sendInfoChange = true;
    }
    else if (field == ChildrenKeys->PrimChildren) {
        // XXX:OrderNotification:
        // Sdf's change protocol does not have a "children changed"
        // message; instead it relies on a combination of "order changed"
        // and "child added/removed" messages.  To avoid sending a
        // potentially misleading "order changed" message when in fact
        // children have been added and removed, we compare the old
        // and new children lists and only send an "order changed"
        // message if this is a pure order change.
        if (_IsOrderChangeOnly(oldVal, newVal)) {
            _GetListFor(changes, layer).DidReorderPrims(path);
        }
    }
    else if (field == FieldKeys->PropertyOrder) {
        _GetListFor(changes, layer).DidReorderProperties(path);
    }
    else if (field == ChildrenKeys->PropertyChildren) {
        // XXX:OrderNotification: See above.
        if (_IsOrderChangeOnly(oldVal, newVal)) {
            _GetListFor(changes, layer).DidReorderProperties(path);
        }
    }
    else if (field == FieldKeys->VariantSetNames ||
             field == ChildrenKeys->VariantSetChildren) {
        _GetListFor(changes, layer).DidChangePrimVariantSets(path);
        sendInfoChange = true;
    }
    else if (field == FieldKeys->InheritPaths) {
        _GetListFor(changes, layer).DidChangePrimInheritPaths(path);
        sendInfoChange = true;
    }
    else if (field == FieldKeys->Specializes) {
        _GetListFor(changes, layer).DidChangePrimSpecializes(path);
        sendInfoChange = true;
    }
    else if (field == FieldKeys->References) {
        _GetListFor(changes, layer).DidChangePrimReferences(path);
        sendInfoChange = true;
    }
    else if (field == FieldKeys->TimeSamples) {
        _GetListFor(changes, layer).DidChangeAttributeTimeSamples(path);
        sendInfoChange = true;
    }
    else if (field == FieldKeys->ConnectionPaths) {
        _GetListFor(changes, layer).DidChangeAttributeConnection(path);
        sendInfoChange = true;
    }
    else if (field == FieldKeys->TargetPaths) {
        _GetListFor(changes, layer).DidChangeRelationshipTargets(path);
        sendInfoChange = true;
    }
    else if (field == FieldKeys->SubLayers) {
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
            _GetListFor(changes, layer)
                .DidChangeSublayerPaths(*it, SdfChangeList::SubLayerAdded);
        }

        TF_FOR_ALL(it, removedLayers) {
            _GetListFor(changes, layer)
                .DidChangeSublayerPaths(*it, SdfChangeList::SubLayerRemoved);
        }

        sendInfoChange = true;
    }
    else if (field == FieldKeys->SubLayerOffsets) {
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
                        _GetListFor(changes, layer)
                            .DidChangeSublayerPaths(
                                subLayers[i], SdfChangeList::SubLayerOffset);
                    }
                }
            }
        }

        sendInfoChange = true;
    }
    else if (field == FieldKeys->TypeName) {
        if (path.IsMapperPath() || path.IsExpressionPath()) {
            // Mapper and expression typename changes are treated as changes on
            // the owning attribute connection.
            _GetListFor(changes, layer)
                .DidChangeAttributeConnection(path.GetParentPath());
        }
        else {
            sendInfoChange = true;
        }
    }
    else if (field == FieldKeys->TimeCodesPerSecond &&
            TF_VERIFY(path == SdfPath::AbsoluteRootPath())) {
        // Changing TCPS.  If the old or new value is empty, the effective old
        // or new value is the value of FPS, if there is one.  See
        // SdfLayer::GetTimeCodesPerSecond.
        VtValue oldTcps = (
            !oldVal.IsEmpty() ? oldVal :
            layer->GetField(path, FieldKeys->FramesPerSecond));
        const VtValue newTcps = (
            !newVal.IsEmpty() ? newVal :
            layer->GetField(path, FieldKeys->FramesPerSecond));

        _GetListFor(changes, layer).DidChangeInfo(
            path, FieldKeys->TimeCodesPerSecond, std::move(oldTcps), newTcps);
    }
    else if (field == FieldKeys->FramesPerSecond &&
            TF_VERIFY(path == SdfPath::AbsoluteRootPath())) {
        // Announce the change to FPS itself.
        SdfChangeList &list = _GetListFor(changes, layer);
        list.DidChangeInfo(
            path, FieldKeys->FramesPerSecond, VtValue(oldVal), newVal);

        // If the layer doesn't have a value for TCPS, announce a change to TCPS
        // also, since FPS serves as a dynamic fallback for TCPS.  See
        // SdfLayer::GetTimeCodesPerSecond.
        if (!layer->HasField(path, FieldKeys->TimeCodesPerSecond)) {
            list.DidChangeInfo(
                path, FieldKeys->TimeCodesPerSecond, std::move(oldVal), newVal);
        }
    }
    else if (field == ChildrenKeys->ConnectionChildren       ||
        field == ChildrenKeys->ExpressionChildren            ||
        field == ChildrenKeys->RelationshipTargetChildren    ||
        field == ChildrenKeys->VariantChildren               ||
        field == ChildrenKeys->VariantSetChildren) {
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
        sendInfoChange = true;
    }

    if (sendInfoChange) {
        _GetListFor(changes, layer)
            .DidChangeInfo(path, field, std::move(oldVal), newVal);
    }
}

void
Sdf_ChangeManager::DidChangeAttributeTimeSamples(const SdfLayerHandle &layer,
                                                 const SdfPath &attrPath)
{
    _GetListFor(_data.local().changes, layer)
        .DidChangeAttributeTimeSamples(attrPath);
}

void
Sdf_ChangeManager::DidMoveSpec(const SdfLayerHandle &layer,
                               const SdfPath & oldPath, const SdfPath & newPath)
{
    if (!layer->_ShouldNotify())
        return;

    SdfLayerChangeListVec &changes = _data.local().changes;

    if (oldPath.GetParentPath() == newPath.GetParentPath()) {
        // Rename
        if (oldPath.IsPrimPath()) {
            _GetListFor(changes, layer).DidChangePrimName(oldPath, newPath);
        } else if (oldPath.IsPropertyPath()) {
            _GetListFor(changes, layer).DidChangePropertyName(oldPath, newPath);
        } else if (oldPath.IsTargetPath()) {
            const SdfPath& parentPropPath = oldPath.GetParentPath();
            const SdfSpecType specType = layer->GetSpecType(parentPropPath);
            if (specType == SdfSpecTypeAttribute) {
                _GetListFor(changes, layer)
                    .DidChangeAttributeConnection(parentPropPath);
            }
            else if (specType == SdfSpecTypeRelationship) {
                _GetListFor(changes, layer)
                    .DidChangeRelationshipTargets(parentPropPath);
            }
        }
    } else {
        // Reparent
        if (oldPath.IsPrimPath()) {
            _GetListFor(changes, layer).DidMovePrim(oldPath, newPath);
        } else if (oldPath.IsPropertyPath()) {
            _GetListFor(changes, layer).DidRemoveProperty(oldPath, 
                /* hasOnlyRequiredFields = */ false);
            _GetListFor(changes, layer).DidAddProperty(newPath, 
                /* hasOnlyRequiredFields = */ false);
        } else if (oldPath.IsTargetPath()) {
            const SdfPath& oldParentPropPath = oldPath.GetParentPath();
            const SdfPath& newParentPropPath = newPath.GetParentPath();
            const SdfSpecType specType = layer->GetSpecType(oldParentPropPath);
            if (specType == SdfSpecTypeAttribute) {
                _GetListFor(changes, layer)
                    .DidChangeAttributeConnection(oldParentPropPath);
                _GetListFor(changes, layer)
                    .DidChangeAttributeConnection(newParentPropPath);
            }
            else if (specType == SdfSpecTypeRelationship) {
                _GetListFor(changes, layer)
                    .DidChangeRelationshipTargets(oldParentPropPath);
                _GetListFor(changes, layer)
                    .DidChangeRelationshipTargets(newParentPropPath);
            }
        }
    }
}

void
Sdf_ChangeManager::DidAddSpec(const SdfLayerHandle &layer, const SdfPath &path,
    bool inert)
{
    if (!layer->_ShouldNotify())
        return;

    SdfLayerChangeListVec &changes = _data.local().changes;

    if (path.IsPrimPath() || path.IsPrimVariantSelectionPath()) {
        _GetListFor(changes, layer).DidAddPrim(path, /* inert = */ inert);
    } 
    else if (path.IsPropertyPath()) {
        _GetListFor(changes, layer).DidAddProperty(path, 
                                       /* hasOnlyRequiredFields = */ inert);
    } 
    else if (path.IsTargetPath()) {
        _GetListFor(changes, layer).DidAddTarget(path);
    }
    else if (path.IsMapperPath() || path.IsMapperArgPath()) {
        // This is handled when the field on the parent changes
    } 
    else if (path.IsExpressionPath()) {
        _GetListFor(changes, layer)
            .DidChangeAttributeConnection(path.GetParentPath());
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

    SdfLayerChangeListVec &changes = _data.local().changes;

    if (path.IsPrimPath() || path.IsPrimVariantSelectionPath()) {
        _GetListFor(changes, layer).DidRemovePrim(path, /* inert = */ inert);
    } 
    else if (path.IsPropertyPath()) {
        _GetListFor(changes, layer).DidRemoveProperty(path, 
                                          /* hasOnlyRequiredFields = */ inert);
    } 
    else if (path.IsTargetPath()) {
        _GetListFor(changes, layer).DidRemoveTarget(path);
    }
    else if (path.IsMapperPath() || path.IsMapperArgPath()) {
        // This is handled when the field on the parent changes
    } 
    else if (path.IsExpressionPath()) {
        _GetListFor(changes, layer)
            .DidChangeAttributeConnection(path.GetParentPath());
    } 
    else {
        TF_CODING_ERROR("Unsupported Spec Type for <" + path.GetString() + ">");
    }
}

SdfChangeList &
Sdf_ChangeManager::_GetListFor(SdfLayerChangeListVec &theList,
                               SdfLayerHandle const &layer)
{
    auto iter = std::find_if(
        theList.begin(), theList.end(),
        [&layer](SdfLayerChangeListVec::value_type const &p) {
            return p.first == layer;
        });
    if (iter != theList.end()) {
        return iter->second;
    }
    theList.emplace_back(std::piecewise_construct,
                         std::tie(layer), std::tuple<>());
    return theList.back().second;
}

PXR_NAMESPACE_CLOSE_SCOPE
