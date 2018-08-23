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

#ifndef TRACE_EVENT_H
#define TRACE_EVENT_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/category.h"
#include "pxr/base/trace/key.h"

#include "pxr/base/arch/timing.h"

PXR_NAMESPACE_OPEN_SCOPE

class TraceEventData;

////////////////////////////////////////////////////////////////////////////////
/// \class TraceEvent
///
/// This represents an event recorded by a TraceCollector. 
/// It contains a key (name), categoryId, timestamp, type, and optional
/// metadata.
///
class TraceEvent {
public:
    /// Time in "ticks".
    using TimeStamp = uint64_t;
    using Key = TraceKey;

    /// \name Tag enums to select constructors
    /// @{
    enum BeginTag { Begin };
    enum EndTag { End };
    enum TimespanTag { Timespan };
    enum MarkerTag { Marker };
    enum CounterDeltaTag { CounterDelta };
    enum CounterValueTag { CounterValue };
    enum DataTag { Data };
    /// @}

    /// Valid event types
    enum class EventType : uint8_t {
        Unknown, ///< The event is an unknown type.
        Begin, ///< The event represents the beginning timestamp of a scope.
        End, ///< The event represents the ending timestamp of a scope.
        Timespan, ///< The event represents begin and end timestamp of a scope.
        Marker, ///< The event represents an marker without a duration.
        CounterDelta, ///< The event represents a change in a counter.
        CounterValue, ///< The event represents the value of a counter.
        ScopeData,
        ///< The event stores data that is associated with its enclosing scope.
    };

    /// The different types of data that can be stored in a TraceEvent instance.
    enum class DataType : uint8_t {
        String, ///< The event is storing a string.
        Boolean, ///< The event is storing a bool.
        Int, ///< The event is storing an integer.
        UInt, ///< The event is storing an unsigned integer.
        Float, ///< The event is storing an double.
        Invalid ///< The event is not storing any data.
    };

    ///  Return this event's key.
    const Key& GetKey() const { return _key; }

    ///  Return the time stamp associated with this event.
    TRACE_API TimeStamp GetTimeStamp() const;

    ///  Return the counter value associated with this event.
    TRACE_API double GetCounterValue() const;

    /// Returns the event's category id.
    TraceCategoryId GetCategory() const { return _category; }

    /// Returns the start time of a timespan event.
    TRACE_API TimeStamp GetStartTimeStamp() const;

    /// Returns the end time of a timespan event.
    TRACE_API TimeStamp GetEndTimeStamp() const;

    /// Returns the data stored in a data event.
    TRACE_API TraceEventData GetData() const;

    /// Returns the type of the event.
    TRACE_API EventType GetType() const;

    /// \name Constructors
    /// @{

    /// Constructor for Begin events that will automatically set the 
    /// timestamp from the current time.
    TraceEvent(BeginTag, const Key& key, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::Begin),
        _time(ArchGetTickTime()) {
    }
    
    /// Constructor for Begin events that takes a specific TimeStamp \a ts.
    TraceEvent( BeginTag, 
                const Key& key,
                TimeStamp ts,
                TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::Begin),
        _time(ts) {
    }

    /// Constructor for End events that will automatically set the
    /// timestamp from the current time.
    TraceEvent(EndTag, const Key& key, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::End),
        _time(ArchGetTickTime()) {
    }
    
    /// Constructor for End events that takes a specific TimeStamp \a ts.
    TraceEvent( EndTag, 
                const Key& key,
                TimeStamp ts,
                TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::End),
        _time(ts) {
    }

    /// Constructor for Timespan events that takes a TimeStamp \a starttime
    /// and automatically sets the end timestamp from the current time.
    TraceEvent(
        TimespanTag, const Key& key, TimeStamp startTime, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::Timespan),
        _time(ArchGetTickTime()) {
        new (&_payload) TimeStamp(startTime);
    }

    /// Constructor for Timespan events that takes the start time and end time.
    TraceEvent(
        TimespanTag, const Key& key, 
        TimeStamp startTime, TimeStamp endTime,
        TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::Timespan),
        _time(endTime) {
        new (&_payload) TimeStamp(startTime);
    }

    /// Constructor for Marker events that will automatically set the
    /// timestamp from the current time.
    TraceEvent(MarkerTag, const Key& key, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::Marker),
        _time(ArchGetTickTime()) {
    }

    /// Constructor for Mark events that takes a specific TimeStamp \a ts.
    TraceEvent( MarkerTag, 
                const Key& key,
                TimeStamp ts,
                TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::Marker),
        _time(ts) {
    }

    /// Constructor for Counter delta events.
    TraceEvent( CounterDeltaTag,
                const Key& key, 
                double value, 
                TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::CounterDelta),
        _time(ArchGetTickTime()) {
        new (&_payload) double(value);
    }

    /// Constructor for Counter value events.
    TraceEvent( CounterValueTag,
                const Key& key, 
                double value, 
                TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _type(_InternalEventType::CounterValue),
        _time(ArchGetTickTime()) {
        new (&_payload) double(value);
    }

    /// \name Constructors for data events
    /// @{
    TraceEvent(DataTag, const Key& key, bool data, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _dataType(DataType::Boolean),
        _type(_InternalEventType::ScopeData),
        _time(ArchGetTickTime()) {
        new (&_payload) bool(data);
    }

    TraceEvent(DataTag, const Key& key, int data, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _dataType(DataType::Int),
        _type(_InternalEventType::ScopeData),
        _time(ArchGetTickTime()) {
        new (&_payload) int64_t(data);
    }

    TraceEvent(DataTag, const Key& key, int64_t data, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _dataType(DataType::Int),
        _type(_InternalEventType::ScopeData),
        _time(ArchGetTickTime()) {
        new (&_payload) int64_t(data);
    }

    TraceEvent(DataTag, const Key& key, uint64_t data, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _dataType(DataType::UInt),
        _type(_InternalEventType::ScopeData),
        _time(ArchGetTickTime()) {
        new (&_payload) uint64_t(data);
    }

    TraceEvent(DataTag, const Key& key, double data, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _dataType(DataType::Float),
        _type(_InternalEventType::ScopeData),
        _time(ArchGetTickTime()) {
        new (&_payload) double(data);
    }

    TraceEvent(DataTag, const Key& key, const char* data, TraceCategoryId cat) :
        _key(key),
        _category(cat),
        _dataType(DataType::String),
        _type(_InternalEventType::ScopeDataLarge),
        _time(ArchGetTickTime()) {
        new (&_payload) const char*(data);
    }
    /// @}

    /// @}

    /// Sets the events timestamp to \p time.
    void SetTimeStamp(TimeStamp time) { _time = time; }
private:
    // Valid event types. This type has more detail that the public facing
    // EventType enum.
    enum class _InternalEventType : uint8_t {
        Begin,
        End,
        Timespan,
        Marker,
        CounterDelta,
        CounterValue,
        ScopeData,
        ScopeDataLarge,
    };

    using PayloadStorage = std::aligned_storage<8, 8>::type;

    Key _key;
    TraceCategoryId _category;
    DataType _dataType;
    _InternalEventType _type;
    TimeStamp _time;
    PayloadStorage _payload;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_EVENT_H
