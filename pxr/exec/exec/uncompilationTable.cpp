//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/uncompilationTable.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_UncompilationTable::AddRulesForNode(
    VdfId nodeId,
    const EsfJournal &journal)
{
    TRACE_FUNCTION();

    const Exec_NodeUncompilationTarget target(nodeId);

    for (const auto& [path, editReasons] : journal) {
        _FindOrInsert(path).emplace_back(target, editReasons);
    }
}

void
Exec_UncompilationTable::AddRulesForInput(
    VdfId nodeId,
    const TfToken &inputName,
    const EsfJournal &journal)
{
    TRACE_FUNCTION();

    // All rules created for this input *must* be copy-constructed from the same
    // Exec_InputUncompilationTarget. This ensures all the rules share a common
    // identity. The first of these rules to match a scene change will
    // invalidate the shared identity, which in turn will disable the remaining
    // rules constructed from this target.
    const Exec_InputUncompilationTarget target(nodeId, inputName);

    for (const auto& [path, editReasons] : journal) {
        _FindOrInsert(path).emplace_back(target, editReasons);
    }
}

Exec_UncompilationTable::Entry
Exec_UncompilationTable::Find(const SdfPath &path)
{
    TRACE_FUNCTION();

    const _ConcurrentMap::iterator foundIter = _ruleSets.find(path);
    if (foundIter != _ruleSets.end()) {
        return {path, foundIter->second};
    }
    return {path, nullptr};
}

std::vector<Exec_UncompilationTable::Entry>
Exec_UncompilationTable::UpdateForRecursiveResync(const SdfPath &path)
{
    TRACE_FUNCTION();

    std::vector<Exec_UncompilationTable::Entry> result;
    _ConcurrentMap::iterator iter = _ruleSets.lower_bound(path);
    const _ConcurrentMap::iterator endIter = _ruleSets.end();
    while (iter != endIter && iter->first.HasPrefix(path)) {
        result.emplace_back(iter->first, std::move(iter->second));
        iter = _ruleSets.unsafe_erase(iter);
    }
    return result;
}

Exec_UncompilationRuleSet &
Exec_UncompilationTable::_FindOrInsert(const SdfPath &path)
{
    _ConcurrentMap::iterator foundIter = _ruleSets.find(path);
    if (foundIter != _ruleSets.end()) {
        return *foundIter->second;
    }
    std::shared_ptr<Exec_UncompilationRuleSet> newRuleSet =
        std::make_shared<Exec_UncompilationRuleSet>();
    const auto [insertedIter, inserted] = _ruleSets.emplace(
        path, std::move(newRuleSet));
    return *insertedIter->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
