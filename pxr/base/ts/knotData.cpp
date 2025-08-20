//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/knotData.h"
#include "pxr/base/ts/valueTypeDispatch.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


Ts_KnotData::Ts_KnotData()
    : time(0.0),
      preTanWidth(0.0),
      postTanWidth(0.0),
      nextInterp(TsInterpHeld),
      curveType(TsCurveTypeBezier),  // curveType for knots is deprecated
      dualValued(false),
      preTanAlgorithm(TsTangentAlgorithmNone),
      postTanAlgorithm(TsTangentAlgorithmNone)
{
}

namespace
{
    template <typename T>
    struct _DataCreator
    {
        void operator()(Ts_KnotData **dataOut)
        {
            *dataOut = new Ts_TypedKnotData<T>();
        }
    };

    template <typename T>
    struct _ProxyCreator
    {
        void operator()(Ts_KnotData *data, Ts_KnotDataProxy **proxyOut)
        {
            *proxyOut = new Ts_TypedKnotDataProxy<T>(
                static_cast<Ts_TypedKnotData<T>*>(data));
        }
    };
}

// static
Ts_KnotData* Ts_KnotData::Create(const TfType valueType)
{
    Ts_KnotData *result = nullptr;
    TsDispatchToValueTypeTemplate<_DataCreator>(
        valueType, &result);
    return result;
}

bool Ts_KnotData::operator==(const Ts_KnotData &other) const
{
    // CurveType for knots has been deprecated and we no longer consider its
    // value when testing for equality.

    return time == other.time
        && preTanWidth == other.preTanWidth
        && postTanWidth == other.postTanWidth
        && dualValued == other.dualValued
        && nextInterp == other.nextInterp;
}

// Compute algorithmic tangents.
template <typename T>
bool Ts_TypedKnotData<T>::UpdateTangents(const Ts_TypedKnotData<T>* prevData,
                                         const Ts_TypedKnotData<T>* nextData,
                                         const TsCurveType curveType)
{
    bool preResult = _UpdateTangent(prevData, nextData,
                                    curveType,
                                    /*updatePre =*/ true);

    bool postResult = _UpdateTangent(prevData, nextData,
                                     curveType,
                                     /*updatePre =*/ false);

    // Return true if we successfully ran any algorithm.
    return preResult || postResult;
}

template <typename T>
bool Ts_TypedKnotData<T>::_UpdateTangent(
    const Ts_TypedKnotData<T>* prevData,
    const Ts_TypedKnotData<T>* nextData,
    const TsCurveType curveType,
    bool updatePre)
{
    TsTangentAlgorithm algorithm = (updatePre
                                    ? preTanAlgorithm
                                    : postTanAlgorithm);

    switch (algorithm)
    {
      case TsTangentAlgorithmNone:
      case TsTangentAlgorithmCustom:  // XXX: Custom is not yet implemented
        // Even the null algorithm may change the tangent widths if the
        // curveType is TsCurveTypeHermite
        if (curveType == TsCurveTypeHermite) {
            if (updatePre && prevData) {
                preTanWidth = (time - prevData->time) / 3;
            } else if (!updatePre && nextData) {
                postTanWidth = (nextData->time - time) / 3;
            }
        }
        // Report success, even if we successfully did nothing
        return true;

      case TsTangentAlgorithmAutoEase:
        // Compute a slope that blends between the slope to the previous and
        // next knots. The tangent width will be 1/3 the distance to the
        // adjacent knot so it's good for both Bezier and Hermite splines.
        return _UpdateTangentAutoEase(prevData, nextData, updatePre);
    }

    // It should be impossible to reach this point.
    TF_CODING_ERROR("Tangent algorithm: '%s' did not return.",
                    TfEnum::GetName(algorithm).c_str());
    return false;
}

