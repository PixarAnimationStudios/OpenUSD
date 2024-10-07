//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/dependentNamespaceEditUtils.h" 
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/dependencies.h"
#include "pxr/usd/pcp/layerRelocatesEditBuilder.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Debug helper for indenting debug messages.
struct _DebugScope {
    _DebugScope() {
        if (ARCH_UNLIKELY(TfDebug::IsEnabled(PCP_NAMESPACE_EDIT))) {
            _debugEnabled = true;
            ++_indentLevel;
        } 
    }
    ~_DebugScope() {
        if (ARCH_UNLIKELY(_debugEnabled)) {
            --_indentLevel;
        } 
    }
    _DebugScope(const _DebugScope &) = delete;

    static void PrintDebug(const std::string &msg) {
        const std::string indent = std::string(_indentLevel * 2, ' ');
        const std::string formattedMsg = 
            indent + TfStringReplace(TfStringTrim(msg), "\n", "\n" + indent) + "\n";
        TfDebug::Helper().Msg(formattedMsg);
    }

    static void PrintDebug(const char *fmt, ...) {
        va_list ap; va_start(ap, fmt);
        _DebugScope::PrintDebug(TfVStringPrintf(fmt, ap));
        va_end(ap);
    }

private:
    bool _debugEnabled = false;
    static int _indentLevel;
};

int _DebugScope::_indentLevel = 0;

#define _PRINT_DEBUG(...) \
    if (ARCH_UNLIKELY(TfDebug::IsEnabled(PCP_NAMESPACE_EDIT))) { \
        _DebugScope::PrintDebug(__VA_ARGS__); \
    } 

#define _PRINT_DEBUG_SCOPE(...) \
    _PRINT_DEBUG(__VA_ARGS__) \
    _DebugScope __debugIndentScope__;

// Specializes nodes may appear in a prim index twice as the nodes are copied
// and "propagated" to be direct children of the root node, for strength 
// ordering purposes, regardless of where the node was originally introduced. 
// The unpropagated node is needed to determine how the node was introduced and
// this function gets that node.
static PcpNodeRef 
_GetUnpropagatedSpecializesNode(const PcpNodeRef &node)
{
    if (!TF_VERIFY(node.GetArcType() == PcpArcTypeSpecialize)) {
        return PcpNodeRef();
    }

    // All specializes nodes are propagated to be direct children of the root
    // node (if they weren't introduced under the root node to begin with). So,
    // if parent is not the root node, it must be the unpropagated specializes 
    // node.
    if (!node.GetParentNode().IsRootNode()) {
        return node;
    }

    // Otherwise this is the propagated specializes node. This may still be its
    // own unpropagated node if
    // 1) its origin node is its parent, i.e. it was directly introduced by the
    //    root node, or
    // 2) its origin node has a different Sdf site which means it is an implied 
    //    specializes node that was implied to the root.
    // Both these cases do not require the introduced specializes to be 
    // propagated to the root so the "unpropagated" node IS the "propagated" 
    // node.
    const PcpNodeRef originNode = node.GetOriginNode();
    if (originNode == node.GetParentNode() ||
            originNode.GetSite() != node.GetSite()) {
        return node;
    }

    // Otherwise this node is the propagated node that was copied from its 
    // origin node which is where the specializes was introduced to the graph.
    // The origin node is the upropagated node.
    return originNode;
}

// Inverse of _GetUnpropagatedSpecializesNode; this gets us the corresponding
// propagated specializes node (the one that can contribute specs) from the
// an unpropagated specializes node (the one that lives where it was 
// introduced).
static PcpNodeRef 
_GetPropagatedSpecializesNode(const PcpNodeRef &node)
{
    if (!TF_VERIFY(node.GetArcType() == PcpArcTypeSpecialize)) {
        return PcpNodeRef();
    }

    // All specializes nodes are propagated to be direct children of the root
    // node (if they weren't introduced under the root node to begin). So if
    // parent is not the root node, it can't be a propagated specializes.
    if (node.GetParentNode().IsRootNode()) {
        return node;
    }

    // We have an unpropagated specializes node that must've been propagated
    // to the root, so we have to find the child node of the root node that
    // was propagated from this node.
    //
    // All contributing specializes nodes are at the end of the root node's
    // child range because of strength ordering so we iterate in reverse and can
    // stop at the first non-specializes node.
    const PcpNodeRef rootNode = node.GetRootNode();
    for (const PcpNodeRef &child : rootNode.GetChildrenReverseRange()) {
        if (child.GetArcType() != PcpArcTypeSpecialize) {
            break;
        }
        // The propagated node will have this node as its origin but it must
        // also share the same Sdf site. Otherwise we could erroneously pick up
        // the implied specializes of this node if this implied a specializes to
        // the root.
        const PcpNodeRef childOrigin = child.GetOriginNode();
        if (childOrigin == node && childOrigin.GetSite() == node.GetSite()) {
            return child;
        }
    }

    // XXX: This case should probably be a coding error.
    return PcpNodeRef();
}

// Helper structure for computing and holding the information about how a node
// was introduced. Note that this is primarily here to abstract away the 
// complexity of determining actual node introduction for specializes nodes.
struct _NodeIntroductionInfo {
    // This node
    PcpNodeRef node;

    // The node that introduced this node. This is almost always the parent node
    // but in the case of a specializes node, it most likely will not be because
    // of "specializes to root child" propagation.
    PcpNodeRef introducingNode;

    // The path of this node when it was introduced to the prim index.
    SdfPath pathAtIntroduction;

    // The path in the introducing node that introduced this node into the tree.
    SdfPath introducingPath;

    _NodeIntroductionInfo(const PcpNodeRef &node_) :
        node(node_)
    {
        // Introducing info is populated from the unpropagated node which, for
        // all cases but specializes, is the node itself
        auto populateIntroducingInfo = 
            [this](const PcpNodeRef &unpropagatedNode) {
                introducingNode = unpropagatedNode.GetParentNode();

                // Even though we need to use the pre-propagation specializes
                // node to determine introduction, we always want to work with
                // the propagated node in the end as it is the active node and
                // has the correct strength order. So make sure the introducing
                // node is the propagated specializes if it is a specializes.
                if (introducingNode && 
                        introducingNode.GetArcType() == PcpArcTypeSpecialize) {
                    introducingNode = 
                        _GetPropagatedSpecializesNode(introducingNode);
                }
                pathAtIntroduction = unpropagatedNode.GetPathAtIntroduction();
                introducingPath = unpropagatedNode.GetIntroPath();
            };

        if (node.GetArcType() == PcpArcTypeSpecialize) {
            // For specializes, the nodes will be propagated to the root (if not
            // directly parented to the root already) for strength ordering 
            // purposes. But we need the node before it was propagated to 
            // determine how it was introduced.
            populateIntroducingInfo(_GetUnpropagatedSpecializesNode(node));
        } else {
            // For all other node types, the node is simply introduced by its
            // parent.
            populateIntroducingInfo(node);
        }
    }
};

// Scratch structure for processing prim move edits.
struct _SpecMovesScratch {
    // List of prim spec move edit paths.
    std::vector<PcpDependentNamespaceEdits::SpecMoveEditDescription> specMoves;

    // List of paths whose specs can optionally be deleted if no other edit 
    // wants to move the spec to another path. These may be present because of
    // implied classes and are processed during finalization of the dependent
    // edits.
    std::vector<SdfPath> optionalSpecDeletes;
};
using _LayerSpecMovesScratch =
    std::unordered_map<SdfLayerHandle, _SpecMovesScratch, TfHash>;

// Class used to process all the edits necessary at dependent node sites in a 
// prim index given one or more initial node site edits.
class _PrimIndexDependentNodeEditProcessor 
{
public:
    _PrimIndexDependentNodeEditProcessor(
        const PcpPrimIndex *primIndex,
        PcpDependentNamespaceEdits *edits,
        _LayerSpecMovesScratch *layerSpecMovesScratch)
        : _primIndex(primIndex)
        , _edits(edits)
        , _layerSpecMovesScratch(layerSpecMovesScratch) 
        {}

    // Adds a task for processing the spec move edit at the given node.
    void AddProcessEditsAtNodeTask(
        const PcpNodeRef &node,
        const SdfPath &oldPath, 
        const SdfPath &newPath,
        bool willBeRelocated) {
        _InsertNodeTask({node, oldPath, newPath, 
            /* isImpliedClassTask = */ false, willBeRelocated});
    }

    // Processes all tasks producing all dependent edits for the prim index
    void ProcessTasks() {
        // _ProcessNextNodeTask returns false if there are no more tasks to 
        // process.
        while (_ProcessNextNodeTask());
    }

private:
    struct _NodeTask {
        PcpNodeRef node;
        SdfPath oldPath;
        SdfPath newPath;
        bool isImpliedClassTask = false;
        bool willBeRelocated = false;
    };

