//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

UsdNamespaceEditor::UsdNamespaceEditor(
    const UsdStageRefPtr &stage, 
    EditOptions &&editOptions) 
    : _stage(stage)
    , _editOptions(std::move(editOptions))
{    
}

UsdNamespaceEditor::UsdNamespaceEditor(
    const UsdStageRefPtr &stage, 
    const EditOptions &editOptions)
    : _stage(stage)
    , _editOptions(editOptions)
{
}

void 
UsdNamespaceEditor::AddDependentStage(const UsdStageRefPtr &stage)
{
    if (!stage || stage == _stage) {
        return;
    }
    _ClearProcessedEdits();
    _dependentStages.insert(stage);
}

void 
UsdNamespaceEditor::RemoveDependentStage(const UsdStageRefPtr &stage)
{
    _ClearProcessedEdits();
    _dependentStages.erase(stage);
}

void 
UsdNamespaceEditor::SetDependentStages(const UsdStageRefPtrVector &stages)
{   
    for (const auto &stage : stages) {
        AddDependentStage(stage);
    }
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
        const _StageSet &dependentStages,
        const UsdNamespaceEditor::_EditDescription &editDesc,
        const UsdNamespaceEditor::EditOptions &editOptions);

private:
    _EditProcessor(
        const UsdStageRefPtr &stage,
        const _StageSet &_dependentStages,
        const UsdNamespaceEditor::_EditDescription &editDesc,
        const UsdNamespaceEditor::EditOptions &editOptions,
        _ProcessedEdit *processedEdit);

    bool _ProcessNewPath();

    void _ProcessPrimEditRequiresRelocates(
        const PcpPrimIndex &primIndex);

    void _ProcessPropEditRequiresRelocates(
        const PcpPrimIndex &primIndex);

    void _GatherLayersToEdit();

    void _GatherTargetListOpEdits();

    void _GatherDependentStageEdits();

    const UsdStageRefPtr & _stage;
    const _StageSet &_dependentStages;
    const UsdNamespaceEditor::_EditDescription & _editDesc;
    const UsdEditTarget &_editTarget;
    const UsdNamespaceEditor::EditOptions & _editOptions;
    _ProcessedEdit *_processedEdit;

    PcpNodeRef _nodeForEditTarget;
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
        _stage, _dependentStages, _editDescription, _editOptions);
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
    const _StageSet &dependentStages,
    const _EditDescription &editDesc,
    const EditOptions &editOptions)
{
    _ProcessedEdit processedEdit;
    _EditProcessor(stage, dependentStages, editDesc, editOptions, &processedEdit);
    return processedEdit;
}

UsdNamespaceEditor::_EditProcessor::_EditProcessor(
    const UsdStageRefPtr &stage,
    const _StageSet &dependentStages,
    const UsdNamespaceEditor::_EditDescription &editDesc,
    const UsdNamespaceEditor::EditOptions &editOptions,
    _ProcessedEdit *processedEdit)
    : _stage(stage)
    , _dependentStages(dependentStages)
    , _editDesc(editDesc)
    , _editTarget(stage->GetEditTarget())
    , _editOptions(editOptions)
    , _processedEdit(processedEdit)
{
    if (editDesc.editType == _EditType::Invalid) {
        _processedEdit->errors.push_back("There are no valid edits to perform");
        return;
    }

    // Copy the edit description.
    _processedEdit->editDescription = _editDesc;

    // Validate whether the stage has the prim or property at the original path
    // that can be namespace edited.
    const UsdPrim prim = stage->GetPrimAtPath(_editDesc.oldPath.GetPrimPath());
    std::string error;
    if (editDesc.IsPropertyEdit()) {
        if (!_IsValidPropertyToEdit(prim, _editDesc.oldPath.GetNameToken(), &error)) {
            _processedEdit->errors.push_back(std::move(error));
            return;
        }
    } else if (!_IsValidPrimToEdit(prim, &error)) {
        _processedEdit->errors.push_back(std::move(error));
        return;
    }

    const PcpPrimIndex &primIndex = prim.GetPrimIndex();
    // XXX: To start, we're only going to perform namespace edit operations 
    // using the root layer stack. This will be updated to support edit targets
    // as a later task.
    _nodeForEditTarget = primIndex.GetRootNode();

    if (!_ProcessNewPath()) {
        return;
    }

    if (_editDesc.IsPropertyEdit()) {
        // Determine if editing the path would require relocates.
        _ProcessPropEditRequiresRelocates(primIndex);
    } else {
        // Determine if editing the path would require relocates.
        _ProcessPrimEditRequiresRelocates(primIndex);
    }

    // Gather all layers with contributing specs to the old path that will need
    // to be edited when the edits are applied.
    _GatherLayersToEdit();

    // Gather all edits that need to be performed on dependent stages for prim
    // indexes that would be affected by the initial layer edits.
    _GatherDependentStageEdits();

    // Gather all the edits that need to be made to target path listOps in 
    // property specs in order to "fix up" properties that have connections or 
    // relationship targets targeting the namespace edited object.
    _GatherTargetListOpEdits();
}

