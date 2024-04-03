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

#include "pxr/base/tf/ostreamMethods.h"

#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/singularTask.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/work/withScopedParallelism.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Stores info about a property spec that has authored attribute connections or 
// relationship targets.
struct _PropertySpecWithAuthoredTargetsInfo {
    // Layer and path of the site of the spec.
    SdfLayerHandle layer;
    SdfPath path;

    // The name of the field in property spec that holds the target list op.
    // This will be ConnectionPaths for attributes and TargetPaths for 
    // relationships.
    TfToken fieldName;

    // The node in the composed prim index that introduces this spec. Necessary 
    // for mapping the target paths to the stage namespace paths as well as 
    // determining if these target paths can be edited with or without relocates.
    PcpNodeRef originatingNode;

    // Gets the targets list op value from this spec.
    SdfPathListOp GetTargetListOp() const
    {
        SdfPathListOp listOp;
        
        if (!layer->HasField(path, fieldName, &listOp)) {
            TF_CODING_ERROR("Spec at site @%s@<%s> is expected to have a "
                "path list op for field %s",
                layer->GetIdentifier().c_str(),
                path.GetText(),
                fieldName.GetText());
        }
        return listOp;
    }
};

using _PropertySpecWithAuthoredTargetsVector = 
    std::vector<_PropertySpecWithAuthoredTargetsInfo>;

// Structure for storing the dependencies between stage object paths and the
// property specs that cause the object to be targeted for attribute connections
// or relationship targets.
struct _TargetingPropertyDependencies {
    // The map of each stage property path to the property specs (ordered 
    // strongest to weakest) that provide opinions for the property's targets 
    // (relationship) or connections (attribute)
    std::unordered_map<SdfPath, _PropertySpecWithAuthoredTargetsVector, TfHash> 
        composedPropertyToSpecsWithAuthoredTargetsMap;

    // A table of stage object path to the list of property paths that have
    // specs with list ops that contain a path that maps to this object path.
    SdfPathTable<SdfPathVector> targetedPathToTargetingPropertiesPathTable;
};

// Helper for collecting all targeting property dependencies on a stage.
class _TargetingPropertyDependencyCollector
{
public:
    // Gets all the targeting property dependencies for all object paths on 
    // the given stage
    static _TargetingPropertyDependencies GetDependencies(
        const UsdStageRefPtr &stage) 
    {
        _TargetingPropertyDependencyCollector impl;
        impl._Run(stage);
        return std::move(impl._result);
    }

private:
    WorkDispatcher _dispatcher;
    WorkSingularTask _consumerTask;

    struct _WorkQueueEntry {
        SdfPath composedPropertyPath;
        _PropertySpecWithAuthoredTargetsVector propSpecsWithAuthoredTargets;
        SdfPathSet targetedPaths;
    };

    tbb::concurrent_queue<_WorkQueueEntry> _workQueue;

    _TargetingPropertyDependencies _result;

    explicit _TargetingPropertyDependencyCollector()
        : _consumerTask(_dispatcher, [this]() { _ConsumerTask(); }) {}

    void _Run(const UsdStageRefPtr &stage) {
        WorkWithScopedParallelism([this, &stage]() {
            const auto range = stage->GetPseudoRoot().GetDescendants();
            WorkParallelForEach(range.begin(), range.end(),
                [this](UsdPrim const &prim) { _VisitPrim(prim);});
            _dispatcher.Wait();
        });
    }

