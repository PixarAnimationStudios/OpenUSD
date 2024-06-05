//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_EVAL_CACHE_H
#define PXR_BASE_TS_EVAL_CACHE_H

#include "pxr/pxr.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/ts/keyFrameUtils.h"
#include "pxr/base/ts/mathUtils.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/tf.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

class TsKeyFrame;
template <typename T> class Ts_TypedData;

// Bezier data.  This holds two beziers (time and value) as both points
// and the coefficients of a cubic polynomial.
template <typename T>
class Ts_Bezier {
public:
    Ts_Bezier() { }
    Ts_Bezier(const TsTime timePoints[4], const T valuePoints[4]);
    void DerivePolynomial();

public:
    TsTime timePoints[4];
    TsTime timeCoeff[4];
    T valuePoints[4];
    T valueCoeff[4];
};

template <typename T>
Ts_Bezier<T>::Ts_Bezier(const TsTime time[4], const T value[4])
{
    timePoints[0]  = time[0];
    timePoints[1]  = time[1];
    timePoints[2]  = time[2];
    timePoints[3]  = time[3];
    valuePoints[0] = value[0];
    valuePoints[1] = value[1];
    valuePoints[2] = value[2];
    valuePoints[3] = value[3];
    DerivePolynomial();
}

template <typename T>
void
Ts_Bezier<T>::DerivePolynomial()
{
    timeCoeff[0]   =        timePoints[0];
    timeCoeff[1]   = -3.0 * timePoints[0] +
                      3.0 * timePoints[1];
    timeCoeff[2]   =  3.0 * timePoints[0] +
                     -6.0 * timePoints[1] +
                      3.0 * timePoints[2];
    timeCoeff[3]   = -1.0 * timePoints[0] +
                      3.0 * timePoints[1] +
                     -3.0 * timePoints[2] +
                            timePoints[3];
    valueCoeff[0]  =        valuePoints[0];
    valueCoeff[1]  = -3.0 * valuePoints[0] +
                      3.0 * valuePoints[1];
    valueCoeff[2]  =  3.0 * valuePoints[0] +
                     -6.0 * valuePoints[1] +
                      3.0 * valuePoints[2];
    valueCoeff[3]  = -1.0 * valuePoints[0] +
                      3.0 * valuePoints[1] +
                     -3.0 * valuePoints[2] +
                            valuePoints[3];
}

class Ts_UntypedEvalCache {
public:
    typedef std::shared_ptr<Ts_UntypedEvalCache> SharedPtr;

    /// Construct and return a new eval cache for the given keyframes.
    static SharedPtr New(const TsKeyFrame &kf1, const TsKeyFrame &kf2);

    virtual VtValue Eval(TsTime) const = 0;
    virtual VtValue EvalDerivative(TsTime) const = 0;
    
    // Equivalent to invoking New() and Eval(time) on the newly created cache,
    // but without the heap allocation.
    static VtValue EvalUncached(const TsKeyFrame &kf1,
                                const TsKeyFrame &kf2,
                                TsTime time);

    // Equivalent to invoking New() and EvalDerivative(time) on the newly
    // created cache, but without the heap allocation.
    static VtValue EvalDerivativeUncached(const TsKeyFrame &kf1,
                                          const TsKeyFrame &kf2,
                                          TsTime time);

protected:
    ~Ts_UntypedEvalCache() = default;

    // Compute the Bezier control points.
    template <typename T>
    static void _SetupBezierGeometry(TsTime* timePoints, T* valuePoints,
                                     const Ts_TypedData<T>* kf1,
                                     const Ts_TypedData<T>* kf2);

    // Compute the time coordinate of the 2nd Bezier control point.  This
    // synthesizes tangents for held and linear knots.
    template <typename T>
    static TsTime _GetBezierPoint2Time(const Ts_TypedData<T>* kf1,
                                         const Ts_TypedData<T>* kf2);

    // Compute the time coordinate of the 3rd Bezier control point.  This
    // synthesizes tangents for held and linear knots.
    template <typename T>
    static TsTime _GetBezierPoint3Time(const Ts_TypedData<T>* kf1,
                                         const Ts_TypedData<T>* kf2);

