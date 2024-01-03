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
#define OPENEXR_EXPORT static
#define EXR_EXPORT static
#define EXR_INTERNAL static
#ifdef IMATH_HALF_SAFE_FOR_C
#    undef IMATH_HALF_SAFE_FOR_C
#endif
#define OPENEXR_VERSION_MAJOR 3 //@OpenEXR_VERSION_MAJOR@
#define OPENEXR_VERSION_MINOR 2 //@OpenEXR_VERSION_MINOR@
#define OPENEXR_VERSION_PATCH 0 //@OpenEXR_VERSION_PATCH@
#define ILMTHREAD_THREADING_ENABLED
#define ILMBASE_THREADING_ENABLED
#define OPENEXR_C_STANDALONE
// pxr end

/// \addtogroup ExportMacros
/// @{

#ifndef EXR_EXPORT
// are we making a DLL under windows (might be msvc or mingw or others)
#    if defined(OPENEXR_DLL)

// when building as a DLL for windows, typical dllexport/import case
// where we need to switch depending on whether we are compiling
// internally or not
#        if defined(OPENEXRCORE_EXPORTS)
#            define EXR_EXPORT __declspec (dllexport)
#        else
#            define EXR_EXPORT __declspec (dllimport)
#        endif

#    else
#        define EXR_EXPORT static
#    endif
#endif 

#ifndef EXR_INTERNAL
#    define EXR_INTERNAL
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