    friend std::ostream &
    operator<<(std::ostream &out, const _NodeTask &nodeTask) 
    {
        out << nodeTask.node << "\n    move <" << nodeTask.oldPath << 
            "> to <" << nodeTask.newPath << ">";
        if (nodeTask.isImpliedClassTask) {
            out << " (isImpliedClassTask)";
        }
        if (nodeTask.willBeRelocated) {
            out << " (willBeRelocated)";
        }
        return out;
    }

    void _InsertNodeTask(_NodeTask &&nodeTask);

    bool _ProcessNextNodeTask();

    void _ProcessNextImpliedClass(
        const _NodeTask &nodeTask);

    void _ProcessDependentNodePathAtIntroductionChange(
        const _NodeIntroductionInfo &nodeIntroInfo,
        const SdfPath &oldPath,
        const SdfPath &newPath);

    void _AddSpecMoveEdits(const _NodeTask &nodeTask);

    bool _HasUneditedUpstreamSpecConflicts(
        const PcpNodeRef &siteEditNode,
        const SdfPath &siteEditPath);

    bool _HasConflictingSpecsInUneditedNodes(
        const PcpNodeRef &siteEditNode,
        PcpNodeRef *firstConflictingNode);

    bool _HasConflictingChildSpecsInUneditedNodes(
        const PcpNodeRef &siteEditNode,
        const TfToken &childName,
        PcpNodeRef *firstConflictingNode);

    const PcpPrimIndex *_primIndex;
    PcpDependentNamespaceEdits *_edits;
    _LayerSpecMovesScratch *_layerSpecMovesScratch;

    std::vector<_NodeTask> _nodeTasks;       
    std::unordered_set<PcpNodeRef, TfHash> _nodesVisitedByEditTasks;
};

void
_PrimIndexDependentNodeEditProcessor::_InsertNodeTask(_NodeTask &&nodeTask) 
{
    _PRINT_DEBUG(
        "Inserting node task: %s", TfStringify(nodeTask).c_str());

    if (_nodeTasks.empty()) {
        _nodeTasks.push_back(std::move(nodeTask));
    } else {
        // The node task list is sorted from strongest node to weakest node
        // and we remove nodes from the end when processing, thus always
        // processing weaker nodes before stronger ones. This is necessary
        // for correctly determining whether a spec move in stronger node 
        // will have a conflict with specs from a weaker node that is not
        // being edited.
        //
        // XXX: Note that this ordering relies on PcpNodeRef's less than
        // operator and the fact that nodes in the same finalized prim index
        // are ordered from strongest to weakest even though the less than
        // operator makes no promise of strength order. It would be prudent
        // to add a comparator for nodes in a finalized prim index that does
        // guarantee strength ordering that we would use here instead.
        auto insertIt = std::lower_bound(
            _nodeTasks.begin(), _nodeTasks.end(), nodeTask, 
                [](const auto &lhs, const auto &rhs) {
                    return lhs.node < rhs.node;
                });
        _nodeTasks.insert(insertIt, std::move(nodeTask));
    }
}


// Helper for getting the edits that need to made to the list op value of one of
// the various composition fields to change oldPath to newPath at the give site.
template <class ListOpValueType>
static void 
_ProcessListOpCompositionFieldEditsAtSite(
    const PcpLayerStackRefPtr &siteLayerStack,
    const SdfPath &sitePath,
    const TfToken &fieldName,
    const SdfPath &oldPath,
    const SdfPath &newPath,
    std::vector<PcpDependentNamespaceEdits::CompositionFieldEdit> *edits)
{
    for (const auto &layer : siteLayerStack->GetLayers()) {
        SdfListOp<ListOpValueType> listOp;
        if (!layer->HasField(sitePath, fieldName, &listOp)) {
            continue;
        }

        auto modifyCallback = [&](const ListOpValueType &item) {
            const SdfPath &path = [](const ListOpValueType &item) {
                if constexpr (std::is_same_v<ListOpValueType, SdfPath>) {
                    return item;
                } else {
                    return item.GetPrimPath();
                }
            }(item);

            // If the path is empty, there's nothing to modify.
            if (path.IsEmpty()) {
                return std::optional<ListOpValueType>(item);
            }
            // If the path doesn't start with the old path, it is not 
            // affected and returned unmodified.
            if (!path.HasPrefix(oldPath)) {
                return std::optional<ListOpValueType>(item);
            }
            // Otherwise we found an affected path. If we've deleted
            // the old path, delete this target item.
            if (newPath.IsEmpty()) {
                return std::optional<ListOpValueType>();
            }
            // Otherwise update the path of this target item for the 
            // new path.
            SdfPath modifiedPath = path.ReplacePrefix(oldPath, newPath);
            if constexpr (std::is_same_v<ListOpValueType, SdfPath>) {
                return std::optional<ListOpValueType>(modifiedPath);
            } else {
                ListOpValueType modified(item);
                modified.SetPrimPath(modifiedPath);
                return std::optional<ListOpValueType>(modified);
            }
        };

        if (listOp.ModifyOperations(modifyCallback)) {
            _PRINT_DEBUG(
                "Adding %s composition field edit at @%s@<%s>:\n"
                "  new %s value = %s",
                fieldName.GetText(),
                layer->GetIdentifier().c_str(),
                sitePath.GetText(),
                fieldName.GetText(),
                TfStringify(listOp).c_str());

            edits->push_back({
                layer, sitePath, fieldName, VtValue::Take(listOp)
            });         
        }
    }
}

// Helper for getting the edits that need to made to the relocates layer 
// metadata in the given layer stack to move oldPath to newPath.
static void
_ProcessRelocatesFieldEditsAtSite(
    const PcpLayerStackRefPtr &siteLayerStack,
    const SdfPath &oldPath,
    const SdfPath &newPath,
    PcpDependentNamespaceEdits::LayerRelocatesEdits *edits)
{
    // We may have to update the relocates for any layer in the introducing node
    // layer stack that has any relocates.
    for (const auto &layer : siteLayerStack->GetLayers()) {
        if (!layer->HasRelocates()) {
            continue;
        }

        // Update the relocates paths to move any that are affected by the old
        // path to use the new path. The layer relocates builder utility makes
        // sure to remove relocates that effectively deleted by this update.
        //
        // Since all relocates are defined in the same metadata field on the 
        // layer regardless of the prim paths they affect, we may already have
        // edits to this layer's relocates from a different dependency. We need 
        // to add any updates to these existing edits so that we don't undo them
        // if present.
        if (auto it = edits->find(layer); it != edits->end()) {
            if (Pcp_ModifyRelocates(&it->second, oldPath, newPath)) {
                _PRINT_DEBUG_SCOPE(
                    "Updating layer metadata relocates for layer @%s@ to:",
                    layer->GetIdentifier().c_str());
                _PRINT_DEBUG(TfStringify(it->second).c_str());
            }            
        } else {
            SdfRelocates editedRelocates = layer->GetRelocates();
            if (Pcp_ModifyRelocates(&editedRelocates, oldPath, newPath)) {
                _PRINT_DEBUG_SCOPE(
                    "Updating layer metadata relocates for layer @%s@ to:",
                    layer->GetIdentifier().c_str());
                _PRINT_DEBUG(TfStringify(editedRelocates).c_str());

                edits->emplace(layer, std::move(editedRelocates));
            }
        }
    }
}

