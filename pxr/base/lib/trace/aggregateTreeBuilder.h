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

#ifndef TRACE_AGGREGATE_TREE_BUILDER_H
#define TRACE_AGGREGATE_TREE_BUILDER_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/collection.h"
#include "pxr/base/trace/aggregateTree.h"
#include "pxr/base/trace/eventTree.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// \class Trace_AggregateTreeBuilder
///
/// This class populates a tree of TraceAggregateTree instances from
/// TraceCollection instances.
///
///
class Trace_AggregateTreeBuilder : private TraceCollection::Visitor
{
public:
    static void AddCollectionDataToTree(
        TraceAggregateTree* tree, const TraceCollection& collection);

private:
    Trace_AggregateTreeBuilder(
        TraceAggregateTree* tree, const TraceCollection& collection);

    void _ProcessCounters(const TraceCollection& collection);

    void _CreateAggregateNodes();

    // TraceCollection::Visitor interface
    virtual void OnBeginCollection() override;
    virtual void OnEndCollection() override;
    virtual void OnBeginThread(const TraceThreadId& threadId) override;
    virtual void OnEndThread(const TraceThreadId& threadId) override;
    virtual bool AcceptsCategory(TraceCategoryId categoryId) override;
    virtual void OnEvent(
        const TraceThreadId& threadIndex, 
        const TfToken& key, 
        const TraceEvent& e) override;

    void _OnCounterEvent(const TraceThreadId& threadIndex, 
        const TfToken& key, 
        const TraceEvent& e);

    TraceAggregateNodePtr _FindAggregateNode(
        const TraceThreadId& threadId, const TraceEvent::TimeStamp ts) const ;

    TraceAggregateTree* _aggregateTree;
    TraceEventTreeRefPtr _tree;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_AGGREGATE_TREE_BUILDER_H
