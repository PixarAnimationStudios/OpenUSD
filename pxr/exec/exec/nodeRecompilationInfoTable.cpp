//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/nodeRecompilationInfoTable.h"

#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/nodeRecompilationInfo.h"

#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/types.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_NodeRecompilationInfoTable::WillDeleteNode(const VdfNode *const node)
{
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node->GetId());

    // It is possible that we never grew the vector large enough for this node's
    // index. In this case, there is no info to delete.
    if (nodeIndex >= _storageVector.size()) {
        return;
    }

    _Storage &storage = _storageVector[nodeIndex];

    // There is no info to delete if the _Storage at this index does not have
    // emplaced info.
    if (!storage.isInfoConstructed) {
        return;
    }

    // Delete the info.
    auto *const info =
        reinterpret_cast<Exec_NodeRecompilationInfo *>(storage.buffer);
    info->~Exec_NodeRecompilationInfo();
    storage.isInfoConstructed = false;
}

void
Exec_NodeRecompilationInfoTable::SetNodeRecompilationInfo(
    const VdfNode *const node,
    const EsfObject &provider,
    const EsfSchemaConfigKey dispatchingSchemaId,
    Exec_InputKeyVectorConstRefPtr &&inputKeys)
{
    // TODO: This tag currently fails to collect any allocations because the
    // tbb allocator doesn't doesn't obtain allocations from malloc. This is
    // something we can potentially address now that we are implementing our
    // own zero allocator.
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);

    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node->GetId());

    // Grow the vector to ensure we can store recompilation info at `nodeIndex`.
    _storageVector.grow_to_at_least(nodeIndex + 1);

    // Get a pointer to the storage. The _Storage instance itself may or may
    // not yet have been constructed by grow_to_at_least. In either case, we can
    // safely construct a _Storage at the element's address because the
    // constructor is a no-op. This makes it well-defined to read members of the
    // _Storage, as in `isInfoConstructed` below.
    //
    // It is also well-defined if another thread were to initialize a _Storage
    // at the same address while we are executing this function. In that case,
    // the `storage` variable defined here will refer to the object initialized
    // by the other thread; but since the constructor is a no-op, this has no
    // effect on the underlying memory.
    _Storage *const storage = new (&_storageVector[nodeIndex]) _Storage();

    // If this node index had previously stored recompilation info, then it
    // cleared the `isInfoConstructed` flag when the node was deleted. If this
    // is the first time using the _Storage at `nodeIndex`, then the flag will
    // be false, because the memory was provided by a zero allocator. 
    //
    // This flag is true iff recompilation info has been emplaced in the
    // storage's buffer, in which case it is an error to re-use this storage.
    if (!TF_VERIFY(!storage->isInfoConstructed,
        "Cannot set recompilation info for node '%s' at index %u, because that "
        "index is already in use.",
        node->GetDebugName().c_str(),
        nodeIndex)) {
        return;
    }

    // Initialize recompilation info in the storage's buffer.
    ::new (storage->buffer) Exec_NodeRecompilationInfo(
        provider,
        dispatchingSchemaId,
        std::move(inputKeys));
    storage->isInfoConstructed = true;
}

const Exec_NodeRecompilationInfo *
Exec_NodeRecompilationInfoTable::GetNodeRecompilationInfo(
    const VdfNode *const node) const
{
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node->GetId());

    // It is possible that we never grew the vector large enough for this node's
    // index. In this case, there is no info.
    if (nodeIndex >= _storageVector.size()) {
        return nullptr;
    }

    const _Storage &storage = _storageVector[nodeIndex];

    // The storage at this index might not have any recompilation info.
    if (!storage.isInfoConstructed) {
        return nullptr;
    }

    return reinterpret_cast<const Exec_NodeRecompilationInfo *>(storage.buffer);
}

PXR_NAMESPACE_CLOSE_SCOPE