// Processes necessary edits when an edit at a node will change its path at 
// introduction.
void 
_PrimIndexDependentNodeEditProcessor::_ProcessDependentNodePathAtIntroductionChange(
    const _NodeIntroductionInfo &nodeIntroInfo,
    const SdfPath &oldPath,
    const SdfPath &newPath)
{
    // When an edit affects a node's path at introduction, then we can try to
    // keep the arc that introduced the node as still composing this arc by
    // updating the introducing composition field to target the new path.
    const auto &introducingLayerStack = 
        nodeIntroInfo.introducingNode.GetLayerStack();
    switch (nodeIntroInfo.node.GetArcType()) {
    case PcpArcTypeReference:
        _ProcessListOpCompositionFieldEditsAtSite<SdfReference>(
            introducingLayerStack, nodeIntroInfo.introducingPath, 
            SdfFieldKeys->References, oldPath, newPath, 
            &_edits->compositionFieldEdits);
        break;
    case PcpArcTypePayload:
        _ProcessListOpCompositionFieldEditsAtSite<SdfPayload>(
            introducingLayerStack, nodeIntroInfo.introducingPath, 
            SdfFieldKeys->Payload, oldPath, newPath, 
            &_edits->compositionFieldEdits);
        break;
    case PcpArcTypeInherit:
        _ProcessListOpCompositionFieldEditsAtSite<SdfPath>(
            introducingLayerStack, nodeIntroInfo.introducingPath, 
            SdfFieldKeys->InheritPaths, oldPath, newPath, 
            &_edits->compositionFieldEdits);
        break;
    case PcpArcTypeSpecialize:
        _ProcessListOpCompositionFieldEditsAtSite<SdfPath>(
            introducingLayerStack, nodeIntroInfo.introducingPath,
            SdfFieldKeys->Specializes, oldPath, newPath, 
            &_edits->compositionFieldEdits);
        break;
    case PcpArcTypeVariant:
        // There is nothing that needs to be done for variant arc as there
        // is no explicit path to the variant that needs to be updated. The
        // parent node site will always be the variant node path with the
        // final variant selection stripped.
        return;
    case PcpArcTypeRelocate:
        // Relocates are slightly different in that the arc is introduced by
        // the presence of relocates metadata on the layer. We can update this
        // layer metadata to refer to the new path when composing relocates.
        _ProcessRelocatesFieldEditsAtSite(
            introducingLayerStack, oldPath, newPath, 
            &_edits->dependentRelocatesEdits);
        break;
    default:
        TF_CODING_ERROR("Unhandled composition arc");
        break;
    };

    // If the introduced path has been moved to a new existing path, just
    // updating the introducing composition arc is all that is needed to 
    // composed the same specs from the new location...
    if (!newPath.IsEmpty()) {
        return;
    }

    // ...but if the introduced path has been deleted, that results in removing
    // the arc to this node and we have to clean up (i.e. delete) specs in
    // the introducing node that are meant to compose over namespace 
    // children of the now deleted composition arc.
    _PRINT_DEBUG_SCOPE("Deleted composition arc to <%s> introduced at path <%s> in "
        "node %s. Must remove specs in introducing node that compose "
        "over deleted child specs.",
        nodeIntroInfo.pathAtIntroduction.GetText(),
        nodeIntroInfo.introducingPath.GetText(),
        TfStringify(nodeIntroInfo.introducingNode).c_str());

    if (nodeIntroInfo.node.IsDueToAncestor()) {
        _PRINT_DEBUG("Introduced node is ancestral; adding task to delete "
            "specs at introducing node %s",
            TfStringify(nodeIntroInfo.introducingNode).c_str());

        // If the node is due a ancestral prim index then it is already
        // the namespace descendant of the deleted composition arc. So we 
        // just add task to delete the corresponding mapped specs at the 
        // introducing node (which are just at the introducing node's 
        // path)
        _InsertNodeTask({
            nodeIntroInfo.introducingNode, 
            nodeIntroInfo.introducingNode.GetPath(), SdfPath()});
    } else {
        _PRINT_DEBUG("Introduced node is direct; adding tasks to delete "
            "children specs at introducing node %s",
            TfStringify(nodeIntroInfo.introducingNode).c_str());

        // Otherwise, our node is a direct arc that has been deleted.
        // We don't want to delete the introducing node's specs just
        // because its composition arc disappeared (XXX: or do we? does
        // a referenced prim count as "defining" the prim that 
        // referenced it), but we do want to delete any child prim specs
        // of the introducing site that would compose over the namespace
        // children originally provided by this deleted arc.
        //
        // First we have to compute the names of all children as 
        // composed from the subtree starting at this arc.
        //
        // XXX: Do we need to do anything about prohibited child? It is
        // a composition error to have specs at the prohibited children
        // in the first place so ignoring them for now, but if these 
        // specs are present, they could now appear as valid namespace
        // children where they weren't before.
        //
        // XXX: Also another note: this only looks for immediate namespace 
        // children which typically automatically covers descendants of those
        // children. But in the case where a sibling composition arc is still
        // present that continues to compose a namespace child that would 
        // otherwise be deleted by this task (but cannot be because of the 
        // sibling arc), we do not recurse into grandchildren and so on to find
        // specs in the introducing node that would compose over namespace 
        // grandchildren that are defined by the now deleted arc.
        TfTokenVector children;
        PcpTokenSet prohibitedChildren;
        _primIndex->ComputePrimChildNamesInSubtree(
            nodeIntroInfo.node, &children, &prohibitedChildren);
        // Add a task for deleting each corresponding child path in
        // the introducing node.
        for (const TfToken &childName : children) {
            _InsertNodeTask({
                nodeIntroInfo.introducingNode, 
                nodeIntroInfo.introducingPath.AppendChild(childName), 
                SdfPath()});
        }
    }
}

// Adds spec move edits to the scratch space for layers affected by this node
// edit task.
void 
_PrimIndexDependentNodeEditProcessor::_AddSpecMoveEdits(
    const _NodeTask &nodeTask)
{
    const PcpNodeRef &node = nodeTask.node;
    
    // For obvious reasons we skip nodes that can't contribute specs. These will
    // typically be relocates nodes where we never allow specs at the relocation
    // source path to contribute opinions.
    if (!node.CanContributeSpecs()) {
        _PRINT_DEBUG(
            "Skipping spec edits for %s node %s which cannot contribute specs",
            TfStringify(node.GetArcType()).c_str(),
            TfStringify(node.GetSite()).c_str());
        return;
    }

    // Spec deletion may be optional when the deletion is due to an implied 
    // class dependency. Here's an example:
    // layer1
    //   /Class (will be directly inherited)
    //      /Child
    //
    //   /Instance1 (inherits = /Class)
    //
    // layer2
    //   /Class (will be an implied inherit from across /Prim1's reference)
    //      /Child
    //
    //   /Prim1 (references = @layer1@</Instance1>)
    // 
    // If were to start with an edit in layer1 to move /Class/Child to 
    // /MovedChild, that means we move the original spec outside of the scope of 
    // /Instance1's inherit to /Class. When we then process the implied class
    // dependency of /Class/Child in layer2, we *could* process it as a move 
    // of /Class/Child to /MovedChild as well, but we instead process it as
    // a delete as there are no prim indexes that would compose the specs from
    // layer2's /MovedChild.
    // 
    // However, if in layer1 we also had
    //   /Instance2 (inherits = /Class/Child)
    // and in layer2 we also had
    //   /Prim2 (references = @layer1@</Instance2>)
    // 
    // Now, a move of /Class/Child to /MovedChild in layer1 would process 
    // additional edits to ones already stated above. First, the inherits path
    // in /Instance2 would be updated to point at /MovedChild instead of 
    // /Class/Child. Second, we'd also process an implied class dependendency in
    // layer2 to move /Class/Child to /MovedChild so that /Prim2 still composes
    // those implied class specs.
    // 
    // But since these are additional dependencies, we have both an edit 
    // requesting to delete /Class/Child in layer2 and an edit requesting to 
    // move it to /MovedChild. But as was indicated, the deletion of 
    // /Class/Child wasn't strictly necessary so another edit wanting to move it
    // to /MovedChild is acceptable and preferred. Thus, we mark any deletes 
    // that come from implied class task as optional so that they can be 
    // overridden if another edit should take precedence.
    const bool isOptionalDelete = 
        nodeTask.isImpliedClassTask && nodeTask.newPath.IsEmpty();

    // The old path may be an ancestor path of the node's path. If so the spec
    // we're moving for this node is the node path itself.
    const SdfPath &oldSpecPath = 
        node.GetPath().HasPrefix(nodeTask.oldPath) ? 
            node.GetPath() : nodeTask.oldPath;

    // Check if there are any specs from upstream nodes that will remain 
    // composing into the oldSpecPath at this node (because those specs 
    // themselves exist and aren't being edited). This can occur if there's a
    // sibling composition arc that introduces the same spec but isn't dependent
    // on the initial edits that we're accounting for. In this case we do not
    // move the specs at this node and log a warning as we only want to move
    // these specs if it constitutes moving the entire composed prim stack at
    // this node.
    //
    // We ignore this check if the node has new relocates that will be applied
    // to it by the initial edit. Relocates are used specifically for "moving"
    // specs from weaker nodes without editing their specs so we actually expect
    // unedited spec conflicts in this case and have used relocates to handle
    // them.
    if (!nodeTask.willBeRelocated &&
             _HasUneditedUpstreamSpecConflicts(node, oldSpecPath)) {
        return;
    }

    // Map the old spec path to the new spec location.
    const SdfPath newSpecPath = isOptionalDelete ?
        SdfPath::EmptyPath() :
        oldSpecPath.ReplacePrefix(nodeTask.oldPath, nodeTask.newPath);

    // Collect every layer in the node's layer stack that has a spec at the old
    // path and can move that spec to the new path..
    SdfLayerHandleVector layersToEdit = PcpGatherLayersToEditForSpecMove(
        node.GetLayerStack(), oldSpecPath, newSpecPath, &_edits->errors);
    for (SdfLayerHandle &layer : layersToEdit) {
        // Print debug before adding as we're moving the layer.
        _PRINT_DEBUG(
            "Added spec edit from <%s> to <%s> %s on layer @%s@",
            oldSpecPath.GetText(), newSpecPath.GetText(), 
            (isOptionalDelete ? "(edit is optional)" : ""),
            layer->GetIdentifier().c_str()
        );

        // Add spec moves to the scratch space so that we can process optional
        // deletes when we finalize the edits.
        _SpecMovesScratch &specMovesScratch = 
            (*_layerSpecMovesScratch)[std::move(layer)];
        if (isOptionalDelete) {
            specMovesScratch.optionalSpecDeletes.push_back(oldSpecPath);
        } else {
            specMovesScratch.specMoves.push_back({oldSpecPath, newSpecPath});
        }
    }
}