    void _VisitPrim(UsdPrim const &prim) {

        std::unordered_map<SdfPath, _WorkQueueEntry, TfHash> 
            workEntriesPerProperty;

        // Use a resolver to get all of the prim's property opinions that 
        // provide attribute connections or relationship targets in strength
        // order.
        for(Usd_Resolver res(&(prim.GetPrimIndex())); 
                res.IsValid(); res.NextLayer()) {

            const SdfLayerRefPtr &layer = res.GetLayer();
            const SdfPath &primSpecPath = res.GetLocalPath();

            // Get the names of properties that are locally authored on this
            // prim spec. 
            TfTokenVector primSpecPropertyNames;
            if (!layer->HasField(
                    primSpecPath,
                    SdfChildrenKeys->PropertyChildren,
                    &primSpecPropertyNames)) {
                continue;
            }

            // Now we look through property specs looking for ones with 
            // connections or relationship targets
            for (const TfToken &propName : primSpecPropertyNames) {

                // Get the property spec path in this layer.
                SdfPath localPropPath = primSpecPath.AppendProperty(propName);

                // Get the target path field name for the property based on 
                // whether it's an attribute or relationship.
                const SdfSpecType specType = layer->GetSpecType(localPropPath);
                TfToken targetPathListOpField;
                if (specType == SdfSpecTypeAttribute) {
                    targetPathListOpField = SdfFieldKeys->ConnectionPaths;
                } else if (specType == SdfSpecTypeRelationship) {
                    targetPathListOpField = SdfFieldKeys->TargetPaths;
                } else {
                    TF_CODING_ERROR("Spec type for property child of at site "
                        "@%s@<%s> is not an attribute or relationship",
                        layer->GetIdentifier().c_str(),
                        localPropPath.GetText());
                    continue;
                }

                // Get the target path list op for the property spec skipping
                // specs that don't have opinions on this field.
                SdfPathListOp targetPathsListOp;
                if (!layer->HasField<SdfPathListOp>(
                        localPropPath, targetPathListOpField, &targetPathsListOp)) {
                    continue;
                }

                // Add or get the work entry for the composed property path so
                // we can add this spec's info to it.
                _WorkQueueEntry &workEntry = workEntriesPerProperty.emplace(
                    prim.GetPrimPath().AppendProperty(propName), 
                    _WorkQueueEntry()).first->second;

                const PcpNodeRef node = res.GetNode();

                // Helper for collecting the target paths from a listOp item 
                // vector and adding them to the work entry's target paths list,
                // mapping the path to the root node (stage namespace) if
                // necessary.
                auto collectMappedPathsFn = 
                    [&](const SdfPathListOp::ItemVector &items) {
                        if (node.IsRootNode() || 
                                node.GetMapToRoot().IsIdentity()) {
                            workEntry.targetedPaths.insert(
                                items.begin(), items.end());
                        } else {
                            for (const auto &item : items) {
                                SdfPath mappedItem = 
                                    node.GetMapToRoot().MapSourceToTarget(item);
                                if (!mappedItem.IsEmpty()) {
                                    workEntry.targetedPaths.insert(
                                        std::move(mappedItem));
                                }
                            }
                        }
                    };

                // Collect all the target paths found anywhere in the listOp
                // as all these paths count as a dependency that may need to 
                // fixed after a namespace edit. 
                if (targetPathsListOp.IsExplicit()) {
                    collectMappedPathsFn(targetPathsListOp.GetExplicitItems());
                } else {
                    collectMappedPathsFn(targetPathsListOp.GetAddedItems());
                    collectMappedPathsFn(targetPathsListOp.GetAppendedItems());
                    collectMappedPathsFn(targetPathsListOp.GetDeletedItems());
                    collectMappedPathsFn(targetPathsListOp.GetOrderedItems());
                    collectMappedPathsFn(targetPathsListOp.GetPrependedItems());
                }

                // Add the prop spec info to the contributing prop specs for 
                // this composed entry.
                workEntry.propSpecsWithAuthoredTargets.push_back({
                    layer, 
                    std::move(localPropPath), 
                    std::move(targetPathListOpField),
                    node
                });
            }
        }

        // With all the target dependency work done for every property of this
        // prim, we can queue each property up to be added to the result.
        if (!workEntriesPerProperty.empty()) {
            for (auto &[propPath, workEntry] : workEntriesPerProperty) {
                // Copy the composed property path into the entry before moving
                // it to the queue.
                workEntry.composedPropertyPath = propPath;
                _workQueue.push(std::move(workEntry));
            }
            _consumerTask.Wake();
        }
    };

    void _ConsumerTask() {
        _WorkQueueEntry queueEntry;
        while (_workQueue.try_pop(queueEntry)) {
            // Store the prop specs (with targets) for the composed property in
            // result.
            _result.composedPropertyToSpecsWithAuthoredTargetsMap.emplace(
                queueEntry.composedPropertyPath, 
                std::move(queueEntry.propSpecsWithAuthoredTargets));

            // Add the mapping of each targeted path to the composed property
            // which we now know targets it.
            for (const auto &targetedPath : queueEntry.targetedPaths) {
                _result.targetedPathToTargetingPropertiesPathTable[targetedPath]
                    .push_back(queueEntry.composedPropertyPath);
            }
        }
    }
};

} // end anonymous namespace

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
UsdNamespaceEditor::DeletePropertyAtPath(
    const SdfPath &path)
{
    return _AddPropertyDelete(path);
}

bool 
UsdNamespaceEditor::MovePropertyAtPath(
    const SdfPath &path, 
    const SdfPath &newPath)
{
    return _AddPropertyMove(path, newPath);
}

bool 
UsdNamespaceEditor::DeleteProperty(
    const UsdProperty &property)
{
    return _AddPropertyDelete(property.GetPath());
}

