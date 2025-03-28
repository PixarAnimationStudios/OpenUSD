//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_SPLINE_DATA_H
#define PXR_BASE_TS_SPLINE_DATA_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/knotData.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/stl.h"

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <utility>
#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

class TsSpline;


// Primary data structure for splines.  Abstract; subclasses store knot data,
// which is flexibly typed (double/float/half).  This is the unit of data that
// is managed by shared_ptr, and forms the basis of copy-on-write data sharing.
//
struct Ts_SplineData
{
public:
    // If valueType is known, create a TypedSplineData of the specified type.
    // If valueType is unknown, create a TypedSplineData<double> to store
    // overall spline parameters in the absence of a value type; this assumes
    // that when knots arrive, they are most likely to be double-typed.  If
    // overallParamSource is provided, it is a previous overall-only struct, and
    // our guess about double was wrong, so we are transferring the overall
    // parameters.
    static Ts_SplineData* Create(
        TfType valueType,
        const Ts_SplineData *overallParamSource = nullptr);

    virtual ~Ts_SplineData();

public:
    // Virtual interface for typed data.

    virtual TfType GetValueType() const = 0;
    virtual size_t GetKnotStructSize() const = 0;
    virtual Ts_SplineData* Clone() const = 0;

    virtual bool operator==(const Ts_SplineData &other) const = 0;

    virtual void ReserveForKnotCount(size_t count) = 0;
    virtual void PushKnot(
        const Ts_KnotData *knotData,
        const VtDictionary &customData) = 0;
    virtual size_t SetKnot(
        const Ts_KnotData *knotData,
        const VtDictionary &customData) = 0;

    virtual Ts_KnotData* CloneKnotAtIndex(size_t index) const = 0;
    virtual Ts_KnotData* CloneKnotAtTime(TsTime time) const = 0;
    virtual Ts_KnotData* GetKnotPtrAtIndex(size_t index) = 0;
    virtual Ts_TypedKnotData<double>
        GetKnotDataAsDouble(size_t index) const = 0;

    virtual void ClearKnots() = 0;
    virtual void RemoveKnotAtTime(TsTime time) = 0;

    virtual void ApplyOffsetAndScale(
        TsTime offset,
        double scale) = 0;

    virtual bool HasValueBlocks() const = 0;
    virtual bool HasValueBlockAtTime(TsTime time) const = 0;

public:
    // Returns whether there is a valid inner-loop configuration.  If
    // firstProtoIndexOut is provided, it receives the index of the first knot
    // in the prototype.
    bool HasInnerLoops(
        size_t *firstProtoIndexOut = nullptr) const;

public:
    // BITFIELDS - note: for enum-typed bitfields, we declare one bit more than
    // is minimally needed to represent all declared enum values.  For example,
    // TsCurveType has only two values, so it should be representable in one
    // bit.  However, compilers are free to choose the underlying representation
    // of enums, and some platforms choose signed values, meaning that we
    // actually need one bit more, so that we can hold the sign bit.  We could
    // declare the enums with unsigned underlying types, but that runs into a
    // gcc 9.2 bug.  We can spare the extra bit; alignment means there is no
    // difference in struct size.

    // If true, our subtype is authoritative; we know our value type.  If false,
    // then no value type was provided at initialization, and no knots have been
    // set.  In the latter case, we exist only to store overall parameters, and
    // we have been presumptively created as TypedSplineData<double>.
    bool isTyped : 1;

    // Whether ApplyOffsetAndScale applies to values also.
    bool timeValued : 1;

    // Overall spline parameters.
    TsCurveType curveType : 2;
    TsExtrapolation preExtrapolation;
    TsExtrapolation postExtrapolation;
    TsLoopParams loopParams;

    // A duplicate of the knot times, so that we can maximize locality while
    // performing binary searches for knots.  This is part of the evaluation hot
    // path; given an eval time, we must find either the knot at that time, or
    // the knots before and after that time.  The entries in this vector
    // correspond exactly to the entries in the 'knots' vector in
    // Ts_TypedSplineData.  Times are unique and sorted in ascending order.
    std::vector<TsTime> times;

    // Custom data for knots, sparsely allocated, keyed by time.
    std::unordered_map<TsTime, VtDictionary> customData;
};


