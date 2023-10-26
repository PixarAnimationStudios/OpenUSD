//
// Copyright 2023 Pixar
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
#include "pxr/usd/usd/namespaceEditor.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/resolver.h"

#include "pxr/usd/pcp/layerStack.h"

#include "pxr/usd/sdf/cleanupEnabler.h"
#include "pxr/usd/sdf/namespaceEdit.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/pathTable.h"

#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/singularTask.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/work/withScopedParallelism.h"

PXR_NAMESPACE_OPEN_SCOPE

static
std::string
_GetErrorString(const std::vector<std::string> &errors) 
{
    return TfStringJoin(errors, "; ");
}

UsdNamespaceEditor::UsdNamespaceEditor(const UsdStageRefPtr &stage) 
    : _stage(stage)
{    
}

bool 
UsdNamespaceEditor::DeletePrimAtPath(
    const SdfPath &path)
{
    return _AddPrimDelete(path);
}

bool 
UsdNamespaceEditor::MovePrimAtPath(
    const SdfPath &path, 
    const SdfPath &newPath)
{
    return _AddPrimMove(path, newPath);
}

bool 
UsdNamespaceEditor::DeletePrim(
    const UsdPrim &prim)
{
    return _AddPrimDelete(prim.GetPrimPath());
}

bool
UsdNamespaceEditor::RenamePrim(
    const UsdPrim &prim, 
    const TfToken &newName)
{
    return _AddPrimMove(
        prim.GetPrimPath(), prim.GetPrimPath().ReplaceName(newName));
}

bool 
UsdNamespaceEditor::ReparentPrim(
    const UsdPrim &prim, 
    const UsdPrim &newParent)
{
    return _AddPrimMove(
        prim.GetPrimPath(), newParent.GetPrimPath().AppendChild(prim.GetName()));
}

bool 
UsdNamespaceEditor::ReparentPrim(
    const UsdPrim &prim, 
    const UsdPrim &newParent,
    const TfToken &newName)
{
    return _AddPrimMove(
        prim.GetPrimPath(), newParent.GetPrimPath().AppendChild(newName));
}

bool 
UsdNamespaceEditor::ApplyEdits()
{
    _ProcessEditsIfNeeded();
    if (!_processedEdit) {
        TF_CODING_ERROR("Failed to process edits");
        return false;
    }
    const bool success = _ApplyProcessedEdit(*_processedEdit);

    // Always clear the processed edits after applying them.
    _ClearProcessedEdits();
    return success;
}

bool 
UsdNamespaceEditor::CanApplyEdits(std::string *whyNot) const
{
    _ProcessEditsIfNeeded();
    if (!_processedEdit) {
        TF_CODING_ERROR("Failed to process edits");
        return false;
    }

    if (!_processedEdit->errors.empty()) {
        if (whyNot) {
            *whyNot = _GetErrorString(_processedEdit->errors);
        }
        return false;
    }

    return true;
}

bool 
UsdNamespaceEditor::_AddPrimDelete(const SdfPath &oldPath) 
{
    // We always clear the processed edits when a new edit is added.
    _ClearProcessedEdits();

    // Prim delete is described as moving from the old path to the empty path.
    _editDescription.oldPath = oldPath;
    _editDescription.newPath = SdfPath();

    // The path must be an absolute path to a prim.
    if (!oldPath.IsPrimPath() || !oldPath.IsAbsolutePath()) {
        TF_CODING_ERROR("Invalid path '%s' provided as the source for "
            "a prim namespace edit; the path must be an absolute prim path.", 
            oldPath.GetText());
        _editDescription.editType = _EditType::Invalid;
        return false;
    }
    
    _editDescription.editType = _EditType::DeletePrim;
    return true;
}

bool 
UsdNamespaceEditor::_AddPrimMove(const SdfPath &oldPath, const SdfPath &newPath) 
{
    // We always clear the processed edits when a new edit is added.
    _ClearProcessedEdits();

    _editDescription.oldPath = oldPath;
    _editDescription.newPath = newPath;

    // Both paths must be an absolute paths to a prim.
    if (!oldPath.IsPrimPath() || !oldPath.IsAbsolutePath()) {
        TF_CODING_ERROR("Invalid path '%s' provided as the source for "
            "a prim namespace edit; the path must be an absolute prim path.", 
            oldPath.GetText());
        _editDescription.editType = _EditType::Invalid;
        return false;
    }

    if (!newPath.IsPrimPath() || !newPath.IsAbsolutePath()) {
        TF_CODING_ERROR("Invalid path '%s' provided as the destination for a "
            "prim namespace edit; the path must be an absolute prim path.", 
            newPath.GetText());
        _editDescription.editType = _EditType::Invalid;
        return false;
    }

    // Determine whether the paths represent a rename or a reparent.
    if (oldPath.GetParentPath() == newPath.GetParentPath()) {
        _editDescription.editType = _EditType::RenamePrim;
    } else {
        _editDescription.editType = _EditType::ReparentPrim;
    }

    return true;
}

