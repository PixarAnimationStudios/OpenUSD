//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compiledOutputCache.h"

#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
Exec_CompiledOutputCache::Insert(
    const Exec_OutputKey::Identity &key,
    const VdfMaskedOutput &maskedOutput)
{
    const auto [it, inserted] = _outputMap.emplace(key, maskedOutput);
    if (!inserted) {
        return false;
    }

    // We expect that the caller provided a non-null VdfOutput. If not, we still
    // inserted the entry and must return true here.
    if (!TF_VERIFY(maskedOutput.GetOutput())) {
        return true;
    }

    const VdfId nodeId = maskedOutput.GetOutput()->GetNode().GetId();
    _reverseMap[nodeId].push_back(key);
    return true;
}

std::tuple<const VdfMaskedOutput &, bool>
Exec_CompiledOutputCache::Find(const Exec_OutputKey::Identity &key) const
{
    const _OutputMap::const_iterator it = _outputMap.find(key);
    if (it == _outputMap.end()) {
        return {_invalidMaskedOutput, false};
    }

    return {it->second, true};
}

void
Exec_CompiledOutputCache::EraseByNodeId(VdfId nodeId)
{
    const _ReverseMap::iterator it = _reverseMap.find(nodeId);
    
    // Not finding an entry in the reverse map is not an error, because there
    // are a couple instances where nodes won't have any output keys associated
    // with them (e.g. leaf nodes, value conversion nodes).
    if (it == _reverseMap.end()) {
        return;
    }

    for (const Exec_OutputKey::Identity &key : it->second) {
        _outputMap.unsafe_erase(key);
    }
    _reverseMap.unsafe_erase(it);
}

PXR_NAMESPACE_CLOSE_SCOPE
