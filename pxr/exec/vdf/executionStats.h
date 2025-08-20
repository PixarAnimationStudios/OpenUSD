//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTION_STATS_H
#define PXR_EXEC_VDF_EXECUTION_STATS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/types.h"
#include "pxr/exec/vdf/node.h"

#include "pxr/base/arch/timing.h"

#include <tbb/concurrent_queue.h>
#include <tbb/enumerable_thread_specific.h>

#include <deque>
#include <optional>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNetwork;

/// Execution stats profiling event logger.
///
/// Clients must use a VdfExecutionStatsProcessor to interact with the results
/// logged in the stats object.
///
class VdfExecutionStats
{
    // Flag for a begin event.
    // 
    constexpr static uint8_t _StartFlag = 0x80;

    // Flag for a corresponding end event.
    // 
    constexpr static uint8_t _EndFlag = 0xC0;

public:

    /// The upper 2 bits are reserved as a flag for the event type:
    /// Highest bit : time event flag
    /// 2nd high bit : time end event flag
    ///
    /// The lower 6 bits are the base type of the event. Scoped events are 
    /// automatically tagged as begin and end events. 
    ///
    enum EventType : uint8_t {
        // Base enum of timed events
        NodeEvaluateEvent                   = 0x0,
        NodePrepareEvent                    = 0x1,
        NodeRequiredInputsEvent             = 0x2,
        NodeInputsTaskEvent                 = 0x3,

        // Single events
        NodeDidComputeEvent                 = 0x10,
        ElementsCopiedEvent                 = 0x11,
        ElementsProcessedEvent              = 0x12,
        RequestedOutputInSpeculationsEvent  = 0x13,

        // NOTE : All event types must be less than or equal to MaxEvent
        MaxEvent                            = 0x3F
    };

    typedef uint64_t EventData;

    /// Execution Stats event. Event struct that is pushed onto event vector.
    /// Should never be constructed outside of this class.
    ///
    struct Event {
        VdfId                   nodeId;
        EventData               data;  
        EventType               event;

        Event(
            EventType event, 
            VdfId nodeId, 
            EventData data) :
            nodeId(nodeId), data(data), event(event) {}
    };

    /// Scoped event that automatically logs when created and destroyed. 
    /// Should be used in the event of logging time intervals over logging
    /// single begin and end events via LogTimestamp.
    ///
    struct ScopedEvent {
        VDF_API
        ScopedEvent(
            VdfExecutionStats* stats, 
            const VdfNode& node, 
            EventType eventType);

        VDF_API
        ~ScopedEvent();

    protected:
        VdfExecutionStats*    _stats;

    private:
        const VdfNode*        _node;
        EventType             _event;
    };

    /// Scoped event that automatically pushes and pops malloc tags for the 
    /// given VdfNode.
    ///
    struct ScopedMallocEvent : public ScopedEvent {
        VDF_API
        ScopedMallocEvent(
            VdfExecutionStats* stats, 
            const VdfNode& node,
            EventType eventType);

        VDF_API
        ~ScopedMallocEvent();

    private:
        std::string _tagName;
    };

    /// Constructor for parent execution stats that have no invoking node.
    ///
    VDF_API
    explicit VdfExecutionStats(const VdfNetwork* network);

    /// Destructor
    ///
    VDF_API
    ~VdfExecutionStats();

    /// Log event API. Used to log a single event.
    ///
    void LogData(EventType event, const VdfNode& node, EventData data) {
        _Log(event, node.GetId(), data);
    }

    /// Log timestamp API.  Used to log a single timestamp.
    ///
    void LogTimestamp(EventType event, const VdfNode& node) {
        _LogTime(event, node);
    }

    /// Logs timestamped begin event.  Automatically flags the event.  
    /// NOTE : To get automatic begin and end logging on scope, use ScopedEvent.
    ///
    void LogBeginTimestamp(EventType event, const VdfNode& node) {
        _LogTime(_TagBegin(event), node);
    }

    /// Logs timestamped end event. Automatically flags the event.
    /// NOTE : To get automatic begina dn end logging on scope, use ScopedEvent.
    ///
    void LogEndTimestamp(EventType event, const VdfNode& node) {
        _LogTime(_TagEnd(event), node);
    }

    /// Push execution stats onto the hierarchy queue.
    ///
    VDF_API
    VdfExecutionStats* AddSubStat(
        const VdfNetwork* network, 
        const VdfNode* invokingNode);

    /// Returns the invoking node, if any.
    ///
    const std::optional<VdfId>& GetInvokingNodeId() const {
        return _invokingNodeId;
    }

    /// Returns a unique name for the given node.
    ///
    VDF_API
    static std::string GetMallocTagName(
        const VdfId *invokingNodeId,
        const VdfNode &node);

    /// Returns the base event (e.g. event type specified by the lower 6 bits).
    ///
    static EventType GetBaseEvent(EventType event) {
        return (EventType)(static_cast<uint8_t>(event) & 0x3F);
    }

    /// Returns true if the event is an end event (e.g. if the second highest
    /// bit is set).
    ///
    static bool IsEndEvent(EventType event) {
        return 0x40 & static_cast<uint8_t>(event);
    }

    /// Returns true if the event is a begin event.
    ///
    static bool IsBeginEvent(EventType event) {
        return !IsEndEvent(event);
    }

protected:
    /// Sub stat constructor. Only called from _AddSubStat.
    ///
    VDF_API
    VdfExecutionStats(const VdfNetwork* network, VdfId nodeId);

    /// Logs data.
    ///
    void _Log(EventType event, VdfId nodeId, EventData data) 
    {
        _events.local().events.push_back(Event(event, nodeId, data));
    }

    /// Logs timestamp.
    ///
    void _LogTime(EventType event, const VdfNode& node)
    {
        _Log(event, node.GetId(), ArchGetTickTime());
    }

    /// Adds sub stat.
    ///
    VDF_API
    VdfExecutionStats* _AddSubStat(
        const VdfNetwork* network,
        VdfId invokingNodeId);

    /// Tags the begin flag.
    ///
    EventType _TagBegin(EventType event) {
        return (EventType)(static_cast<uint8_t>(event) | _StartFlag);
    }

    /// Tags the end flag.
    ///
    EventType _TagEnd(EventType event) {
        return (EventType)(static_cast<uint8_t>(event) | _EndFlag);
    }

private:  
    friend class VdfExecutionStatsProcessor;

    // Pointer to the VdfNetwork for whose nodes this execution stats tracks.
    //
    const VdfNetwork* _network;

    // Index of the VdfNode that calls into this network. 
    //
    std::optional<VdfId> _invokingNodeId;

    // A structure of events recorded per thread.
    //
    struct _PerThreadEvents {
        _PerThreadEvents() : threadId(std::this_thread::get_id()) {}

        // The thread id
        std::thread::id threadId;

        // The vector of events recorded
        typedef std::deque<Event> EventVector;
        EventVector events;
    };

    // The per-thread event vectors.
    //
    tbb::enumerable_thread_specific<_PerThreadEvents> _events;

    // Concurrent vector of VdfExecutionStats to keep track of execution stats
    // from networks (i.e. sharing network) that are invoked during computation
    // while profiling.
    //
    tbb::concurrent_queue<VdfExecutionStats*> _subStats;
};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
