//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/valueUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

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
    _TryApplyLayerOffsetToValue<SdfTimeCode>(value, offset) ||
    _TryApplyLayerOffsetToValue<VtArray<SdfTimeCode>>(value, offset) ||
    _TryApplyLayerOffsetToValue<VtDictionary>(value, offset) ||
    _TryApplyLayerOffsetToValue<SdfTimeSampleMap>(value, offset);
}

PXR_NAMESPACE_CLOSE_SCOPE
