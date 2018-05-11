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

#include "pxr/base/trace/eventNode.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TraceEventNodeRefPtr
TraceEventNode::Append(
    const TfToken &key, 
    TraceCategoryId category, 
    TimeStamp beginTime, 
    TimeStamp endTime,
    bool separateEvents)
{
    TraceEventNodeRefPtr n = 
        TraceEventNode::New(
            key, category, beginTime, endTime, separateEvents);
    _children.push_back(n);
    return n;
}

void
TraceEventNode::Append(TraceEventNodeRefPtr node)
{
    _children.push_back(node);
}

void 
TraceEventNode::SetBeginAndEndTimesFromChildren()
{
    if (_children.empty()) {
        _beginTime = 0;
        _endTime = 0;
        return;
    }

    _beginTime = std::numeric_limits<TimeStamp>::max();
    _endTime   = std::numeric_limits<TimeStamp>::min();

    for (const TraceEventNodeRefPtr& c : _children) {
        _beginTime = std::min(_beginTime, c->GetBeginTime());
        _endTime   = std::max(_endTime, c->GetEndTime());
    }

}

void
TraceEventNode::AddAttribute(
    const TfToken& key, const AttributeData& attr)
{
    _attributes.emplace(key, attr);
}

PXR_NAMESPACE_CLOSE_SCOPE