bool
UsdNamespaceEditor::_EditProcessor::_ProcessNewPath()
{
    // Empty path is a delete so the new path is automatically valid.
    if (_editDesc.newPath.IsEmpty()) {
        return true;
    }

    // For move edits we'll have a new path; verify that the stage doesn't 
    // already have an object at that path.
    if (_stage->GetObjectAtPath(_editDesc.newPath)) {
        _processedEdit->errors.push_back(
            "An object already exists at the new path");
        return false;
    }

    // For reparenting we have additional behaviors and validation to perform.
    if (_editDesc.editType == _EditType::Reparent) {

        // Validate that the stage does have a prim at the new parent path to 
        // reparent to.
        std::string whyNot;
        if (!_IsValidNewParentPath(_stage, _editDesc.oldPath, 
                _editDesc.newPath.GetParentPath(), &whyNot)) {
            _processedEdit->errors.push_back(std::move(whyNot));
            return false;
        }
    }

    // For property edits we're done at this point.
    if (_editDesc.IsPropertyEdit()) {
        return true;
    }

    // For prim moves, we need to check whether the new path is prohibited 
    // because of relocates.
    // The parent prim will be able to tell us if the child name that we're
    // moving and/or renaming this to is prohibited.
    UsdPrim newParentPrim = _stage->GetPrimAtPath(
        _editDesc.newPath.GetParentPath());
    if (!newParentPrim) {
        TF_CODING_ERROR("Parent prim at path %s does not exist",
            _editDesc.newPath.GetParentPath().GetText());
        return false;
    }

    // XXX: We compute the prohibited children from the parent prim
    // index. Given that the prohibited children are always composed
    // with the actual child names, we could cache this when the 
    // stage is populated and exposed the prohibited children in API on 
    // UsdPrim. But for now we'll compose them as needed when processing
    // namespace edits.
    const PcpPrimIndex &newParentPrimIndex = newParentPrim.GetPrimIndex();
    TfTokenVector childNames;
    PcpTokenSet prohibitedChildren;
    newParentPrimIndex.ComputePrimChildNames(
        &childNames, &prohibitedChildren);

    // If the parent does not prohibit a child with our name, we're good, 
    // otherwise we can't move the prim to the new path.
    if (prohibitedChildren.count(_editDesc.newPath.GetNameToken()) == 0) {
        return true;
    }

    // But there is one exception! If this layer stack has a relocation from the
    // new path to the old path, then we are allowed to move the prim back to
    // its original location by removing the relocation.
    const SdfRelocatesMap &localRelocates =
        _nodeForEditTarget.GetLayerStack()->GetIncrementalRelocatesSourceToTarget();
    const auto foundIt = localRelocates.find(_editDesc.newPath);
    if (foundIt != localRelocates.end() && foundIt->second == _editDesc.oldPath) {
        return true;
    }

    _processedEdit->errors.push_back("The new path is a prohibited child of "
        "its parent path because of existing relocates.");
    return false;
}