void 
UsdNamespaceEditor::_ClearProcessedEdits()
{
    _processedEdit.reset();
} 

void 
UsdNamespaceEditor::_ProcessEditsIfNeeded() const
{
    // We can skip processing the edits if they've already been processed so
    // we don't have to repeat the same work between calls to CanApplyEdits and
    // ApplyEdits.
    if (_processedEdit) {
        return;
    }
    _processedEdit = _ProcessEdit(_editDescription);
}


static 
bool 
_IsValidPrimToEdit(
    const UsdPrim &prim, 
    std::string *whyNot = nullptr) 
{
    // Prim to edit must exist
    if (!prim) {
        if (whyNot) {
            *whyNot = "The prim to edit is not a valid prim";
        }
        return false;
    }
    // Prim to edit must not be a prototype.
    if (prim.IsInPrototype()) {
        if (whyNot) {
            *whyNot = "The prim to edit belongs to a prototype prim";
        }
        return false;
    }
    // Prim to edit must not be a prototype proxy.
    if (prim.IsInstanceProxy()) {
        if (whyNot) {
            *whyNot = "The prim to edit is a prototype proxy descendant of an "
                "instance prim";
        }
        return false;
    }
    return true;
}

static 
bool 
_IsValidNewParentPath(
    const UsdObject &objectToEdit,
    const SdfPath &newParentPath, 
    std::string *whyNot = nullptr) 
{
    const UsdPrim newParentPrim = 
        objectToEdit.GetStage()->GetPrimAtPath(newParentPath);

    // New parent prim must exist
    if (!newParentPrim) {
        if (whyNot) {
            *whyNot = "The new parent prim is not a valid prim";
        }
        return false;
    }
    // New parent prim must not be a prototype.
    if (newParentPrim.IsInPrototype()) {
        if (whyNot) {
            *whyNot = "The new parent prim belongs to a prototype prim";
        }
        return false;
    }
    // New parent prim must not be a prototype proxy.
    if (newParentPrim.IsInstanceProxy()) {
        if (whyNot) {
            *whyNot = "The new parent prim is a prototype proxy descendant "
                      "of an instance prim";
        }
        return false;
    }
    if (objectToEdit.Is<UsdPrim>()) {
        // New parent prim must not be instance prim.
        if (newParentPrim.IsInstance()) {
            if (whyNot) {
                *whyNot = "The new parent prim is an instance prim whose "
                          "children are provided exclusively by its prototype";
            }
            return false;
        }
        // Prims can't be reparented under themselves.
        if (newParentPrim == objectToEdit) {
            if (whyNot) {
                *whyNot = "The new parent prim is the same as the prim to move";
            }
            return false;
        }
        if (newParentPath.HasPrefix(objectToEdit.GetPrimPath())) {
            if (whyNot) {
                *whyNot = "The new parent prim is a descendant of the prim to "
                          "move";
            }
            return false;
        }
    } else {
        if (newParentPrim.IsPseudoRoot()) {
            if (whyNot) {
                *whyNot = "The new parent prim for a property cannot be the "
                          "pseudo-root";
            }
            return false;
        }
    }
    return true;
}

