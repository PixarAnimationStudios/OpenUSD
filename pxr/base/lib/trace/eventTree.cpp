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

#include "pxr/base/trace/eventTree.h"

#include "pxr/pxr.h"

#include "pxr/base/trace/eventTreeBuilder.h"

#include "pxr/base/js/value.h"

#include <boost/optional.hpp>

PXR_NAMESPACE_OPEN_SCOPE

TraceEventTreeRefPtr
TraceEventTree::New(
        const TraceCollection& collection,
        const CounterMap* initialCounterValues)
{
    Trace_EventTreeBuilder graphBuilder;
    if (initialCounterValues) {
        graphBuilder.SetCounterValues(*initialCounterValues);
    }
    graphBuilder.CreateTree(collection);
    return graphBuilder.GetTree();
}

void
TraceEventTree::Add(const TraceCollection& collection)
{
    CounterMap currentCounters = GetFinalCounterValues();
    TraceEventTreeRefPtr newGraph = New(collection, &currentCounters);
    Merge(newGraph);
}

void
TraceEventTree::Merge(const TraceEventTreeRefPtr& tree)
{
    // Add the node to the tree.
    for(TraceEventNodeRefPtr newThreadNode 
            : tree->GetRoot()->GetChildrenRef()) {

        const TraceEventNodeRefPtrVector& threadNodes =
            _root->GetChildrenRef();

        // Find if the tree already has a node for child thread.
        auto it = std::find_if(
                threadNodes.begin(), 
                threadNodes.end(), 
                [&](const TraceEventNodeRefPtr& node) {
                    return node->GetKey() == newThreadNode->GetKey();
                });

        if (it != threadNodes.end()) {
            // Add the nodes thread children from child into the current tree.
            for(TraceEventNodeRefPtr threadChild 
                    : newThreadNode->GetChildrenRef()) {
                (*it)->Append(threadChild);
            }
            // Update the thread times from the newly added children.
            (*it)->SetBeginAndEndTimesFromChildren();
        } else {
            // Add the thread if it wasn't already in the tree.
            _root->Append(newThreadNode);
        }
    }

    // Add the counter data.
    for (CounterValuesMap::value_type& p : tree->_counters) {
        CounterValuesMap::iterator it = _counters.find(p.first);
        if (it == _counters.end()) {
            // Add new counter values;
            _counters.insert(p);
        } else {
            // Merge new counter values to existing counter values.
            const size_t originalSize = it->second.size();
            it->second.insert(
                it->second.end(), p.second.begin(), p.second.end());
            std::inplace_merge(
                it->second.begin(), 
                it->second.begin() + originalSize,
                it->second.end());
        }
    }

    // Add the marker data.
    for (MarkerValuesMap::value_type& p : tree->_markers) {
        MarkerValuesMap::iterator it = _markers.find(p.first);
        if (it == _markers.end()) {
            // Add new markers values;
            _markers.insert(p);
        } else {
            // Merge new marker values to existing marker values.
            const size_t originalSize = it->second.size();
            it->second.insert(
                it->second.end(), p.second.begin(), p.second.end());
            std::inplace_merge(
                it->second.begin(), 
                it->second.begin() + originalSize,
                it->second.end());
        }
    }
}

static 
JsValue
_TimeStampToChromeTraceValue(TraceEvent::TimeStamp t)
{
    // Chrome trace format uses timestamps in microseconds.
    return JsValue(ArchTicksToNanoseconds(t)/1000.0);
}


