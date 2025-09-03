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

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/pointerAndBits.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(TraceEventNode);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceEventNode
///
/// TraceEventNode is used to represents call tree of a trace. Each node 
/// represents a Begin-End trace event pair, or a single Timespan event. This is
/// useful for timeline views of a trace.
///

class TraceEventNode : public TfSimpleRefBase {
public:

    using TimeStamp = TraceEvent::TimeStamp;
    using AttributeData = TraceEventData;
    using AttributeMap = std::multimap<TfToken, AttributeData>;

    /// Creates a new root node.
    ///
    TRACE_API static TraceEventNodeRefPtr New();
    
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
    TRACE_API TraceEventNodeRefPtr Append(const TfToken &key, 
                                          TraceCategoryId category,
                                          TimeStamp beginTime,
                                          TimeStamp endTime,
                                          bool separateEvents);
    
    /// Appends \p node as a child node.
    TRACE_API void Append(TraceEventNodeRefPtr node);
    
    /// Returns the name of this node.
    const TfToken &GetKey() const { return _key; }

    /// Returns the category of this node.
    TraceCategoryId GetCategory() const { return _category; }

    /// Sets this node's begin and end time to the time extents of its direct 
    /// children.
    TRACE_API void SetBeginAndEndTimesFromChildren();

    /// \name Profile Data Accessors
    /// @{

    /// Returns the time that this scope started.
    TimeStamp GetBeginTime() const { return _beginTime; }

    /// Returns the time that this scope ended.
    TimeStamp GetEndTime() const { return _endTime; }

    /// @}

    /// \name Children Accessors
    /// @{

    /// Returns a TfSpan of references to the children of this node.
    TfSpan<const TraceEventNodeRefPtr> GetChildrenRef() const {
        return _children;
    }

    /// @}

    /// Return the data associated with this node.
    TRACE_API const AttributeMap& GetAttributes() const;

    /// Add data to this node.
    TRACE_API void AddAttribute(const TfToken& key, AttributeData&& attr);

    /// Returns whether this node was created from a Begin-End pair or a single
    /// Timespan event.
    bool IsFromSeparateEvents() const {
        return _attributesAndSeparateEvents.BitsAs<bool>();
    }

    ~TraceEventNode() {
        if (AttributeMap *attrMap = _attributesAndSeparateEvents.Get()) {
            _DeleteAttrMap(attrMap);
        }
    }

private:

    TraceEventNode(
        const TfToken &key,
        TraceCategoryId category,
        TimeStamp beginTime, 
        TimeStamp endTime,
        TraceEventNodeRefPtrVector&& children,
        bool separateEvents)

        : _category(category)
        , _key(key)
        , _beginTime(beginTime)
        , _endTime(endTime)
        , _children(std::make_move_iterator(children.begin()),
                    std::make_move_iterator(children.end()))
        , _attributesAndSeparateEvents(nullptr, separateEvents)
    {
    }

    // Out-of-line to avoid inlining the multimap dtor code.
    TRACE_API void _DeleteAttrMap(AttributeMap *attrMap);

    // _category (4 bytes) is first so it packs with TfRefBase's 4-byte count.
    const TraceCategoryId _category;
    const TfToken _key;
    TimeStamp _beginTime;
    TimeStamp _endTime;
    // Empirical results show ~85% of nodes have < 2 children.
    TfSmallVector<TraceEventNodeRefPtr, 1> _children;
    TfPointerAndBits<AttributeMap> _attributesAndSeparateEvents;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_EVENT_NODE_H
