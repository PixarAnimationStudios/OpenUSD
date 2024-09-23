//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_MATH_H
#define PXR_BASE_GF_MATH_H

/// \file gf/math.h
/// \ingroup group_gf_BasicMath
/// Assorted mathematical utility functions.

#include "pxr/pxr.h"
#include "pxr/base/arch/math.h"
#include "pxr/base/gf/api.h"
#include "pxr/base/gf/traits.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// Returns true if \p a and \p b are with \p epsilon of each other.
/// \ingroup group_gf_BasicMath
inline bool GfIsClose(double a, double b, double epsilon) {
    return fabs(a-b) < epsilon;
}

/// Converts an angle in radians to degrees.
/// \ingroup group_gf_BasicMath
inline double	GfRadiansToDegrees(double radians) {
    return radians * (180.0 / M_PI);
}

/// Converts an angle in degrees to radians.
/// \ingroup group_gf_BasicMath
inline double	GfDegreesToRadians(double degrees) {
    return degrees * (M_PI / 180.0);
}

/// Smooth step function using a cubic hermite blend.
/// \ingroup group_gf_BasicMath
///
/// Returns 0 if \p val <= \p min, and 1 if \p val >= \p max.
/// As \p val varies between \p min and \p max, the return value smoothly
/// varies from 0 to 1 using a cubic hermite blend, with given slopes at the
/// min and max points.  The slopes are in the space that min and max are in.
GF_API
double GfSmoothStep(double min, double max, double val, 
    double slope0 = 0.0, double slope1 = 0.0);

/// Smooth Step with independently controllable shoulders
/// \ingroup group_gf_BasicMath
///
/// Based on an idea and different implementation by Rob Cook.  See his
/// notes attached at the end.
///
/// I (whorfin) extended this to have independently controllable shoulders at
/// either end, and to specify shoulders directly in the domain of the curve.
/// Rob's derivation frankly confused me, so I proceeded slightly differently.
/// This derivation has more degrees of freedom but is the same order, so some
/// tricks must be done.
///
/// Summary: This function is similar to "smoothstep" except that instead of
/// using a Hermite curve, the interpolation is done with a linear ramp with
/// smooth shoulders (i.e., C1 = continuous first derivatives).
///
/// Conceptually, it's a line with variable C1 zero-slope junctures.
///
///     Additionally, w0 + w1 <= 1.  Otherwise, the curves will take up
///     more space than is available, and "that would be bad".
///
///     A value of 0 for w0 and w1 gives a pure linear ramp.
///     A reasonable value for a symmetric smooth ramp is .2 for w0 and w1.
///        this means that the middle 60% of the ramp is linear, and the left
///        20% and right 20% are the transition into and out of the linear ramp.
///
/// The ramp looks like this:
/// <pre>
///                              smooth ********** <-result = 1
///                                  ***|
///                                **   |
///                               * |   |
///                      linear  *  |   |
///                             *   |   |
///                            *    |   |
///                   smooth **     |   tmax = end of ramp
///                       *** |     |
///    result=0 -> *******    |     tmax - w1*(tmax-tmin) = end of linear region
///                      |    |
///                      |    tmin + w0*(tmax-tmin) = start of linear region
///                      |
///                      tmin = start of ramp
/// </pre>
///  Derivation:
///
///  We're going to splice parabolas onto both ends for the "0 slope smooth"
///  connectors.  So we therefore constrain the parabolic sections to have
///  a given width and given slope (the slope of the connecting line segment)
///  at the "w" edge.
///
///  We'll first derive the equations for the parabolic splicing segment,
///  expressed at the origin (but generalizable by flipping).
///
///  Given:
/// <pre>
///  f(t) = a t� + b t + c
///  f(0) = 0
///  f'(0) = 0
///  f(w) = y    At the "w" edge of the shoulder, value is y
///  f'(w) = s   ...what is the slope there? s...
///
///  -->
///      c = 0
///      b = 0
///      a = � s/w
///      y = � w s
///  -->
///      g(t,w,s) = � s t� / w   # Our parabolic segment
/// </pre>
///
///  Now, in our desired composite curve, the slope is the same at
///  both endpoints (since they're connected by a line).
///  This slope is (1-y0-y1)/(1-w0-w1) [from simple geometry].
///
///  More formally, let's express the constraints
///  Given:
/// <pre>
///  y(w,s) = w s /2
///  s = ( 1 - y(w0, s) - y(w1, s) ) / (1 - w0 - w1)
///
///  -->
///      s(w0,w1) = 2 / (2 - w0 - w1)
/// </pre>
///
///      So now we're done; we splice these two together and connect
///      with a line.
///
///  The domain and range of this function is [0,1]
/// <pre>
///      f(t, w0, w1) =
///              g(t, w0, s(w0,w1))      t<w0
///
///          1-g(1-t, w1, s(w0,w1))      t>1-w1
///
///          s(w0,w1) t - y(w0, s(w0,w1))    w0 <= t <= 1-w1
/// </pre>
///
///  Expanding and collecting terms gives us the result expressed in the
///  code below.  We also generalize to tmin/tmax form, in keeping with
///  smoothstep.  This simply involves reranging to [0,1] on input.
///
/// @param     tmin     where the ramp starts
/// @param     tmax     where the ramp ends (must be > tmin)
/// @param     t        location to evaluate in this call
/// @param     w0       size of the first smooth section as a fraction of
///                     the size of the ramp (tmax-tmin). This value must
///                     be in the range 0-1.
/// @param     w1       size of the second smooth section as a fraction of
///                     the size of the ramp (tmax-tmin). This value must
///                     be in the range 0-1.
GF_API
double GfSmoothRamp(double tmin, double tmax, double t, double w0, double w1); 