bool
UsdNamespaceEditor::RenameProperty(
    const UsdProperty &property, 
    const TfToken &newName)
{
    return _AddPropertyMove(
        property.GetPath(), property.GetPath().ReplaceName(newName));
}

bool 
UsdNamespaceEditor::ReparentProperty(
    const UsdProperty &property, 
    const UsdPrim &newParent)
{
    return _AddPropertyMove(
        property.GetPath(), newParent.GetPrimPath().AppendProperty(property.GetName()));
}

bool 
UsdNamespaceEditor::ReparentProperty(
    const UsdProperty &property, 
    const UsdPrim &newParent,
    const TfToken &newName)
{
    return _AddPropertyMove(
        property.GetPath(), newParent.GetPrimPath().AppendProperty(newName));
}

bool 
UsdNamespaceEditor::ApplyEdits()
{
    _ProcessEditsIfNeeded();
    if (!_processedEdit) {
        TF_CODING_ERROR("Failed to process edits");
        return false;
    }
    const bool success = _processedEdit->Apply();

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

    return _processedEdit->CanApply(whyNot);
}

static bool 
_IsValidPrimEditPath(const SdfPath &path) 
{
    return path.IsPrimPath() && path.IsAbsolutePath() && 
        !path.ContainsPrimVariantSelection();
}

static bool 
_IsValidPropertyEditPath(const SdfPath &path) 
{
    return path.IsPrimPropertyPath() && path.IsAbsolutePath() && 
        !path.ContainsPrimVariantSelection();
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
    if (!_IsValidPrimEditPath(oldPath)) {
        TF_CODING_ERROR("Invalid path '%s' provided as the source for "
            "a prim namespace edit.", oldPath.GetText());
        _editDescription.editType = _EditType::Invalid;
        return false;
    }
    
    _editDescription.editType = _EditType::Delete;
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
    if (!_IsValidPrimEditPath(oldPath)) {
        TF_CODING_ERROR("Invalid path '%s' provided as the source for "
            "a prim namespace edit.", oldPath.GetText());
        _editDescription.editType = _EditType::Invalid;
        return false;
    }

    if (!_IsValidPrimEditPath(newPath)) {
        TF_CODING_ERROR("Invalid path '%s' provided as the destination for a "
            "prim namespace edit.", newPath.GetText());
        _editDescription.editType = _EditType::Invalid;
        return false;
    }

    // Determine whether the paths represent a rename or a reparent.
    if (oldPath.GetParentPath() == newPath.GetParentPath()) {
        _editDescription.editType = _EditType::Rename;
    } else {
        _editDescription.editType = _EditType::Reparent;
    }

    return true;
}

bool 
UsdNamespaceEditor::_AddPropertyDelete(const SdfPath &oldPath) 
{
    // We always clear the processed edits when a new edit is added.
    _ClearProcessedEdits();

    // Property delete is described as moving from the old path to the empty path.
    _editDescription.oldPath = oldPath;
    _editDescription.newPath = SdfPath();

    // The path must be an absolute path to a property.
    if (!_IsValidPropertyEditPath(oldPath)) {
        TF_CODING_ERROR("Invalid path '%s' provided as the source for "
            "a property namespace edit.", oldPath.GetText());
        _editDescription.editType = _EditType::Invalid;
        return false;
    }
    
    _editDescription.editType = _EditType::Delete;
    return true;
}

bool 
UsdNamespaceEditor::_AddPropertyMove(
    const SdfPath &oldPath, const SdfPath &newPath) 
{
    // We always clear the processed edits when a new edit is added.
    _ClearProcessedEdits();

    _editDescription.oldPath = oldPath;
    _editDescription.newPath = newPath;

    // Both paths must be an absolute paths to a property.
    if (!_IsValidPropertyEditPath(oldPath)) {
        TF_CODING_ERROR("Invalid path '%s' provided as the source for "
            "a property namespace edit.", oldPath.GetText());
        _editDescription.editType = _EditType::Invalid;
        return false;
    }

    if (!_IsValidPropertyEditPath(newPath)) {
        TF_CODING_ERROR("Invalid path '%s' provided as the destination for a "
            "property namespace edit.", newPath.GetText());
        _editDescription.editType = _EditType::Invalid;
        return false;
    }

    // Determine whether the paths represent a rename or a reparent.
    if (oldPath.GetPrimPath() == newPath.GetPrimPath()) {
        _editDescription.editType = _EditType::Rename;
    } else {
        _editDescription.editType = _EditType::Reparent;
    }

    return true;
}

void 
UsdNamespaceEditor::_ClearProcessedEdits()
{
    _processedEdit.reset();
} 