// Concrete subclass of Ts_SplineData.  Templated on T, the value type.
//
template <typename T>
struct Ts_TypedSplineData final :
    public Ts_SplineData
{
public:
    TfType GetValueType() const override;
    size_t GetKnotStructSize() const override;
    Ts_SplineData* Clone() const override;

    bool operator==(const Ts_SplineData &other) const override;

    void ReserveForKnotCount(size_t count) override;
    void PushKnot(
        const Ts_KnotData *knotData,
        const VtDictionary &customData) override;
    size_t SetKnot(
        const Ts_KnotData *knotData,
        const VtDictionary &customData) override;

    Ts_KnotData* CloneKnotAtIndex(size_t index) const override;
    Ts_KnotData* CloneKnotAtTime(TsTime time) const override;
    Ts_KnotData* GetKnotPtrAtIndex(size_t index) override;
    Ts_TypedKnotData<double>
        GetKnotDataAsDouble(size_t index) const override;

    void ClearKnots() override;
    void RemoveKnotAtTime(TsTime time) override;

    // Apply offset and scale to all spline data.
    // 
    // If \p scale is negative, a coding error is generated. This is because 
    // the spline is not only scaled, but also time-reversed. Doing so can
    // lead to incorrect evaluation results with any scenario where direction
    // of time is assumed, like dual-value knots, inner looping,
    // segment interpolation mode assignment, etc.
    void ApplyOffsetAndScale(
        TsTime offset,
        double scale) override;

    bool HasValueBlocks() const override;
    bool HasValueBlockAtTime(TsTime time) const override;

public:
    // Per-knot data.
    std::vector<Ts_TypedKnotData<T>> knots;
};


// Data-access helpers for the Ts implementation.  The untyped functions are
// friends of TsSpline, and retrieve private data pointers.

Ts_SplineData*
Ts_GetSplineData(TsSpline &spline);

const Ts_SplineData*
Ts_GetSplineData(const TsSpline &spline);

template <typename T>
Ts_TypedSplineData<T>*
Ts_GetTypedSplineData(TsSpline &spline);

template <typename T>
const Ts_TypedSplineData<T>*
Ts_GetTypedSplineData(const TsSpline &spline);


////////////////////////////////////////////////////////////////////////////////
// TEMPLATE IMPLEMENTATIONS

template <typename T>
TfType Ts_TypedSplineData<T>::GetValueType() const
{
    if (!isTyped)
    {
        return TfType();
    }

    return Ts_GetType<T>();
}

template <typename T>
size_t Ts_TypedSplineData<T>::GetKnotStructSize() const
{
    return sizeof(Ts_TypedKnotData<T>);
}

template <typename T>
Ts_SplineData*
Ts_TypedSplineData<T>::Clone() const
{
    return new Ts_TypedSplineData<T>(*this);
}

template <typename T>
bool Ts_TypedSplineData<T>::operator==(
    const Ts_SplineData &other) const
{
    // Compare non-templated data.
    if (isTyped != other.isTyped
        || timeValued != other.timeValued
        || curveType != other.curveType
        || preExtrapolation != other.preExtrapolation
        || postExtrapolation != other.postExtrapolation
        || loopParams != other.loopParams
        || customData != other.customData)
    {
        return false;
    }

    // Downcast to our value type.  If other is not of the same type, we're not
    // equal.
    const Ts_TypedSplineData<T>* const typedOther =
        dynamic_cast<const Ts_TypedSplineData<T>*>(&other);
    if (!typedOther)
    {
        return false;
    }

    // Compare all knots.
    return knots == typedOther->knots;
}

template <typename T>
void Ts_TypedSplineData<T>::ReserveForKnotCount(
    const size_t count)
{
    times.reserve(count);
    knots.reserve(count);
}

template <typename T>
void Ts_TypedSplineData<T>::PushKnot(
    const Ts_KnotData* const knotData,
    const VtDictionary &customDataIn)
{
    const Ts_TypedKnotData<T>* const typedKnotData =
        static_cast<const Ts_TypedKnotData<T>*>(knotData);

    times.push_back(knotData->time);
    knots.push_back(*typedKnotData);

    if (!customDataIn.empty())
    {
        customData[knotData->time] = customDataIn;
    }
}