void 
UsdNamespaceEditor::_EditProcessor::_ProcessPrimEditRequiresRelocates(
    const PcpPrimIndex &primIndex)
{
    const bool requiresRelocatesAuthoring = [&](){
        // First check: if the path that is being moved or deleted is already
        // the target of a relocation in the local layer stack, then the 
        // local layer relocates will need to be updated to perform the edit
        // operation.
        const SdfRelocatesMap &targetToSourceRelocates =
            _nodeForEditTarget.GetLayerStack()
                ->GetIncrementalRelocatesTargetToSource();
        if (targetToSourceRelocates.count(_editDesc.oldPath)) {
            return true;
        }

        // Check to see if there are any contributing specs that would require 
        // relocates. These are specs that would continue to be mapped to the 
        // same path across the edit target's node even after all specs are 
        // edited in its layer stack.
        const auto range = _nodeForEditTarget.GetChildrenRange();
        for (const auto &child : range) {
            // If a child node is a direct arc, we can skip it and its entire 
            // subtree as all the specs at or below this node are mapped to the 
            // prim path (whatever it may be) through this child node.
            if (!child.IsDueToAncestor()) {
                continue;
            }

            // Since the child node is an ancestral arc, the mapping of specs 
            // across the child node is not affected by the path of this prim 
            // itself and will continue to map to the original path after the
            // edit. So if there are any specs in the child's subtree, then this
            // edit will require relocates
            PcpNodeRange subtreeRange = primIndex.GetNodeSubtreeRange(child);
            for (const PcpNodeRef &subtreeNode : subtreeRange) {
                // A node has contributing specs if it has specs and is not 
                // inert.
                if (subtreeNode.HasSpecs() && !subtreeNode.IsInert()) {
                    return true;
                }
            }
        }

        return false;
    }();

    if (!requiresRelocatesAuthoring) {
        return;
    }

    // If relocates authoring is not allowed, log an error and return; we
    // won't be able to apply this edit.
    if (!_editOptions.allowRelocatesAuthoring) {
        _processedEdit->errors.push_back("The prim to edit requires "
            "authoring relocates since it composes opinions "
            "introduced by ancestral composition arcs; relocates "
            "authoring must be enabled to perform this edit");
        return;
    }

    // Otherwise, log that we will author relocates so that this will be 
    // accounted for when we compute the dependent stage namespace edits.
    _processedEdit->willAuthorRelocates = true;
}

void 
UsdNamespaceEditor::_EditProcessor::_ProcessPropEditRequiresRelocates(
    const PcpPrimIndex &primIndex)
{
    const TfToken &propName = _editDesc.oldPath.GetNameToken();

    // Check to see if there are any contributing specs that would require 
    // relocates. These are specs that would continue to be mapped to the 
    // same path across the edit target's node even after all specs are edited
    // in its layer stack.
    //
    // As opposed to prims, all nodes are essentially "ancestral arcs" for 
    // properties since properties don't define composition arcs. So we look
    // for property specs in every node under the edit target node as those 
    // can't be namespace edited without relocates.
    PcpNodeRange subtreeRange = 
        primIndex.GetNodeSubtreeRange(_nodeForEditTarget);

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
            // There is no plan to support relocates for properties so it's an 
            // error if the edit requires it.
            _processedEdit->errors.push_back("The property to edit requires "
                "authoring relocates since it composes opinions "
                "introduced by ancestral composition arcs; authoring "
                "relocates is not supported for properties");
            return;
        }
    }
}

void 
UsdNamespaceEditor::_EditProcessor::_GatherLayersToEdit()
{
    // Get all the layers in the layer stack where the edits will be performed.
    const SdfLayerRefPtrVector &layers = 
        _nodeForEditTarget.GetLayerStack()->GetLayers();

    // Until we support edit targets, verify that the stage's current edit 
    // target maps to the prim's local opinions in the root layer stack.
    if (!_editTarget.GetMapFunction().IsIdentityPathMapping()) {
        _processedEdit->errors.push_back("Edit targets that map paths across "
            "composition arcs are not currently supported for namespace "
            "editing");
        return;
    }
    if (std::find(layers.begin(), layers.end(), _editTarget.GetLayer()) 
            == layers.end()) {
        _processedEdit->errors.push_back("Edit targets with layers outside of "
            "the root layer stack are not currently supported for namespace "
            "editing");
        return;
    }

    _processedEdit->layersToEdit = PcpGatherLayersToEditForSpecMove(
        _nodeForEditTarget.GetLayerStack(),
        _editDesc.oldPath, _editDesc.newPath, &_processedEdit->errors);
}

