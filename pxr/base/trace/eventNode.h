//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_EVENT_NODE_H
#define PXR_BASE_TRACE_EVENT_NODE_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/eventData.h"

#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/declarePtrs.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(TraceEventNode);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceEventNode
///
/// TraceEventNode is used to represents call tree of a trace. Each node 
/// represents a Begin-End trace event pair, or a single Timespan event. This is
/// useful for timeline views of a trace.
///

class TraceEventNode : public TfRefBase {
public:

    using TimeStamp = TraceEvent::TimeStamp;
    using AttributeData = TraceEventData;
    using AttributeMap = std::multimap<TfToken, AttributeData>;

    /// Creates a new root node.
    ///
    static TraceEventNodeRefPtr New() {
        return TraceEventNode::New(
            TfToken("root"), TraceCategory::Default, 0.0, 0.0, {}, false);
    }

    /// Creates a new node with \p key, \p category, \p beginTime and 
    /// \p endTime.
    static TraceEventNodeRefPtr New(const TfToken &key,
                          const TraceCategoryId category,
                          const TimeStamp beginTime,
                          const TimeStamp endTime,
                          TraceEventNodeRefPtrVector&& children,
                          const bool separateEvents) {
        return TfCreateRefPtr(
            new TraceEventNode(
                key,
                category,
                beginTime,
                endTime,
                std::move(children),
                separateEvents));
    }

    /// Appends a new child node with \p key, \p category, \p beginTime and 
    /// \p endTime.
    TraceEventNodeRefPtr Append(const TfToken &key, 
                                      TraceCategoryId category,
                                      TimeStamp beginTime,
                                      TimeStamp endTime,
                                      bool separateEvents);

    /// Appends \p node as a child node.
    void Append(TraceEventNodeRefPtr node);

    /// Returns the name of this node.
    TfToken GetKey() { return _key;}

    /// Returns the category of this node.
    TraceCategoryId GetCategory() const { return _category; }

    /// Sets this node's begin and end time to the time extents of its direct 
    /// children.
    void SetBeginAndEndTimesFromChildren();

    /// \name Profile Data Accessors
    /// @{

    /// Returns the time that this scope started.
    TimeStamp GetBeginTime() { return _beginTime; }

    /// Returns the time that this scope ended.
    TimeStamp GetEndTime()   { return _endTime; }

    /// @}

    /// \name Children Accessors
    /// @{

    /// Returns references to the children of this node.
    const TraceEventNodeRefPtrVector &GetChildrenRef() {
        return _children;
    }

    /// @}

    /// Return the data associated with this node.
    const AttributeMap& GetAttributes() const { return _attributes; }

    /// Add data to this node.
    void AddAttribute(const TfToken& key, const AttributeData& attr);

    /// Returns whether this node was created from a Begin-End pair or a single
    /// Timespan event.
    bool IsFromSeparateEvents() const {
        return _fromSeparateEvents;
    }

private:

    TraceEventNode(
        const TfToken &key,
        TraceCategoryId category,
        TimeStamp beginTime, 
        TimeStamp endTime,
        TraceEventNodeRefPtrVector&& children,
        bool separateEvents)

        : _key(key)
        , _category(category)
        , _beginTime(beginTime)
        , _endTime(endTime)
        , _children(std::move(children))
        , _fromSeparateEvents(separateEvents)
    {}


    TfToken _key;
    TraceCategoryId _category;
    TimeStamp _beginTime;
    TimeStamp _endTime;
    TraceEventNodeRefPtrVector _children;
    bool _fromSeparateEvents;

    AttributeMap _attributes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_EVENT_NODE_H