template <typename T>
bool
Ts_TypedKnotData<T>::_UpdateTangentAutoEase(
    const Ts_TypedKnotData<T>* prevData,
    const Ts_TypedKnotData<T>* nextData,
    bool updatePre)
{
    // This algorithm matches the "Auto Ease" algorithm from Maya or the open
    // source animX code, with some modifications for additional features that
    // exist only in TsSplines like dual valued knots or value blocks.

    // Bail if we're asked to compute a tangent that's off the end of the
    // spline in the extrapolation regions. We have nothing to blend and
    // those tangents have no effect on the curve.
    if ((updatePre && !prevData) || (!updatePre && !nextData)) {
        return false;
    }

    double prevTime = prevData ? prevData->time : 0.0;
    double nextTime = nextData ? nextData->time : 0.0;

    // If there is a discontinuity of any kind in the spline, the conputed slope
    // is 0. Discontinuities include the first or last knot, a dual valued knot,
    // or value blocks.
    const bool discontinuity = !prevData ||
                               !nextData ||
                               dualValued ||
                               nextInterp == TsInterpValueBlock ||
                               prevData->nextInterp == TsInterpValueBlock;
    if (discontinuity) {
        // Set the tangent slope to 0 and the width to 1/3 the segment width.
        if (updatePre) {
            preTanSlope = T(0.0);
            preTanWidth = (time - prevTime) / 3;
        } else {
            postTanSlope = T(0.0);
            postTanWidth = (nextTime - time) / 3;
        }
        return true;
    }

    // We have knots on either side. Blend between the slopes of the lines
    // from this knot to the other knots. Do the math in double precision.
    const double prevValue = prevData->value;
    const double knotValue = value;
    const double nextValue = nextData->GetPreValue();

    const double prevSlope = (knotValue - prevValue) / (time - prevTime);
    const double nextSlope = (nextValue - knotValue) / (nextTime - time);

    double slope = 0.0;
    // Only calculate slope if this knot's value is between the previous and
    // next values (if prevSlope and nextSlope have the same sign). Otherwise
    // the slope will always be 0.0
    if (prevSlope * nextSlope > 0) {
        const double f = (time - prevTime) / (nextTime - prevTime);

        // Cubic interpolation
        //
        // This is a family of cubic functions g with c in [-1/2 .. 1], and
        // f in [0,1]
        //
        // g(c,f) = 0.5 + (f-0.5)*(1-c+4*c*(f-0.5)^2)
        //
        // We fix c at 0.5 and let u = (f - 0.5) to get
        // g(u) = 0.5 + u * (0.5 + 2 * u * u)

        // Ease interpolation - this is the cubic interpolation that
        // uses the coefficient that gives the strongest influence to
        // its closest neighbor.
        //
        const double u = f - 0.5;

        const double g = 0.5 + u * (0.5 + 2 * u * u);

        // Now use g to interpolate the slope
        //
        slope = GfLerp(g, prevSlope, nextSlope);

        // Clamp, we already know that prevSlope and nextSlope are both positive
        // or both negative and not zero.
        if (nextSlope > 0) {
            // The slopes are all positive
            slope = std::min({slope, 3 * nextSlope, 3 * prevSlope});
        } else {
            // The slopes are all negative
            slope = std::max({slope, 3 * nextSlope, 3 * prevSlope});
        }
    }

    double width = (updatePre ? time - prevTime : nextTime - time) / 3.0;

    // The time values are all guaranteed to be different so the slope cannot be
    // vertical, but it can be arbitrarily close. So we need to take care with
    // our data conversion, especially when converting to GfHalf. Even though
    // the tangent widths are double values, we may need to increase them in
    // concert with reduced slope values to ensure that the endpoint of the
    // tangent is in the right area.
    //
    // As a concrete example, we have a test case with a tangent that has a
    // slope of 1.0e+12 and a width of 1.0e-12. So the end point of the tangent
    // would be in the vicinity of (0, 1.0). If we niavely convert the slope to
    // std::numeric_limits<GfHalf>::max() without changing the width then we get
    // a slope of 65504.0 and the width remains at 1.0e-12 which puts the end
    // point of the tangent at (1.0e-12, 6.5504e-8) which is in the vicinity of
    // (0.0, 0.0). In this case we need to change the width to height/slope (in
    // this case to 1.0/65504.0) to get a tangent end point at (1.5e-5, 1.0)
    //
    // Recompute the width if necessary to keep the tangent endpoint in the
    // right vicinity.
    T typedSlope = T(slope);

    // See if we overflowed the type.
    //
    // XXX: There is currently an issue on Mac where std::isfinite(GfHalf) is
    // failing because std::is_arithmetic<GfHalf>::value is false. Work around
    // it by casting the T back to a double. If it overflowed to infinity,
    // casting it back will preserve the infinite value.
    if (!std::isfinite(double(typedSlope))) {
        double height = slope * width;
        T typedSlope = T(std::copysign(double(std::numeric_limits<T>::max()),
                                       slope));
        width = height / typedSlope;
    }

    if (updatePre) {
        preTanSlope = typedSlope;
        preTanWidth = width;
    } else {
        postTanSlope = typedSlope;
        postTanWidth = width;
    }

    return true;
}

// static
std::unique_ptr<Ts_KnotDataProxy>
Ts_KnotDataProxy::Create(Ts_KnotData *data, const TfType valueType)
{
    Ts_KnotDataProxy *result = nullptr;
    TsDispatchToValueTypeTemplate<_ProxyCreator>(
        valueType, data, &result);
    return std::unique_ptr<Ts_KnotDataProxy>(result);
}

Ts_KnotDataProxy::~Ts_KnotDataProxy() = default;

// Instantiate the tangent updating methods
#define _MAKE_CLAUSE(unused, tuple)                                     \
template bool                                                           \
    Ts_TypedKnotData<TS_SPLINE_VALUE_CPP_TYPE(tuple)>::UpdateTangents(  \
        const Ts_TypedKnotData<TS_SPLINE_VALUE_CPP_TYPE(tuple)>* prevData, \
        const Ts_TypedKnotData<TS_SPLINE_VALUE_CPP_TYPE(tuple)>* nextData, \
        const TsCurveType curveType);                                   \
template bool                                                           \
    Ts_TypedKnotData<TS_SPLINE_VALUE_CPP_TYPE(tuple)>::_UpdateTangent(  \
        const Ts_TypedKnotData<TS_SPLINE_VALUE_CPP_TYPE(tuple)>* prevData, \
        const Ts_TypedKnotData<TS_SPLINE_VALUE_CPP_TYPE(tuple)>* nextData, \
        const TsCurveType curveType,                                    \
        bool updatePre);                                                \
template bool                                                           \
    Ts_TypedKnotData<TS_SPLINE_VALUE_CPP_TYPE(tuple)>::_UpdateTangentAutoEase( \
        const Ts_TypedKnotData<TS_SPLINE_VALUE_CPP_TYPE(tuple)>* prevData, \
        const Ts_TypedKnotData<TS_SPLINE_VALUE_CPP_TYPE(tuple)>* nextData, \
        bool updatePre);

TF_PP_SEQ_FOR_EACH(_MAKE_CLAUSE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES)
#undef _MAKE_CLAUSE

PXR_NAMESPACE_CLOSE_SCOPE
