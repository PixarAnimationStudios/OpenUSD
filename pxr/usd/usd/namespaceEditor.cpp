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

// Stores info about a prim or property spec with authored path-bearing fields 
// that will need to be updated as part of a namespace edit (e.g. attribute 
// connections or relationship targets).
struct _SpecWithPathBearingFieldsInfo {
    // Layer and path of the site of the spec.
    SdfLayerHandle layer;
    SdfPath path;

    // The name of the field in the prim/property spec.
    TfToken fieldName;

    // The node in the composed prim index that introduces this spec. Necessary 
    // for mapping paths to the stage namespace as well as determining if these 
    // paths can be edited with or without relocates.
    PcpNodeRef originatingNode;

    // Gets the field's value from this spec.
    VtValue GetFieldValue() const
    {
        VtValue val;
        TF_VERIFY(layer->HasField(path, fieldName, &val));
        if (val.IsEmpty()) {
            TF_CODING_ERROR("Field %s at site @%s@<%s> is expected to exist.",
                fieldName.GetText(),
                layer->GetIdentifier().c_str(),
                path.GetText());
            return VtValue();
        }
        return val;
    }
};

using _SpecWithPathBearingFieldsVector = 
    std::vector<_SpecWithPathBearingFieldsInfo>;

// Structure for storing the dependencies between stage object paths and the
// prim or property specs that have fields with paths that include that object.
struct _Dependencies {
    // The map of each stage path to the prim or property specs (ordered 
    // strongest to weakest) that provide opinions for the spec's path-
    // bearing fields.
    std::unordered_map<SdfPath, _SpecWithPathBearingFieldsVector, TfHash> 
        composedObjectToSpecsWithPathBearingFieldsMap;

    // A table of stage object path to the list of paths that have
    // specs with path-bearing fields that map to this object path.
    SdfPathTable<SdfPathVector> targetedPathToTargetingSpecPathsTable;
};
// ------------------------------------------------------------------------- //
// Transforms for fields
// ------------------------------------------------------------------------- //

// This struct encapsulates the state required to perform a namespace edit.
struct _NamespaceEditXf
{
    SdfPath oldPath;
    SdfPath newPath;
};

// This struct is used to collect the paths that a certain field refers to.
// For example, when used to transform a targetPaths field, it collects all the 
// paths stored in the targetPath list op's explicit, append, prepend, and delete
// item vectors. This transformation collects these paths and saves them in this 
// struct for use after the transform is applied. The transform itself does not 
// change the returned field value.
struct _CollectPathsXf
{
    PcpNodeRef node;
    mutable SdfPathSet paths;
};

// Append an SdfPath to its transform.
SdfPath
_CollectPath(
    SdfPath const &path, _CollectPathsXf const &xf)
{
    xf.paths.insert(path);
    return path;
}

// Namespace edit SdfPath by replacing its prefix.
SdfPath
_PathReplacePrefix(
    SdfPath const &path, _NamespaceEditXf const &xf)
{
    return path.ReplacePrefix(xf.oldPath, xf.newPath);
}

SdfPathListOp
_PathListOpNamespaceEdit(SdfPathListOp const &listOp, _NamespaceEditXf const& xf)
{
    // Try to modify any paths that need to change because of the
    // edited namespace path.
    SdfPathListOp modifiedListOp(listOp);
    modifiedListOp.ModifyOperations(
        [&](const SdfPath &path) {
            // All paths are always absolute within the layer 
            // data even though they can be specified as relative in
            // the text of a usda file. We verify this absolute path
            // assumption just to make sure.
            if (!path.IsAbsolutePath()) {
                return std::optional<SdfPath>(path);
            }
            // If the path doesn't start with the old path, it is not 
            // affected and returned unmodified.
            if (!path.HasPrefix(xf.oldPath)) {
                return std::optional<SdfPath>(path);
            }
            // Otherwise we found an affected path. If we've deleted
            // the old path, delete this item.
            if (xf.newPath.IsEmpty()) {
                return std::optional<SdfPath>();
            }
            // Otherwise update the path of this item for the 
            // new path.
            return std::optional<SdfPath>(
                path.ReplacePrefix(xf.oldPath, xf.newPath));
    });
    return modifiedListOp;
}

