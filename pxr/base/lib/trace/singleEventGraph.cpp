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

#include "pxr/base/trace/singleEventGraph.h"

#include "pxr/pxr.h"
#include "pxr/base/js/value.h"

#include <boost/optional.hpp>

PXR_NAMESPACE_OPEN_SCOPE


void
TraceSingleEventGraph::Merge(const TraceSingleEventGraphRefPtr& graph) {
    // Add the node to the tree.
    for(TraceSingleEventNodeRefPtr child : graph->GetRoot()->GetChildrenRef()) {
        _root->Append(child);
    }

    // Add the counter data.
    for (CounterValuesMap::value_type& p : graph->_counters) {
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
}

static 
JsValue
_TimeStampToChromeTraceValue(TraceEvent::TimeStamp t)
{
    // Chrome trace format uses timestamps in microseconds.
    return JsValue(ArchTicksToNanoseconds(t)/1000.0);
}


// Recursively adds JSON objects representing call graph nodes to the array.
static 
void TraceSingleEventGraph_AddToJsonArray(
    const TraceSingleEventNodeRefPtr &node,
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
        using AttributeMap = TraceSingleEventNode::AttributeMap;
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
    for (const TraceSingleEventNodeRefPtr& c : node->GetChildrenRef()) {
        TraceSingleEventGraph_AddToJsonArray(c, pid, threadId, array);
    }
}

// Adds Chrome counter events to the events array.
static
void TraceSingleEventGraph_AddCounters(
    const int pid,
    const TraceSingleEventGraph::CounterValuesMap& counters,
    JsArray& events)
{
    for (const TraceSingleEventGraph::CounterValuesMap::value_type& c : counters) {
        for (const TraceSingleEventGraph::CounterValues::value_type& v 
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

JsObject 
TraceSingleEventGraph::CreateChromeTraceObject() const
{
    JsArray eventArray;
    // Chrome Trace format has a pid for each event.  We use a dummy pid.
    const int pid = 0;

    for (const TraceSingleEventNodeRefPtr& c : _root->GetChildrenRef()) {
        // The children of the root represent threads
        TraceThreadId threadId(c->GetKey().GetString());

        for (const TraceSingleEventNodeRefPtr& gc :c->GetChildrenRef()) {
            TraceSingleEventGraph_AddToJsonArray(
                gc,
                pid,
                threadId,
                &eventArray);
        }
    }
    TraceSingleEventGraph_AddCounters(pid, _counters, eventArray);
    JsObject traceObj;

    traceObj["traceEvents"] = eventArray;
    
    return traceObj;
}

TraceSingleEventGraph::CounterMap
TraceSingleEventGraph::GetFinalCounterValues() const {
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