// Checks if there are any specs that will map to the given site edit path in an
// unedited node that is a node descendant of the given node.
bool 
_PrimIndexDependentNodeEditProcessor::_HasUneditedUpstreamSpecConflicts(
    const PcpNodeRef &siteEditNode,
    const SdfPath &siteEditPath)
{
    PcpNodeRef firstConflictingNode;

    // In the specific case where a delete operation causes us to have to remove
    // an introducing composition field value, we'll have a tasks to delete
    // child specs in the introducing layer stack that would otherwise be 
    // composed over children introduced by the deleted arc. In those cases, the
    // site edit path passed to this function would be a namespace descendant of
    // the node path meaning that we're looking for the presence of any 
    // conflicting specs in subtree nodes for that child path. Extract this
    // possible child name here as it determines how we check nodes for
    // conflicting specs.
    if (siteEditNode.GetPath() != siteEditPath) {
        if (siteEditPath.GetParentPath() != siteEditNode.GetPath()) {
            TF_CODING_ERROR("Descendant site path <%s> is not a direct "
                "namespace child of node site path <%s>. Namespace descendant "
                "sites are expected to be at most a direct namespace child.",
                siteEditPath.GetText(), siteEditNode.GetPath().GetText());
            return true;
        }

        // Specifically look for conflicts in the subtree for the namespace 
        // child of the node's path.
        if (!_HasConflictingChildSpecsInUneditedNodes(
            siteEditNode, siteEditPath.GetNameToken(), &firstConflictingNode)) {
            return false;
        }
    // Otherwise, we just need to look for conflicts with the node path.
    } else if (!_HasConflictingSpecsInUneditedNodes(
            siteEditNode, &firstConflictingNode)) {
        return false;
    }

    std::string warning = TfStringPrintf(
        "Cannot edit specs for <%s> on node %s: found conflicting "
        "specs at node %s that will not be edited.",
        siteEditPath.GetText(),
        TfStringify(siteEditNode.GetSite()).c_str(),
        TfStringify(firstConflictingNode.GetSite()).c_str());

    _PRINT_DEBUG(warning);
    _edits->warnings.push_back(std::move(warning));

    return true;
} 

// Finds if there are any conflicting node specs in unedited nodes below the
// given node.
bool 
_PrimIndexDependentNodeEditProcessor::_HasConflictingSpecsInUneditedNodes(
    const PcpNodeRef &siteEditNode,
    PcpNodeRef *firstConflictingNode)
{
    // We only propagate edits up to stronger nodes when handling downstream
    // dependent edits; we do not push edits back down into weaker nodes. Thus,
    // we're looking for any specs in the descendant nodes of the node we're 
    // editing for matching specs that will not be edited.
    const auto range = siteEditNode.GetChildrenRange();
    for (const auto &childNode : range) {

        // If the child node is a direct arc, we can skip it and its entire
        // subtree as all the specs at or below this node are mapped to the
        // node's site path (no matter what it is edited to be) through this
        // child node.
        if (!childNode.IsDueToAncestor()) {
            continue;
        }

        // If the child node has been visited for editing then we can skip the
        // whole subtree as any necessary specs under this child node will have
        // been edited along with the specs at this node.
        if (_nodesVisitedByEditTasks.count(childNode)) {
            continue;
        }

        // Search the child node's subtree for any nodes with contributing 
        // specs that will not be moved along with this node in the edit. 
        // The presence of any of these specs is a conflict in that these specs
        // will no longer be part of the composed prim stack of the prim at its
        // new path after the edits are applied.
        PcpNodeRange subtreeRange = _primIndex->GetNodeSubtreeRange(childNode);
        for (const PcpNodeRef &subtreeNode : subtreeRange) {
            // A node that has not been visited for editing, has specs, and 
            // can contribute those specs will be an edit conflict.
            if (!_nodesVisitedByEditTasks.count(subtreeNode) &&
                    subtreeNode.HasSpecs() && 
                    subtreeNode.CanContributeSpecs()) {
                *firstConflictingNode = subtreeNode;
                return true;
            }
        }
    }
    return false;    
}

// Finds if there are any conflicting specs for the named child of the node path
// in unedited nodes below the given node.
bool 
_PrimIndexDependentNodeEditProcessor::_HasConflictingChildSpecsInUneditedNodes(
    const PcpNodeRef &siteEditNode,
    const TfToken &childName,
    PcpNodeRef *firstConflictingNode)
{
    if (!TF_VERIFY(!childName.IsEmpty())) {
        return false;
    }

    // We only propagate edits up to stronger nodes when handling downstream
    // dependent edits; we do not push edits back down into weaker nodes. Thus,
    // we're looking for any specs in the descendant nodes of the node we're 
    // editing for matching specs that will not be edited.
    const auto range = siteEditNode.GetChildrenRange();
    for (const auto &childNode : range) {

        // If the child node has been visited for editing then we can skip the
        // whole subtree as any necessary specs under this child node will have
        // been edited along with the specs at this node.
        if (_nodesVisitedByEditTasks.count(childNode)) {
            continue;
        }

        // Search the child node's subtree for any nodes with contributing 
        // specs to the namespace child that will not be moved along with this
        // node in the edit. The presence of any of these specs is a conflict in
        // that these specs will no longer be part of the composed prim stack of
        // the child prim at its new path after the edits are applied.
        PcpNodeRange subtreeRange = _primIndex->GetNodeSubtreeRange(childNode);
        for (const PcpNodeRef &subtreeNode : subtreeRange) {
            // Skip nodes that have been visited for editing or can never
            // contribute specs.
            if (_nodesVisitedByEditTasks.count(subtreeNode) || 
                    !subtreeNode.CanContributeSpecs()) {
                continue;
            }

            const PcpLayerStackRefPtr &layerStack = 
                subtreeNode.GetLayerStack();
            
            // Map the child path into the subtree node.
            const SdfPath subtreeNodeChildPath = 
                subtreeNode.GetPath().AppendChild(childName);

            // If the node has specs we then have to compose whether the node
            // has specs for the child path and if it does, then we we found an
            // unedited spec conflict.
            if (subtreeNode.HasSpecs() && PcpComposeSiteHasPrimSpecs(
                    layerStack, subtreeNodeChildPath)) {
                *firstConflictingNode = subtreeNode;
                return true;
            } 

            // But even if we don't have any specs for the child path, we still
            // need to account for the possibility of relocates. For all other
            // arc types, the child spec would have to exist for it to introduce
            // a new direct arc for itself at this node. But relocates arcs are
            // all introduced by layer metadata so a direct relocates could be
            // introduced in the namespace child's prim index at the subtree
            // node even if there are no specs at its site. The presence of a
            // relocates whose target is the child path is the same as a 
            // conflicting spec.
            if (layerStack->HasRelocates() &&
                    layerStack->GetIncrementalRelocatesTargetToSource()
                        .count(subtreeNodeChildPath)) {
                *firstConflictingNode = subtreeNode;
                return true;
            }
        }
    }
    return false;
}

