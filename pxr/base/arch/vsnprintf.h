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
#ifndef PXR_BASE_ARCH_VSNPRINTF_H
#define PXR_BASE_ARCH_VSNPRINTF_H

/// \file arch/vsnprintf.h
/// \ingroup group_arch_Strings
/// Architecture dependent memory-safe sprintf capability

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"
#include "pxr/base/arch/attributes.h"

#include <stdio.h>
#include <stdarg.h>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \addtogroup group_arch_Strings
///@{

/// Return the number of characters (not including the null character)
/// necessary for a particular sprintf into a string.
///
/// \c ArchVsnprintf guarantees the C99 behavior of vsnprintf on all systems:
/// it returns the number of bytes (not including the terminating null
/// character) needed to actually print the requested string.  If \c size
/// indicates that \c str has enough capacity to hold the result, then the
/// function actually prints into \c str.
///
/// You probably want to use the functionality of this call via
/// \c TfStringPrintf().
///
ARCH_API
int ArchVsnprintf(char *str, size_t size, const char *format, va_list ap)
#ifndef doxygen
    ARCH_PRINTF_FUNCTION(3, 0)
#endif /* doxygen */
    ;

/// Returns a string formed by a printf()-like specification.
///
/// \c ArchStringPrintf() is a memory-safe architecture-independent way of 
/// forming a string using printf()-like formatting.  For example,
/// \code
///  string formatMsg(const string& caller, int i, double val[])
///  {
///     return ArchStringPrintf("%s: val[%d] = %g\n", caller.c_str(), i, val[i]);
///  }
/// \endcode
///
/// The function is safe only to the extent that the arguments match the
/// formatting string.  In particular, be careful to pass strings themselve
/// into \c ArchStringPrintf() as in the above example (i.e. \c caller.c_str()
/// as opposed to just passing \c caller).
///
ARCH_API
std::string ArchStringPrintf(const char *fmt, ...)
#ifndef doxygen
    ARCH_PRINTF_FUNCTION(1, 2)
#endif /* doxygen */
    ;

/// Returns a string formed by a printf()-like specification.
///
/// \c ArchVStringPrintf() is equivalent to \c ArchStringPrintf() except that
/// it is called with a \c va_list instead of a variable number of arguments.
/// \c ArchVStringPrintf() does not call the \c va_end macro. Consequently,
/// the value of \c ap is undefined after the call. A functions that calls \c
/// ArchVStringPrintf() should call \c va_end(ap) itself afterwards.
///
ARCH_API
std::string ArchVStringPrintf(const char *fmt, va_list ap)
#ifndef doxygen
    ARCH_PRINTF_FUNCTION(1, 0)
    ;
#endif

/// @}

PXR_NAMESPACE_CLOSE_SCOPE
    
#endif // PXR_BASE_ARCH_VSNPRINTF_H