class UsdNamespaceEditor::_EditProcessor {

public:
    // Creates a processed edit from an edit description.
    static UsdNamespaceEditor::_ProcessedEdit ProcessEdit(
        const UsdStageRefPtr &stage,
        const UsdNamespaceEditor::_EditDescription &editDesc);

private:
    static void _ProcessPrimEditRequiresRelocates(
        const _EditDescription &editDesc,
        const PcpPrimIndex &primIndex,
        const PcpNodeRef &nodeForEditTarget,
        _ProcessedEdit *processedEdit);

    static void _ProcessPropEditRequiresRelocates(
        const _EditDescription &editDesc,
        const PcpPrimIndex &primIndex,
        const PcpNodeRef &nodeForEditTarget,
        _ProcessedEdit *processedEdit);

    static void _GatherLayersToEdit(
        const _EditDescription &editDesc,
        const UsdEditTarget &editTarget,
        const PcpPrimIndex &primIndex,
        _ProcessedEdit *processedEdit);

    static void _GatherTargetListOpEdits(
        const UsdStageRefPtr &stage,
        const _EditDescription &editDesc,
        _ProcessedEdit *processedEdit);
};

void 
UsdNamespaceEditor::_ProcessEditsIfNeeded() const
{
    // We can skip processing the edits if they've already been processed so
    // we don't have to repeat the same work between calls to CanApplyEdits and
    // ApplyEdits.
    if (_processedEdit) {
        return;
    }
    _processedEdit = UsdNamespaceEditor::_EditProcessor::ProcessEdit(
        _stage, _editDescription);
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
_IsValidPropertyToEdit(
    const UsdPrim &prim,
    const TfToken &propertyName, 
    std::string *whyNot = nullptr) 
{
    // Property to edit must exist
    if (!prim.HasProperty(propertyName)) {
        if (whyNot) {
            *whyNot = "The property to edit is not a valid property";
        }
        return false;
    }
    // Property to edit must not belong to a prototype.
    if (prim.IsInPrototype()) {
        if (whyNot) {
            *whyNot = "The property to edit belongs to a prototype prim";
        }
        return false;
    }
    // Property to edit must not belong to a prototype proxy.
    if (prim.IsInstanceProxy()) {
        if (whyNot) {
            *whyNot = "The property to edit belongs to an instance prototype "
                "proxy";
        }
        return false;
    }
    // Property to edit must not be a built-in schema property
    if (prim.GetPrimDefinition().GetPropertyDefinition(propertyName)) {
        if (whyNot) {
            *whyNot = "The property to edit is a built-in property of its prim";
        }
        return false;
    }
    return true;
}

static 
bool 
_IsValidNewParentPath(
    const UsdStageRefPtr &stage,
    const SdfPath &pathToEdit,
    const SdfPath &newParentPath,
    std::string *whyNot = nullptr) 
{
    const UsdPrim newParentPrim = stage->GetPrimAtPath(newParentPath);

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

    if (pathToEdit.IsPrimPropertyPath()) {
        // Properties can't be parented under the pseudo-root.
        if (newParentPrim.IsPseudoRoot()) {
            if (whyNot) {
                *whyNot = "The new parent prim for a property cannot be the "
                            "pseudo-root";
            }
            return false;
        }
    } else {
        // Prims cannot not be parented under an instance prim.
        if (newParentPrim.IsInstance()) {
            if (whyNot) {
                *whyNot = "The new parent prim is an instance prim whose "
                            "children are provided exclusively by its prototype";
            }
            return false;
        }
        // Prims can't be reparented under themselves.
        if (newParentPath == pathToEdit) {
            if (whyNot) {
                *whyNot = "The new parent prim is the same as the prim to move";
            }
            return false;
        }
        if (newParentPath.HasPrefix(pathToEdit)) {
            if (whyNot) {
                *whyNot = "The new parent prim is a descendant of the prim to "
                            "move";
            }
            return false;
        }
    }

    return true;
}

UsdNamespaceEditor::_ProcessedEdit 
UsdNamespaceEditor::_EditProcessor::ProcessEdit(
    const UsdStageRefPtr &stage,
    const _EditDescription &editDesc)
{
    _ProcessedEdit processedEdit;

    if (editDesc.editType == _EditType::Invalid) {
        processedEdit.errors.push_back("There are no valid edits to perform");
        return processedEdit;
    }

    // Add the edit to the processed SdfBatchNamespaceEdit. We use the index of
    // "Same" specifically so renames don't move the object (it has no effect
    // for any edits other than rename)
    processedEdit.edits.Add(
        editDesc.oldPath, editDesc.newPath, SdfNamespaceEdit::Same);

    // Validate whether the stage has the prim or property at the original path
    // that can be namespace edited.
    const UsdPrim prim = stage->GetPrimAtPath(editDesc.oldPath.GetPrimPath());
    std::string error;
    if (editDesc.IsPropertyEdit()) {
        if (!_IsValidPropertyToEdit(prim, editDesc.oldPath.GetNameToken(), &error)) {
            processedEdit.errors.push_back(std::move(error));
            return processedEdit;
        }
    } else if (!_IsValidPrimToEdit(prim, &error)) {
        processedEdit.errors.push_back(std::move(error));
        return processedEdit;
    }

    // For move edits we'll have a new path; verify that the stage doesn't 
    // already have an object at that path.
    if (!editDesc.newPath.IsEmpty() && 
            stage->GetObjectAtPath(editDesc.newPath)) {
        processedEdit.errors.push_back("An object already exists at the new path");
        return processedEdit;
    }

    // For reparenting we have additional behaviors and validation to perform.
    if (editDesc.editType == _EditType::Reparent) {
        // For each layer we edit, we may need to create new overs for the new 
        // parent path and delete inert ancestor overs after moving a prim or 
        // property from its original parent, so add this info to the processed 
        // edit.
        processedEdit.createParentSpecIfNeededPath = 
            editDesc.newPath.GetParentPath();
        processedEdit.removeInertAncestorOvers = true;

        // Validate that the stage does have a prim at the new parent path to 
        // reparent to.
        std::string whyNot;
        if (!_IsValidNewParentPath(stage, editDesc.oldPath, 
                processedEdit.createParentSpecIfNeededPath, &whyNot)) {
            processedEdit.errors.push_back(std::move(whyNot));
            return processedEdit;
        }
    }

    // Gather all layers with contributing specs to the old path that will need
    // to be edited when the edits are applied. This will also determine if the
    // edit requires relocates.
    _GatherLayersToEdit(
        editDesc, 
        stage->GetEditTarget(), 
        prim.GetPrimIndex(), 
        &processedEdit);

    // At the point in which this function is called, the prim must exist on the
    // stage. So if we didn't find any specs to edit, then we must've found 
    // specs across a composition arc that requires relocates. Verifying this
    // as a sanity check.
    if (processedEdit.layersToEdit.empty()) {
        TF_VERIFY(processedEdit.requiresRelocates);
        return processedEdit;
    }

    // Gather all the edits that need to be made to target path listOps in 
    // property specs in order to "fix up" properties that have connections or 
    // relationship targets targeting the namespace edited object.
    _GatherTargetListOpEdits(stage, editDesc, &processedEdit);

    return processedEdit;
}

/*static*/
void 
UsdNamespaceEditor::_EditProcessor::_ProcessPrimEditRequiresRelocates(
    const _EditDescription &editDesc,
    const PcpPrimIndex &primIndex,
    const PcpNodeRef &nodeForEditTarget,
    _ProcessedEdit *processedEdit)
{
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

                // Relocates support is not implemented yet so it's currently an
                // error if the edit requires it..
                if (editDesc.editType == _EditType::Delete) {
                    // "Relocates" means deactivating the prim in the deletion 
                    // case.
                    processedEdit->errors.push_back("The prim to delete must "
                        "be deactivated rather than deleted since it composes "
                        "opinions introduced by ancestral composition arcs; "
                        "deletion via deactivation is not yet supported");
                } else {
                    processedEdit->errors.push_back("The prim to move requires "
                        "authoring relocates since it composes opinions "
                        "introduced by ancestral composition arcs; authoring "
                        "relocates is not yet supported");
                }
    
                // Once we've determined the edits require relocates, we're done.
                return;
            }
        }
    }
}

