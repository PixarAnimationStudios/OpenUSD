//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/uncompiler.h"

#include "pxr/exec/exec/program.h"
#include "pxr/exec/exec/runtime.h"
#include "pxr/exec/exec/uncompilationRuleSet.h"
#include "pxr/exec/exec/uncompilationTable.h"
#include "pxr/exec/exec/uncompilationTarget.h"

#include "pxr/base/trace/trace.h"
#include "pxr/exec/esf/editReason.h"
#include "pxr/usd/sdf/path.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Visits an Exec_UncompilationTarget to determine if the target is still
// valid for uncompilation.
//
class Exec_Uncompiler::_IsValidTargetVisitor {
public:
    Exec_Uncompiler &uncompiler;

    // Exec_NodeUncompilationTarget is valid if its node id refers to an
    // existing node.
    //
    bool operator()(Exec_NodeUncompilationTarget &target) const {
        return uncompiler._program->GetNodeById(target.GetNodeId());
    }

    // Exec_InputUncompilationTarget is valid if has has not been invalidated
    // by an earlier scene change, and it refers to a node that still exists
    // in the network.
    //
    bool operator()(Exec_InputUncompilationTarget &target) const {
        if (!target.IsValid()) {
            return false;
        }

        if (!uncompiler._program->GetNodeById(*target.GetNodeId())) {
            // The node no longer exists. By invalidating the target here, other
            // rules for the same target don't need to re-check the existence of
            // the node.
            target.Invalidate();
            return false;
        }

        return true;
    }
};

// Visits an Exec_UncompilationTarget to uncompile objects from the network.
class Exec_Uncompiler::_UncompileTargetVisitor {
public:
    Exec_Uncompiler &uncompiler;

    // Exec_NodeUncompilationTargets need to clear all runtime data associated
    // with the node before disconnecting and deleting the node.
    //
    void operator()(Exec_NodeUncompilationTarget &target) const {
        VdfNode *const node =
            uncompiler._program->GetNodeById(target.GetNodeId());
        
        uncompiler._runtime->DeleteData(*node);
        uncompiler._program->DisconnectAndDeleteNode(node);
    }

    // Exec_InputUncompilationTarget needs to disconnect connections from the
    // targeted input.
    //
    void operator()(Exec_InputUncompilationTarget &target) const {
        VdfNode *const node =
            uncompiler._program->GetNodeById(*target.GetNodeId());
        VdfInput *const input = node->GetInput(*target.GetInputName());

        uncompiler._program->DisconnectInput(input);

        // Once the input has been uncompiled, we invalidate existing rules that
        // refer to the same input, so that they don't trigger on future scene
        // changes. When the input is recompiled, exec will create a new set of
        // rules for the input, which are not invalidated by this call.
        target.Invalidate();
    }
};

void
Exec_Uncompiler::UncompileForSceneChange(
    const SdfPath &path,
    const EsfEditReason editReasons)
{
    if (editReasons == EsfEditReason::None) {
        return;
    }

    TRACE_FUNCTION();

    if (editReasons & EsfEditReason::ResyncedObject) {
        // Resyncs are recursive, so we need to process resyncs for the changed
        // path, and for all descendant paths. This simultaneously removes the
        // matching rule sets from the uncompilation table.
        const std::vector<Exec_UncompilationTable::Entry> tableEntries =
            _program->ExtractUncompilationRuleSetsForResync(path);

        for (const Exec_UncompilationTable::Entry &tableEntry : tableEntries) {
            _ProcessUncompilationRuleSet(
                tableEntry.path, 
                editReasons, 
                tableEntry.ruleSet.get());
        }
        return;
    }

    // For non-resync changes, we only process a single rule set for the changed
    // path.
    const Exec_UncompilationTable::Entry tableEntry =
        _program->GetUncompilationRuleSetForPath(path);
    
    // If there are no rules for this path, then there's nothing to do.
    if (!tableEntry.ruleSet) {
        return;
    }

    _ProcessUncompilationRuleSet(
        tableEntry.path,
        editReasons, 
        tableEntry.ruleSet.get());
}

void
Exec_Uncompiler::_ProcessUncompilationRuleSet(
    const SdfPath &path,
    const EsfEditReason editReasons,
    Exec_UncompilationRuleSet *const ruleSet)
{
    TRACE_FUNCTION();

    // TODO: Add debug flags to trace the uncompilation process, in which
    // logging the path will be helpful.
    (void)path;

    Exec_UncompilationRuleSet::iterator ruleSetIter = ruleSet->begin();
    while (ruleSetIter != ruleSet->end()) {
        Exec_UncompilationRule &rule = *ruleSetIter;
        
        // If the target of the rule is no longer valid, then we "garbage
        // collect" that rule from the rule set. This can happen if
        // uncompilation rules for another path uncompiled the same object in
        // the network.
        const bool isValidTarget =
            std::visit(_IsValidTargetVisitor{*this}, rule.target);
        if (!isValidTarget) {
            if (editReasons & EsfEditReason::ResyncedObject) {
                // If the change is a recursive resync, don't bother erasing the
                // individual rule, because the entire rule set is already going
                // to be destroyed.
                ++ruleSetIter;
            } else {
                ruleSetIter = ruleSet->erase(ruleSetIter);
            }
            continue;
        }

        // Skip this rule if its edit reasons are not applicable to this change.
        if (!(rule.reasons & editReasons)) {
            ++ruleSetIter;
            continue;
        }

        // Uncompile the objects targeted by the rule.
        std::visit(_UncompileTargetVisitor{*this}, rule.target);
        _didUncompile = true;

        // The rule has triggered and is no longer valid.
        ruleSetIter = ruleSet->erase(ruleSetIter);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