// The purpose of this function is to map the new path that a node's specs are
// being moved to into the corresponding path in its introducing node. It's 
// important to note why we can't just use the node's GetMapToParent() function
// to perform this mapping. One reason is that the map function for certain 
// arcs, e.g. example inherits or internal references, will have an identity
// mapping that will cause some paths to map that we actually shouldn't in the
// context of determining whether the moved spec will still be composed in the
// updated prim indexes. Another reason is that the introducing node isn't 
// necessarily the node's parent in the case of specializes nodes that are
// propagated to the root. And yet another reason is that map functions strip
// out variant selections which will not give us the correct path if the
// introducing node is a variant node.
static
SdfPath 
_MapNewPathToIntroducingNode(
    const _NodeIntroductionInfo &nodeIntroInfo,
    const SdfPath &newPath)
{
    // We only end up processing a relocates node if another dependent node in
    // its subtree is processed and we have to propagate the spec move up the 
    // graph. In this case, any relocates in this node's layer stack that need
    // to be applied will have already been applied to the new path when the
    // descendant node was processed. So we return the new path as is for 
    // a relocates node.
    if (nodeIntroInfo.node.GetArcType() == PcpArcTypeRelocate) {
        return newPath;
    }

    // If the new path is outside of the namespace of the introduced node, it 
    // will no longer map to the introducing node. This is equivalent to
    // deleting the descendant specs from the introducing node, and accordingly,
    // means the corresponding specs should be deleted from the introducing
    // node.
    //
    // Note that an exception to this case would be if this node is a relocates
    // node, then moving specs outside of the relocation path just means that is
    // no longer affected by the relocation and should just exist at the 
    // unrelocated path. But we've already handled the relocates case.
    if (!newPath.HasPrefix(nodeIntroInfo.pathAtIntroduction)) {
        return SdfPath();
    }

    // Map the new path into the introducing node.
    const SdfPath newPathInIntroducingNode = newPath.ReplacePrefix(
        nodeIntroInfo.pathAtIntroduction, 
        nodeIntroInfo.introducingPath);
      
    // If the introducing node's layer has no relocates we're done.
    const PcpLayerStackRefPtr &introducingLayerStack = 
        nodeIntroInfo.introducingNode.GetLayerStack();
    if (!introducingLayerStack->HasRelocates()) {
        return newPathInIntroducingNode;
    }

    // Otherwise, relocates on the introducing node's layer stack may affect the 
    // new spec path regardless of whether the relocates are composed as part
    // of this prim index. We need to account for the potential relocation of
    // the new path here to move the specs to the correct final location when we
    // process the introducing node.
    //
    // It is possible that the new path, when mapped to the introducing node, 
    // will be partially relocated within the context of the layer stack's 
    // relocates map. An example of where this can occur is if a prim is 
    // relocated, then a child of this prim introduces a reference (from the
    // post-relocation path) and then a child composed from that reference is 
    // then also relocated. To get the final fully relocated new spec path
    // easily, we map the path to its absolute source path before then mapping
    // the absolute source path to its final target path.
    auto mapPath = [](const SdfRelocatesMap &reloMap, const SdfPath &path) { 
        // The best match relocate is the longest path in the map that is
        // a prefix of our path. Since the relocates map is ordered by path, 
        // we can find the best match by searching the map in reverse for the
        // first matching prefix.     
        auto bestMatchIt = std::find_if(reloMap.rbegin(), reloMap.rend(), 
            [&path](const auto& entry) {
                return path.HasPrefix(entry.first);
            });
        return bestMatchIt == reloMap.rend() ?
            path : path.ReplacePrefix(bestMatchIt->first, bestMatchIt->second);
    };
    const auto &sourceToTarget = 
        introducingLayerStack->GetRelocatesSourceToTarget();
    const auto &targetToSource = 
        introducingLayerStack->GetRelocatesTargetToSource();
    return mapPath(sourceToTarget, 
        mapPath(targetToSource, newPathInIntroducingNode));
}

// Processes the next available node task. Returns true if a task was processed,
// false if there are no tasks to process.
bool 
_PrimIndexDependentNodeEditProcessor::_ProcessNextNodeTask()
{
    // If the task list is empty, return that we're done.
    if (_nodeTasks.empty()) {
        return false;
    }

    // Pop the last task off the node tasks. This will be the task for the
    // weakest node we added a task for which is important for determining 
    // edited vs unedited nodes when looking for subtree spec conflicts.
    _NodeTask nodeTask = _nodeTasks.back();
    _nodeTasks.pop_back();

    _PRINT_DEBUG_SCOPE(
        "Processing node task: %s", TfStringify(nodeTask).c_str());

    const PcpNodeRef &node = nodeTask.node;
    const SdfPath &oldPath = nodeTask.oldPath;
    const SdfPath &newPath = nodeTask.newPath;

    // Mark this node as visited so that stronger nodes know that necessary 
    // spec edits (if any) will have been performed for this node.
    _nodesVisitedByEditTasks.insert(node);

    // Add any edits for moving specs that are needed for the the node
    _AddSpecMoveEdits(nodeTask);

    // If we hit the root node, there are no additional downstream dependencies
    // on this node to check for.
    if (node.GetArcType() == PcpArcTypeRoot) {
        return true;
    }

    // If the node is class node, this will process and add the correct edit
    // task for its next implied node if there is one.
    _ProcessNextImpliedClass(nodeTask);

    // Get node introduction info to determine how to process this dependency.
    _NodeIntroductionInfo nodeIntroInfo(node);

    _PRINT_DEBUG("Node was introduced as path <%s> by the path <%s> from "
        "introducing node %s",
        nodeIntroInfo.pathAtIntroduction.GetText(),
        nodeIntroInfo.introducingPath.GetText(),
        TfStringify(nodeIntroInfo.introducingNode).c_str());

    // If the edit affects the node's path at introduction, we need to fix up 
    // the composition field that introduces the arc to point at the new path.
    if (nodeIntroInfo.pathAtIntroduction.HasPrefix(oldPath)) {
        _PRINT_DEBUG(
            "Spec move at <%s> affects this node's path at introduction <%s>",
            oldPath.GetText(), nodeIntroInfo.pathAtIntroduction.GetText());
        _ProcessDependentNodePathAtIntroductionChange(
            nodeIntroInfo, oldPath, newPath);
    } else {
        // Otherwise the edit affects specs that are a namespace descendant of
        // this node. In this case we need to move the corresponding descendant
        // specs in the introducing node so that they continue to compose
        // together in the same composed prim on the dependant stage(s).
        SdfPath oldPathInIntroducingNode = oldPath.ReplacePrefix(
            nodeIntroInfo.pathAtIntroduction, 
            nodeIntroInfo.introducingPath);

        // Determine the new path in the introducing node.
        SdfPath newPathInIntroducingNode = _MapNewPathToIntroducingNode(
            nodeIntroInfo, newPath);

        _PRINT_DEBUG(
            "Spec move affects a namespace descendant of this node at "
            "introduction\nAdding task to move the corresponding specs in the "
            "introducing node: "
            "\n    oldPath <%s> maps to <%s> in introducing node "
            "\n    newPath <%s> maps to <%s> in introducing node ",
            oldPath.GetText(), 
            oldPathInIntroducingNode.GetText(),
            newPath.GetText(),
            newPathInIntroducingNode.GetText());
        
        _InsertNodeTask({
            nodeIntroInfo.introducingNode, 
            oldPathInIntroducingNode, 
            newPathInIntroducingNode});
    }
    
    return true;
}

// Helper function for computing the transfer function from an origin class 
// tree source parent to an implied class tree destination parent.
static PcpMapFunction
_ComputeImpliedClassTransferFunction(
    const PcpNodeRef &sourceParent,
    const PcpNodeRef &destParent)
{
    // Start with a function that maps the source parent at introduction to the
    // path in its parent that introduced it.
    PcpMapFunction transferFunction = PcpMapFunction::Create(
        {{sourceParent.GetPathAtIntroduction(),
            sourceParent.GetIntroPath()}}, 
            SdfLayerOffset());

    // Typically, the destination parent will be the source parent node's parent
    // itself. But in some cases (like when the source parent's parent is a 
    // relocates node) the destination parent will be a further ancestor. So we
    // get the map function for each parent to its own parent and compose it
    // into the transfer function as necessary until we've reached the
    // destination parent.
    for (PcpNodeRef transferNode = sourceParent.GetParentNode();
            transferNode != destParent;
            transferNode = transferNode.GetParentNode()) {
        transferFunction = PcpMapFunction::Create(
            {{transferNode.GetPathAtIntroduction(),
            transferNode.GetIntroPath()}}, 
            SdfLayerOffset()).Compose(transferFunction);
    }

    // Lastly add the identity mapping to the transfer function as class nodes
    // outside of the parent's domain are still implied as global inherits.
    return PcpMapExpression::Constant(
        transferFunction).AddRootIdentity().Evaluate();
}

// Gets the source parent node of the class tree that the origin node was 
// implied from. This is based off of the logic in _EvalImpliedClassTree in 
// primIndex.cpp
static PcpNodeRef 
_GetImpliedClassTreeSourceParent(const PcpNodeRef originNode) {
    if (!TF_VERIFY(PcpIsClassBasedArc(originNode.GetArcType()))) {
        return PcpNodeRef();
    }

    // Start with the origin node's parent as the assumed source parent node. 
    // But it may not be if this node was implied as part of a whole class tree
    // whose root is is an ancestor node of this node. This loop will determine
    // this and find the real source parent node.
    PcpNodeRef sourceParent = originNode.GetParentNode();
    for (; !sourceParent.IsRootNode(); 
            sourceParent = sourceParent.GetParentNode()) {

        // A non-class arc is never part of nested implied class tree so it must
        // be the source parent.
        if (!PcpIsClassBasedArc(sourceParent.GetArcType())) {
            break;
        }

        // XXX: In the case where an inherit arc nested directly under a 
        // specializes arc, we have a known issue where we can't reliably 
        // determine the class structure due to bidirectional propagation of
        // specializes nodes that can leave inherits nodes without origin nodes
        // to help us jump between propagated and unpropagated sections of the
        // tree. Since it would complex to fully determine implied class 
        // relationships in this situation and we plan to change how we process
        // speciliazes in prim indexes in the near future, we're just going to
        // give up on this case with a warning for now.
        if (originNode.GetArcType() == PcpArcTypeInherit &&
            sourceParent.GetArcType() == PcpArcTypeSpecialize &&
                _GetUnpropagatedSpecializesNode(sourceParent) != sourceParent) {
            TF_WARN("Unable to fix specs for implied inherits for an inherit "
                "node %s nested under the specializes node %s. This is a known "
                "bug that we cannot correct find the implied inherit node to "
                "fix in this scenario.",
                TfStringify(originNode.GetSite()).c_str(),
                TfStringify(sourceParent.GetSite()).c_str());
            return PcpNodeRef();
        }

        // A class based parent arc may still be the source parent if it is a
        // more ancestral arc than this class origin node. Class nodes that
        // are all introduced at the same namespace depth are implied as whole
        // subtree but the tree can still implied across a class parent that 
        // was introduced ancestrally.
        if (originNode.GetDepthBelowIntroduction() < 
               sourceParent.GetDepthBelowIntroduction()) {
            break;
        }
    }

    return sourceParent;
}