    // Compute the value coordinate of the 2nd Bezier control point.  This
    // synthesizes tangents for held and linear knots.
    template <typename T>
    static T _GetBezierPoint2Value(const Ts_TypedData<T>* kf1,
                                   const Ts_TypedData<T>* kf2);

    // Compute the value coordinate of the 3rd Bezier control point.  This
    // synthesizes tangents for held and linear knots.
    template <typename T>
    static T _GetBezierPoint3Value(const Ts_TypedData<T>* kf1,
                                   const Ts_TypedData<T>* kf2);

    // Compute the value coordinate of the 4th Bezier control point.  This
    // synthesizes tangents for held and linear knots.
    template <typename T>
    static T _GetBezierPoint4Value(const Ts_TypedData<T>* kf1,
                                   const Ts_TypedData<T>* kf2);
};

template <typename T, bool INTERPOLATABLE = TsTraits<T>::interpolatable >
class Ts_EvalCache;

template <typename T>
class Ts_EvalQuaternionCache : public Ts_UntypedEvalCache {
protected:
    static_assert(std::is_same<T, GfQuatf>::value
            || std::is_same<T, GfQuatd>::value
            , "T must be Quatd or Quatf");
    Ts_EvalQuaternionCache(const Ts_EvalQuaternionCache<T> * rhs);
    Ts_EvalQuaternionCache(const Ts_TypedData<T>* kf1,
            const Ts_TypedData<T>* kf2);
    Ts_EvalQuaternionCache(const TsKeyFrame & kf1,
            const TsKeyFrame & kf2);

public:
    T TypedEval(TsTime) const;
    T TypedEvalDerivative(TsTime) const;

    VtValue Eval(TsTime t) const override;
    VtValue EvalDerivative(TsTime t) const override;
private:
    void _Init(const Ts_TypedData<T>* kf1, const Ts_TypedData<T>* kf2);
    double _kf1_time, _kf2_time;
    T _kf1_value, _kf2_value;
    TsKnotType _kf1_knot_type;
};

template<>
class Ts_EvalCache<GfQuatf, true> final
    : public Ts_EvalQuaternionCache<GfQuatf> {
public:
    Ts_EvalCache(const Ts_EvalCache<GfQuatf, true> *rhs) :
        Ts_EvalQuaternionCache<GfQuatf>(rhs) {}
    Ts_EvalCache(const Ts_TypedData<GfQuatf>* kf1,
            const Ts_TypedData<GfQuatf>* kf2) :
        Ts_EvalQuaternionCache<GfQuatf>(kf1, kf2) {}
    Ts_EvalCache(const TsKeyFrame & kf1, const TsKeyFrame & kf2) :
        Ts_EvalQuaternionCache<GfQuatf>(kf1, kf2) {}

    typedef std::shared_ptr<Ts_EvalCache<GfQuatf, true> > TypedSharedPtr;

    /// Construct and return a new eval cache for the given keyframes.
    static TypedSharedPtr New(const TsKeyFrame &kf1, const TsKeyFrame &kf2);
};

template<>
class Ts_EvalCache<GfQuatd, true> final
    : public Ts_EvalQuaternionCache<GfQuatd> {
public:
    Ts_EvalCache(const Ts_EvalCache<GfQuatd, true> *rhs) :
        Ts_EvalQuaternionCache<GfQuatd>(rhs) {}
    Ts_EvalCache(const Ts_TypedData<GfQuatd>* kf1,
            const Ts_TypedData<GfQuatd>* kf2) :
        Ts_EvalQuaternionCache<GfQuatd>(kf1, kf2) {}
    Ts_EvalCache(const TsKeyFrame & kf1, const TsKeyFrame & kf2) :
        Ts_EvalQuaternionCache<GfQuatd>(kf1, kf2) {}

    typedef std::shared_ptr<Ts_EvalCache<GfQuatd, true> > TypedSharedPtr;

    /// Construct and return a new eval cache for the given keyframes.
    static TypedSharedPtr New(const TsKeyFrame &kf1, const TsKeyFrame &kf2);

};