// Recursively adds JSON objects representing call tree nodes to the array.
static 
void TraceEventTree_AddToJsonArray(
    const TraceEventNodeRefPtr &node,
    const int pid,
    const TraceThreadId& threadId,
    JsArray *array)
{
    std::string categoryList("");

    JsObject dict;

    // Add begin time
    std::vector<std::string> catList = 
        TraceCategory::GetInstance().GetCategories(node->GetCategory());
    for (const std::string& catName : catList) {
        if (categoryList.length() > 0) {
            categoryList.append(",");
        }
        categoryList.append(catName);
    }
    dict["cat"]  = JsValue(categoryList);
    dict["libTraceCatId"] = JsValue(static_cast<uint64_t>(node->GetCategory()));
    dict["pid"]  = JsValue(pid);
    dict["tid"]  = JsValue(threadId.ToString()); 
    dict["name"] = JsValue(node->GetKey().GetString());

    dict["ts"]   = _TimeStampToChromeTraceValue(node->GetBeginTime());

    if (!node->GetAttributes().empty()) {
        JsObject attrs;
        using AttributeMap = TraceEventNode::AttributeMap;
        for (const AttributeMap::value_type& it : node->GetAttributes()) {
            if (attrs.find(it.first.GetString()) == attrs.end()) {
                using AttrItr = AttributeMap::const_iterator;
                using Range = std::pair<AttrItr,AttrItr>;
                Range range =node->GetAttributes().equal_range(it.first);
                if (std::distance(range.first, range.second) == 1) {
                    attrs.emplace(range.first->first.GetString(), 
                        range.first->second.ToJson());
                } else {
                    JsArray values;
                    for (AttrItr i = range.first; i != range.second; ++i) {
                        values.push_back(i->second.ToJson());
                    }
                    attrs.emplace(it.first.GetString(), std::move(values));
                }
            }
        }
        dict["args"] = attrs;
    }

    if (!node->IsFromSeparateEvents()) 
    {
        dict["ph"] = JsValue("X"); // Complete event
        dict["dur"] = _TimeStampToChromeTraceValue(
            node->GetEndTime() - node->GetBeginTime());
        array->push_back(dict);
    } else {
        dict["ph"] = JsValue("B"); // begin time
        array->push_back(dict);
        
        // Remove the args attribute so it is not also written in the end event.
        JsObject::iterator argIt = dict.find("args");
        if (argIt != dict.end()) {
            dict.erase(argIt);
        }

        // Add end time
        dict["ph"]   = JsValue("E"); // end time
        dict["ts"]   = _TimeStampToChromeTraceValue(node->GetEndTime());

        array->push_back(dict);
    }

    // Recurse on the children
    for (const TraceEventNodeRefPtr& c : node->GetChildrenRef()) {
        TraceEventTree_AddToJsonArray(c, pid, threadId, array);
    }
}

// Adds Chrome counter events to the events array.
static
void TraceEventTree_AddCounters(
    const int pid,
    const TraceEventTree::CounterValuesMap& counters,
    JsArray& events)
{
    for (const TraceEventTree::CounterValuesMap::value_type& c : counters) {
        for (const TraceEventTree::CounterValues::value_type& v 
            : c.second) {

            JsObject dict;
            dict["cat"] = JsValue("");
            // Chrome counters are process scoped so the thread id does not seem
            // to have an impact.
            dict["tid"] = JsValue(0);
            dict["pid"]  = JsValue(pid);
            dict["name"] = JsValue(c.first.GetString());
            dict["ph"]   = JsValue("C"); // Counter
            dict["ts"]   = _TimeStampToChromeTraceValue(v.first);
            JsObject values;
            values["value"] = JsValue(v.second);
            dict["args"] = values;
            events.push_back(dict);
        }
    }
}

// Adds Chrome instant events to the events array.
static
void TraceEventTree_AddMarkers(
    const int pid,
    const TraceEventTree::MarkerValuesMap& markers,
    JsArray& events)
{
    for (const TraceEventTree::MarkerValuesMap::value_type& m : markers) {
        for (const TraceEventTree::MarkerValues::value_type& v 
            : m.second) {

            JsObject dict;
            dict["cat"] = JsValue("");
            dict["tid"] = JsValue(v.second.ToString());
            dict["pid"]  = JsValue(pid);
            dict["name"] = JsValue(m.first.GetString());
            dict["ph"]   = JsValue("I"); // Mark
            dict["s"]   = JsValue("t"); // Scope
            dict["ts"]   = _TimeStampToChromeTraceValue(v.first);
            events.push_back(dict);
        }
    }
}


JsObject 
TraceEventTree::CreateChromeTraceObject() const
{
    JsArray eventArray;
    // Chrome Trace format has a pid for each event.  We use a dummy pid.
    const int pid = 0;

    for (const TraceEventNodeRefPtr& c : _root->GetChildrenRef()) {
        // The children of the root represent threads
        TraceThreadId threadId(c->GetKey().GetString());

        for (const TraceEventNodeRefPtr& gc :c->GetChildrenRef()) {
            TraceEventTree_AddToJsonArray(
                gc,
                pid,
                threadId,
                &eventArray);
        }
    }
    TraceEventTree_AddCounters(pid, _counters, eventArray);
    TraceEventTree_AddMarkers(pid, _markers, eventArray);
    JsObject traceObj;

    traceObj["traceEvents"] = eventArray;
    
    return traceObj;
}

TraceEventTree::CounterMap
TraceEventTree::GetFinalCounterValues() const {
    CounterMap finalValues;
    
    for (const CounterValuesMap::value_type& p : _counters) {
        const CounterValues& counterValues = p.second;
        if (!counterValues.empty()) {
            finalValues[p.first] = counterValues.rbegin()->second;
        }
    }

    return finalValues;
}

PXR_NAMESPACE_CLOSE_SCOPE