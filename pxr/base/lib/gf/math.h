//
// Copyright 2016 Pixar
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
#ifndef GF_MATH_H
#define GF_MATH_H

/// \file gf/math.h
/// \ingroup group_gf_BasicMath
/// Assorted mathematical utility functions.

#include "pxr/pxr.h"
#include "pxr/base/arch/math.h"
#include "pxr/base/gf/api.h"

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // GF_MATH_H 