// Partial specialization for types that cannot be interpolated.
template <typename T>
class Ts_EvalCache<T, false> final : public Ts_UntypedEvalCache {
public:
    Ts_EvalCache(const Ts_EvalCache<T, false> * rhs);
    Ts_EvalCache(const Ts_TypedData<T>* kf1, const Ts_TypedData<T>* kf2);
    Ts_EvalCache(const TsKeyFrame & kf1, const TsKeyFrame & kf2);
    T TypedEval(TsTime) const;
    T TypedEvalDerivative(TsTime) const;

    VtValue Eval(TsTime t) const override;
    VtValue EvalDerivative(TsTime t) const override;

    typedef std::shared_ptr<Ts_EvalCache<T, false> > TypedSharedPtr;

    /// Construct and return a new eval cache for the given keyframes.
    static TypedSharedPtr New(const TsKeyFrame &kf1, const TsKeyFrame &kf2);

private:
    T _value;
};

// Partial specialization for types that can be interpolated.
template <typename T>
class Ts_EvalCache<T, true> final : public Ts_UntypedEvalCache {
public:
    Ts_EvalCache(const Ts_EvalCache<T, true> * rhs);
    Ts_EvalCache(const Ts_TypedData<T>* kf1, const Ts_TypedData<T>* kf2);
    Ts_EvalCache(const TsKeyFrame & kf1, const TsKeyFrame & kf2);
    T TypedEval(TsTime) const;
    T TypedEvalDerivative(TsTime) const;

    VtValue Eval(TsTime t) const override;
    VtValue EvalDerivative(TsTime t) const override;

    const Ts_Bezier<T>* GetBezier() const;

    typedef std::shared_ptr<Ts_EvalCache<T, true> > TypedSharedPtr;

    /// Construct and return a new eval cache for the given keyframes.
    static TypedSharedPtr New(const TsKeyFrame &kf1, const TsKeyFrame &kf2);

private:
    void _Init(const Ts_TypedData<T>* kf1, const Ts_TypedData<T>* kf2);

private:
    bool _interpolate;
    
    // Value to use when _interpolate is false.
    T _value;

    Ts_Bezier<T> _cache;
};

////////////////////////////////////////////////////////////////////////
// Ts_UntypedEvalCache

template <typename T>
TsTime
Ts_UntypedEvalCache::_GetBezierPoint2Time(const Ts_TypedData<T>* kf1,
                                            const Ts_TypedData<T>* kf2)
{
    switch (kf1->_knotType) {
    default:
    case TsKnotHeld:
    case TsKnotLinear:
        return (2.0 * kf1->GetTime() + kf2->GetTime()) / 3.0;

    case TsKnotBezier:
        return kf1->GetTime() + kf1->_rightTangentLength;
    }
}

template <typename T>
TsTime
Ts_UntypedEvalCache::_GetBezierPoint3Time(const Ts_TypedData<T>* kf1,
                                            const Ts_TypedData<T>* kf2)
{
    // If the the first keyframe is held then the we treat the third bezier
    // point as held too.
    TsKnotType knotType = (kf1->_knotType == TsKnotHeld) ? 
        TsKnotHeld : kf2->_knotType;

    switch (knotType) {
    default:
    case TsKnotHeld:
    case TsKnotLinear:
        return (kf1->GetTime() + 2.0 * kf2->GetTime()) / 3.0;

    case TsKnotBezier:
        return kf2->GetTime() - kf2->_leftTangentLength;
    }
}

template <typename T>
T
Ts_UntypedEvalCache::_GetBezierPoint2Value(const Ts_TypedData<T>* kf1,
                                             const Ts_TypedData<T>* kf2)
{
    switch (kf1->_knotType) {
    default:
    case TsKnotHeld:
        return kf1->_GetRightValue();

    case TsKnotLinear:
        return (1.0 / 3.0) *
            (2.0 * kf1->_GetRightValue() +
                (kf2->_isDual ? kf2->_GetLeftValue() : kf2->_GetRightValue()));

    case TsKnotBezier:
        return kf1->_GetRightValue() + 
            kf1->_rightTangentLength * kf1->_GetRightTangentSlope();
    }
}

