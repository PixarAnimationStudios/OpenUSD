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

#include "pxr/base/js/json.h"

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
double
_TimeStampToChromeTraceValue(TraceEvent::TimeStamp t)
{
    // Chrome trace format uses timestamps in microseconds.
    return ArchTicksToNanoseconds(t)/1000.0;
}

// Recursively writes JSON objects representing call tree nodes to the array.
static 
void TraceEventTree_WriteToJsonArray(
    const TraceEventNodeRefPtr &node,
    const int pid,
    const TraceThreadId& threadId,
    JsWriter& js)
{
    std::string categoryList("");

    // Add begin time
    std::vector<std::string> catList = 
        TraceCategory::GetInstance().GetCategories(node->GetCategory());
    for (const std::string& catName : catList) {
        if (categoryList.length() > 0) {
            categoryList.append(",");
        }
        categoryList.append(catName);
    }
    auto writeCommonEventData = [&]() {
        js.BeginObject();
        js.WriteKeyValue("cat", categoryList);
        js.WriteKeyValue("libTraceCatId",
            static_cast<uint64_t>(node->GetCategory()));
        js.WriteKeyValue("pid",pid);
        js.WriteKeyValue("tid",threadId.ToString());
        js.WriteKeyValue("name",node->GetKey().GetString());
    };
    writeCommonEventData();
    js.WriteKeyValue("ts", _TimeStampToChromeTraceValue(node->GetBeginTime()));

    if (!node->GetAttributes().empty()) {
        js.WriteKey("args");
        js.BeginObject();
        
        using AttributeMap = TraceEventNode::AttributeMap;
        std::unordered_set<TfToken, TfToken::HashFunctor> visitedKeys;
        for (const AttributeMap::value_type& it : node->GetAttributes()) {
            if (visitedKeys.find(it.first) == visitedKeys.end()) {
                visitedKeys.insert(it.first);
                using AttrItr = AttributeMap::const_iterator;
                using Range = std::pair<AttrItr,AttrItr>;
                Range range = node->GetAttributes().equal_range(it.first);
                if (std::distance(range.first, range.second) == 1) {
                    js.WriteKey(range.first->first.GetString());
                    range.first->second.WriteJson(js);
                } else {
                    js.WriteKey(it.first.GetString());
                    js.WriteArray(range.first, range.second,
                        [](JsWriter& js, AttrItr i) {
                            i->second.WriteJson(js);
                        }
                    );
                }
            }
        }
        js.EndObject();
    }

    if (!node->IsFromSeparateEvents()) 
    {
        js.WriteKeyValue("ph","X"); // Complete event
        js.WriteKeyValue("dur",_TimeStampToChromeTraceValue(
            node->GetEndTime() - node->GetBeginTime()));
        js.EndObject();
    } else {
        js.WriteKeyValue("ph","B"); // begin event
        js.EndObject();
        
        writeCommonEventData();
        js.WriteKeyValue("ph","E"); // end event
        js.WriteKeyValue("ts",
            _TimeStampToChromeTraceValue(node->GetEndTime()));
        js.EndObject();
    }

    // Recurse on the children
    for (const TraceEventNodeRefPtr& c : node->GetChildrenRef()) {
        TraceEventTree_WriteToJsonArray(c, pid, threadId, js);
    }
}

// Writes Chrome counter events to the events array.
static
void TraceEventTree_WriteCounters(
    const int pid,
    const TraceEventTree::CounterValuesMap& counters,
    JsWriter& js)
{
    for (const TraceEventTree::CounterValuesMap::value_type& c : counters) {
        for (const TraceEventTree::CounterValues::value_type& v 
            : c.second) {
            
            js.WriteObject(
                "cat", "",
                // Chrome counters are process scoped so the thread id does not
                // seem to have an impact.
                "tid", 0,
                "pid", pid,
                "name", c.first.GetString(),
                "ph", "C",
                "ts", _TimeStampToChromeTraceValue(v.first),
                "args", [&v](JsWriter& js) {
                    js.WriteObject("value", v.second);
                }
            );
        }
    }
}

// Writes Chrome instant events to the events array.
static
void TraceEventTree_WriteMarkers(
    const int pid,
    const TraceEventTree::MarkerValuesMap& markers,
    JsWriter& js)
{
    for (const TraceEventTree::MarkerValuesMap::value_type& m : markers) {
        for (const TraceEventTree::MarkerValues::value_type& v 
            : m.second) {

            js.WriteObject(
                "cat", "",
                "tid", v.second.ToString(),
                "pid", pid,
                "name", m.first.GetString(),
                "ph", "I", // Mark
                "s", "t", // Scope
                "ts", _TimeStampToChromeTraceValue(v.first)
            );
        }
    }
}

void 
TraceEventTree::WriteChromeTraceObject(
    JsWriter& writer, ExtraFieldFn extraFields) const
{
    writer.BeginObject();
    writer.WriteKey("traceEvents");
    writer.BeginArray();

    // Chrome Trace format has a pid for each event.  We use a dummy pid.
    const int pid = 0;

    for (const TraceEventNodeRefPtr& c : _root->GetChildrenRef()) {
        // The children of the root represent threads
        TraceThreadId threadId(c->GetKey().GetString());

        for (const TraceEventNodeRefPtr& gc :c->GetChildrenRef()) {
            TraceEventTree_WriteToJsonArray(
                gc,
                pid,
                threadId,
                writer);
        }
    }
    TraceEventTree_WriteCounters(pid, _counters, writer);
    TraceEventTree_WriteMarkers(pid, _markers, writer);

    writer.EndArray();

    // Write any extra fields into the object.
    if (extraFields) {
        extraFields(writer);
    }

    writer.EndObject();
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