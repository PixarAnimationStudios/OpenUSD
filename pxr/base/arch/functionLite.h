//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_FUNCTION_LITE_H
#define PXR_BASE_ARCH_FUNCTION_LITE_H

/// \file arch/functionLite.h
/// Define preprocessor function name macros.
///
/// This file defines preprocessor macros for getting the current function
/// name and related information so they can be used in a architecture
/// independent manner.  This file needs to be public but shouldn't be 
/// included directly by anything outside of \c pxr/base/tf.

#include "pxr/base/arch/defines.h"
// Note: this file specifically does not include <string>.

#define __ARCH_FUNCTION__ __func__

#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_ICC) || \
    defined(ARCH_COMPILER_CLANG)
#    define __ARCH_PRETTY_FUNCTION__ __PRETTY_FUNCTION__
#elif defined(ARCH_COMPILER_MSVC)
#    define __ARCH_PRETTY_FUNCTION__ __FUNCSIG__
#else
#    define __ARCH_PRETTY_FUNCTION__ __ARCH_FUNCTION__
#endif /* defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_ICC) ||
          defined(ARCH_COMPILER_CLANG)*/

#if defined(BUILD_COMPONENT_SRC_PREFIX)
#    define __ARCH_FILE__ BUILD_COMPONENT_SRC_PREFIX __FILE__
#else
#    define __ARCH_FILE__ __FILE__
#endif /* defined(BUILD_COMPONENT_SRC_PREFIX) */

#endif // PXR_BASE_ARCH_FUNCTION_LITE_H