template <typename T>
T
Ts_UntypedEvalCache::_GetBezierPoint3Value(const Ts_TypedData<T>* kf1,
                                             const Ts_TypedData<T>* kf2)
{
    // If the first key frame is held then the we just use the first key frame's
    // value
    if (kf1->_knotType == TsKnotHeld) {
        return kf1->_GetRightValue();
    }

    switch (kf2->_knotType) {
    default:
    case TsKnotHeld:
        if (kf1->_knotType != TsKnotLinear) {
            return kf2->_isDual ? kf2->_GetLeftValue() : kf2->_GetRightValue();
        }
        // Fall through to linear case if the first knot is linear
        [[fallthrough]];

    case TsKnotLinear:
        return (1.0 / 3.0) *
               (kf1->_GetRightValue() + 2.0 * 
                   (kf2->_isDual ? kf2->_GetLeftValue() : kf2->_GetRightValue()));

    case TsKnotBezier:
        return (kf2->_isDual ? kf2->_GetLeftValue() : kf2->_GetRightValue()) -
                kf2->_leftTangentLength * kf2->_GetLeftTangentSlope();
    }
}

template <typename T>
T
Ts_UntypedEvalCache::_GetBezierPoint4Value(const Ts_TypedData<T>* kf1,
                                             const Ts_TypedData<T>* kf2)
{
    // If the first knot is held then the last value is still the value of 
    // the first knot, otherwise it's the left side of the second knot
    if (kf1->_knotType == TsKnotHeld) {
        return kf1->_GetRightValue();
    } else {
        return (kf2->_isDual ? kf2->_GetLeftValue() : kf2->_GetRightValue());
    }
}

template <typename T>
void
Ts_UntypedEvalCache::_SetupBezierGeometry(
    TsTime* timePoints, T* valuePoints,
    const Ts_TypedData<T>* kf1, const Ts_TypedData<T>* kf2)
{
    timePoints[0]  = kf1->GetTime();
    timePoints[1]  = _GetBezierPoint2Time(kf1, kf2);
    timePoints[2]  = _GetBezierPoint3Time(kf1, kf2);
    timePoints[3]  = kf2->GetTime();
    valuePoints[0] = kf1->_GetRightValue();
    valuePoints[1] = _GetBezierPoint2Value(kf1, kf2);
    valuePoints[2] = _GetBezierPoint3Value(kf1, kf2);
    valuePoints[3] = _GetBezierPoint4Value(kf1, kf2);
}

////////////////////////////////////////////////////////////////////////
// Ts_EvalCache non-interpolatable

template <typename T>
Ts_EvalCache<T, false>::Ts_EvalCache(const Ts_EvalCache<T, false> * rhs)
{
    _value = rhs->_value;
}

template <typename T>
Ts_EvalCache<T, false>::Ts_EvalCache(const Ts_TypedData<T>* kf1,
                                         const Ts_TypedData<T>* kf2)
{
    if (!kf1 || !kf2) {
        TF_CODING_ERROR("Constructing an Ts_EvalCache from invalid keyframes");
        return;
    }

    _value = kf1->_GetRightValue();
}

template <typename T>
Ts_EvalCache<T, false>::Ts_EvalCache(const TsKeyFrame &kf1,
    const TsKeyFrame &kf2)
{
    // Cast to the correct typed data.  This is a private class, and we assume
    // callers are passing only keyframes from the same spline, and correctly
    // arranging our T to match.
    Ts_TypedData<T> *data = 
        static_cast<Ts_TypedData<T> const*>(Ts_GetKeyFrameData(kf1));

    _value = data->_GetRightValue();
}

template <typename T>
VtValue
Ts_EvalCache<T, false>::Eval(TsTime t) const {
    return VtValue(TypedEval(t));
}

