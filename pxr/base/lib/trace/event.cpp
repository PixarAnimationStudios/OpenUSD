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

#include "pxr/base/trace/event.h"

#include "pxr/pxr.h"
#include "pxr/base/trace/eventData.h"

PXR_NAMESPACE_OPEN_SCOPE

TraceEvent::TimeStamp
TraceEvent::GetTimeStamp() const
{
    return _time;
}

double
TraceEvent::GetCounterValue() const
{
    return (_type == _InternalEventType::CounterDelta
        || _type == _InternalEventType::CounterValue)
        ? *reinterpret_cast<const double*>(&_payload) : 0.0;
}

TraceEventData
TraceEvent::GetData() const
{
    // Make sure the types we care about can be stored in the payload
    static_assert(sizeof(PayloadStorage) >= sizeof(TimeStamp), "Payload Error");
    static_assert(
        alignof(PayloadStorage) >= alignof(TimeStamp), "Payload Error");

    static_assert(sizeof(PayloadStorage) >= sizeof(double), "Payload Error");
    static_assert(alignof(PayloadStorage) >= alignof(double), "Payload Error");

    static_assert(sizeof(PayloadStorage) >= sizeof(void*), "Payload Error");
    static_assert(alignof(PayloadStorage) >= alignof(void*), "Payload Error");

    if (_type == _InternalEventType::ScopeData 
        || _type == _InternalEventType::ScopeDataLarge) {
        const void* data = _type == _InternalEventType::ScopeData ? &_payload 
            : *reinterpret_cast<const void * const *>(&_payload);
        switch (_dataType) {
            case DataType::Boolean:
                return TraceEventData(*reinterpret_cast<const bool*>(data));
            case DataType::Int:
                return TraceEventData(*reinterpret_cast<const int64_t*>(data));
            case DataType::UInt:
                return TraceEventData(*reinterpret_cast<const uint64_t*>(data));
            case DataType::Float:
                return TraceEventData(*reinterpret_cast<const double*>(data));
            case DataType::String:
                return TraceEventData(
                    std::string(reinterpret_cast<const char*>(data)));
            case DataType::Invalid:
                return TraceEventData();
        }
    }
    return TraceEventData();
}

TraceEvent::TimeStamp
TraceEvent::GetStartTimeStamp() const
{
    return _type != _InternalEventType::Timespan ? 0 
        : *reinterpret_cast<const TimeStamp*>(&_payload);
}

TraceEvent::TimeStamp
TraceEvent::GetEndTimeStamp() const
{
    return _type != _InternalEventType::Timespan ? 0 :  _time;
}

TraceEvent::EventType
TraceEvent::GetType() const {
    switch (_type) {
        case _InternalEventType::Begin: return EventType::Begin;
        case _InternalEventType::End: return EventType::End;
        case _InternalEventType::Timespan: return EventType::Timespan;
        case _InternalEventType::CounterDelta: return EventType::CounterDelta;
        case _InternalEventType::CounterValue: return EventType::CounterValue;
        case _InternalEventType::ScopeData: return EventType::ScopeData;
        case _InternalEventType::ScopeDataLarge: return EventType::ScopeData;
    }
    return EventType::Unknown;
}

PXR_NAMESPACE_CLOSE_SCOPE