// Another helper that is useful for getting the destination parent of an 
// implied node and its origin, by finding the closest shared ancestor node 
// of two nodes.
//
// XXX: Note that this function relies on PcpNodeRef's less than operator and 
// the fact that nodes in the same finalized prim index are ordered from
// strongest to weakest even though the less than operator makes no promise of
// strength order. It would be prudent to add a comparator for nodes in a
// finalized prim index that does guarantee strength ordering that we would use
// here instead.
static PcpNodeRef 
_GetClosestSharedAncestorNode(
    const PcpNodeRef &node1, const PcpNodeRef &node2) {

    if (node1 == node2) {
        return node1;
    }

    // A parent node is always stronger than all its descendants. So starting
    // with the weaker node, walk up its parent nodes until we hit the first
    // node stronger than the stronger node. This will be the closest shared
    // ancestor.
    auto getSharedParent = [](
        const PcpNodeRef &strongerNode, const PcpNodeRef &weakerNode) {

        for (PcpNodeRef parent = weakerNode.GetParentNode(); 
                parent; parent = parent.GetParentNode()) {
            if (parent < strongerNode) {
                return parent;
            }
        }
        TF_VERIFY(false, "Nodes %s and %s do not share an ancestor",
            TfStringify(strongerNode).c_str(),
            TfStringify(weakerNode).c_str());
        return PcpNodeRef();
    };

    return node1 > node2 ? 
        getSharedParent(node2, node1) : 
        getSharedParent(node1, node2);
}

using _ImpliedNodeAndTransferFunction = std::pair<PcpNodeRef, PcpMapFunction>;

// Returns implied node info for the specializes node that was introduced as an
// implied specializes of the given node which must be a post-propagation
// specializes node.
static _ImpliedNodeAndTransferFunction
_GetNextImpliedSpecializes(
    const PcpNodeRef &propagatedSpecializesOriginNode)
{
    const PcpNodeRef &unpropagatedSpecializesOriginNode = 
        _GetUnpropagatedSpecializesNode(propagatedSpecializesOriginNode);

    // Since all specializes nodes are propagated to the root node, we can just
    // iterate over the root nodes children to find the specializes that was
    // implied from the given origin. And because all specializes nodes are 
    // weakest, we can iterate over the children in reverse and stop at the 
    // first non-specializes we find.
    const PcpNodeRef rootNode = unpropagatedSpecializesOriginNode.GetRootNode();
    for (const auto &child : rootNode.GetChildrenReverseRange()) {
        if (child.GetArcType() != PcpArcTypeSpecialize) {
            break;
        }

        // All child nodes of the root are propagated specializes, so get the
        // unpropagated node for the child and if it has the unpropagated origin
        // as its origin node, the child is the propagated implied specializes.
        const PcpNodeRef unpropagatedChild =
            _GetUnpropagatedSpecializesNode(child);
        const PcpNodeRef unpropagatedChildOrigin = 
            unpropagatedChild.GetOriginNode();
        // Skip if the unpropagated child has no origin.
        if (unpropagatedChildOrigin == unpropagatedChild.GetParentNode()) {
            continue;
        }
        if (unpropagatedChildOrigin == unpropagatedSpecializesOriginNode) {
            // We found the node with the correct origin; compute the implied
            // node info. We need the implied class source parent and
            // destination parent to compute the transfer function. We compute
            // the source parent from the unpropagated origin node. Then we can
            // use the source parent and the unpropagated implied node to 
            // figure out the destination parent (which is also unpropagated).
            // With the source and destination parents both being in the 
            // unpropagated tree, they have the correct ancestor hierarchy to 
            // allow us to compute the transfer function.
            const PcpNodeRef sourceParent = _GetImpliedClassTreeSourceParent(
                unpropagatedSpecializesOriginNode);
            const PcpNodeRef destParent = _GetClosestSharedAncestorNode(
                sourceParent, unpropagatedChild);
            // The implied node and origin nodes we return in this info need to 
            // be the propagated versions of the specializes nodes.
            return {
                child, 
                _ComputeImpliedClassTransferFunction(sourceParent, destParent)};
        }
    }

    return {};
}

// Helper for finding implied inherit nodes. Finds the class based node in the
// subtree starting at node whose origin node is the given origin node.
static PcpNodeRef
_FindClassNodeWithOriginInSubtree(
    const PcpNodeRef &subtreeRoot,
    const PcpNodeRef &originNode)
{
    const auto subtreeRange = Pcp_GetSubtreeRange(subtreeRoot);
    for (auto it = subtreeRange.begin(); it != subtreeRange.end(); ++it) {
        // Nested class arcs are implied as subtrees of only class based arcs
        // so we can prune the search at any non-class based arc.
        if (!PcpIsClassBasedArc(it->GetArcType())) {
            it.PruneChildren();
            continue;
        }
        // The subtree has node has the correct origin, return it.
        if (it->GetOriginNode() == originNode) {
            return *it;
        }       
    }

    return PcpNodeRef();
}

// Returns implied node info for the inherits node that was introduced as an
// implied inherits of the given node which must be an inherits node.
static _ImpliedNodeAndTransferFunction
_GetNextImpliedInherit(const PcpNodeRef &originNode)
{
    if (!TF_VERIFY(originNode.GetArcType() == PcpArcTypeInherit)) {
        return {};
    }

    // Inherits are implied from the origin node's original parent to a direct
    // ancestor of the parent node. Thus, we're looking a node whose origin is 
    // this node in a subtree of one of the ancestors of its parent
    
    // First get the source parent node of the root of the class tree that was
    // implied together and cause the origin node to be implied.
    const PcpNodeRef sourceParent = _GetImpliedClassTreeSourceParent(originNode);
    if (!sourceParent) {
        return {};
    }

    // Walk up the tree of destination parents looking a node in an implied
    // class subtree that has the origin node where looking for.
    PcpNodeRef destParent = sourceParent.GetParentNode();
    PcpNodeRef skipChild = sourceParent;
    PcpNodeRef curOriginNode = originNode;

    while (destParent) {
        // Check the child subtrees under the destination parent
        const auto range = destParent.GetChildrenRange();
        for (const auto &child : range) {
            // Skip the child that we walked up to the destination parent from.
            if (child == skipChild) {
                continue;
            }

            // Search for the node with our origin in the child's subtree as it
            // may be part of a nested class hierarchy.
            PcpNodeRef impliedNode = 
                _FindClassNodeWithOriginInSubtree(child, curOriginNode);
            if (impliedNode) {
                // We found the implied node with our desired origin node, but 
                // it may be inert. This will typically occur when an inherit is
                // implied to root of a subtree that is being added as a 
                // relocates node. Implied inherits add under relocates nodes 
                // are only placeholders for continuing to imply the inherit up
                // the tree after the subtee is added, so if we encounter an 
                // inert node, we have to keep going, looking for an implied 
                // node whose origin is the fournd node to get the real implied
                // inherit.
                if (!impliedNode.CanContributeSpecs()) {
                    curOriginNode = impliedNode;
                    break;
                }

                // Otherwise, we found the implied node; return the info with
                // the computed transfer function.
                return {
                    impliedNode,
                    _ComputeImpliedClassTransferFunction(
                        sourceParent, destParent)};
            }
        }

        // If we didn't find the implied node under the implied to parent node,
        // we continue up the tree as it could've been implied under a different
        // ancestor. This can happen when the origin node is implied across a 
        // relocate node which skips the relocate to imply the node to the 
        // relocate's parent.
        skipChild = destParent;
        destParent = destParent.GetParentNode();
    }

    return {};
}