/*static*/
void 
UsdNamespaceEditor::_EditProcessor::_ProcessPropEditRequiresRelocates(
    const _EditDescription &editDesc,
    const PcpPrimIndex &primIndex,
    const PcpNodeRef &nodeForEditTarget,
    _ProcessedEdit *processedEdit)
{
    const TfToken &propName = editDesc.oldPath.GetNameToken();

    // Check to see if there are any contributing specs that would require 
    // relocates. These are specs that would continue to be mapped to the 
    // same path across the edit target's node even after all specs are edited
    // in its layer stack.
    //
    // As opposed to prims, all nodes are essentially "ancestral arcs" for 
    // properties since properties don't define composition arcs. So we look
    // for property specs in every node under the edit target node as those 
    // can't be namespace edited without relocates.
    PcpNodeRange subtreeRange = primIndex.GetNodeSubtreeRange(nodeForEditTarget);

    // Skip the node itself; we want to check its descendants.
    ++subtreeRange.first;
    for (const PcpNodeRef &subtreeNode : subtreeRange) {

        // Skip nodes that don't contribute specs.
        if (!subtreeNode.HasSpecs() || subtreeNode.IsInert()) {
            continue;
        }

        // Map the property path to this node so we can search its layers for 
        // specs. If the property path can't be mapped, we can skip this node.
        // Note that we use the node's path and append the property name instead
        // of using the map function of the node to map the property path. This
        // is because variant arcs don't include the variant selection in the
        // map function (but do in the site path) so we won't get the correct
        // variant property path via the map function.
        SdfPath mappedPropertyPath = 
            subtreeNode.GetPath().AppendProperty(propName);
        if (mappedPropertyPath.IsEmpty()) {
            continue;
        }

        // Search the layers in the layer stack to see if any of them have a
        // property spec for the mapped property.
        const SdfLayerRefPtrVector &layers = 
            subtreeNode.GetLayerStack()->GetLayers();
        const bool hasPropertySpecs = std::any_of(layers.begin(), layers.end(),
            [&](const auto &layer) { 
                return layer->HasSpec(mappedPropertyPath);
            });

        // If we found a property spec, the edit requires relocates.
        if (hasPropertySpecs) {
            processedEdit->requiresRelocates = true;

            // Relocates support is not implemented yet so it's currently an 
            // error if the edit requires it.
            if (editDesc.editType == _EditType::Delete) {
                // "Relocates" means deactivating the property in the deletion 
                // case.
                processedEdit->errors.push_back("The property to delete must "
                    "be deactivated rather than deleted since it composes "
                    "opinions introduced by ancestral composition arcs; "
                    "deletion via deactivation is not yet supported");
            } else {
                processedEdit->errors.push_back("The property to move requires "
                    "authoring relocates since it composes opinions "
                    "introduced by ancestral composition arcs; authoring "
                    "relocates is not supported for properties");
            }

            // Once we've determined the edits require relocates, we're done.
            return;
        }
    }
}