template <typename T>
VtValue
Ts_EvalCache<T, false>::EvalDerivative(TsTime t) const {
    return VtValue(TypedEvalDerivative(t));
}

template <typename T>
T
Ts_EvalCache<T, false>::TypedEval(TsTime) const
{
    return _value;
}

template <typename T>
T
Ts_EvalCache<T, false>::TypedEvalDerivative(TsTime) const
{
    return TsTraits<T>::zero;
}

////////////////////////////////////////////////////////////////////////
// Ts_EvalCache interpolatable

template <typename T>
Ts_EvalCache<T, true>::Ts_EvalCache(const Ts_EvalCache<T, true> * rhs)
{
    _interpolate = rhs->_interpolate;
    _value = rhs->_value;
    _cache = rhs->_cache;
}

template <typename T>
Ts_EvalCache<T, true>::Ts_EvalCache(const Ts_TypedData<T>* kf1,
                                        const Ts_TypedData<T>* kf2)
{
    _Init(kf1,kf2);
}

template <typename T>
Ts_EvalCache<T, true>::Ts_EvalCache(const TsKeyFrame &kf1,
    const TsKeyFrame &kf2)
{
    // Cast to the correct typed data.  This is a private class, and we assume
    // callers are passing only keyframes from the same spline, and correctly
    // arranging our T to match.
    _Init(static_cast<Ts_TypedData<T> const*>(Ts_GetKeyFrameData(kf1)),
          static_cast<Ts_TypedData<T> const*>(Ts_GetKeyFrameData(kf2)));
}

template <typename T>
void
Ts_EvalCache<T, true>::_Init(
    const Ts_TypedData<T>* kf1,
    const Ts_TypedData<T>* kf2)
{
    if (!kf1 || !kf2) {
        TF_CODING_ERROR("Constructing an Ts_EvalCache from invalid keyframes");
        return;
    }

    // Curve for same knot types or left half of blend for different knot types
    _SetupBezierGeometry(_cache.timePoints, _cache.valuePoints, kf1, kf2);
    _cache.DerivePolynomial();

    if (kf1->ValueCanBeInterpolated() && kf2->ValueCanBeInterpolated()) {
        _interpolate = true;
    } else {
        _interpolate = false;
        _value = kf1->_GetRightValue();
    }
}

template <typename T>
VtValue
Ts_EvalCache<T, true>::Eval(TsTime t) const {
    return VtValue(TypedEval(t));
}

template <typename T>
VtValue
Ts_EvalCache<T, true>::EvalDerivative(TsTime t) const {
    return VtValue(TypedEvalDerivative(t));
}

template <typename T>
T
Ts_EvalCache<T, true>::TypedEval(TsTime time) const
{
    if (!_interpolate)
        return _value;

    double u = GfClamp(Ts_SolveCubic(_cache.timeCoeff, time), 0.0, 1.0);
    return Ts_EvalCubic(_cache.valueCoeff, u);
}

template <typename T>
T
Ts_EvalCache<T, true>::TypedEvalDerivative(TsTime time) const
{
    if (!TsTraits<T>::supportsTangents || !_interpolate) {
        return TsTraits<T>::zero;
    }

    // calculate the derivative as
    // u = t^-1(time)
    //   dx(u)
    //   ----
    //    du        dx(u)
    // --------  =  -----
    //   dt(u)      dt(u)
    //   ----
    //    du
    double u;
    u = GfClamp(Ts_SolveCubic(_cache.timeCoeff, time), 0.0, 1.0);
    T x = Ts_EvalCubicDerivative(_cache.valueCoeff, u);
    TsTime t = Ts_EvalCubicDerivative(_cache.timeCoeff, u);
    T derivative = x * (1.0 / t);
    return derivative;
}

template <typename T>
const Ts_Bezier<T>*
Ts_EvalCache<T, true>::GetBezier() const
{
    return &_cache;
}

