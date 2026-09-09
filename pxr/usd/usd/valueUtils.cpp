//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/ts/valueTypeDispatch.h"
#include "pxr/usd/usd/valueUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

template <typename S>
struct _EvalSplineFunctor
{
    template <typename T>
    void operator()(const TsSpline& spline, UsdTimeCode localTime,
                    T* result, bool* successOut)
    {
        S val;
        auto evalFunc = !localTime.IsPreTime() ?
                            &TsSpline::Eval<S> : &TsSpline::EvalPreValue<S>;
        if (!(spline.*evalFunc)(localTime.GetValue(), &val)) {
            return;
        }
        *successOut = Usd_SetValue(result, val);
    }
};

} // anonymous namespace

void
Usd_MergeTimeSamples(std::vector<double> * const timeSamples, 
                     const std::vector<double> &additionalTimeSamples,
                     std::vector<double> * tempUnionTimeSamples)
{
    std::vector<double> temp; 
    if (!tempUnionTimeSamples)
        tempUnionTimeSamples = &temp;

    tempUnionTimeSamples->resize(timeSamples->size() + additionalTimeSamples.size());

    const auto &it = std::set_union(timeSamples->begin(), timeSamples->end(), 
                              additionalTimeSamples.begin(), 
                              additionalTimeSamples.end(), 
                              tempUnionTimeSamples->begin());
    tempUnionTimeSamples->resize(std::distance(tempUnionTimeSamples->begin(), it));
    timeSamples->swap(*tempUnionTimeSamples);
}

// Apply the offset to the value if it's holding the templated type.
template <class T>
static bool
_TryApplyLayerOffsetToValue(VtValue *value, const SdfLayerOffset &offset)
{
    if (value->IsHolding<T>()) {
        T v;
        value->UncheckedSwap(v);
        Usd_ApplyLayerOffsetToValue(&v, offset);
        value->UncheckedSwap(v);
        return true;
    }
    return false;
}

void
Usd_ApplyLayerOffsetToValue(VtValue *value, const SdfLayerOffset &offset)
{
    // Try applying the offset for each of our supported value types.
    _TryApplyLayerOffsetToValue<GfTimeCode>(value, offset) ||
    _TryApplyLayerOffsetToValue<VtArray<GfTimeCode>>(value, offset) ||
    _TryApplyLayerOffsetToValue<VtArrayEdit<GfTimeCode>>(value, offset) ||
    _TryApplyLayerOffsetToValue<GfDuration>(value, offset) ||
    _TryApplyLayerOffsetToValue<VtArray<GfDuration>>(value, offset) ||
    _TryApplyLayerOffsetToValue<VtArrayEdit<GfDuration>>(value, offset) ||
    _TryApplyLayerOffsetToValue<VtDictionary>(value, offset) ||
    _TryApplyLayerOffsetToValue<SdfTimeSampleMap>(value, offset);
}

template <class T>
bool
Usd_QuerySpline(
    const TsSpline& spline,
    UsdTimeCode timeCode,
    T* result)
{
    bool success = false;
    // Use the spline's value type to dispatch to the appropriate evaluator.
    TsDispatchToValueTypeTemplate<_EvalSplineFunctor>(
        spline.GetValueType(), spline, timeCode,
        result, &success);

    return success;
}

#define _INSTANTIATE_QUERY_SPLINE(unused, elem)                 \
    template bool Usd_QuerySpline(                              \
        const TsSpline&, UsdTimeCode,                           \
        TS_SPLINE_VALUE_CPP_TYPE(elem)*);

TF_PP_SEQ_FOR_EACH(_INSTANTIATE_QUERY_SPLINE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES)
#undef _INSTANTIATE_QUERY_SPLINE

template bool Usd_QuerySpline(
    const TsSpline&, UsdTimeCode,
    SdfAbstractDataValue*);
template bool Usd_QuerySpline(
    const TsSpline&, UsdTimeCode,
    VtValue*);

PXR_NAMESPACE_CLOSE_SCOPE