/*static*/
void 
UsdNamespaceEditor::_EditProcessor::_GatherLayersToEdit(
    const _EditDescription &editDesc,
    const UsdEditTarget &editTarget,
    const PcpPrimIndex &primIndex,
    _ProcessedEdit *processedEdit)
{
    // XXX: To start, we're only going to perform namespace edit operations 
    // using the root layer stack. This will be updated to support edit targets
    // as a later task.
    const PcpNodeRef nodeForEditTarget = primIndex.GetRootNode();
    
    // Get all the layers in the layer stack where the edits will be performed.
    const SdfLayerRefPtrVector &layers = 
        nodeForEditTarget.GetLayerStack()->GetLayers();

    // Until we support edit targets, verify that the stage's current edit 
    // target maps to the prim's local opinions in the root layer stack.
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
        if (layer->HasSpec(editDesc.oldPath)) {
            processedEdit->layersToEdit.push_back(layer);
        }
    }

    // Determine if editing the path would require relocates.
    if (editDesc.IsPropertyEdit()) {
        _ProcessPropEditRequiresRelocates(
            editDesc, primIndex, nodeForEditTarget, processedEdit);
    } else {
        _ProcessPrimEditRequiresRelocates(
            editDesc, primIndex, nodeForEditTarget, processedEdit);
    }

    // Validate whether the necessary spec edits can actually be performed on
    // each layer that needs to be edited.
    for (const auto &layer : processedEdit->layersToEdit) {
        // The layer itself needs to be editable  
        if (!layer->PermissionToEdit()) {
            processedEdit->errors.push_back(TfStringPrintf("The spec @%s@<%s> "
                "cannot be edited because the layer is not editable",
                layer->GetIdentifier().c_str(),
                editDesc.oldPath.GetText()));
        }
        // If we're moving an object to a new path, the layer cannot have a
        // spec already at the new path.
        if (!editDesc.newPath.IsEmpty() && layer->HasSpec(editDesc.newPath)) {
            processedEdit->errors.push_back(TfStringPrintf("The spec @%s@<%s> "
                "cannot be moved to <%s> because a spec already exists at "
                "the new path",
                layer->GetIdentifier().c_str(),
                editDesc.oldPath.GetText(),
                editDesc.newPath.GetText()));
        }
    }
}

