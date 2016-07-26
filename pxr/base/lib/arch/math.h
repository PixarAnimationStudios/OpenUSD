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
#ifndef ARCH_MATH_H
#define ARCH_MATH_H

/*!
 * \file math.h
 * \brief Architecture-specific math function calls.
 * \ingroup group_arch_Math
 */

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/inttypes.h"
#include <math.h>
#include <cmath>

#if defined(ARCH_OS_WINDOWS)
#include <float.h>
#endif

/*!
 * \brief This is the smallest value e such that 1+e^2 == 1, using floats.
 * \ingroup group_arch_Math
 */

#if defined (ARCH_CPU_INTEL) || defined(doxygen)
/*
 * True for all IEEE754 chipsets.
 */
#define ARCH_MIN_FLOAT_EPS_SQR      0.000244141F

/*!
 * \brief Three-valued sign.  Return 1 if val > 0, 0 if val == 0, or -1 if val <
 * 0;
 */
inline long ArchSign(long val) {
    return (val > 0) - (val < 0);
}

/*!
 * \brief Returns The IEEE-754 bit pattern of the specified single precision
 * value as a 32-bit unsigned integer.
 * \ingroup group_arch_Math
 */
inline uint32_t ArchFloatToBitPattern(float v) {
    union {
        float _float;
        uint32_t _uint;
    } value;
    value._float = v;
    return value._uint;
}

/*!
 * \brief Returns The single precision floating point value corresponding to
 * the given IEEE-754 bit pattern.
 * \ingroup group_arch_Math
 */
inline float ArchBitPatternToFloat(uint32_t v) {
    union {
        uint32_t _uint;
        float _float;
    } value;
    value._uint = v;
    return value._float;
}

/*!
 * \brief Returns The IEEE-754 bit pattern of the specified double precision
 * value as a 64-bit unsigned integer.
 * \ingroup group_arch_Math
 */
inline uint64_t ArchDoubleToBitPattern(double v) {
    union {
        double _double;
        uint64_t _uint;
    } value;
    value._double = v;
    return value._uint;
}

/*!
 * \brief Returns The double precision floating point value corresponding to
 * the given IEEE-754 bit pattern.
 * \ingroup group_arch_Math
 */
inline double ArchBitPatternToDouble(uint64_t v) {
    union {
        uint64_t _uint;
        double _double;
    } value;
    value._uint = v;
    return value._double;
}

#else
#error Unknown system architecture.
#endif


/*
 * A bunch of math functions operating on floats.
 */

#if defined(ARCH_OS_LINUX) || defined(doxygen)

/*!
 * \brief Computes the sine and cosine of the specified value as a float.
 * \ingroup group_arch_Math
 */
inline void ArchSinCosf(float v, float *s, float *c) { sincosf(v, s, c); }

/*!
 * \brief Computes the sine and cosine of the specified value as a double.
 * \ingroup group_arch_Math
 */
inline void ArchSinCos(double v, double *s, double *c) { sincos(v, s, c); }

#elif defined(ARCH_OS_DARWIN) || defined(ARCH_OS_WINDOWS)

inline void ArchSinCosf(float v, float *s, float *c) {
    *s = std::sin(v);
    *c = std::cos(v);  
}
inline void ArchSinCos(double v, double *s, double *c) {
    *s = std::sin(v);
    *c = std::cos(v);  
}

#else
#error Unknown architecture.
#endif

#endif // ARCH_MATH_H