SdfPathListOp
_PathListOpCollectPathsXf(
    SdfPathListOp const &listOp, _CollectPathsXf const &xf)
{
    // Helper for collecting the paths from a listOp item vector and adding 
    // them to the work entry's paths list, mapping the path to the root node 
    // (stage namespace) if necessary.
    auto ApplyXfFn = [&](const SdfPathListOp::ItemVector &items) {
        for (const SdfPath &item : items) {
            VtValueTryTransform(VtValue(item), xf);
        }
    };

    // Apply the transform to all paths found anywhere in the listOp
    // as all these paths count as a dependency that may need to 
    // fixed after a namespace edit. 
    if (listOp.IsExplicit()) {
        ApplyXfFn(listOp.GetExplicitItems());
    } else {
        ApplyXfFn(listOp.GetAddedItems());
        ApplyXfFn(listOp.GetAppendedItems());
        ApplyXfFn(listOp.GetDeletedItems());
        ApplyXfFn(listOp.GetOrderedItems());
        ApplyXfFn(listOp.GetPrependedItems());
    }

    return listOp;
}

SdfPathExpression
_PathExprReplacePrefix(
    SdfPathExpression const &pathExpr, _NamespaceEditXf const &xf)
{
    return pathExpr.ReplacePrefix(xf.oldPath, xf.newPath);
}

SdfPathExpression
_CollectPrefixes(
    SdfPathExpression const &pathExpr, _CollectPathsXf const &xf)
{
    using PathExpr = SdfPathExpression;
    using Op = PathExpr::Op;
    using PathPattern = PathExpr::PathPattern;
    using ExpressionReference = PathExpr::ExpressionReference;

    // Ignore logical operators.
    auto logic = [&xf](Op op, int argIndex) {};

    // Collect the expression reference path.
    auto mapRef =
        [&xf](ExpressionReference const &ref) {
        if (!ref.path.IsEmpty()) {
            xf.paths.insert(ref.path);
        }
    };
    
    // Collect the path pattern prefix, if any.
    auto mapPattern =
        [&xf](PathPattern const &pattern) {
        if (!pattern.GetPrefix().IsEmpty()) {
            xf.paths.insert(pattern.GetPrefix());
        }
    };

    // Walk the expression to map.
    pathExpr.Walk(logic, mapRef, mapPattern);
    return pathExpr;
}

TF_REGISTRY_FUNCTION(VtValue)
{
    // Transforms for SdfPath
    VtRegisterTransform(_CollectPath);
    VtRegisterTransform(_PathReplacePrefix);

    // Transforms for SdfPathListOp
    VtRegisterTransform(_PathListOpCollectPathsXf);
    VtRegisterTransform(_PathListOpNamespaceEdit);

    // Transforms for SdfPathExpression
    VtRegisterTransform(_PathExprReplacePrefix);
    VtRegisterTransform(_CollectPrefixes);
}

// Helper for collecting all dependencies on a stage.
class _DependencyCollector
{
public:
    // Gets all dependencies (fields containing paths) for all objects on 
    // the given stage
    static _Dependencies GetDependencies(
        const UsdStageRefPtr &stage) 
    {
        TRACE_FUNCTION();
        _DependencyCollector impl;
        impl._Run(stage);
        return std::move(impl._result);
    }

private:
    WorkDispatcher _dispatcher;
    WorkSingularTask _consumerTask;

    struct _WorkQueueEntry {
        SdfPath composedObjectPath;
        _SpecWithPathBearingFieldsVector specsWithPathBearingFields;
        SdfPathSet paths;
    };

