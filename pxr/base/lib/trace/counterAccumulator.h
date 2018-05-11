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

#ifndef TRACE_COUNTER_ACCUMULATOR_H
#define TRACE_COUNTER_ACCUMULATOR_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/collection.h"

#include "pxr/base/tf/token.h"

#include <map>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// \class TraceCounterAccumulator
///
/// This class accumulates counter values from TraceCollection instances. 
/// A TraceCounterAccumulator instance can accumulate counters from multiple 
/// collections or the state of the counters can be set explicitly through
/// SetCurrentValues().
///
///
class TraceCounterAccumulator : private TraceCollection::Visitor {
public:
    using CounterValues = std::vector<std::pair<TraceEvent::TimeStamp, double>>;
    using CounterValuesMap =
        std::unordered_map<TfToken, CounterValues, TfToken::HashFunctor>;
    using CounterMap =
        std::unordered_map<TfToken, double, TfToken::HashFunctor>;

    /// Constructor.
    TraceCounterAccumulator() = default;

    /// Returns a map of the counter values over time.
    const CounterValuesMap& GetCounters() const { 
        return _counterValuesOverTime;
    }

    /// Sets the current value of the counters.
    TRACE_API void SetCurrentValues(const CounterMap&);

    /// Returns the current value of the counters.
    const CounterMap& GetCurrentValues() const {
        return _currentValues;
    }

    /// Reads events /p collection and updates the current values of the
    /// counters.
    TRACE_API void Update(const TraceCollection& collection);

protected:
    /// Determines whether or not counter events with \p id should be processed.
    virtual bool _AcceptsCategory(TraceCategoryId id) = 0;

private:
    // TraceCollection::Visitor Interface
    virtual void OnBeginCollection() override;
    virtual void OnEndCollection() override;
    virtual void OnBeginThread(const TraceThreadId&) override;
    virtual void OnEndThread(const TraceThreadId&) override;
    virtual bool AcceptsCategory(TraceCategoryId) override;
    virtual void OnEvent(
        const TraceThreadId&, const TfToken&, const TraceEvent&) override;

    struct _CounterValue {
        double value;
        bool isDelta;
    };

    using _CounterDeltaValues = std::map<TraceEvent::TimeStamp, _CounterValue>;
    using _CounterDeltaMap = std::map<TfToken, _CounterDeltaValues>;

    _CounterDeltaMap _counterDeltas;
    CounterValuesMap _counterValuesOverTime;
    CounterMap _currentValues;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //TRACE_COUNTER_ACCUMULATOR_H