//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#ifndef TRACE_SINGLE_EVENT_REPORT_H
#define TRACE_SINGLE_EVENT_REPORT_H

#include "pxr/pxr.h"

#include "pxr/base/trace/collection.h"
#include "pxr/base/trace/singleEventNode.h"
#include "pxr/base/trace/singleEventGraph.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class TraceSingleEventTreeReport
///
/// This class creates a tree of TraceSingleEventGraph instances from
/// TraceCollection instances.
///
class TraceSingleEventTreeReport
    : public TraceCollection::Visitor {
public:
    /// Constructor.
    TraceSingleEventTreeReport();

    /// Returns the root of the tree.
    TraceSingleEventGraphRefPtr GetGraph() { return _graph; }

    /// \name TraceCollection::Visitor Interface
    /// @{
    virtual void OnBeginCollection() override;
    virtual void OnEndCollection() override;
    virtual bool AcceptsCategory(TraceCategoryId) override;
    virtual void OnBeginThread(const TraceThreadId&) override;
    virtual void OnEndThread(const TraceThreadId&) override;
    virtual void OnEvent(
        const TraceThreadId&, const TfToken&, const TraceEvent&) override;
    /// @}

private:

    // Helper class for event graph creation.
    struct _PendingSingleEventNode {
        using TimeStamp = TraceEvent::TimeStamp;

        struct AttributeData {
            TimeStamp time;
            TfToken key;
            TraceSingleEventNode::AttributeData data;
        };

        _PendingSingleEventNode( const TfToken& key, 
                                 TraceCategoryId category,
                                 TimeStamp start);
        TraceSingleEventNodeRefPtr Close(TimeStamp end, bool separateEvents);

        TfToken key;
        TraceCategoryId category;
        TimeStamp start;
        std::vector<TraceSingleEventNodeRefPtr> children;
        std::vector<AttributeData> attributes;
    };

    void _OnBegin(const TraceThreadId&, const TfToken&, const TraceEvent&);
    void _OnEnd(const TraceThreadId&, const TfToken&, const TraceEvent&);
    void _OnCounterDelta(const TraceThreadId&, const TfToken&, const TraceEvent&);
    void _OnCounterValue(const TraceThreadId&, const TfToken&, const TraceEvent&);
    void _OnData(const TraceThreadId&, const TfToken&, const TraceEvent&);
    void _OnTimespan(const TraceThreadId&, const TfToken&, const TraceEvent&);

    using _PendingNodeStack = std::vector<_PendingSingleEventNode>;
    using _ThreadStackMap = std::map<TraceThreadId, _PendingNodeStack>;

    struct _CounterValue {
        double value;
        bool isDelta;
    };

    using _CounterValues = std::map<TraceEvent::TimeStamp, _CounterValue>;
    using _CounterMap = std::map<TfToken, _CounterValues>;

    TraceSingleEventNodeRefPtr _root;
    _ThreadStackMap _threadStacks;
    _CounterMap _counterDeltas;
    TraceSingleEventGraphRefPtr _graph;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_SINGLE_EVENT_REPORT_H