    tbb::concurrent_queue<_WorkQueueEntry> _workQueue;

    _Dependencies _result;

    explicit _DependencyCollector()
        : _consumerTask(_dispatcher, [this]() { _ConsumerTask(); }) {}

    void _Run(const UsdStageRefPtr &stage) {
        WorkWithScopedParallelism([this, &stage]() {
            const auto range = stage->GetPseudoRoot().GetAllDescendants();
            WorkParallelForEach(range.begin(), range.end(),
                [this](UsdPrim const &prim) { _VisitPrim(prim);});
            _dispatcher.Wait();
        });
    }

    void _VisitPrim(UsdPrim const &prim) {

        std::unordered_map<SdfPath, _WorkQueueEntry, TfHash> 
            workEntriesPerObject;

        // Use a resolver to get all of the prim's field opinions in strength
        // order. We collect information on fields that need to be updated as
        // part of the namespace edit.
        for(Usd_Resolver res(&(prim.GetPrimIndex())); 
                res.IsValid(); res.NextLayer()) {

            const SdfLayerRefPtr &layer = res.GetLayer();
            const SdfPath &primSpecPath = res.GetLocalPath();
            const PcpNodeRef node = res.GetNode();

            // Get any prim fields that might need to be updated.
            for (const TfToken &fieldName : layer->ListFields(primSpecPath)) {
                // Skip composition fields that are SdfPathListOp-valued 
                // (inherits, specializes): these are already tracked in 
                // PcpDependentNamespaceEdits::compositionFieldEdits.
                if (fieldName == SdfFieldKeys->InheritPaths ||
                    fieldName == SdfFieldKeys->Specializes) {
                    continue;
                }

                // Get the field's value.
                VtValue val;
                TF_VERIFY(layer->HasField(primSpecPath, fieldName, &val));

                // Collect any paths that might be present in this field.
                // If the returned value is empty, it means the field type
                // is not path-bearing and does not need updating.
                _CollectPathsXf xf = {node, SdfPathSet()};
                if (!VtValueTryTransform(val, xf).IsEmpty()) {
                    // Add or get the work entry for the composed prim path so
                    // we can add this spec's info to it.
                    _WorkQueueEntry &workEntry = workEntriesPerObject.try_emplace(
                        prim.GetPrimPath(), _WorkQueueEntry()).first->second;

                    for (const auto& path : xf.paths) {
                        // Map the path into stage namespace
                        SdfPath mappedPath = 
                            node.GetMapToRoot().MapSourceToTarget(path);
                        if (!mappedPath.IsEmpty()) {
                            workEntry.paths.insert(std::move(mappedPath));
                        }
                    }

                    // Add the prim spec info to the contributing prim specs for 
                    // this composed entry.
                    workEntry.specsWithPathBearingFields.push_back({
                        layer, 
                        primSpecPath, 
                        std::move(fieldName),
                        node
                    });
                }
            }

            // Get the names of properties that are locally authored on this
            // prim spec. 
            TfTokenVector primSpecPropertyNames;
            if (!layer->HasField(
                    primSpecPath,
                    SdfChildrenKeys->PropertyChildren,
                    &primSpecPropertyNames)) {
                continue;
            }

            // Now we look through property specs looking for any with 
            // fields to update.
            for (const TfToken &propName : primSpecPropertyNames) {
                // Get the property spec path in this layer.
                SdfPath localPropPath = primSpecPath.AppendProperty(propName);

                // Iterate through the property's fields.
                for (const TfToken &fieldName : layer->ListFields(localPropPath)) {
                    // Get the field's value.
                    VtValue val;
                    TF_VERIFY(layer->HasField(localPropPath, fieldName, &val));

                    // Collect any paths that might be present in this field.
                    // If the returned value is empty, it means the field type
                    // is not path-bearing and does not need updating.
                    _CollectPathsXf xf = {node, SdfPathSet()};
                    if (VtValueTryTransform(val, xf).IsEmpty()) {
                        continue;
                    }

                    // Add or get the work entry for the composed property path so
                    // we can add this spec's info to it.
                    _WorkQueueEntry &workEntry = workEntriesPerObject.try_emplace(
                        prim.GetPrimPath().AppendProperty(propName), 
                        _WorkQueueEntry()).first->second;

                    for (const auto& path : xf.paths) {
                        // Map the path into stage namespace
                        SdfPath mappedPath = 
                            node.GetMapToRoot().MapSourceToTarget(path);
                        if (!mappedPath.IsEmpty()) {
                            workEntry.paths.insert(std::move(mappedPath));
                        }
                    }

                    // Add the prop spec info to the contributing prop specs for 
                    // this composed entry.
                    workEntry.specsWithPathBearingFields.push_back({
                        layer, 
                        localPropPath, 
                        std::move(fieldName),
                        node
                    });
                }
            }
        }

        // With all the dependency work done for this prim and its properties, 
        // we can queue each one up to be added to the result.
        if (!workEntriesPerObject.empty()) {
            for (auto &[path, workEntry] : workEntriesPerObject) {
                // Copy the composed object path into the entry before moving
                // it to the queue.
                workEntry.composedObjectPath = path;
                _workQueue.push(std::move(workEntry));
            }
            _consumerTask.Wake();
        }
    };

