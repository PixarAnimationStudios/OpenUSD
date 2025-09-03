//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_POOL_CHAIN_INDEXER_H
#define PXR_EXEC_VDF_POOL_CHAIN_INDEXER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/dynamicTopologicalSorter.h"
#include "pxr/exec/vdf/poolChainIndex.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

/// Forward declarations
class VdfConnection;
class VdfOutput;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfPoolChainIndexer
///
/// A class that uniquely indexes all VdfOutputs in point pool chains of a 
/// given network such that outputs of nodes further downstream are guarenteed
/// to have higher index (outputs of adjacent nodes in the pool chain do not
/// necessarily have consecutive indices, e.g., in the presence of parallel
/// movers).
///
class VdfPoolChainIndexer
{
public:

    /// Constructs a pool chain indexer.
    ///
    VDF_API
    VdfPoolChainIndexer();

    /// Destructor
    ///
    VDF_API
    ~VdfPoolChainIndexer();

    /// Returns the index of the output.
    /// 
    /// \note This method is not thread-safe.
    ///
    VDF_API
    VdfPoolChainIndex GetIndex(const VdfOutput &output) const;

    /// Insert a connection into the indexed ordering.  Non-pool connections
    /// are ignored.
    /// 
    /// \note It is safe to call this method concurrently with Insert() and
    /// Remove().
    ///
    VDF_API
    void Insert(const VdfConnection &connection);

    /// Remove a connection from the indexed ordering.  Non-pool connections
    /// are ignored.
    /// 
    /// \note It is safe to call this method concurrently with Insert() and
    /// Remove().
    ///
    VDF_API
    void Remove(const VdfConnection &connection);

    /// Remove all outputs.
    /// 
    /// \note This method is not thread-safe.
    ///
    VDF_API
    void Clear();

private:

    // The topological sorter for pool outputs.
    using _PoolOutputSorter = VdfDynamicTopologicalSorter<const VdfOutput *>;
    _PoolOutputSorter _sorter;

    // Synchronize concurrent sorter access from Insert() and Remove().
    std::mutex _sorterMutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
