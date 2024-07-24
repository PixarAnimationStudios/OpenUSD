//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_AGGREGATE_NODE_H
#define PXR_BASE_TRACE_AGGREGATE_NODE_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/threads.h"

#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/arch/timing.h"

#include <vector>
#include "pxr/base/tf/denseHashMap.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(TraceAggregateNode);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceAggregateNode
///
/// A representation of a call tree. Each node represents one or more calls that
/// occurred in the trace. Multiple calls to a child node are aggregated into one
/// node.
///

class TraceAggregateNode : public TfRefBase, public TfWeakBase {
public:

    using This = TraceAggregateNode;
    using ThisPtr = TraceAggregateNodePtr;
    using ThisRefPtr = TraceAggregateNodeRefPtr;

    using TimeStamp = TraceEvent::TimeStamp;

    // This class is only used for validity checks.
    // FIXME: This class should be removed.
    class Id
    {
    public:
        Id() : _valid(false) {}
        Id(const TraceThreadId&) : _valid(true) {}
        bool IsValid() const { return _valid; }
    private:
        bool _valid;
    };

    static ThisRefPtr New() {
        return This::New(Id(), TfToken("root"), 0, 0);
    }

    static ThisRefPtr New(const Id &id,
                          const TfToken &key,
                          const TimeStamp ts,
                          const int count = 1,
                          const int exclusiveCount = 1) {
        return TfCreateRefPtr(new This(id, key, ts, count, exclusiveCount));
    }

    TRACE_API TraceAggregateNodeRefPtr 
    Append(Id id, const TfToken &key, TimeStamp ts,
           int c = 1, int xc = 1);

    TRACE_API void Append(TraceAggregateNodeRefPtr child);

    /// Returns the node's key.
    TfToken GetKey() { return _key;}

    /// Returns the node's id.
    const Id &GetId() { return _id;}

    /// \name Profile Data Accessors
    /// @{

    /// Returns the total time of this node ands its children.
    TimeStamp GetInclusiveTime() { return _ts; }

    /// Returns the time spent in this node but not its children.
    TRACE_API TimeStamp GetExclusiveTime(bool recursive = false);

    /// Returns the call count of this node. \p recursive determines if 
    /// recursive calls are counted.
    int GetCount(bool recursive = false) const {
        return recursive ? _recursiveCount : _count; 
    }

    /// Returns the exclusive count.
    int GetExclusiveCount() const { return _exclusiveCount; }

    /// @}


    /// \name Counter Value Accessors
    /// @{

    TRACE_API void AppendInclusiveCounterValue(int index, double value);

    TRACE_API double GetInclusiveCounterValue(int index) const;

    TRACE_API void AppendExclusiveCounterValue(int index, double value);

    TRACE_API double GetExclusiveCounterValue(int index) const;

    /// @}

    /// Recursively calculates the inclusive counter values from the inclusive 
    /// and exclusive counts of child nodes.
    TRACE_API void CalculateInclusiveCounterValues();


    /// \name Children Accessors
    /// @{
    const TraceAggregateNodePtrVector GetChildren() {
        // convert to a vector of weak ptrs
        return TraceAggregateNodePtrVector( _children.begin(),_children.end() );
    }

    const TraceAggregateNodeRefPtrVector &GetChildrenRef() {
        return _children;
    }

    TRACE_API TraceAggregateNodeRefPtr GetChild(const TfToken &key);
    TraceAggregateNodeRefPtr GetChild(const std::string &key) {
        return GetChild(TfToken(key));
    }

    /// @}


    /// Sets whether or not this node is expanded in a gui.
    void SetExpanded(bool expanded) {
        _expanded = expanded;
    }

    /// Returns whether this node is expanded in a gui.
    bool IsExpanded() {
        return _expanded;
    }

    /// Subtract \p scopeOverhead cost times the number of descendant nodes from
    /// the inclusive time of each node.  If \p numDescendantNodes is not null,
    /// add the number of nodes descendant to this node (not including this
    /// node) to *numDescendantNodes.  Also for any nodes with descendants that
    /// are "noisy" wrt \p timerQuantum, do not subtract their times from the
    /// parent's exclusive time, but instead set their times to zero.  This way
    /// we retain the sample count, but do not pollute the parent node's
    /// exclusive time with noise.
    TRACE_API void AdjustForOverheadAndNoise(
        TimeStamp scopeOverhead, TimeStamp timerQuantum,
        uint64_t *numDescendantNodes = nullptr);

    /// \name Recursion
    /// @{

    /// Scans the tree for recursive calls and updates the recursive counts.
    ///
    /// This call leaves the tree topology intact, and only updates the
    /// recursion-related data in the node.  Prior to this call, recursion
    /// data is invalid in the node.
    TRACE_API void MarkRecursiveChildren();

    /// Returns true if this node is simply a marker for a merged recursive 
    /// subtree; otherwise returns false.
    ///
    /// This value is meaningless until this node or any of its ancestors have 
    /// been marked with MarkRecursiveChildren().
    bool IsRecursionMarker() const { return _isRecursionMarker; }

    /// Returns true if this node is the head of a recursive call tree
    /// (i.e. the function has been called recursively).  
    ///
    /// This value is meaningless until this node or any of its ancestors have 
    /// been marked with MarkRecursiveChildren().
    bool IsRecursionHead() const { return _isRecursionHead; }

    /// @}


private:

    TraceAggregateNode(const Id &id, const TfToken &key, TimeStamp ts,
              int count, int exclusiveCount) :
        _id(id), _key(key), _ts(ts), _exclusiveTs(ts),
        _count(count), _exclusiveCount(exclusiveCount),
        _recursiveCount(count), _recursiveExclusiveTs(ts), _expanded(false), 
        _isRecursionMarker(false), _isRecursionHead(false),
        _isRecursionProcessed(false) {}

    using _ChildDictionary = TfDenseHashMap<TfToken, size_t, TfHash>;

    void _MergeRecursive(const TraceAggregateNodeRefPtr &node);

    void _SetAsRecursionMarker(TraceAggregateNodePtr parent);

    Id _id;
    TfToken _key;

    TimeStamp _ts;  
    TimeStamp _exclusiveTs;
    int _count;
    int _exclusiveCount;

    // We keep the recursive counts separate so that we don't mess with 
    // the collected data.
    int _recursiveCount;
    TraceAggregateNodePtr _recursionParent;
    TimeStamp _recursiveExclusiveTs;

    TraceAggregateNodeRefPtrVector _children;
    _ChildDictionary _childrenByKey;

    // A structure that holds on to the inclusive and exclusive counter
    // values. These values are usually populated together, so it's beneficial
    // to maintain them in a tightly packed structure.
    struct _CounterValue {
        _CounterValue() : inclusive(0.0), exclusive(0.0) {}
        double inclusive;
        double exclusive;
    };

    using _CounterValues = TfDenseHashMap<int, _CounterValue, TfHash>;

    // The counter values associated with specific counter indices
    _CounterValues _counterValues;
    
    unsigned int
    // If multiple Trace Editors are to be pointed at the same Reporter, this
    // might have to be changed
                _expanded:1,

    // This flag keeps track of whether or not this node is simply intended
    // as a marker for the start of a recursive call tree.
                _isRecursionMarker:1,

    // This flag keeps track of whether or not a node is the head of a
    // recursive call tree.  In other words, if it is a function that has been
    // called recursively.
                _isRecursionHead:1,

    // This flag is used during recursive traversal to mark the node as having 
    // been visited and avoid too much processing.
                _isRecursionProcessed:1;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_AGGREGATE_NODE_H