void 
UsdNamespaceEditor::_EditProcessor::_GatherTargetListOpEdits(
    const UsdStageRefPtr &stage,
    const _EditDescription &editDesc,
    _ProcessedEdit *processedEdit)
{
    // Gather all the dependencies from stage namespace path to properties with 
    // relationship targets or attributes connections that depend on that 
    // namespace path.
    _TargetingPropertyDependencies deps =
         _TargetingPropertyDependencyCollector::GetDependencies(stage);

    // With all the target path dependencies we need to determine which 
    // targeting properties are affected by this particular edit. If the edit 
    // was to a prim, the affected target paths will be any descendants of the
    // original prim path, thus we have to get all properties targeting any 
    // descendant of the changed path.
    SdfPathSet propPathsWithAffectedTargets;
    const auto range = 
        deps.targetedPathToTargetingPropertiesPathTable.FindSubtreeRange(
            editDesc.oldPath);
    for (auto it = range.first; it != range.second; ++it) {
        const SdfPathVector &propPaths = it->second;
        propPathsWithAffectedTargets.insert(propPaths.begin(), propPaths.end());
    }

    // Now for each targeting property gather the edits that need to be made to
    // the layer specs in order to update the affected targets.
    for (const SdfPath &propertyPath : propPathsWithAffectedTargets) {

        // Every property path listed as dependency must have a list of property
        // specs that provide target opinions.
        const _PropertySpecWithAuthoredTargetsVector *propertySpecs = 
            TfMapLookupPtr(deps.composedPropertyToSpecsWithAuthoredTargetsMap, 
                propertyPath);
        if (!TF_VERIFY(propertySpecs)) {
            continue;
        }

        // First we're only going to look at property specs that originated from
        // the root node of the prim index (local opinions). These specs can 
        // be edited to update the target paths.
        for (const auto &specInfo : *propertySpecs) {
            // Stop when we hit a non-root node as the property specs are in
            // strength order.
            if (!specInfo.originatingNode.IsRootNode()) {
                break;
            }

            // Get the current value of the target field list op for the spec
            // and try to modify any paths that need to change because of the
            // edited namespace path.
            SdfPathListOp targetListOp = specInfo.GetTargetListOp();
            if (targetListOp.ModifyOperations(
                [&](const SdfPath &path) {
                    // All target paths are always absolute within the layer 
                    // data even though they can be specified as relative in
                    // the text of a usda file. We verify this absolute path
                    // assumption just to make sure.
                    if (!TF_VERIFY(path.IsAbsolutePath())) {
                        return std::optional<SdfPath>(path);
                    }
                    // If the path doesn't start with the old path, it is not 
                    // affected and returned unmodified.
                    if (!path.HasPrefix(editDesc.oldPath)) {
                        return std::optional<SdfPath>(path);
                    }
                    // Otherwise we found an affected path. If we've deleted
                    // the old path, delete this target item.
                    if (editDesc.newPath.IsEmpty()) {
                        return std::optional<SdfPath>();
                    }
                    // Otherwise update the path of this target item for the 
                    // new path.
                    return std::optional<SdfPath>(
                        path.ReplacePrefix(editDesc.oldPath, editDesc.newPath));
                }))
            {
                // If the target list op was modified, add the edit we need
                // to perform for this spec in the processed edit.
                processedEdit->targetPathListOpEdits.push_back(
                    {specInfo.layer->GetPropertyAtPath(specInfo.path),
                     specInfo.fieldName, 
                     std::move(targetListOp)});
            }
        }

        // For target paths that are contributed by specs that originate across
        // arcs below the root node, we can't edit these specs directly. 
        // Instead we'd need relocates to map these paths. In this case we find
        // compose the target list, excluding the root node opinions, to see if 
        // any of them would be affected by the namespace edit and therefore 
        // require a relocates
        SdfPathVector targetsRequireRelocates;

        // Iterate in weakest to strongest applying each list op to get the 
        // composed targets below the root node.
        for (auto rIt = propertySpecs->rbegin(); rIt != propertySpecs->rend(); 
                ++rIt) {
            const _PropertySpecWithAuthoredTargetsInfo &specInfo = *rIt;

            // Stop when we hit a spec originating from the root node
            if (specInfo.originatingNode.IsRootNode()) {
                break;
            }

            // Apply each list op, translating the paths into stage namespace.
            rIt->GetTargetListOp().ApplyOperations(&targetsRequireRelocates,
                [&](SdfListOpType opType, const SdfPath& inPath) {

                    const SdfPath translatedPath = 
                        rIt->originatingNode.GetMapToRoot().MapSourceToTarget(inPath);
                    // Skip paths that don't map. Also skip paths that aren't 
                    // affected by the namespace edit; we don't care about these 
                    // either.
                    if (translatedPath.IsEmpty() ||
                        !translatedPath.HasPrefix(editDesc.oldPath)) {
                        return std::optional<SdfPath>();
                    }
                    return std::optional<SdfPath>(translatedPath);
                });
        }

        // If the any of the targets require relocates, store this as a target
        // list op error in the processed edit.
        if (!targetsRequireRelocates.empty()) {
            const bool isAttribute = 
                stage->GetObjectAtPath(propertyPath).Is<UsdAttribute>();
            processedEdit->targetPathListOpErrors.push_back(TfStringPrintf(
                "The %s at '%s' has the following %s paths [%s] which require "
                "authoring relocates to be retargeted because they are "
                "introduced by opinions from composition arcs; authoring "
                "relocates is not yet supported",
                isAttribute ? "attribute" : "relationship",
                propertyPath.GetText(),
                isAttribute ? "connection" : "relationship target",
                TfStringify(targetsRequireRelocates).c_str()));
        }
    }
}