// Processes what the next implied class is for the tasks node if we have a 
// class based arc and adds a task to process it.
void _PrimIndexDependentNodeEditProcessor::_ProcessNextImpliedClass(
    const _NodeTask &nodeTask)
{
    const PcpNodeRef &node = nodeTask.node;
    const SdfPath &oldPath = nodeTask.oldPath;
    const SdfPath &newPath = nodeTask.newPath;

    // Get the implied node and transfer function if possible.
    const auto [impliedNode, transferFunction] =
        node.GetArcType() == PcpArcTypeSpecialize ?
            _GetNextImpliedSpecializes(node) :
            node.GetArcType() == PcpArcTypeInherit ?
                _GetNextImpliedInherit(node) :
                _ImpliedNodeAndTransferFunction();
    if (!impliedNode) {
        return;
    }

    _PRINT_DEBUG_SCOPE("Processing implied class "
        "\n  origin class node: %s"
        "\n  implied class node: %s"
        "\n  implied class transfer function: %s",
        TfStringify(node).c_str(),
        TfStringify(impliedNode).c_str(),
        transferFunction.GetString().c_str());

    // For implied class nodes, the true introduction of the arc happens when
    // the class arc is introduced directly by an authored inherits or 
    // specializes field. This direct node will be the origin root for all
    // implied nodes. We use the path at origin root as the "real" path at 
    // introduction for the implied nodes as calling GetPathAtIntroduction() on
    // the implied node itself will only give us the path used when the node was
    // added to the tree, which can be at a farther depth in namespace if, for
    // instance, the node is implied from an ancestral class arc in a subroot
    // path arc's subtree.
    const SdfPath originPathAtClassIntroduction = 
        node.GetPathAtOriginRootIntroduction();
    const SdfPath impliedPathAtClassIntroduction = 
        impliedNode.GetPathAtOriginRootIntroduction();

    _PRINT_DEBUG("Origin node path at class introduction: <%s>\n"
                 "Implied node path at class introduction: <%s>",
        originPathAtClassIntroduction.GetText(), 
        impliedPathAtClassIntroduction.GetText());

    SdfPath oldPathInImpliedNode;
    SdfPath newPathInImpliedNode;

    // If the edit affects the origin node's path at class introduction, then
    // it also affects the implied node's path at class introduction. In this
    // case we use the transfer function to map both the old and new paths 
    // into the implied node to get the edit to process in the implied node
    if (originPathAtClassIntroduction.HasPrefix(oldPath)) {
        _PRINT_DEBUG("Spec move at <%s> affects implied node's path at "
            "introduction", oldPath.GetText());
        oldPathInImpliedNode = 
            transferFunction.MapSourceToTarget(oldPath);
        newPathInImpliedNode = 
            transferFunction.MapSourceToTarget(newPath);
    } else {
        // Otherwise we have an edit to the namespace descendant of the origin
        // node that then also affects a namespace descendant of the implied 
        // node.
        _PRINT_DEBUG("Spec move at <%s> affects implied node's descendant", 
            oldPath.GetText());
        
        // We can map the old path directly from the origin node to the implied
        // node.
        oldPathInImpliedNode = oldPath.ReplacePrefix(
            originPathAtClassIntroduction, 
            impliedPathAtClassIntroduction);

        // The new path may not map to the implied node if it was moved out
        // the origin nodes class introduction namespace which results in a 
        // delete at the implied node.
        newPathInImpliedNode = newPath.HasPrefix(originPathAtClassIntroduction) ?
            newPath.ReplacePrefix(
                originPathAtClassIntroduction, 
                impliedPathAtClassIntroduction) :
            SdfPath();
    }    

    _PRINT_DEBUG("Mapped old path <%s> into implied node as <%s>",
        oldPath.GetText(), oldPathInImpliedNode.GetText());
    _PRINT_DEBUG("Mapped new path <%s> into implied node as <%s>",
        newPath.GetText(), newPathInImpliedNode.GetText());

    // Add the task and mark it as a implied class task so we handle deletes
    // properly.
    _InsertNodeTask({
        impliedNode, oldPathInImpliedNode, newPathInImpliedNode, 
        /*isImpliedClassTask*/ true});
}

// Finalizes the spec move edits into a single list of edits that can all be
// performed in order with no errors.
static void 
_FinalizeSpecMoveEdits(PcpDependentNamespaceEdits *edits,
    _LayerSpecMovesScratch &&layerSpecMovesScratch)
{
    TRACE_FUNCTION();

    // Compare function for sorting spec move edit descriptions by old path
    // first and then by new path.
    auto specMoveDescLessThan = [](
        const PcpDependentNamespaceEdits::SpecMoveEditDescription &lhs, 
        const PcpDependentNamespaceEdits::SpecMoveEditDescription &rhs) 
    {
        return std::tie(lhs.oldPath, lhs.newPath) < 
                std::tie(rhs.oldPath, rhs.newPath);
    };

    // For each layer, we're going to want to perform each prim spec move in
    // order, so we need to make sure we don't have any redundant edits as the
    // edit will fail if a prior edit would cause the prim spec to no longer
    // exist. 
    for (auto &[layer, specMovesScratch] : layerSpecMovesScratch) {
        auto &specMoves = specMovesScratch.specMoves;
        auto &optionalSpecDeletes = specMovesScratch.optionalSpecDeletes;

        if (specMoves.size() > 1) {
            // Sort the spec edits, by oldPath and then new path.
            std::sort(specMoves.begin(), specMoves.end(), specMoveDescLessThan);

            // Remove any duplicate edits which will be adjacent because of 
            // sorting.
            for (auto it = specMoves.begin(), next = it+1;
                    next != specMoves.end(); /*no advance*/) {
                // If two edits are moving the same path, they are either
                // redundant or an error.
                if (next->oldPath == it->oldPath) {                    
                    if (next->newPath == it->newPath) {
                        // Redundant. Delete one and move on.
                        next = specMoves.erase(next);
                        continue;
                    } else {
                        // Otherwise, not redundant. Add the error. Note that
                        // we don't remove either edit.
                        edits->errors.push_back(TfStringPrintf(
                            "Dependent edit conflict: Trying to move spec at "
                            "layer @%s@ and path <%s> to both new paths <%s> "
                            "and <%s>",
                            layer->GetIdentifier().c_str(),
                            it->oldPath.GetText(),
                            it->newPath.GetText(),
                            next->newPath.GetText()));
                    }
                }
                ++it;
                ++next;
            }
        }

        // We may have marked some specs for this layer as optional delete. 
        // For each, we look for an existing edit for the same path in the
        // current spec moves. If we don't find one, then we add the delete
        // edit, otherwise we just ignore the delete.
        for (const auto &deleteSpecPath : optionalSpecDeletes) {
            PcpDependentNamespaceEdits::SpecMoveEditDescription specDelete{
                deleteSpecPath, SdfPath()};
            auto insertIt = std::lower_bound(
                specMoves.begin(), specMoves.end(), specDelete, 
                specMoveDescLessThan);
            if (insertIt == specMoves.end() || 
                    insertIt->oldPath != deleteSpecPath) {
                // Insert to maintain sort order 
                specMoves.insert(insertIt, std::move(specDelete));
            }
        }

        // Remove any edits that would be subsumed by an ancestor edit.
        for (auto it = specMoves.begin(); it != specMoves.end(); 
                /*no advance*/ ) {
            const SdfPath &thisOldPath = it->oldPath;
            const SdfPath &thisNewPath = it->newPath;

            // Find the closest spec edit before this one which moves an 
            // ancestor of this path.
            if (auto otherIt = std::find_if(
                    std::make_reverse_iterator(it), specMoves.rend(), 
                    [&](const auto &other) { 
                        return thisOldPath.HasPrefix(other.oldPath);
                    });
                    otherIt != specMoves.rend()) {

                // If the other ancestor edit would map this edit's old path to
                // the same new path, we don't need this edit and remove it.
                if ((otherIt->newPath.IsEmpty() && thisNewPath.IsEmpty()) ||
                    thisOldPath.ReplacePrefix(
                        otherIt->oldPath, otherIt->newPath) == thisNewPath) {
                    it = specMoves.erase(it);
                    continue;
                }
            }
            ++it;
        }

        // Move the finalized spec moves from scratch to the result spec move
        // edits for the layer.
        edits->layerSpecMoves.emplace(layer, std::move(specMoves));
    }  
}

}; // End anonymous namespace