UsdNamespaceEditor::_ProcessedEdit 
UsdNamespaceEditor::_ProcessEdit(
    const _EditDescription &editDesc) const
{
    _ProcessedEdit processedEdit;

    if (editDesc.editType == _EditType::Invalid) {
        processedEdit.errors.push_back("There are no valid edits to perform");
        return processedEdit;
    }

    // Add the edit to the processed SdfBatchNamespaceEdit.
    processedEdit.edits.Add(editDesc.oldPath, editDesc.newPath);

    // Validate wheter the stage has prim at the original path that can be 
    // namespace edited.
    const UsdPrim prim = _stage->GetPrimAtPath(editDesc.oldPath);
    std::string error;
    if (!_IsValidPrimToEdit(prim, &error)) {
        processedEdit.errors.push_back(std::move(error));
        return processedEdit;
    }

    // For move edits we'll have a new path; verify that the stage doesn't 
    // already have an object at that path.
    if (!editDesc.newPath.IsEmpty() && 
            _stage->GetObjectAtPath(editDesc.newPath)) {
        processedEdit.errors.push_back("An object already exists at the new path");
        return processedEdit;
    }

    // For prim reparenting we have additional behaviors and validation to 
    // perform.
    if (editDesc.editType == _EditType::ReparentPrim) {
        // For each layer we edit, we may need to create new overs for the new 
        // parent path and delete inert ancestor overs after moving a prim from
        // its original parent, so add this info to the processed edit.
        processedEdit.createParentSpecIfNeededPath = 
            editDesc.newPath.GetParentPath();
        processedEdit.removeInertAncestorOvers = true;

        // Validate that the stage does have a prim at the new parent path to 
        // reparent to.
        std::string whyNot;
        if (!_IsValidNewParentPath(
                prim, processedEdit.createParentSpecIfNeededPath, &whyNot)) {
            processedEdit.errors.push_back(std::move(whyNot));
            return processedEdit;
        }
    }

    // Gather all layers with contributing specs to the old path that will need
    // to be edited when the edits are applied. This will also determine if the
    // edit requires relocates.
    _GatherLayersToEdit(prim, &processedEdit);

    // Validate whether the necessary spec edits can actually be performed on
    // each layer that needs to be edited.
    for (const auto &layer : processedEdit.layersToEdit) {
        // The layer itself needs to be editable  
        if (!layer->PermissionToEdit()) {
            processedEdit.errors.push_back(TfStringPrintf("The spec @%s@<%s> "
                "cannot be edited because the layer is not editable",
                layer->GetIdentifier().c_str(),
                editDesc.oldPath.GetText()));
        }
        // If we're moving an object to a new path, the layer cannot have a
        // spec already at the new path.
        if (!editDesc.newPath.IsEmpty() && layer->HasSpec(editDesc.newPath)) {
            processedEdit.errors.push_back(TfStringPrintf("The spec @%s@<%s> "
                "cannot be moved to <%s> because a spec already exists at "
                "the new path",
                layer->GetIdentifier().c_str(),
                editDesc.oldPath.GetText(),
                editDesc.newPath.GetText()));
        }
    }

    // Relocates support is not implemented yet so it's currently an error if 
    // the edit requires it..
    if (processedEdit.requiresRelocates) {
        if (editDesc.editType == _EditType::DeletePrim) {
            // "Relocates" means deactivating the prim in the deletion case.
            processedEdit.errors.push_back("The prim to delete requires "
                "deactivation to be deleted because of specs introduced across "
                "an ancestral composition arc; deletion via deactivation is "
                "not supported yet");
        } else {
            processedEdit.errors.push_back("The prim to move requires "
                "relocates to be moved because of specs introduced across an "
                "ancestral composition arc; relocates are not supported yet");
        }
    }

    return processedEdit;
}

