//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_DIAGNOSTIC_HELPER_H
#define PXR_BASE_TF_DIAGNOSTIC_HELPER_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/defines.h"

// XXX: This include is a hack to avoid build errors due to
// incompatible macro definitions in pyport.h on macOS.
#include <locale>

#include <any>
#include <string>

// Follow up changes should more tightly scope these to just where it's needed
// in pxr.
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
// Include <unistd.h> to provide _exit for tf/debugger.cpp and dependencies
// that were previously transitively getting this from boost
#include <unistd.h>
// Include <cstring> to provide memset, memcmp, and memcpy for dependencies
// that were previously transitively getting them from boost
#include <cstring>
#endif

PXR_NAMESPACE_OPEN_SCOPE

typedef std::any TfDiagnosticInfo;
class TfCallContext;
enum TfDiagnosticType : int;
class TfEnum;
class TfError;

TF_API void
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg);

TF_API void
Tf_PostErrorHelper(
    const TfCallContext &context,
    TfDiagnosticType code,
    const std::string &msg);

TF_API void
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(3, 4);

TF_API void
Tf_PostErrorHelper(
    const TfCallContext &context,
    TfDiagnosticType code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(3, 4);

TF_API void
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const std::string &msg);

TF_API void
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(4, 5);

TF_API void
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const TfDiagnosticInfo &info,
    const std::string &msg);

TF_API void
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const TfDiagnosticInfo &info,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(4, 5);

TF_API void
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg);

TF_API void
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(3, 4);


// Helper functions for posting a warning with TF_WARN.
TF_API void
Tf_PostWarningHelper(const TfCallContext &context,
                     const std::string &msg);

TF_API void
Tf_PostWarningHelper(const TfCallContext &context,
                     const char *fmt, ...)  ARCH_PRINTF_FUNCTION(2, 3);

TF_API void
Tf_PostWarningHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg);

TF_API void
Tf_PostWarningHelper(
    const TfCallContext &context,
    TfDiagnosticType code,
    const std::string &msg);

TF_API void
Tf_PostWarningHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(3, 4);

TF_API void
Tf_PostWarningHelper(
    const TfCallContext &context,
    TfDiagnosticType code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(3, 4);

TF_API void
Tf_PostWarningHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const std::string &msg);

TF_API void
Tf_PostWarningHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(4, 5);

TF_API void
Tf_PostStatusHelper(
    const TfCallContext &context, 
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(2, 3);

TF_API void
Tf_PostStatusHelper(
    const TfCallContext &context, 
    const std::string &msg);


TF_API void
Tf_PostStatusHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg);

TF_API void
Tf_PostStatusHelper(
    const TfCallContext &context, 
    const TfEnum &code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(3, 4);

TF_API void
Tf_PostStatusHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const std::string &msg);

TF_API void
Tf_PostStatusHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(4, 5);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_DIAGNOSTIC_HELPER_H