/// Returns the inner product of \c x with itself: specifically, \c x*x.
/// Defined for \c int, \c float, \c double, and all \c GfVec types.
/// \ingroup group_gf_BasicMath
template <class T>
inline double GfSqr(const T& x) {
    return x * x;
}

/// Return the signum of \p v (i.e. -1, 0, or 1).
///
/// The type \c T must implement the < and > operators; the function returns
/// zero only if value neither positive, nor negative.
///
/// \ingroup group_gf_BasicMath
template <typename T>
inline T
GfSgn(T v) {
    return (v < 0) ? -1 : ((v > 0) ? 1 : 0);
}

/// Return sqrt(\p f).
/// \ingroup group_gf_BasicMath
inline double GfSqrt(double f) { return std::sqrt(f); }
/// Return sqrt(\p f).
/// \ingroup group_gf_BasicMath
inline float GfSqrt(float f) { return std::sqrt(f); }

/// Return exp(\p f).
/// \ingroup group_gf_BasicMath
inline double GfExp(double f) { return std::exp(f); }
/// Return exp(\p f).
/// \ingroup group_gf_BasicMath
inline float GfExp(float f) { return std::exp(f); }

/// Return log(\p f).
/// \ingroup group_gf_BasicMath
inline double GfLog(double f) { return std::log(f); }
/// Return log(\p f).
/// \ingroup group_gf_BasicMath
inline float GfLog(float f) { return std::log(f); }

/// Return floor(\p f).
/// \ingroup group_gf_BasicMath
inline double GfFloor(double f) { return std::floor(f); }
/// Return floor(\p f).
/// \ingroup group_gf_BasicMath
inline float GfFloor(float f) { return std::floor(f); }

/// Return ceil(\p f).
/// \ingroup group_gf_BasicMath
inline double GfCeil(double f) { return std::ceil(f); }
/// Return ceil(\p f).
/// \ingroup group_gf_BasicMath
inline float GfCeil(float f) { return std::ceil(f); }

/// Return abs(\p f).
/// \ingroup group_gf_BasicMath
inline double GfAbs(double f) { return std::fabs(f); }
/// Return abs(\p f).
/// \ingroup group_gf_BasicMath
inline float GfAbs(float f) { return std::fabs(f); }

/// Return round(\p f).
/// \ingroup group_gf_BasicMath
inline double GfRound(double f) { return std::rint(f); }
/// Return round(\p f).
/// \ingroup group_gf_BasicMath
inline float GfRound(float f) { return std::rint(f); }

/// Return pow(\p f, \p p).
/// \ingroup group_gf_BasicMath
inline double GfPow(double f, double p) { return std::pow(f, p); }
/// Return pow(\p f, \p p).
/// \ingroup group_gf_BasicMath
inline float GfPow(float f, float p) { return std::pow(f, p); }