void 
UsdNamespaceEditor::_EditProcessor::_GatherTargetListOpEdits()
{
    // Gather all the dependencies from stage namespace path to properties with 
    // relationship targets or attributes connections that depend on that 
    // namespace path.
    _TargetingPropertyDependencies deps =
         _TargetingPropertyDependencyCollector::GetDependencies(_stage);

    // With all the target path dependencies we need to determine which 
    // targeting properties are affected by this particular edit. If the edit 
    // was to a prim, the affected target paths will be any descendants of the
    // original prim path, thus we have to get all properties targeting any 
    // descendant of the changed path.
    SdfPathSet propPathsWithAffectedTargets;
    const auto range = 
        deps.targetedPathToTargetingPropertiesPathTable.FindSubtreeRange(
            _editDesc.oldPath);
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
                    if (!path.HasPrefix(_editDesc.oldPath)) {
                        return std::optional<SdfPath>(path);
                    }
                    // Otherwise we found an affected path. If we've deleted
                    // the old path, delete this target item.
                    if (_editDesc.newPath.IsEmpty()) {
                        return std::optional<SdfPath>();
                    }
                    // Otherwise update the path of this target item for the 
                    // new path.
                    return std::optional<SdfPath>(
                        path.ReplacePrefix(_editDesc.oldPath, _editDesc.newPath));
                }))
            {
                // If the target list op was modified, add the edit we need
                // to perform for this spec in the processed edit.
                _processedEdit->targetPathListOpEdits.push_back(
                    {specInfo.layer->GetPropertyAtPath(specInfo.path),
                     specInfo.fieldName, 
                     std::move(targetListOp)});
            }
        }

        // If the edit will author relocates for the primary edit, then the target
        // paths authored across composition arcs will also be mapped by the
        // relocation. 
        if (_processedEdit->willAuthorRelocates) {
            continue;
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
                        !translatedPath.HasPrefix(_editDesc.oldPath)) {
                        return std::optional<SdfPath>();
                    }
                    return std::optional<SdfPath>(translatedPath);
                });
        }

        // If the any of the targets require relocates, store this as a target
        // list op error in the processed edit.
        if (!targetsRequireRelocates.empty()) {
            const bool isAttribute = 
                _stage->GetObjectAtPath(propertyPath).Is<UsdAttribute>();
            _processedEdit->targetPathListOpErrors.push_back(TfStringPrintf(
                "Fixing the %s paths %s for the %s at '%s' would require "
                "'%s'to be relocated but we do not introduce relocates for %s.",
                isAttribute ? "connection" : "relationship",
                TfStringify(targetsRequireRelocates).c_str(),
                isAttribute ? "attribute" : "relationship",
                propertyPath.GetText(),
                _editDesc.oldPath.GetText(),
                _editDesc.IsPropertyEdit() ? 
                    "properties ever" :
                    "prims that do not have opinions across composition arcs"
                ));
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

static bool 
_ApplyLayerSpecMove(
    const SdfLayerHandle &layer, const SdfPath &oldPath, const SdfPath &newPath)
{
    // Create an SdfBatchNamespaceEdit for the path move. We use the index of
    // "Same" specifically so renames don't move the object out of its original
    // order (it has no effect for any edits other than rename)
    SdfBatchNamespaceEdit batchEdit;
    batchEdit.Add(oldPath, newPath, SdfNamespaceEdit::Same);

    // Implementation function as this is optionally called with a cleanup 
    // enabler depending on the edit type.
    auto applyEditsToLayersFn = [&](const SdfPath &createParentSpecIfNeededPath) {
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
        if (!layer->Apply(batchEdit)) {
            TF_CODING_ERROR("Failed to apply batch edit '%s' on layer '%s' "
                "which is necessary to apply edits. The edit will be "
                "incomplete.",
                TfStringify(batchEdit.GetEdits()).c_str(),
                layer->GetIdentifier().c_str());
            return false;
        }

        return true;
    };

    const bool isReparent = !newPath.IsEmpty() &&
        newPath.GetParentPath() != oldPath.GetParentPath();
    if (isReparent) {
        // Moving a spec may leave the ancnestor specs as an inert overs. This
        // could easily be caused by reparenting a prim back to its original
        // parent (essentially an "undo") after a reparent that needed to create
        // new overs. Using a cleanup enabler will (after all specs are moved) 
        // handle deleting any inert "dangling" overs that are ancestors of the
        // moved path so that a reparent plus an "undo" can effectively leave
        // layers in their original state.
        SdfCleanupEnabler cleanupEnabler;
        if (!applyEditsToLayersFn(newPath.GetParentPath())) {
            return false;
        }
    } else {
        if (!applyEditsToLayersFn(SdfPath::EmptyPath())) {
            return false;
        }
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

    SdfChangeBlock changeBlock;

    if (editDescription.IsPropertyEdit()) {
        // For a property edit, we just have to move the specs in the layers to
        // edit.
        for (const auto &layer : layersToEdit) {
            _ApplyLayerSpecMove(layer, 
                editDescription.oldPath, editDescription.newPath);
        }
    } else {
        // For prim edits, the dependent stage edits are always computed for 
        // at least the primary stage so all necessary edits will be contained
        // in those computed edits.
        for (const auto &[layer, editVec] : 
                dependentStageNamespaceEdits.layerSpecMoves) {
            for (const auto &edit : editVec) {
                _ApplyLayerSpecMove(layer, edit.oldPath, edit.newPath);
            }
        }

        for (const auto &edit : 
                dependentStageNamespaceEdits.compositionFieldEdits) {
            edit.layer->SetField(edit.path, edit.fieldName, edit.newFieldValue);
        }

        for (const auto &[layer, relocates] : 
                dependentStageNamespaceEdits.dependentRelocatesEdits) {
            layer->SetRelocates(relocates);
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
        TF_WARN("Failed to update the following targets and/or connections for "
            "the namespace edit: %s",
            _GetErrorString(targetPathListOpErrors).c_str());
    }

    return true;
}

void
UsdNamespaceEditor::_EditProcessor::_GatherDependentStageEdits()
{
    // Composition dependencies are only relevant for prim namespace edits.
    if (_editDesc.IsPropertyEdit()) {
        return;
    }

    // Get the PcpCaches for each dependent stage. The primary stage is always
    // a dependent so put its cache at the front. Note that _dependentStages 
    // are a uniqued set and should never contain the primary stage.
    std::vector<const PcpCache *> dependentCaches;
    dependentCaches.reserve(_dependentStages.size() + 1);
    dependentCaches.push_back(_stage->_GetPcpCache());
    for (const auto &stage : _dependentStages) {
        dependentCaches.push_back(stage->_GetPcpCache());
    }

    // If we need and allow relocates for the primary edit, then we pass the 
    // layer stack where we'll author them to the dependent edits function
    // which will compute the layer stack's relocates edits for us.
    const PcpLayerStackRefPtr &addRelocatesToLayerStack = 
        _processedEdit->willAuthorRelocates ?
            _nodeForEditTarget.GetLayerStack() : PcpLayerStackRefPtr();

    // Gather all the dependent edits for all stage PcpCaches.
    _processedEdit->dependentStageNamespaceEdits = 
        PcpGatherDependentNamespaceEdits(
            _editDesc.oldPath, _editDesc.newPath, _processedEdit->layersToEdit,
            addRelocatesToLayerStack, _editTarget.GetLayer(), 
            dependentCaches);

    // XXX: We may want an option to allow users to treat warnings as errors or
    // to return warning as part of calling CanApplyEdits. But for now we just
    // emit the warnings.
    if (!_processedEdit->dependentStageNamespaceEdits.warnings.empty()) {
        TF_WARN("Encountered warnings processing dependent namespace edits: %s",
            TfStringJoin(_processedEdit->dependentStageNamespaceEdits.warnings, 
                         "\n  ").c_str());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

