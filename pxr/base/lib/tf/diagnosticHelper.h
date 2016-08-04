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
#ifndef TF_DIAGNOSTIC_HELPER_H
#define TF_DIAGNOSTIC_HELPER_H

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/attributes.h"

#include <boost/any.hpp>
#include <cstdarg>
#include <string>

typedef boost::any TfDiagnosticInfo;
class TfCallContext;
class TfEnum;
class TfError;

TF_API bool
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg);

TF_API bool
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(3, 4);

TF_API bool
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const std::string &msg);

TF_API bool
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(4, 5);

TF_API bool
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const TfDiagnosticInfo &info,
    const std::string &msg);

TF_API bool
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const TfDiagnosticInfo &info,
    const char *fmt, ...) ARCH_PRINTF_FUNCTION(4, 5);

TF_API bool
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg);

TF_API bool
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
    const TfEnum &code,
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


#endif // TF_DIAGNOSTIC_HELPER_H