template <typename T>
size_t Ts_TypedSplineData<T>::SetKnot(
    const Ts_KnotData* const knotData,
    const VtDictionary &customDataIn)
{
    const Ts_TypedKnotData<T>* const typedKnotData =
        static_cast<const Ts_TypedKnotData<T>*>(knotData);

    // Use binary search to find insert-or-overwrite position.
    const auto it =
        std::lower_bound(times.begin(), times.end(), knotData->time);
    const size_t idx =
        it - times.begin();
    const bool overwrite =
        (it != times.end() && *it == knotData->time);

    // Insert or overwrite new time and knot data.
    if (overwrite)
    {
        times[idx] = knotData->time;
        knots[idx] = *typedKnotData;
    }
    else
    {
        times.insert(it, knotData->time);
        knots.insert(knots.begin() + idx, *typedKnotData);
    }

    // Store customData, if any.
    if (!customDataIn.empty())
    {
        customData[knotData->time] = customDataIn;
    }

    return idx;
}

template <typename T>
Ts_KnotData*
Ts_TypedSplineData<T>::CloneKnotAtIndex(
    const size_t index) const
{
    return new Ts_TypedKnotData<T>(knots[index]);
}

template <typename T>
Ts_KnotData*
Ts_TypedSplineData<T>::CloneKnotAtTime(
    const TsTime time) const
{
    const auto it = std::lower_bound(times.begin(), times.end(), time);
    if (it == times.end() || *it != time)
    {
        return nullptr;
    }

    const auto knotIt = knots.begin() + (it - times.begin());
    return new Ts_TypedKnotData<T>(*knotIt);
}

template <typename T>
Ts_KnotData*
Ts_TypedSplineData<T>::GetKnotPtrAtIndex(
    const size_t index)
{
    return &(knots[index]);
}

// Depending on T, this is either a verbatim copy or an increase in precision.
template <typename T>
Ts_TypedKnotData<double>
Ts_TypedSplineData<T>::GetKnotDataAsDouble(
    const size_t index) const
{
    const Ts_TypedKnotData<T> &in = knots[index];
    Ts_TypedKnotData<double> out;

    // Use operator= to copy base-class members.  This is admittedly weird, but
    // it will continue working if members are added to the base class.
    static_cast<Ts_KnotData&>(out) = static_cast<const Ts_KnotData&>(in);

    // Copy derived members individually.
    out.value = in.value;
    out.preValue = in.preValue;
    out.preTanSlope = in.preTanSlope;
    out.postTanSlope = in.postTanSlope;

    return out;
}

template <typename T>
void Ts_TypedSplineData<T>::ClearKnots()
{
    times.clear();
    customData.clear();
    knots.clear();
}

template <typename T>
void Ts_TypedSplineData<T>::RemoveKnotAtTime(
    const TsTime time)
{
    const auto it = std::lower_bound(times.begin(), times.end(), time);
    if (it == times.end() || *it != time)
    {
        TF_CODING_ERROR("Cannot remove nonexistent knot from SplineData");
        return;
    }

    const size_t idx = it - times.begin();
    times.erase(it);
    customData.erase(time);
    knots.erase(knots.begin() + idx);
}

template <typename T>
static void _ApplyOffsetAndScaleToKnot(
    Ts_TypedKnotData<T>* const knotData,
    const TsTime offset,
    const double scale)
{
    // In our private implementation, we must have set a positive scale.
    TF_VERIFY(scale > 0);

    // Process knot time (absolute).
    knotData->time = knotData->time * scale + offset;

    // Process tangent widths (relative, strictly positive).
    knotData->preTanWidth *= scale;
    knotData->postTanWidth *= scale;

    // Process slopes (inverse relative).
    knotData->preTanSlope /= scale;
    knotData->postTanSlope /= scale;
}

