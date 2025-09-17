//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//


#include "pxr/base/trace/counterAccumulator.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

void
TraceCounterAccumulator::Update(const TraceCollection& col)
{
    col.Iterate(*this);
}

void
TraceCounterAccumulator::OnBeginCollection()
{
}

void
TraceCounterAccumulator::OnEndCollection()
{
    // Convert the counter deltas and values to absolute values;
    for (const _CounterDeltaMap::value_type& c : _counterDeltas) {
        double curValue = _currentValues[c.first];
        
        for (const _CounterDeltaValues::value_type& v : c.second) {
            if (v.second.isDelta) {
                curValue += v.second.value;
            } else {
                curValue = v.second.value;
            }
            _counterValuesOverTime[c.first].emplace_back(v.first, curValue);
        }
        _currentValues[c.first] = curValue;
    }
    _counterDeltas.clear();
}

void
TraceCounterAccumulator::OnBeginThread(const TraceThreadId&)
{
    // Do Nothing
}
void
TraceCounterAccumulator::OnEndThread(const TraceThreadId&)
{
    // Do Nothing
}

void
TraceCounterAccumulator::OnEvent(
    const TraceThreadId&, const TfToken& key, const TraceEvent& e)
{
    switch (e.GetType()) {
        case TraceEvent::EventType::CounterDelta:
        {
            _counterDeltas[key].insert(
                std::make_pair(e.GetTimeStamp(),
                    _CounterValue{e.GetCounterValue(), true}));
            break;
        }
        case TraceEvent::EventType::CounterValue:
        {
            _counterDeltas[key].insert(
                std::make_pair(e.GetTimeStamp(),
                    _CounterValue{e.GetCounterValue(), false}));
            break;
        }
        default:
            break;
    }
}
bool
TraceCounterAccumulator::AcceptsCategory(TraceCategoryId id)
{
    return _AcceptsCategory(id);
}

void
TraceCounterAccumulator::SetCurrentValues(
    const CounterMap& values)
{
    _currentValues = values;
}

PXR_NAMESPACE_CLOSE_SCOPE