/*static*/
void 
UsdNamespaceEditor::_GatherLayersToEdit(
    const UsdPrim &primToEdit,
    _ProcessedEdit *processedEdit)
{
    const PcpPrimIndex &primIndex = primToEdit.GetPrimIndex();

    // XXX: To start, we're only going to perform namespace edit operations 
    // using the root layer stack. This will be updated to support edit targets
    // as a later task.
    const PcpNodeRef nodeForEditTarget = primIndex.GetRootNode();
    
    // Get all the layers in the layer stack where the edits will be performed.
    const SdfLayerRefPtrVector &layers = 
        nodeForEditTarget.GetLayerStack()->GetLayers();

    // Until we support edit targets, verify that the stage's current edit 
    // target maps to the prim's local opinions in the root layer stack.
    const UsdEditTarget &editTarget = primToEdit.GetStage()->GetEditTarget();
    if (!editTarget.GetMapFunction().IsIdentityPathMapping()) {
        processedEdit->errors.push_back("Edit targets that map paths across "
            "composition arcs are not currently supported for namespace "
            "editing");
        return;
    }
    if (std::find(layers.begin(), layers.end(), editTarget.GetLayer()) 
            == layers.end()) {
        processedEdit->errors.push_back("Edit targets with layers outside of "
            "the root layer stack are not currently supported for namespace "
            "editing");
        return;
    }

    // Collect every prim spec that exists for this prim path in the layer 
    // stack's layers.
    for (const SdfLayerRefPtr &layer : layers) {
        if (layer->HasSpec(primToEdit.GetPrimPath())) {
            processedEdit->layersToEdit.push_back(layer);
        }
    }

    // Check to see if there are any contributing specs that would require 
    // relocates. These are specs that would continue to be mapped to the 
    // same path across the edit target's node even after all specs are edited
    // in its layer stack.
    const auto range = nodeForEditTarget.GetChildrenRange();
    for (const auto &child : range) {
        // If a child node is a direct arc, we can skip it and its entire 
        // subtree as all the specs at or below this node are mapped to the 
        // prim path (whatever it may be) through this child node.
        if (!child.IsDueToAncestor()) {
            continue;
        }

        // Since the child node is an ancestral arc, the mapping of specs across
        // the child node is not affected by the path of this prim itself and
        // will continue to map to the original path after the edit. So if there
        // are any specs in the child's subtree, then this edit will require
        // relocates
        PcpNodeRange subtreeRange = primIndex.GetNodeSubtreeRange(child);
        for (const PcpNodeRef &subtreeNode : subtreeRange) {
            // A node has contributing specs if it has specs and is not inert.
            if (subtreeNode.HasSpecs() && !subtreeNode.IsInert()) {
                processedEdit->requiresRelocates = true;
                break;
            }
        }
    }

    // At the point in which this function is called, the prim must exist on the
    // stage. So if we didn't find any specs to edit, then we must've found 
    // specs across a composition arc that requires relocates. Verifying this
    // as a sanity check.
    if (processedEdit->layersToEdit.empty()) {
        TF_VERIFY(processedEdit->requiresRelocates);
    }
}

/*static*/
bool 
UsdNamespaceEditor::_ApplyProcessedEdit(const _ProcessedEdit &processedEdit)
{
    // This is to try to preemptively prevent partial edits when if any of the 
    // necessary specs can't be renamed.
    if (!processedEdit.errors.empty()) {
        TF_CODING_ERROR(TfStringPrintf("Failed to apply edits to the stage "
            "because of the following errors: %s",
            _GetErrorString(processedEdit.errors).c_str()));
        return false;
    }

    // Implementation function as this is optionally called with a cleanup 
    // enabler depending on the edit type.
    auto applyEditsToLayersFn = [&]() {
        for (const auto &layer : processedEdit.layersToEdit) {
            // While we do require that the new parent exists on the composed 
            // stage when doing a reparent operation, that doesn't guarantee 
            // that parent spec exists on every layer in which we have to move
            // the source spec. Thus we need to ensure the parent spec of the 
            // new location exists by adding required overs if necessary. 
            if (!processedEdit.createParentSpecIfNeededPath.IsEmpty() &&
                !SdfJustCreatePrimInLayer(
                    layer, processedEdit.createParentSpecIfNeededPath)) {
                TF_CODING_ERROR("Failed to find or create new parent spec "
                    "at path '%s' on layer '%s' which is necessary to "
                    "apply edits. The edit will be incomplete.",
                    processedEdit.createParentSpecIfNeededPath.GetText(),
                    layer->GetIdentifier().c_str());
                return false;
            }

            // Apply the namespace edits to the layer.
            if (!layer->Apply(processedEdit.edits)) {
                TF_CODING_ERROR("Failed to apply batch edit '%s' on layer '%s' "
                    "which is necessary to apply edits. The edit will be "
                    "incomplete.",
                    TfStringify(processedEdit.edits.GetEdits()).c_str(),
                    layer->GetIdentifier().c_str());
                return false;
            }
        }
        return true;
    };

    SdfChangeBlock changeBlock;
    if (processedEdit.removeInertAncestorOvers) {
        // Moving a spec may leave the ancnestor specs as an inert overs. This
        // could easily be caused by reparenting a prim back to its original
        // parent (essentially an "undo") after a reparent that needed to create
        // new overs. Using a cleanup enabler will (after all specs are moved) 
        // handle deleting any inert "dangling" overs that are ancestors of the
        // moved path so that a reparent plus an "undo" can effectively leave
        // layers in their original state.
        SdfCleanupEnabler cleanupEnabler;
        return applyEditsToLayersFn();
    } else {
        return applyEditsToLayersFn();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

