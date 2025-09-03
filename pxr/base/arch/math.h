//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_MATH_H
#define PXR_BASE_ARCH_MATH_H

/// \file arch/math.h
/// \ingroup group_arch_Math
/// Architecture-specific math function calls.

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/inttypes.h"

#if defined(ARCH_COMPILER_MSVC)
#include <intrin.h>
#endif

#include <cmath>
#if !defined(M_PI)
#define M_PI 3.14159265358979323846
#endif

PXR_NAMESPACE_OPEN_SCOPE

/// \addtogroup group_arch_Math
///@{

#if defined (ARCH_CPU_INTEL) || defined (ARCH_CPU_ARM) ||  \
    defined(ARCH_OS_WASM_VM) || defined (doxygen)

/// This is the smallest value e such that 1+e^2 == 1, using floats.
/// True for all IEEE754 chipsets.
#define ARCH_MIN_FLOAT_EPS_SQR      0.000244141F

/// Three-valued sign.  Return 1 if val > 0, 0 if val == 0, or -1 if val < 0.
inline long ArchSign(long val) {
    return (val > 0) - (val < 0);
}

/// Returns The IEEE-754 bit pattern of the specified single precision value
/// as a 32-bit unsigned integer.
inline uint32_t ArchFloatToBitPattern(float v) {
    union {
        float _float;
        uint32_t _uint;
    } value;
    value._float = v;
    return value._uint;
}

/// Returns The single precision floating point value corresponding to the
/// given IEEE-754 bit pattern.
inline float ArchBitPatternToFloat(uint32_t v) {
    union {
        uint32_t _uint;
        float _float;
    } value;
    value._uint = v;
    return value._float;
}

/// Returns The IEEE-754 bit pattern of the specified double precision value
/// as a 64-bit unsigned integer.
inline uint64_t ArchDoubleToBitPattern(double v) {
    union {
        double _double;
        uint64_t _uint;
    } value;
    value._double = v;
    return value._uint;
}

/// Returns The double precision floating point value corresponding to the
/// given IEEE-754 bit pattern.
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

#if defined(ARCH_OS_LINUX) || defined(doxygen)

/// Computes the sine and cosine of the specified value as a float.
inline void ArchSinCosf(float v, float *s, float *c) { sincosf(v, s, c); }

/// Computes the sine and cosine of the specified value as a double.
inline void ArchSinCos(double v, double *s, double *c) { sincos(v, s, c); }

#elif defined(ARCH_OS_DARWIN) || defined(ARCH_OS_WINDOWS) || \
      defined(ARCH_OS_WASM_VM)

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


/// Return the number of consecutive 0-bits in \p x starting from the least
/// significant bit position.  If \p x is 0, the result is undefined.
inline int
ArchCountTrailingZeros(uint64_t x)
{
#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG) && \
    !defined(ARCH_OS_WASM_VM)
    return __builtin_ctzl(x);
#elif defined(ARCH_COMPILER_MSVC)
    unsigned long index;
    _BitScanForward64(&index, x);
    return index;
#else
    // Flip trailing zeros to 1s, and clear all other bits, then count.
    x = (x ^ (x - 1)) >> 1;
    int c = 0;
    for (; x; ++c) {
        x >>= 1;
    }
    return c;
#endif
}


///@}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_MATH_H
