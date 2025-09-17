//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_ERROR_H
#define PXR_BASE_ARCH_ERROR_H

/// \file arch/error.h
/// \ingroup group_arch_Diagnostics
/// Low-level fatal error reporting.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/functionLite.h"
#include <stddef.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Print message to standard error and abort program.
///
/// \param msg The reason for the failure.
/// \param funcName The name of the function that \c Arch_Error was called from.
/// \param lineNo The line number of the file that \c Arch_Error was called from.
/// \param fileName The name of the file that \c Arch_Error was called from.
///
/// \private
[[noreturn]]
ARCH_API
void Arch_Error(const char* msg, const char* funcName,
                size_t lineNo, const char* fileName);

/// Print warning message to standard error, but continue execution.
///
/// \param msg The reason for the warning.
/// \param funcName The name of the function that \c Arch_Warning was called from.
/// \param lineNo The line number of the file that \c Arch_Warning was called from.
/// \param fileName The name of the file that \c Arch_Warning was called from.
///
/// \private
ARCH_API
void Arch_Warning(const char* msg, const char* funcName,
                  size_t lineNo, const char* fileName);

/// \addtogroup group_arch_Diagnostics
///@{

/// Unconditionally aborts the program.
///
/// \param msg is a literal string, a \c const \c char* (but not
///        an \c std::string) that describes why the program is aborting.
/// \hideinitializer
#define ARCH_ERROR(msg) \
    Arch_Error(msg, __ARCH_FUNCTION__, __LINE__, __ARCH_FILE__)

/// Prints a warning message to stderr.
///
/// \param msg is a literal string, a \c const \c char* (but not
///        an \c std::string).
/// \hideinitializer
#define ARCH_WARNING(msg) \
    Arch_Warning(msg, __ARCH_FUNCTION__, __LINE__, __ARCH_FILE__)

/// Aborts the program if \p cond evaluates to false.
/// \hideinitializer
#define ARCH_AXIOM(cond) \
    if (!(cond)) ARCH_ERROR("[" #cond "] axiom failed")

///@}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_ERROR_H
