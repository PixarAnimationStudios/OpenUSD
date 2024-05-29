//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_COUNTER_ACCUMULATOR_H
#define PXR_BASE_TRACE_COUNTER_ACCUMULATOR_H

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

    using _CounterDeltaValues =
        std::multimap<TraceEvent::TimeStamp, _CounterValue>;
    using _CounterDeltaMap = std::map<TfToken, _CounterDeltaValues>;

    _CounterDeltaMap _counterDeltas;
    CounterValuesMap _counterValuesOverTime;
    CounterMap _currentValues;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_BASE_TRACE_COUNTER_ACCUMULATOR_H