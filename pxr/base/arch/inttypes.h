//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_INTTYPES_H
#define PXR_BASE_ARCH_INTTYPES_H

/// \file arch/inttypes.h
/// \ingroup group_arch_Bits
/// Define integral types.
///
/// By including this file, the "standard" integer types \c int16_t,
/// \c int32_t, and \c int64_t are all defined, as are their unsigned
/// counterparts \c uint16_t, \c uint32_t, and \c uint64_t.  This also
/// includes the macros for limits, constants, and printf format specifiers.

// These defines should in theory not be needed to get the related sized-int
// macros, as this was not adopted by the C++ committee and was dropped by the C
// committee, but glibc erroneously "respects" them so we need to have them.
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <cinttypes>
#include <cstdint>

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#include <sys/types.h>

PXR_NAMESPACE_OPEN_SCOPE

typedef unsigned char uchar;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_INTTYPES_H
