//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/valueTypeDispatch.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
    template <typename T>
    struct _Creator
    {
        void operator()(Ts_SplineData** dataOut)
        {
            *dataOut = new Ts_TypedSplineData<T>();
        }
    };
}

// static
Ts_SplineData* Ts_SplineData::Create(
    const TfType valueType,
    const Ts_SplineData* const overallParamSource)
{
    // If type wasn't specified, use double.
    const TfType actualType = (valueType ? valueType : Ts_GetType<double>());

    // Create the specific subtype.
    Ts_SplineData *result = nullptr;
    TsDispatchToValueTypeTemplate<_Creator>(
        actualType, &result);

    // Calling code should always have verified supported value type.
    if (!result)
    {
        return nullptr;
    }

    // Fill in default values that aren't built into the member types.  This
    // static method serves as our constructor, so we need these explicitly.
    result->timeValued = false;
    result->curveType = TsCurveTypeBezier;

    // Write the flag that indicates whether this is real or temporary data.
    result->isTyped = bool(valueType);

    // If we are being created to replace temporary data, copy overall members.
    if (overallParamSource)
    {
        result->curveType = overallParamSource->curveType;
        result->preExtrapolation = overallParamSource->preExtrapolation;
        result->postExtrapolation = overallParamSource->postExtrapolation;
        result->loopParams = overallParamSource->loopParams;
    }

    return result;
}

Ts_SplineData::~Ts_SplineData() = default;

bool Ts_SplineData::HasInnerLoops(
    size_t* const firstProtoIndexOut) const
{
    // Must have nonzero, positive prototype interval width.
    if (loopParams.protoEnd <= loopParams.protoStart)
    {
        return false;
    }

    // Must have nonzero loop count in at least one direction.
    if (!loopParams.numPreLoops && !loopParams.numPostLoops)
    {
        return false;
    }

    // Must have a knot at the prototype start time.
    const auto it =
        std::lower_bound(times.begin(), times.end(), loopParams.protoStart);
    if (it == times.end() || *it != loopParams.protoStart)
    {
        return false;
    }

    // Return the start knot index if requested.
    if (firstProtoIndexOut)
    {
        *firstProtoIndexOut = it - times.begin();
    }

    // Inner looping is valid.
    return true;
}

TsTime Ts_SplineData::GetPreExtrapTime() const
{
    if (times.empty()) {
        TF_CODING_ERROR("GetPreExtrapTime called on spline with no knots.");
        return 0.0;
    }

    TsTime result = times.front();

    if (HasInnerLoops()) {
        const double loopSpan = loopParams.protoEnd - loopParams.protoStart;
        const TsTime loopBegin = loopParams.protoStart -
                                 loopParams.numPreLoops * loopSpan;
        result = std::min(result, loopBegin);
    }

    return result;
}

TsTime Ts_SplineData::GetPostExtrapTime() const
{
    if (times.empty()) {
        TF_CODING_ERROR("GetPostExtrapTime called on spline with no knots.");
        return 0.0;
    }

    TsTime result = times.back();

    if (HasInnerLoops()) {
        const double loopSpan = loopParams.protoEnd - loopParams.protoStart;
        const TsTime loopBegin = loopParams.protoEnd +
                                 loopParams.numPostLoops * loopSpan;
        result = std::max(result, loopBegin);
    }

    return result;
}

double Ts_SplineData::GetPreExtrapValue() const
{
    if (times.empty()) {
        TF_CODING_ERROR("GetPreExtrapValue called on spline with no knots.");
        return 0.0;
    }

    size_t firstProtoIndex;
    if (HasInnerLoops(&firstProtoIndex)) {
        const double loopSpan = loopParams.protoEnd - loopParams.protoStart;
        const TsTime loopBegin = loopParams.protoStart -
                                 loopParams.numPreLoops * loopSpan;
        if (loopBegin <= times.front()) {
            return (GetKnotPreValueAsDouble(firstProtoIndex) -
                    loopParams.numPreLoops * loopParams.valueOffset);
        }
    }

    return GetKnotPreValueAsDouble(0);
}

double Ts_SplineData::GetPostExtrapValue() const
{
    if (times.empty()) {
        TF_CODING_ERROR("GetPostExtrapValue called on spline with no knots.");
        return 0.0;
    }

    size_t firstProtoIndex;
    if (HasInnerLoops(&firstProtoIndex)) {
        const double loopSpan = loopParams.protoEnd - loopParams.protoStart;
        const TsTime loopEnd = loopParams.protoEnd +
                               loopParams.numPostLoops * loopSpan;
        if (loopEnd >= times.back()) {
            return (GetKnotValueAsDouble(firstProtoIndex) +
                    (loopParams.numPostLoops + 1) * loopParams.valueOffset);
        }
    }

    return GetKnotValueAsDouble(times.size() - 1);
}
    
Ts_SplineData*
Ts_GetSplineData(TsSpline &spline)
{
    return spline._data.get();
}

const Ts_SplineData*
Ts_GetSplineData(const TsSpline &spline)
{
    return spline._data.get();
}


PXR_NAMESPACE_CLOSE_SCOPE