template <typename T>
void Ts_TypedSplineData<T>::ApplyOffsetAndScale(
    const TsTime offset,
    const double scale)
{
    if (scale <= 0)
    {
        TF_CODING_ERROR("Applying zero or negative scale to spline data, "
                        "collapsing/reversing time and spline representation "
                        "is not allowed.");
        return;
    }

    // The spline is changed in the time dimension only.
    // Different parameters are affected in different ways:
    // - Absolute times (e.g. knot times): apply scale and offset.
    // - Relative times (e.g. tan widths): apply scale only.
    // - Inverse relative (slopes): slope = height/width, so we apply 1/scale.

    // Scale extrapolation slopes if applicable (inverse relative).
    if (preExtrapolation.mode == TsExtrapSloped)
    {
        preExtrapolation.slope /= scale;
    }
    if (postExtrapolation.mode == TsExtrapSloped)
    {
        postExtrapolation.slope /= scale;
    }

    // Process inner-loop params.
    if (loopParams.protoEnd > loopParams.protoStart)
    {
        // Process start and end times (absolute).
        loopParams.protoStart = loopParams.protoStart * scale + offset;
        loopParams.protoEnd = loopParams.protoEnd * scale + offset;
    }

    // Process knot-times vector (absolute).
    for (TsTime &time : times) {
        time = time * scale + offset;
    }

    // Process knots.  Duplicate the logic that is applied unconditionally, so
    // that we can rip through the entire vector just once, and we don't have to
    // do the if-check on each iteration.
    if (timeValued)
    {
        for (Ts_TypedKnotData<T> &knotData : knots)
        {
            _ApplyOffsetAndScaleToKnot(&knotData, offset, scale);

            // Process time values (absolute).
            knotData.value =
                static_cast<T>(knotData.value * scale + offset);
            knotData.preValue =
                static_cast<T>(knotData.preValue * scale + offset);
        }
    }
    else
    {
        for (Ts_TypedKnotData<T> &knotData : knots) {
            _ApplyOffsetAndScaleToKnot(&knotData, offset, scale);
        }
    }

    // Re-index custom data.  Times are adjusted absolutely.
    if (!customData.empty())
    {
        std::unordered_map<TsTime, VtDictionary> newCustomData;
        for (const auto &mapPair : customData) {
            newCustomData[mapPair.first * scale + offset] = mapPair.second;
        }
        customData.swap(newCustomData);
    }
}

template <typename T>
bool Ts_TypedSplineData<T>::HasValueBlocks() const
{
    if (knots.empty())
    {
        return false;
    }

    if (preExtrapolation.mode == TsExtrapValueBlock
        || postExtrapolation.mode == TsExtrapValueBlock)
    {
        return true;
    }

    for (const Ts_TypedKnotData<T> &knotData : knots)
    {
        if (knotData.nextInterp == TsInterpValueBlock)
        {
            return true;
        }
    }

    return false;
}

template <typename T>
bool Ts_TypedSplineData<T>::HasValueBlockAtTime(
    const TsTime time) const
{
    // If no knots, no blocks.
    if (knots.empty())
    {
        return false;
    }

    // Find first knot at or after time.
    const auto lbIt =
        std::lower_bound(times.begin(), times.end(), time);

    // If time is after all knots, return whether we have blocked
    // post-extrapolation.
    if (lbIt == times.end())
    {
        return postExtrapolation.mode == TsExtrapValueBlock;
    }

    // If there is a knot at this time, return whether its segment has blocked
    // interpolation.
    if (*lbIt == time)
    {
        const auto knotIt = knots.begin() + (lbIt - times.begin());
        return knotIt->nextInterp == TsInterpValueBlock;
    }

    // If time is before all knots, return whether we have blocked
    // pre-extrapolation.
    if (lbIt == times.begin())
    {
        return preExtrapolation.mode == TsExtrapValueBlock;
    }

    // Between knots.  Return whether the segment that we're in has blocked
    // interpolation.
    const auto knotIt = knots.begin() + (lbIt - times.begin());
    return (knotIt - 1)->nextInterp == TsInterpValueBlock;
}

template <typename T>
Ts_TypedSplineData<T>*
Ts_GetTypedSplineData(TsSpline &spline)
{
    return static_cast<Ts_TypedSplineData<T>*>(
        Ts_GetSplineData(spline));
}

template <typename T>
const Ts_TypedSplineData<T>*
Ts_GetTypedSplineData(const TsSpline &spline)
{
    return static_cast<Ts_TypedSplineData<T>*>(
        Ts_GetSplineData(spline));
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif
