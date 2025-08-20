//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_NODE_PROCESS_INVALIDATION_INTERFACE_H
#define PXR_EXEC_VDF_NODE_PROCESS_INVALIDATION_INTERFACE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorInterface;
class VdfInputSpecs;
class VdfNetwork;

/// \class VdfNodeProcessInvalidationInterface
///
/// Interface class for nodes that receive notification about input invalidation
/// via a virtual method.
///
class VDF_API_TYPE VdfNodeProcessInvalidationInterface
{
public:
    /// Process invalidation on all the nodes contained in the inputs sets. 
    ///
    VDF_API
    static void ProcessInvalidation(
        const VdfExecutorInterface *executor,
        const VdfNodeToInputPtrVectorMap &inputs);

    /// Process invalidation on the specified node via the given inputs. 
    ///
    VDF_API
    static void ProcessInvalidation(
        const VdfExecutorInterface *executor,
        const VdfNodeProcessInvalidationInterface &node,
        const VdfInputPtrVector &inputs);

protected:

    /// The invalidation callback on this node.
    ///
    virtual void _ProcessInvalidation(
        const VdfInputPtrVector *inputs,
        const VdfExecutorInterface *executor) const = 0;

    /// Process invalidation for one entry in the map.
    ///
    VDF_API
    static void _ProcessInvalidationForEntry(
        const VdfExecutorInterface *executor,
        const VdfNodeToInputPtrVectorMap::value_type &entry);

    /// Protected destructor.
    ///
    VDF_API
    virtual ~VdfNodeProcessInvalidationInterface();

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