PcpDependentNamespaceEdits
PcpGatherDependentNamespaceEdits(
    const SdfPath &oldPrimPath,
    const SdfPath &newPrimPath,
    const SdfLayerHandleVector &affectedLayers,
    const PcpLayerStackRefPtr &affectedRelocatesLayerStack,
    const SdfLayerHandle &addRelocatesToLayerStackEditLayer,
    const std::vector<const PcpCache *> &dependentCaches)
{
    TRACE_FUNCTION();

    // Initialize result
    PcpDependentNamespaceEdits edits;

    // Scratch space for spec move edits.
    _LayerSpecMovesScratch layerSpecMovesScratch;

    // We don't author new relocates for the dependent prim indexes outside of 
    // the explicit new relocates that we will determine for the 
    // affectedRelocatesLayerStack if it is provided. Because of this, at each 
    // dependent node we look for conflicting specs in its subtree that will not
    // be edited (and otherwise would require something like relocates) in order
    // to log a warning that the composed prim stack won't be fully  maintained
    // by the edit. However, we won't have conflicting specs in nodes that are
    // affected by the relocates edits above even if its subtree has unedited
    // conflicting specs as the new relocates will effectively move those specs
    // for us. All of this is to say that we need to pass the fact that the
    // layer will have a relocates edit to the layer's dependent nodes so that
    // they know to skip the conflicting subtree specs check.
    //
    // XXX: Note that this is actually a little simplified as what we really 
    // need to know at each node is whether its layer stack's compose relocates
    // will, after the above relocates are applied, effectively relocate the
    // subtree specs that would've otherwise had to be moved. But that is much
    // more complex and this simpler method gets the job done for the vast
    // majority of cases.
    //
    // Create a new list of each of the input affected layers paired with 
    // whether it has a relocates edit (which we initialize to false for all to
    // start.)
    std::vector<std::pair<SdfLayerHandle, bool>> 
        affectedLayersAndHasRelocatesEdits;
    affectedLayersAndHasRelocatesEdits.reserve(affectedLayers.size());
    for (const auto &layer : affectedLayers) {
        affectedLayersAndHasRelocatesEdits.push_back({layer, false});
    }

    // If we were passed a layer stack to add relocates to, we'll use the 
    // relocates edit builder to process those now.
    if (affectedRelocatesLayerStack) {
        PcpLayerRelocatesEditBuilder builder(
            affectedRelocatesLayerStack, addRelocatesToLayerStackEditLayer);
        std::string error;
        if (!builder.Relocate(oldPrimPath, newPrimPath, &error)) {
            TF_CODING_ERROR("Cannot get relocates edits because: %s", 
                error.c_str());                       
        }
        // For each initial relocates edit, we do three things:
        // 1. Make sure the layer is put in the list of affected layers if it
        //    isn't already. Adding a relocate is the same as moving a spec as
        //    far as needing to update dependent prim indexes is concerned.
        // 2. Marking the affected layer as having a relocates edit for when we
        //    add the initial dependent node tasks.
        // 3. Move the edit into relocates edit results that is returned at the
        //    end.
        for (auto &relocatesEdit : builder.GetEdits()) {
            // Scoping for safety/clarity as the const reference of layer to
            // relocatesEdit.first would become invalid after the relocatesEdit 
            // is inserted by rvalue reference.
            {
                const SdfLayerHandle &layer = relocatesEdit.first;
                auto foundIt = std::find_if(
                    affectedLayersAndHasRelocatesEdits.begin(), 
                    affectedLayersAndHasRelocatesEdits.end(),
                    [&](const auto &entry) {
                        return layer == entry.first;});
                if (foundIt == affectedLayersAndHasRelocatesEdits.end()) {
                    affectedLayersAndHasRelocatesEdits.push_back({layer, true});
                } else {
                    foundIt->second = true; 
                }
            }
            edits.dependentRelocatesEdits.insert(std::move(relocatesEdit));
        }
    }

    for (const PcpCache *cache : dependentCaches) {
        _PRINT_DEBUG_SCOPE(
            "Computing dependent namespace edits for PcpCache %s", 
            TfStringify(cache->GetLayerStackIdentifier()).c_str());

        // For each layer that will be edited, we find all the prim indexes 
        // that depend on the old prim site in this layer and determine what
        // additional edits are necesssary to propagate edits to composition
        // dependencies as best as possible.
        for (const auto &entry : affectedLayersAndHasRelocatesEdits) {

            const SdfLayerHandle &layer = entry.first;
            const bool hasRelocatesEdits = entry.second;

            // Find all prim indexes which depend on the old prim path in this
            // layer. We recurse on site because moving or deleting a prim spec
            // moves also moves all descendant specs and we need to fix up 
            // direct dependencies on those paths as well. We do not recurse
            // on the found prim indexes since the edits affecting the directly
            // dependent prim index automatically affect namespace descendant
            // prim indexes. We also filter on existing computed prim indexes
            // as we will not be force computing prim indexes that have not
            // been computed yet to process edit dependencies.
            PcpDependencyVector deps =
                cache->FindSiteDependencies(
                    layer, oldPrimPath,
                    PcpDependencyTypeAnyNonVirtual,
                    /* recurseOnSite */ true,
                    /* recurseOnIndex */ false,
                    /* filter */ true);

            _PRINT_DEBUG(
                "Found %lu dependencies for spec edit at site @%s@<%s>.", 
                deps.size(),
                layer->GetIdentifier().c_str(),
                oldPrimPath.GetText());

            using _DependencySet =
                std::unordered_set<std::pair<SdfPath, SdfPath>, TfHash>; 
            _DependencySet seenDependencies;
            for(const PcpDependency &dep: deps) {
                // The dependency vector returned by FindSiteDependencies is
                // known to frequently contain duplicates, sometimes with 
                // several duplicates for the same dependency. So we need to 
                // avoid repeating work.
                if (!seenDependencies.emplace(
                        dep.indexPath, dep.sitePath).second) {
                    _PRINT_DEBUG(
                        "Skipping duplicate dependency for prim index <%s> "
                        "which depends on site path <%s>", 
                        dep.indexPath.GetText(), dep.sitePath.GetText());
                    continue;
                }

                _PRINT_DEBUG_SCOPE(
                    "Processing dependency for prim index <%s> which depends "
                    "on site path <%s>", 
                    dep.indexPath.GetText(), dep.sitePath.GetText());

                // We filtered on existing prim indexes so the dependent prim
                // index must be in the cache.
                const PcpPrimIndex *primIndex = 
                    cache->FindPrimIndex(dep.indexPath);
                if (!TF_VERIFY(primIndex)) {
                    continue;
                }

                // Find all the nodes in the dependent prim index that depend on
                // the site and add a task for each to be processed for 
                // dependent edits and then process these edits.
                // Note that the processor may process additional nodes as 
                // necessary in addition to the dependent nodes we found 
                // here.              
                _PrimIndexDependentNodeEditProcessor dependentNodeProcessor(
                    primIndex, &edits, &layerSpecMovesScratch);
                Pcp_ForEachDependentNode(
                    dep.sitePath, layer, dep.indexPath, *cache,
                    [&](const SdfPath &depIndexPath, const PcpNodeRef &node) {
                        // If the dependent layer is was affected by the 
                        // initial relocates edit, indicate in the node task 
                        // that the node task has a relocates edit. 
                        // XXX: This is the part that is a little oversimplified
                        // as was mentioned earlier in this function.
                        dependentNodeProcessor.AddProcessEditsAtNodeTask(
                            node, oldPrimPath, newPrimPath, 
                            /*willBeRelocated = */ hasRelocatesEdits);
                    });
                dependentNodeProcessor.ProcessTasks();               
            }
        }
    }

    // Processing these dependencies may result in redundant edits especially
    // when multiple dependent caches are involved. The finalize step ensures
    // we return a fully executable set of edits with no redundancies and/or
    // inconsistencies.
    _FinalizeSpecMoveEdits(&edits, std::move(layerSpecMovesScratch));
    return edits;
}

SdfLayerHandleVector
PcpGatherLayersToEditForSpecMove(
    const PcpLayerStackRefPtr &layerStack,
    const SdfPath &oldSpecPath,
    const SdfPath &newSpecPath,
    std::vector<std::string> *errors)
{
    SdfLayerHandleVector layersToEdit;

    // Get all the layers in the layer stack where the edits will be performed.
    const SdfLayerRefPtrVector &layers = layerStack->GetLayers();

    // Collect every prim spec that exists for this prim path in the layer 
    // stack's layers.
    for (const SdfLayerRefPtr &layer : layers) {
        if (layer->HasSpec(oldSpecPath)) {
            layersToEdit.push_back(layer);
        }
    }

    // Validate whether the necessary spec edits can actually be performed on
    // each layer that needs to be edited.
    for (const auto &layer : layersToEdit) {
        // The layer itself needs to be editable  
        if (!layer->PermissionToEdit()) {
            errors->push_back(TfStringPrintf("The spec @%s@<%s> "
                "cannot be edited because the layer is not editable",
                layer->GetIdentifier().c_str(),
                oldSpecPath.GetText()));
        }
        // If we're moving an object to a new path, the layer cannot have a
        // spec already at the new path.
        if (!newSpecPath.IsEmpty() && layer->HasSpec(newSpecPath)) {
            errors->push_back(TfStringPrintf("The spec @%s@<%s> "
                "cannot be moved to <%s> because a spec already exists at "
                "the new path",
                layer->GetIdentifier().c_str(),
                oldSpecPath.GetText(),
                newSpecPath.GetText()));
        }
    }

    return layersToEdit;
}

PXR_NAMESPACE_CLOSE_SCOPE
