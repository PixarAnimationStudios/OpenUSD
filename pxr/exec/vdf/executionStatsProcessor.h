//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTION_STATS_PROCESSOR_H
#define PXR_EXEC_VDF_EXECUTION_STATS_PROCESSOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/executionStats.h"
#include "pxr/exec/vdf/types.h"

#include <optional>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

class VDF_API_TYPE VdfExecutionStatsProcessor 
{
public:

    /// A per-thread id.
    ///
    typedef std::thread::id ThreadId;

    /// Destructor.
    ///
    VDF_API
    virtual ~VdfExecutionStatsProcessor();

    /// Processes the given execution stats to fill the processor with processed
    /// stats. 
    ///
    VDF_API
    void Process(const VdfExecutionStats* stats);

    /// Returns the network pointer.  Should only be called after Process has
    /// been called.
    ///
    /// XXX :
    /// In the future, we should remove direct network access and limit network
    /// access via an API in the processor that only allows client classes to
    /// access node pointers if they are valid / current.
    ///
    VDF_API
    const VdfNetwork* GetNetwork() const;

protected:
    /// Protected constructor.
    ///
    VDF_API
    VdfExecutionStatsProcessor();

    /// Virtual method for processing a single event given a thread id and 
    /// an event.  Called by _ProcessEvents.
    ///
    virtual void _ProcessEvent(
        ThreadId threadId,
        const VdfExecutionStats::Event& event) = 0;

    /// Virtual method for processing a single sub stat given a sub stat.
    /// Called by _ProcessSubStats.
    /// NOTE : No child class hold on to the subStats pointer.  The lifetime of 
    ///        substats is managed inside MfExecInterpreter, so there is no 
    ///        guarantee that the subStats pointer will be valid except during
    ///        scope of this function.
    ///
    virtual void _ProcessSubStat(const VdfExecutionStats* subStat) = 0;

    /// Virtual method that runs before processing. Should be used to set up
    /// results.
    ///
    virtual void _PreProcess() {}

    /// Virtual method that runs after processing. Should be used to aggregate 
    /// results.
    ///
    virtual void _PostProcess() {}

private:
    /// Goes through all event vectors and calls _ProcessEvent on each event.
    /// Arbitrarily assigns a thread id to each event vector. So it is 
    /// guaranteed that events in the same event vector will have the same 
    /// thread id.  However, this does not necessarily map to the os thread id.
    ///
    VDF_API
    void _ProcessEvents(const VdfExecutionStats* stats);

    /// Goes through all sub stats and calls _ProcessSubStat on each substat.
    ///
    VDF_API
    void _ProcessSubStats(const VdfExecutionStats* stats);

private:
    const VdfNetwork*       _network;
    std::optional<VdfId>    _invokingNodeId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* VDF_EXECUTION_STATS_PROCESSOR */