bool 
UsdNamespaceEditor::_ProcessedEdit::CanApply(std::string *whyNot) const
{
    // Only errors that prevent the object from being moved or deleted in stage
    // namespace prevent the edits from being applied. Errors in edits like 
    // relationship target or connection path fixups do not prevent the rest
    // of the edits from being applied.
    if (!errors.empty()) {
        if (whyNot) {
            *whyNot = _GetErrorString(errors);
        }
        return false;
    }

    return true;
}

bool 
UsdNamespaceEditor::_ProcessedEdit::Apply()
{
    // This is to try to preemptively prevent partial edits when if any of the 
    // necessary specs can't be renamed.
    if (std::string errorMsg; !CanApply(&errorMsg)) {
        TF_CODING_ERROR(TfStringPrintf("Failed to apply edits to the stage "
            "because of the following errors: %s", errorMsg.c_str()));
        return false;
    }

    // Implementation function as this is optionally called with a cleanup 
    // enabler depending on the edit type.
    auto applyEditsToLayersFn = [&]() {
        for (const auto &layer : layersToEdit) {
            // While we do require that the new parent exists on the composed 
            // stage when doing a reparent operation, that doesn't guarantee 
            // that parent spec exists on every layer in which we have to move
            // the source spec. Thus we need to ensure the parent spec of the 
            // new location exists by adding required overs if necessary. 
            if (!createParentSpecIfNeededPath.IsEmpty() &&
                !SdfJustCreatePrimInLayer(
                    layer, createParentSpecIfNeededPath)) {
                TF_CODING_ERROR("Failed to find or create new parent spec "
                    "at path '%s' on layer '%s' which is necessary to "
                    "apply edits. The edit will be incomplete.",
                    createParentSpecIfNeededPath.GetText(),
                    layer->GetIdentifier().c_str());
                return false;
            }

            // Apply the namespace edits to the layer.
            if (!layer->Apply(edits)) {
                TF_CODING_ERROR("Failed to apply batch edit '%s' on layer '%s' "
                    "which is necessary to apply edits. The edit will be "
                    "incomplete.",
                    TfStringify(edits.GetEdits()).c_str(),
                    layer->GetIdentifier().c_str());
                return false;
            }
        }
        return true;
    };

    SdfChangeBlock changeBlock;
    if (removeInertAncestorOvers) {
        // Moving a spec may leave the ancnestor specs as an inert overs. This
        // could easily be caused by reparenting a prim back to its original
        // parent (essentially an "undo") after a reparent that needed to create
        // new overs. Using a cleanup enabler will (after all specs are moved) 
        // handle deleting any inert "dangling" overs that are ancestors of the
        // moved path so that a reparent plus an "undo" can effectively leave
        // layers in their original state.
        SdfCleanupEnabler cleanupEnabler;
        if (!applyEditsToLayersFn()) {
            return false;
        }
    } else {
        if (!applyEditsToLayersFn()) {
            return false;
        }
    }

    // Perform any target path listOp fixups necessary now that the namespace 
    // edits have been successfully performed.
    for (const TargetPathListOpEdit &edit : targetPathListOpEdits) {
        // It's possible the spec no longer exists if the property holding
        // the target field was deleted by the namespace edit operation 
        // itself.
        if (edit.propertySpec) {
            edit.propertySpec->SetField(edit.fieldName, edit.newFieldValue);
        }
    }

    // Errors in fixing up targets do not prevent us from applying namespace
    // edits, but we report them as warnings.
    if (!targetPathListOpErrors.empty()) {
        TF_WARN("The follow target path or connections could not be "
            "updated for the namespace edit: %s",
            _GetErrorString(targetPathListOpErrors).c_str());
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

