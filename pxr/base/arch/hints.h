//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_HINTS_H
#define PXR_BASE_ARCH_HINTS_H

#include "pxr/base/arch/defines.h"

/// \file arch/hints.h
/// Compiler hints.
///
/// \c ARCH_LIKELY(bool-expr) and \c ARCH_UNLIKELY(bool-expr) will evaluate to
/// the value of bool-expr but will also emit compiler intrinsics providing
/// hints for branch prediction if the compiler has such intrinsics.  It is
/// advised that you only use these in cases where you empirically know the
/// outcome of bool-expr to a very high degree of certainty.  For example,
/// fatal-error cases, invariants, first-time initializations, etc.

#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)

#define ARCH_LIKELY(x) (__builtin_expect((bool)(x), true))
#define ARCH_UNLIKELY(x) (__builtin_expect((bool)(x), false))

#else

#define ARCH_LIKELY(x) (x)
#define ARCH_UNLIKELY(x) (x)

#endif

/// \c ARCH_GUARANTEE_TO_COMPILER(bool-expr) informs the compiler about value
/// constraints to help it make better optimizations.  It is of critical
/// importance that the guarantee is in fact always 100% true, otherwise the
/// compiler may generate invalid code.
///
/// This can be useful, for example, when an out-of-line function call could
/// potentially change a value previously known to compiler, but does not, based
/// on code invariants.
///
/// This hint is best used after investigating generated assembly code and
/// seeing useless/unreachable code being generated that can be prevented with
/// this hint.  This hint is often times not necessary, and should never be
/// inserted on a whim.  The compiler will run with the promises we make it to
/// the ends of the earth, and this can lead to surprising results
/// (e.g. undefined behavior) if we are not careful.

#if defined(ARCH_COMPILER_GCC) || \
    defined(ARCH_COMPILER_CLANG) || \
    defined(ARCH_COMPILER_ICC)

// Intentionally using __builtin_unreachable on clang for consistency, since
// __builtin_assume does not evaluate the expression, and our only option on gcc
// is the __builtin_unreachable branch.

#define ARCH_GUARANTEE_TO_COMPILER(x) \
    if (static_cast<bool>(x)) { } else { __builtin_unreachable(); }

#else

#define ARCH_GUARANTEE_TO_COMPILER(x)

#endif


#endif // PXR_BASE_ARCH_HINTS_H