    void _ConsumerTask() {
        _WorkQueueEntry queueEntry;
        while (_workQueue.try_pop(queueEntry)) {
            // Store the specs for the composed object in _result.
            _result.composedObjectToSpecsWithPathBearingFieldsMap.emplace(
                queueEntry.composedObjectPath, 
                std::move(queueEntry.specsWithPathBearingFields));

            // Add the mapping of each path to the composed prim/property
            // which we now know has a path-bearing field that maps to it.
            for (const auto &targetedPath : queueEntry.paths) {
                _result.targetedPathToTargetingSpecPathsTable[targetedPath]
                    .push_back(queueEntry.composedObjectPath);
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
    TRACE_FUNCTION();

    _ProcessEditsIfNeeded();
    TF_VERIFY(_processedEdit);

    // We create a namespace edit change block for each stage that is edited
    // by this editor. This is so the stage can appropriately parse the layer 
    // and PcpCache changes that are processed by the UsdStage and classify 
    // namespace edits in the ObjectsChanged notice.
    //
    // Note that this only includes the primary edit stage and any explicitly
    // added dependent stage. We do NOT attempt to search for open stages that
    // just happen to be affected by the edits that will be applied.
    std::vector<UsdStage::_NamespaceEditsChangeBlock> namespaceEditChangeBlocks;
    namespaceEditChangeBlocks.reserve(_dependentStages.size() + 1);

    auto addChangesForStage = [&](const UsdStageRefPtr &stage) {
        // The expected namespace changes to prim index or property paths are 
        // stored per PcpCache in the dependent namespace edits so we look up 
        // the expected changes for this stage's cache.
        const auto *pathChangesForStage = TfMapLookupPtr(
            _processedEdit->dependentStageNamespaceEdits.dependentCachePathChanges, 
            stage->_GetPcpCache());
        if (!pathChangesForStage) {
            return;
        }

        // Convert the expected namespace changes to what's needed for the
        // change block by removing old paths that don't exist on the stage. 
        UsdStage::_NamespaceEditsChangeBlock::ExpectedNamespaceEditChangeVector
            changeBlockExpectedChanges;
        changeBlockExpectedChanges.reserve(pathChangesForStage->size());
        for (const auto &change : *pathChangesForStage) {
            if (change.oldPath.IsPropertyPath()) {
                UsdProperty oldProp= stage->GetPropertyAtPath(change.oldPath);
                if (!oldProp) {
                    continue;
                }
                // Note that we don't need to compute and add a prim stack for 
                // properties (as we do below for prims) as the change handling 
                // in UsdStage doesn't need it.
                changeBlockExpectedChanges.push_back({
                    change.oldPath, change.newPath});
            } else {
                    UsdPrim oldPrim = stage->GetPrimAtPath(change.oldPath);
                if (!oldPrim) {
                    continue;
                }
                // We compute the current prim stack for the old prim before
                // edits are applied to be compared with the prim stack after 
                // the edits are applied.
                changeBlockExpectedChanges.push_back({
                    change.oldPath, change.newPath, oldPrim.GetPrimStack()});
            }
        }

        // Create the change block for this stage's expected prim/property 
        // namespace changes.
        namespaceEditChangeBlocks.emplace_back(
            stage, std::move(changeBlockExpectedChanges));
    };

    addChangesForStage(_stage);
    for (const auto &stage : _dependentStages) {
        addChangesForStage(stage);
    }

    const bool success = _processedEdit->Apply();

    // Always clear the processed edits after applying them.
    _ClearProcessedEdits();
    return success;
}

bool 
UsdNamespaceEditor::CanApplyEdits(std::string *whyNot) const
{
    CanApplyResult ret = CanApplyEdits();
    if (!ret.errors.empty()) {
        *whyNot = _GetErrorString(ret.errors);
    }
    return bool(ret);
}

UsdNamespaceEditor::CanApplyResult
UsdNamespaceEditor::CanApplyEdits() const
{
    TRACE_FUNCTION();

    _ProcessEditsIfNeeded();
    TF_VERIFY(_processedEdit);

    return _processedEdit->CanApply();
}

SdfLayerHandleVector
UsdNamespaceEditor::GetLayersToEdit() {
    TRACE_FUNCTION();

    // Ensure the edit can be applied. Note that CanApplyEdits will process the 
    // edit if needed, which is why we don't have to process it here.
    std::string errorMsg;
    if (!CanApplyEdits(&errorMsg)) {
        TF_CODING_ERROR("Cannot get layers to edit because edit "
            "cannot be applied due to the following errors: %s", errorMsg.c_str());
        return SdfLayerHandleVector();
    }
    
    return _processedEdit->layersToEdit;
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

    void _GatherPathBearingFieldEdits();

    void _GatherPathBearingFieldEditsForStage(
        const UsdStageRefPtr &stage,
        const SdfPath &oldPath,
        const SdfPath &newPath,
        bool willAuthorRelocates);

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
    std::string *whyNot = nullptr,
    std::string *warning = nullptr) 
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
    // Property to edit must not be a built-in schema property.
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
    TRACE_FUNCTION();
    
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

    // Gather all the edits that need to be made to path-bearing fields in 
    // order to "fix up" paths that refer to the namespace edited object.
    // This must run after _GatherDependentStageEdits() as it relies on 
    // processed data gathered by that function.
    _GatherPathBearingFieldEdits();
}

bool
UsdNamespaceEditor::_EditProcessor::_ProcessNewPath()
{
    TRACE_FUNCTION();

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
    TRACE_FUNCTION();

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
    TRACE_FUNCTION();

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
    TRACE_FUNCTION();

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
UsdNamespaceEditor::_EditProcessor::_GatherPathBearingFieldEdits()
{
    TRACE_FUNCTION();

    // dependentCachePathChanges holds the composed prim/property path changes 
    // in each cache's stage namespace caused by the primary edit. 
    const auto &dependentCachePathChanges =
        _processedEdit->dependentStageNamespaceEdits.dependentCachePathChanges;

    // Gather edits for the primary stage.
    const auto primaryIt =
        dependentCachePathChanges.find(_stage->_GetPcpCache());
    if (primaryIt != dependentCachePathChanges.end()) {
        for (const auto &edit : primaryIt->second) {
            _GatherPathBearingFieldEditsForStage(
                _stage, edit.oldPath, edit.newPath,
                _processedEdit->willAuthorRelocates);
        }
    }

    // Gather edits for all dependent stages.
    for (const UsdStageRefPtr &stage : _dependentStages) {
        const auto it = dependentCachePathChanges.find(stage->_GetPcpCache());
        if (it == dependentCachePathChanges.end()) {
            continue;
        }
        // Relocates are only authored on the primary edit's stage, so 
        // willAuthorRelocates must be false for dependent stages so that paths 
        // that would need relocates produce warnings for the dependent stage.
        for (const auto &edit : it->second) {
            _GatherPathBearingFieldEditsForStage(
                stage, edit.oldPath, edit.newPath,
                /*willAuthorRelocates=*/ false);
        }
    }
}

void
UsdNamespaceEditor::_EditProcessor::_GatherPathBearingFieldEditsForStage(
    const UsdStageRefPtr &stage,
    const SdfPath &oldPath,
    const SdfPath &newPath,
    bool willAuthorRelocates)
{
    TRACE_FUNCTION();
    // Gather all the dependencies from stage namespace path to prims or 
    // properties with path-bearing fields that include that namespace path.
    _Dependencies deps = _DependencyCollector::GetDependencies(stage);

    // With all the dependencies we need to determine which fields are 
    // affected by this particular edit. If the edit was to a prim, the 
    // affected paths will be any descendants of the original prim path, 
    // thus we have to get all fields targeting any descendant of the 
    // changed path.
    SdfPathSet objectPathsWithAffectedFields;
    const auto range = 
        deps.targetedPathToTargetingSpecPathsTable.FindSubtreeRange(oldPath);
    for (auto it = range.first; it != range.second; ++it) {
        const SdfPathVector &paths = it->second;
        objectPathsWithAffectedFields.insert(paths.begin(), paths.end());
    }

    // Now for each prim/property, gather the edits that need to be made to
    // the layer specs in order to update the affected path-bearing fields.
    for (const SdfPath &path : objectPathsWithAffectedFields) {

        // Every path listed as dependency must have a list of prim or property
        // specs that have path-bearing fields.
        const _SpecWithPathBearingFieldsVector *specs = 
            TfMapLookupPtr(deps.composedObjectToSpecsWithPathBearingFieldsMap, 
                path);
        if (!TF_VERIFY(specs)) {
            continue;
        }

        // First we're only going to look at specs that originated from nodes 
        // within the local layer stack, since these specs can be edited to 
        // update the relevant path-bearing fields.
        for (const auto &specInfo : *specs) {
            // We only care about local and variant opinions here as other arcs
            // within the local layer stack will have their specs fixed up 
            // directly.
            if (specInfo.originatingNode.GetPath().StripAllVariantSelections() 
                != specInfo.originatingNode.GetRootNode().GetPath()) {
                continue;
            }

            // We only want opinions from the root layer stack. Since the specs
            // are in strength order, if we have reached a layer not in the 
            // local layer stack, we are done.
            if (!stage->HasLocalLayer(specInfo.layer)) {
                break;
            }

            // Look for an existing in-progress edit for this spec and field name.
            SdfSpecHandle spec =
                specInfo.layer->GetObjectAtPath(specInfo.path);
            auto existingIt = std::find_if(
                _processedEdit->pathBearingFieldEdits.begin(),
                _processedEdit->pathBearingFieldEdits.end(),
                [&](const _ProcessedEdit::PathBearingFieldEdit &e) {
                    return e.spec == spec &&
                           e.fieldName == specInfo.fieldName;
                });

            // If there is an existing in-progress edit, use its newFieldValue 
            // as the base for transformation.
            const VtValue &baseValue =
                existingIt != _processedEdit->pathBearingFieldEdits.end() ?
                    existingIt->newFieldValue : specInfo.GetFieldValue();

            // Try to update the field's value.
            VtValue modifiedValue = VtValueTryTransform(
                baseValue, _NamespaceEditXf {
                    oldPath, newPath });

            // If the field was modified, save its new value.
            if (baseValue != modifiedValue) {
                if (existingIt != _processedEdit->pathBearingFieldEdits.end()) {
                    existingIt->newFieldValue = std::move(modifiedValue);
                } else {
                    _processedEdit->pathBearingFieldEdits.push_back(
                        {std::move(spec), specInfo.fieldName,
                         std::move(modifiedValue)});
                }
            }
        }

        // If the edit will author relocates for the primary edit, then the 
        // fields authored across composition arcs will also be mapped by the
        // relocation. 
        if (willAuthorRelocates) {
            continue;
        }

        // For fields that are contributed by specs that originate across
        // arcs below the root node, we can't edit these specs directly. 
        // Instead we'd need relocates to map these paths. In this case we 
        // compose the field value, excluding the root node opinions, to see if 
        // any of them would be affected by the namespace edit and therefore 
        // require a relocates.
        std::map<TfToken, SdfPathVector> fieldsRequireRelocates;

        // Iterate in weakest to strongest applying each list op to get the 
        // composed targets below the root node.
        for (auto rIt = specs->rbegin(); rIt != specs->rend(); 
                ++rIt) {
            const _SpecWithPathBearingFieldsInfo &specInfo = *rIt;

            // Stop when we hit a spec originating from the root node
            if (specInfo.originatingNode.IsRootNode()) {
                break;
            }

            // Skip if the spec is from a local variant, as we have handled 
            // it above and it won't require a relocate.
            if (specInfo.originatingNode.GetPath().StripAllVariantSelections() 
                == specInfo.originatingNode.GetRootNode().GetPath()) {
                continue;
            }

            SdfPath pathAtIntroduction = rIt->originatingNode.GetPathAtIntroduction();
            SdfPath translatedPathAtIntroduction = 
                rIt->originatingNode.GetMapToRoot().MapSourceToTarget(
                pathAtIntroduction);

            // Before we check whether the field requires relocates, first check
            // whether the edit could possibly affect anything across this 
            // composition arc. If the edit is strictly to the source prim of 
            // the composition arc or its ancestors, that edit will not 
            // affect any paths coming from the target of the arc since they are 
            // outside the scope of that arc. If so, we can skip this spec.
            if (translatedPathAtIntroduction.HasPrefix(oldPath)) {
                continue;
            } 

            // Get the current value of the field.
            VtValue fieldValue = specInfo.GetFieldValue();

            // Get the list of paths associated with the field.
            _CollectPathsXf xf;
            if (!fieldValue.IsEmpty()) {
                VtValueTryTransform(fieldValue, xf);
            }

            // Translate the paths into stage namespace.
            for (const SdfPath& path : xf.paths) {
                const SdfPath translatedPath = 
                    rIt->originatingNode.GetMapToRoot().MapSourceToTarget(path);
                
                // Skip paths that don't map. Also skip paths that aren't 
                // affected by the namespace edit; we don't care about these 
                // either.
                if (translatedPath.IsEmpty() ||
                    !translatedPath.HasPrefix(oldPath)) {
                    continue;
                }
                fieldsRequireRelocates[specInfo.fieldName].push_back(translatedPath);
            }
        }

        // If any of the fields require relocates, we can't fix them up.
        // Errors in fixing up fields do not prevent us from applying namespace
        // edits, but we report them as warnings.
        for (const auto& it : fieldsRequireRelocates) {
            _processedEdit->warnings.push_back(TfStringPrintf(
                "Fixing the paths %s in the field %s for the object at '%s' would require "
                "'%s'to be relocated but we do not introduce relocates for %s.",
                TfStringify(it.second).c_str(),
                it.first.GetText(),
                path.GetText(),
                oldPath.GetText(),
                oldPath.IsPrimPropertyPath() ?
                    "properties ever" :
                    "prims that do not have opinions across composition arcs"
                ));
        }
    }
}

UsdNamespaceEditor::CanApplyResult
UsdNamespaceEditor::_ProcessedEdit::CanApply() const
{
    // Only errors that prevent the object from being moved or deleted in stage
    // namespace prevent the edits from being applied. Errors in edits like 
    // relationship target or connection path fixups do not prevent the rest
    // of the edits from being applied and are reported as warnings instead.
    CanApplyResult result;
    result.warnings = warnings;  
    result.errors = errors;

    return result;
}

static bool 
_ApplyLayerSpecMove(
    const SdfLayerHandle &layer, const SdfPath &oldPath, const SdfPath &newPath)
{
    TRACE_FUNCTION();

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
    TRACE_FUNCTION();

    // This is to try to preemptively prevent partial edits when if any of the 
    // necessary specs can't be renamed.
    CanApplyResult result = CanApply();
    if (!result) {
        TF_CODING_ERROR("Failed to apply edits to the stage "
            "because of the following errors: %s", 
            _GetErrorString(result.errors).c_str());
        return false;
    } else if (!result.warnings.empty()) {
        TF_WARN(
            "Encountered warnings when applying namespace edit: %s", 
            _GetErrorString(result.warnings).c_str());
    }

    // For both prim and property edits, the dependent stage edits are always 
    // computed for at least the primary stage so all necessary edits will be 
    // contained in the dependent stage computed edits. 
    SdfChangeBlock changeBlock;
    if (editDescription.IsPropertyEdit()) {
        // For property edits, we just apply the actual spec moves.
        for (const auto &[layer, editVec] :
                dependentStageNamespaceEdits.layerSpecMoves) {
            for (const auto &edit : editVec) {
                _ApplyLayerSpecMove(layer, edit.oldPath, edit.newPath);
            }
        }
    } else {
        // For prim edits, there are a couple steps:
        // First, we handle any composition arcs that need to be fixed up.
        for (const auto &edit : 
                dependentStageNamespaceEdits.compositionFieldEdits) {
            edit.layer->SetField(edit.path, edit.fieldName, edit.newFieldValue);
        }

        // Next, we apply the actual spec move. It's important to do this after 
        // the first step to avoid cases where a composition arc's target and 
        // local spec have changed e.g. a prim referencing a sibling when the
        // parent is moved or renamed.
        for (const auto &[layer, editVec] : 
                dependentStageNamespaceEdits.layerSpecMoves) {
            for (const auto &edit : editVec) {
                _ApplyLayerSpecMove(layer, edit.oldPath, edit.newPath);
            }
        }

        // Last, handle relocates. This step doesn't have to happen before spec 
        // moves because relocates are stored in the layer metadata.
        for (const auto &[layer, relocates] : 
                dependentStageNamespaceEdits.dependentRelocatesEdits) {
            layer->SetRelocates(relocates);
        }
    }

    // Perform any path-bearing field fixups necessary now that the namespace 
    // edits have been successfully performed.
    for (const PathBearingFieldEdit &edit : pathBearingFieldEdits) {
        // It's possible the spec no longer exists if the prim or property 
        // holding the field was deleted by the namespace edit operation itself.
        if (edit.spec) {
            edit.spec->SetField(edit.fieldName, edit.newFieldValue);
        }
    }

    return true;
}

void
UsdNamespaceEditor::_EditProcessor::_GatherDependentStageEdits()
{
    TRACE_FUNCTION();

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

    // Copy any warnings encountered into the processed edit.
    std::vector<std::string>& dependentStageWarnings = 
        _processedEdit->dependentStageNamespaceEdits.warnings;
    if (!dependentStageWarnings.empty()) {
        _processedEdit->warnings.insert(_processedEdit->warnings.end(), 
            dependentStageWarnings.begin(), dependentStageWarnings.end());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