template <typename T>
std::shared_ptr<Ts_EvalCache<T, true> >
Ts_EvalCache<T, true>::New(const TsKeyFrame &kf1, const TsKeyFrame &kf2)
{
    // Cast to the correct typed data.  This is a private class, and we assume
    // callers are passing only keyframes from the same spline, and correctly
    // arranging our T to match.
    return static_cast<const Ts_TypedData<T>*>(
        Ts_GetKeyFrameData(kf1))->
            CreateTypedEvalCache(Ts_GetKeyFrameData(kf2));
}

template <typename T>
std::shared_ptr<Ts_EvalCache<T, false> >
Ts_EvalCache<T, false>::New(const TsKeyFrame &kf1, const TsKeyFrame &kf2)
{
    // Cast to the correct typed data.  This is a private class, and we assume
    // callers are passing only keyframes from the same spline, and correctly
    // arranging our T to match.
    return static_cast<const Ts_TypedData<T>*>(
        Ts_GetKeyFrameData(kf1))->
            CreateTypedEvalCache(Ts_GetKeyFrameData(kf2));
}

////////////////////////////////////////////////////////////////////////
// Ts_EvalQuaternionCache

template <typename T>
Ts_EvalQuaternionCache<T>::Ts_EvalQuaternionCache(
    const Ts_EvalQuaternionCache<T> * rhs)
{
    _kf1_knot_type = rhs->_kf1_knot_type;

    _kf1_time = rhs->_kf1_time;
    _kf2_time = rhs->_kf2_time;

    _kf1_value = rhs->_kf1_value;
    _kf2_value = rhs->_kf2_value;
}

template <typename T>
Ts_EvalQuaternionCache<T>::Ts_EvalQuaternionCache(
    const Ts_TypedData<T>* kf1, const Ts_TypedData<T>* kf2)
{
    _Init(kf1,kf2);
}

template <typename T>
Ts_EvalQuaternionCache<T>::Ts_EvalQuaternionCache(const TsKeyFrame &kf1,
    const TsKeyFrame &kf2)
{
    // Cast to the correct typed data.  This is a private class, and we assume
    // callers are passing only keyframes from the same spline, and correctly
    // arranging our T to match.
    _Init(static_cast<Ts_TypedData<T> const*>(Ts_GetKeyFrameData(kf1)),
          static_cast<Ts_TypedData<T> const*>(Ts_GetKeyFrameData(kf2)));
}

template <typename T>
void
Ts_EvalQuaternionCache<T>::_Init(
    const Ts_TypedData<T>* kf1,
    const Ts_TypedData<T>* kf2)
{
    if (!kf1 || !kf2) {
        TF_CODING_ERROR("Constructing an Ts_EvalQuaternionCache"
            " from invalid keyframes");
        return;
    }

    _kf1_knot_type = kf1->_knotType;

    _kf1_time = kf1->GetTime();
    _kf2_time = kf2->GetTime();

    _kf1_value = kf1->_GetRightValue();
    _kf2_value = kf2->_isDual ? kf2->_GetLeftValue() : kf2->_GetRightValue();
}

template <typename T>
VtValue
Ts_EvalQuaternionCache<T>::Eval(TsTime t) const {
    return VtValue(TypedEval(t));
}

template <typename T>
T Ts_EvalQuaternionCache<T>::TypedEval(TsTime time) const
{
    if (_kf1_knot_type == TsKnotHeld) {
        return _kf1_value;
    }

    // XXX: do we want any snapping here?  divide-by-zero avoidance?
    // The following code was in Presto; not sure it belongs in Ts.
    //
    //if (fabs(_kf2_time - _kf1_time) < ARCH_MIN_FLOAT_EPS_SQR) {
    //   return _kf1_value;
    //}

    double u = (time - _kf1_time) / (_kf2_time - _kf1_time);
    return GfSlerp(_kf1_value, _kf2_value, u);
}

template<typename T>
VtValue Ts_EvalQuaternionCache<T>::EvalDerivative(TsTime t) const {
    return VtValue(TypedEvalDerivative(t));
}

template<typename T>
T Ts_EvalQuaternionCache<T>::TypedEvalDerivative(TsTime) const {
    return TsTraits<T>::zero;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
