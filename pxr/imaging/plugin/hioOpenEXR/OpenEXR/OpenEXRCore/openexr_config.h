/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CONF_H
#define OPENEXR_CONF_H
#pragma once

// pxr remove these
//#include <IlmThreadConfig.h>
//#include <ImathConfig.h>
// pxr add these
#include "OpenEXRConfigInternal.h"
#include "openexr_version.h"
#define OPENEXR_HIDDEN static
#define OPENEXR_EXPORT OPENEXR_HIDDEN
#define EXR_INTERNAL static
#ifdef IMATH_HALF_SAFE_FOR_C
#    undef IMATH_HALF_SAFE_FOR_C
#endif
#ifndef ILMTHREAD_THREADING_ENABLED
#define ILMTHREAD_THREADING_ENABLED 1
#endif
// pxr end

/// \addtogroup ExportMacros
/// @{

#if defined(OPENEXR_CORE_FUNCTIONS_EMBEDDED)
#    define EXR_EXPORT OPENEXR_HIDDEN
#else

// are we making a DLL under windows (might be msvc or mingw or others)
#if defined(OPENEXR_DLL)

// when building as a DLL for windows, typical dllexport/import case
// where we need to switch depending on whether we are compiling
// internally or not
#    if defined(OPENEXRCORE_EXPORTS)
#        define EXR_EXPORT __declspec (dllexport)
#    else
#        define EXR_EXPORT __declspec (dllimport)
#    endif

#else

#    define EXR_EXPORT OPENEXR_EXPORT

#endif

#endif

/*
 * MSVC does have printf format checks, but it is not in the form of a
 * function attribute, so just skip for non-GCC/clang builds
 */
#if defined(__GNUC__) || defined(__clang__)
#    define EXR_PRINTF_FUNC_ATTRIBUTE __attribute__ ((format (printf, 3, 4)))
#else
#    define EXR_PRINTF_FUNC_ATTRIBUTE
#endif

/// @}

#endif /* OPENEXR_CONF_H */
