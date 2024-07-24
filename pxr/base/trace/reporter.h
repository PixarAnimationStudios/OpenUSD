//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_REPORTER_H
#define PXR_BASE_TRACE_REPORTER_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/aggregateNode.h"
#include "pxr/base/trace/reporterBase.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/staticTokens.h"

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define TRACE_REPORTER_TOKENS       \
    ((warningString, "WARNING:"))

TF_DECLARE_PUBLIC_TOKENS(TraceReporterTokens, TRACE_API, TRACE_REPORTER_TOKENS);


TF_DECLARE_WEAK_AND_REF_PTRS(TraceAggregateTree);
TF_DECLARE_WEAK_AND_REF_PTRS(TraceEventNode);
TF_DECLARE_WEAK_AND_REF_PTRS(TraceEventTree);

TF_DECLARE_WEAK_AND_REF_PTRS(TraceReporter);

class TraceCollectionAvailable;

////////////////////////////////////////////////////////////////////////////////
/// \class TraceReporter
///
/// This class converts streams of TraceEvent objects into call trees which
/// can then be used as a data source to a GUI or written out to a file.
///
class TraceReporter : 
    public TraceReporterBase {
public:

    TF_MALLOC_TAG_NEW("Trace", "TraceReporter");
    
    using This = TraceReporter;
    using ThisPtr = TraceReporterPtr;
    using ThisRefPtr = TraceReporterRefPtr;

    using Event = TraceEvent;
    using TimeStamp = TraceEvent::TimeStamp;
    using CounterMap = TfHashMap<TfToken, double, TfToken::HashFunctor>;

    /// Create a new reporter with \a label and \a dataSource.
    static ThisRefPtr New(const std::string& label,
                          DataSourcePtr dataSource) {
        return TfCreateRefPtr(new This(label, std::move(dataSource)));
    }

    /// Create a new reporter with \a label and no data source.
    static ThisRefPtr New(const std::string& label) {
        return TfCreateRefPtr(new This(label, nullptr));
    }

    /// Returns the global reporter.
    TRACE_API static TraceReporterPtr GetGlobalReporter();
   
    /// Destructor.
    TRACE_API virtual ~TraceReporter();

    /// Return the label associated with this reporter.
    const std::string& GetLabel() {
        return _label;
    }

    /// \name Report Generation.
    /// @{

    /// Generates a report to the ostream \a s, dividing all times by 
    /// \a iterationCount.
    TRACE_API void Report(
        std::ostream &s,
        int iterationCount=1);

    /// Generates a report of the times to the ostream \a s.
    TRACE_API void ReportTimes(std::ostream &s);

    /// Generates a timeline trace report suitable for viewing in
    /// Chrome's trace viewer.
    TRACE_API void ReportChromeTracing(std::ostream &s);

    /// @}

    /// \name Report Loading.
    /// @{

    /// Aggregate tree and its iteration count, parsed from a report.
    struct ParsedTree {
        TraceAggregateTreeRefPtr tree;
        int iterationCount;  
    };

    /// Load an aggregate tree report from the \p stream, as written by 
    /// Report().
    ///
    /// Since multiple reports may be appended to a given trace file, this will
    /// return a vector of each tree and their iteration count. 
    ///
    /// This will multiply the parsed values for each aggregate tree by their 
    /// iteration count.
    TRACE_API static std::vector<ParsedTree> LoadReport(
        std::istream &stream);

    /// @}

    /// Returns the root node of the aggregated call tree.
    TRACE_API TraceAggregateNodePtr GetAggregateTreeRoot();

    /// Returns the root node of the call tree.
    TRACE_API TraceEventNodeRefPtr GetEventRoot();

    /// Returns the event call tree
    TRACE_API TraceEventTreeRefPtr GetEventTree();

    /// \name Counters
    /// @{

    /// Returns a map of counters (counter keys), associated with their total
    /// accumulated value. Each individual event node in the tree may also hold
    /// on to an inclusive and exclusive value for the given counter.
    TRACE_API const CounterMap & GetCounters() const;

    /// Returns the numeric index associated with a counter key. Counter values
    /// on the event nodes will have to be looked up by the numeric index.
    TRACE_API int GetCounterIndex(const TfToken &key) const;

    /// Add a counter to the reporter. This method can be used to restore a
    /// previous trace state and tree. Note, that the counter being added must
    /// have a unique key and index. The method will return false if a key or
    /// index already exists.
    TRACE_API bool AddCounter(const TfToken &key, int index, double totalValue);

    /// @}

    /// This fully re-builds the event and aggregate trees from whatever the 
    /// current collection holds.  It is ok to call this multiple times in case 
    /// the collection gets appended on inbetween. 
    ///
    /// If we want to have multiple reporters per collector, this will need to
    /// be changed so that all reporters reporting on a collector update their
    /// respective trees. 
    TRACE_API void UpdateTraceTrees();
    
    /// Clears event tree and counters.
    TRACE_API void ClearTree();

    /// \name Report options.
    /// @{

    /// This affects only stack trace event reporting.  If \c true then all
    /// events in a function are grouped together otherwise events are split
    /// out by address.
    TRACE_API void SetGroupByFunction(bool);

    /// Returns the current group-by-function state.
    TRACE_API bool GetGroupByFunction() const;

    /// When stack trace event reporting, this sets whether or not recursive 
    /// calls are folded in the output.  Recursion folding is useful when
    /// the stacks contain deep recursive structures.
    TRACE_API void SetFoldRecursiveCalls(bool);

    /// Returns the current setting for recursion folding for stack trace
    /// event reporting.
    TRACE_API bool GetFoldRecursiveCalls() const;

    /// Set whether or not the reporter should adjust scope times for overhead
    /// and noise.
    TRACE_API void SetShouldAdjustForOverheadAndNoise(bool adjust);

    /// Returns the current setting for addjusting scope times for overhead and
    /// noise.
    TRACE_API bool ShouldAdjustForOverheadAndNoise() const;

    /// @}

    /// Creates a valid TraceAggregateNode::Id object.
    /// This should be used by very few clients for certain special cases.
    /// For most cases, the TraceAggregateNode::Id object should be created and 
    /// populated internally within the Reporter object itself.
    TRACE_API static TraceAggregateNode::Id CreateValidEventId();

protected:

    TRACE_API TraceReporter(const std::string& label,
                   DataSourcePtr dataSource);

private:
    void _ProcessCollection(const TraceReporterBase::CollectionPtr&) override;
    void _RebuildEventAndAggregateTrees();
    void _PrintTimes(std::ostream &s);

private:
    std::string _label;

    bool _groupByFunction;
    bool _foldRecursiveCalls;
    bool _shouldAdjustForOverheadAndNoise;

    TraceAggregateTreeRefPtr _aggregateTree;
    TraceEventTreeRefPtr _eventTree;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_REPORTER_H