/// Return sin(\p v).
/// \ingroup group_gf_BasicMath
inline double GfSin(double v) { return std::sin(v); }
/// Return sin(\p v).
/// \ingroup group_gf_BasicMath
inline float GfSin(float v) { return std::sin(v); }
/// Return cos(\p v).
/// \ingroup group_gf_BasicMath
inline double GfCos(double v) { return std::cos(v); }
/// Return cos(\p v).
/// \ingroup group_gf_BasicMath
inline float GfCos(float v) { return std::cos(v); }
/// Return sin(\p v) in \p s and cos(\p v) in \p c.
/// \ingroup group_gf_BasicMath
inline void GfSinCos(double v, double *s, double *c) { ArchSinCos(v, s, c); }
/// Return sin(\p v) in \p s and cos(\p v) in \p c.
/// \ingroup group_gf_BasicMath
inline void GfSinCos(float v, float *s, float *c) { ArchSinCosf(v, s, c); }

/// Return the resulting of clamping \p value to lie between
/// \p min and \p max. This function is also defined for GfVecs.
/// \ingroup group_gf_BasicMath
inline double GfClamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/// \overload
/// \ingroup group_gf_BasicMath
inline float GfClamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}    

/// The mod function with "correct" behaviour for negative numbers.
///
/// If \p a = \c n \p b for some integer \p n, zero is returned.
/// Otherwise, for positive \p a, the value returned is \c fmod(a,b),
/// and for negative \p a, the value returned is \c fmod(a,b)+b.
///
/// \ingroup group_gf_BasicMath
GF_API
double GfMod(double a, double b);
/// \overload
// \ingroup group_gf_BasicMath
GF_API
float GfMod(float a, float b);

/// Linear interpolation function.
///
/// For any type that supports multiplication by a scalar and binary addition, returns 
/// \code 
/// (1-alpha) * a + alpha * b 
/// \endcode 
/// 
/// \ingroup group_gf_BasicMath
template <class T>
inline T GfLerp( double alpha, const T& a, const T& b) {
    return (1-alpha)* a + alpha * b;
}

/// Returns the smallest of the given \c values.
/// \ingroup group_gf_BasicMath
template <class T>
inline T GfMin(T a1, T a2) {
    return (a1 < a2 ? a1 : a2);
}
template <class T>
inline T GfMin(T a1, T a2, T a3) {
    return GfMin(GfMin(a1, a2), a3);
}
template <class T>
inline T GfMin(T a1, T a2, T a3, T a4) {
    return GfMin(GfMin(a1, a2, a3), a4);
}
template <class T>
inline T GfMin(T a1, T a2, T a3, T a4, T a5) {
    return GfMin(GfMin(a1, a2, a3, a4), a5);
}

/// Returns the largest of the given \c values.
/// \ingroup group_gf_BasicMath
template <class T>
inline T GfMax(T a1, T a2) {
    return (a1 < a2 ? a2 : a1);
}
template <class T>
inline T GfMax(T a1, T a2, T a3) {
    return GfMax(GfMax(a1, a2), a3);
}
template <class T>
inline T GfMax(T a1, T a2, T a3, T a4) {
    return GfMax(GfMax(a1, a2, a3), a4);
}
template <class T>
inline T GfMax(T a1, T a2, T a3, T a4, T a5) {
    return GfMax(GfMax(a1, a2, a3, a4), a5);
}

/// Returns the dot (inner) product of two vectors.
/// For scalar types, this is just the regular product.
/// \ingroup group_gf_BasicMath
template <typename Left, typename Right,
          std::enable_if_t<GfIsArithmetic<Left>::value &&
                           GfIsArithmetic<Right>::value, int> = 0>
inline decltype(std::declval<Left>() * std::declval<Right>())
GfDot(Left left, Right right) {
    return left * right;
}

/// Returns component-wise multiplication of vectors.
/// For scalar types, this is just the regular product.
/// \ingroup group_gf_BasicMath
template <typename Left, typename Right,
          std::enable_if_t<GfIsArithmetic<Left>::value &&
                           GfIsArithmetic<Right>::value, int> = 0>
inline decltype(std::declval<Left>() * std::declval<Right>())
GfCompMult(Left left, Right right) {
    return left * right;
}

/// Returns component-wise quotient of vectors.
/// For scalar types, this is just the regular quotient.
/// \ingroup group_gf_BasicMath
template <typename Left, typename Right,
          std::enable_if_t<GfIsArithmetic<Left>::value &&
                           GfIsArithmetic<Right>::value, int> = 0>
inline decltype(std::declval<Left>() / std::declval<Right>())
GfCompDiv(Left left, Right right) {
    return left / right;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_MATH_H 
