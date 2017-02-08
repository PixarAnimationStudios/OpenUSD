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

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticHelper.h"

#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/enum.h"

#include <stdarg.h>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

// Helper functions for posting an error with TF_ERROR.
bool
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg)
{
    TfDiagnosticMgr::ErrorHelper(context, code,
                                 TfEnum::GetName(code).c_str()).Post(msg);
    return false;
}

bool
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Tf_PostErrorHelper(context, code, TfVStringPrintf(fmt, ap));
    va_end(ap);
    return false;
}

bool
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg)
{
    TfDiagnosticMgr::ErrorHelper(context, code,
                                 TfEnum::GetName(code).c_str()).PostQuietly(msg);
    return false;
}

bool
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const TfDiagnosticInfo& info,
    const std::string &msg)
{
    TfDiagnosticMgr::ErrorHelper(context, code,
                                 TfEnum::GetName(code).c_str()).PostQuietly(msg, info);
    return false;
}

bool
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Tf_PostQuietlyErrorHelper(context, code, TfVStringPrintf(fmt, ap));
    va_end(ap);
    return false;
}

bool
Tf_PostQuietlyErrorHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const TfDiagnosticInfo& info,
    const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Tf_PostQuietlyErrorHelper(context, code, info, TfVStringPrintf(fmt, ap));
    va_end(ap);
    return false;
}

bool
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const std::string &msg)
{
    TfDiagnosticMgr::ErrorHelper(context, code,
                                 TfEnum::GetName(code).c_str()).PostWithInfo(msg, info);
    return false;
}

bool
Tf_PostErrorHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    Tf_PostErrorHelper(context, info,
                       code, TfVStringPrintf(fmt, ap));
    va_end(ap);
    return false;
}

// Helper functions for posting a warning with TF_WARN without a diagnostic code
// or a TfDiagnosticInfo.
void
Tf_PostWarningHelper(const TfCallContext &context, const std::string &msg)
{
    TfDiagnosticMgr::WarningHelper(context, TF_DIAGNOSTIC_WARNING_TYPE,
        TfEnum::GetName(TfEnum(TF_DIAGNOSTIC_WARNING_TYPE)).c_str())
        .Post(msg);
}

void
Tf_PostWarningHelper(const TfCallContext &context, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    Tf_PostWarningHelper(context, TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_PostWarningHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg)
{
    TfDiagnosticMgr::WarningHelper(context, code,
                                   TfEnum::GetName(code).c_str()).Post(msg);
}

void
Tf_PostWarningHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    Tf_PostWarningHelper(context, code, TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_PostWarningHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const std::string &msg)
{
    TfDiagnosticMgr::WarningHelper(context, code,
                                   TfEnum::GetName(code).c_str()).PostWithInfo(msg, info);
}

void
Tf_PostWarningHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    Tf_PostWarningHelper(context, info, code, TfVStringPrintf(fmt, ap));
    va_end(ap);
}

// Helper functions for posting a status message with TF_STATUS without a
// diagnostic code or a TfDiagnosticInfo.
void
Tf_PostStatusHelper(const TfCallContext &context, const std::string &msg)
{
    TfDiagnosticMgr::StatusHelper(context, TF_DIAGNOSTIC_STATUS_TYPE,
                 TfEnum::GetName(TfEnum(TF_DIAGNOSTIC_STATUS_TYPE)).c_str())
        .Post(msg);
}

void
Tf_PostStatusHelper(const TfCallContext &context, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    Tf_PostStatusHelper(context, TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_PostStatusHelper(
    const TfCallContext &context,
    const TfEnum &code,
    const std::string &msg)
{
    TfDiagnosticMgr::StatusHelper(context, code,
                                  TfEnum::GetName(code).c_str()).Post(msg);
}

void
Tf_PostStatusHelper(
    const TfCallContext &context, 
    const TfEnum &code,
    const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    Tf_PostStatusHelper(context, code, TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_PostStatusHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const std::string &msg)
{
    TfDiagnosticMgr::StatusHelper(context, code,
                                  TfEnum::GetName(code).c_str()).PostWithInfo(msg, info);
}

void
Tf_PostStatusHelper(
    const TfCallContext &context,
    const TfDiagnosticInfo &info,
    const TfEnum &code,
    const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Tf_PostStatusHelper(context, info, code, TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_DiagnosticHelper::IssueError(char const *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    TfDiagnosticMgr::ErrorHelper(GetContext(), GetType(),
        TfEnum::GetName(TfEnum(GetType())).c_str()).
        Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_DiagnosticHelper::IssueFatalError(char const *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    TfDiagnosticMgr::FatalHelper(GetContext(), GetType()).
        Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_DiagnosticHelper::IssueWarning(char const *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    TfDiagnosticMgr::WarningHelper(GetContext(), GetType(),
        TfEnum::GetName(TfEnum(GetType())).c_str()).
        Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_DiagnosticHelper::IssueStatus(char const *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    TfDiagnosticMgr::StatusHelper(GetContext(), GetType(),
        TfEnum::GetName(TfEnum(GetType())).c_str()).
        Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_DiagnosticHelper::IssueError(std::string const &msg) const
{
    TfDiagnosticMgr::ErrorHelper(GetContext(), GetType(),
            TfEnum::GetName(TfEnum(GetType())).c_str()).
            Post(msg);
}

void
Tf_DiagnosticHelper::IssueFatalError(std::string const &msg) const
{
    TfDiagnosticMgr::FatalHelper(GetContext(), TfEnum(_type)).Post(msg);
}

void
Tf_DiagnosticHelper::IssueWarning(std::string const &msg) const
{
    TfDiagnosticMgr::WarningHelper(GetContext(), GetType(),
        TfEnum::GetName(TfEnum(GetType())).c_str()).Post(msg);
}

void
Tf_DiagnosticHelper::IssueStatus(std::string const &msg) const
{
    TfDiagnosticMgr::StatusHelper(GetContext(), GetType(),
        TfEnum::GetName(TfEnum(GetType())).c_str()).Post(msg);
}

/*
 * The "lite" versions.
 */

void
Tf_DiagnosticLiteHelper::IssueError(char const *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    TfDiagnosticMgr::ErrorHelper(_context, _type,
        TfEnum::GetName(TfEnum(_type)).c_str()).
        Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_DiagnosticLiteHelper::IssueFatalError(char const *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    TfDiagnosticMgr::FatalHelper(_context, _type).
        Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_DiagnosticLiteHelper::IssueWarning(char const *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    TfDiagnosticMgr::WarningHelper(_context, _type,
        TfEnum::GetName(TfEnum(_type)).c_str()).
        Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
Tf_DiagnosticLiteHelper::IssueStatus(char const *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    TfDiagnosticMgr::StatusHelper(_context, _type,
        TfEnum::GetName(TfEnum(_type)).c_str()).
        Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

PXR_NAMESPACE_CLOSE_SCOPE
