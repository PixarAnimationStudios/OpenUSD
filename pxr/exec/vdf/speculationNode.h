//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SPECULATION_NODE_H
#define PXR_EXEC_VDF_SPECULATION_NODE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/requiredInputsPredicate.h"

#include <tbb/concurrent_hash_map.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class VdfSchedule;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfSpeculationNode
///
/// \brief A node that pulls on a vector of value that are downstream of the
/// current execution position.
///
class VDF_API_TYPE VdfSpeculationNode final : public VdfNode
{
public:
    /// Construct a new speculation node.
    ///
    VDF_API
    VdfSpeculationNode(
        VdfNetwork *network,
        const VdfInputSpecs &inputSpecs,
        const VdfOutputSpecs &outputSpecs);

    /// Returns \c true, indicating that this node performs speculation.
    ///
    virtual bool IsSpeculationNode() const { return true; }

    /// Returns a predicate to determine the required read inputs.
    /// 
    /// For speculation nodes, this is empty since speculation nodes technically
    /// do not need input values (they compute them from their own
    /// executors). Returning anything here would cause infinite loops.
    ///
    virtual VdfRequiredInputsPredicate GetRequiredInputsPredicate(
        const VdfContext &context) const {
        return VdfRequiredInputsPredicate::NoReads(*this);
    }

    /// Executes the speculation node.
    ///
    VDF_API
    virtual void Compute(const VdfContext &context) const;

    /// Returns the schedule for this speculation node.  Schedules
    /// if necessary.
    ///
    VDF_API
    const VdfSchedule &GetSchedule(const VdfSchedule *requestingSched) const;

private:
   
    // Only a network is allowed to delete nodes.
    VDF_API
    virtual ~VdfSpeculationNode();
    
    /// Returns the executor from the given \p context. 
    ///
    const VdfExecutorInterface &_GetContextExecutor(
        const VdfContext &context) const {
        return context._GetExecutor(); 
    }

    /// Returns the request for this speculation node, given the requesting
    /// schedule \p requestingSched.  This is the request that this speculation
    /// node must compute in order to satisfy its input requirements to meet
    /// the output request by \p requestingSched.
    ///
    VdfRequest _GetInputRequest(const VdfSchedule &requestingSched) const;

    /// Looks up a schedule for the given \p inputRequest and schedules it if
    /// necessary.
    ///
    const VdfSchedule *_GetSchedule(const VdfRequest &request) const;

    /// Overridden to provide sparse dependency information in the
    /// input-to-output direction, since all outputs of a speculation
    /// node don't depend on all inputs.
    ///
    virtual VdfMask _ComputeOutputDependencyMask(
        const VdfConnection &inputConnection,
        const VdfMask &inputDependencyMask,
        const VdfOutput &output) const;

    /// Overridden to provide sparse dependency information in the
    /// output-to-input direction, since all outputs of a speculation
    /// node don't depend on all inputs.
    ///
    virtual VdfMask::Bits _ComputeInputDependencyMask(
        const VdfMaskedOutput &maskedOutput,
        const VdfConnection &inputConnection) const;

private:

    // A hash functor for VdfRequest.
    struct _HashRequest {
        bool equal(const VdfRequest &lhs, const VdfRequest &rhs) const {
            return lhs == rhs;
        }

        size_t hash(const VdfRequest &request) const {
            return VdfRequest::Hash()(request);
        }
    };

    // This is a schedule map that holds the schedules that we will use to
    // compute the node.  Invalidation is automatic from the 
    // network for which they are scheduled.
    using _ScheduleMap = tbb::concurrent_hash_map<
        VdfRequest, std::unique_ptr<VdfSchedule>, _HashRequest>;
    mutable _ScheduleMap _scheduleMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
