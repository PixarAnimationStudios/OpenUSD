//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_NODE_RECOMPILATION_INFO_TABLE_H
#define PXR_EXEC_EXEC_NODE_RECOMPILATION_INFO_TABLE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/nodeRecompilationInfo.h"

#include "pxr/base/work/zeroAllocator.h"

#include <tbb/concurrent_vector.h>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

/// Manages Exec_NodeRecompilationInfo instances for each node in the network.
class Exec_NodeRecompilationInfoTable
{
public:
    /// De-initializes recompilation info for the deleted \p node if it has any.
    void WillDeleteNode(const VdfNode *node);

    /// Sets the recompilation info for the given \p node.
    ///
    /// Recompilation info can only be set once per node.
    ///
    /// \note
    /// This method can be called concurrently with itself. It cannot be called
    /// concurrently with GetNodeRecompilationInfo or WillDeleteNode.
    ///
    void SetNodeRecompilationInfo(
        const VdfNode *node,
        const EsfObject &provider,
        EsfSchemaConfigKey dispatchingSchemaId,
        Exec_InputKeyVectorConstRefPtr &&inputKeys);

    /// Gets the recompiation info for the given \p node.
    ///
    /// This returns nullptr if there is no recompilation info for the given
    /// \p node.
    ///
    const Exec_NodeRecompilationInfo *GetNodeRecompilationInfo(
        const VdfNode *node) const;

private:
    struct _Storage
    {
        // _Storage must have a user-defined no-op constructor.
        //
        // By defining the default constructor in this way, any constructor
        // invoked by tbb::concurrent_vector will not alter the underlying
        // bytes.
        _Storage() {}

        // Instances of Exec_NodeRecompilationInfo are emplaced into this
        // buffer.
        alignas(Exec_NodeRecompilationInfo)
        std::byte buffer[sizeof(Exec_NodeRecompilationInfo)];

        // True if the buffer holds an emplaced Exec_NodeRecompilationInfo.
        //
        // Instances of _Storage are allocated by a zero allocator. 
        // Therefore, if memory for a _Storage has been allocated, this flag
        // will be false, even if the _Storage has not been constructed.
        bool isInfoConstructed;
    };

    using _StorageVector =
        tbb::concurrent_vector<_Storage, 
            WorkZeroAllocator<_Storage>>;
    _StorageVector _storageVector;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
