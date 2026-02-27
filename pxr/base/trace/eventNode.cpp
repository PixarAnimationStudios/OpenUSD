//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/eventNode.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TraceEventNodeRefPtr
TraceEventNode::New() {
    return TraceEventNode::New(
        TfToken("root"), TraceCategory::Default,
        /*beginTime=*/0, /*endTime=*/0, /*separateEvents=*/false);
}

TraceEventNode::~TraceEventNode()
{
    delete _attributesAndSeparateEvents.Get();
}

void
TraceEventNode::Append(TraceEventNodeRefPtr &&node)
{
    _children.emplace_back(std::move(node));
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

const TraceEventNode::AttributeMap&
TraceEventNode::GetAttributes() const
{
    static const AttributeMap empty;
    if (AttributeMap const *attrMap = _attributesAndSeparateEvents.Get()) {
        return *attrMap;
    }
    return empty;
}

void
TraceEventNode::AddAttribute(
    const TfToken& key, AttributeData&& attr)
{
    if (!_attributesAndSeparateEvents.Get()) {
        _attributesAndSeparateEvents.Set(new AttributeMap);
    }
    // Place `attr` at the head of the list to facilitate event tree building --
    // that process iterates events in reverse order, so this ends up placing
    // events in forward order.
    _attributesAndSeparateEvents->emplace_hint(
        _attributesAndSeparateEvents->find(key), key, std::move(attr));
}

PXR_NAMESPACE_CLOSE_SCOPE
