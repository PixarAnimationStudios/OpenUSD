//
// Copyright 2020 Pixar
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

#include "pxr/usd/usdUtils/conditionalAbortDiagnosticDelegate.h"
#include "pxr/base/arch/debugger.h"
#include "pxr/base/tf/patternMatcher.h"
#include "pxr/base/tf/stackTrace.h"

#include <cstdio>

PXR_NAMESPACE_OPEN_SCOPE

// Helper function to print diagnostics in same format as TfDiagnosticMgr
void _PrintDiagnostic(const TfEnum &code, const TfCallContext &ctx,
                 const std::string& msg, const TfDiagnosticInfo &info);

// Helper function to construct patternMatchers 
std::vector<TfPatternMatcher> constructPatternFilters(
        const std::vector<std::string>& filters) {
    std::vector<TfPatternMatcher> patternMatchers;
    patternMatchers.reserve(filters.size());
    for (const std::string& filter : filters) {
        patternMatchers.push_back(TfPatternMatcher(filter, true, true));
        if (!patternMatchers.back().IsValid()) {
            TF_WARN("Invalid pattern string: %s", filter.c_str());
        }
    }
    return patternMatchers;
}

UsdUtilsConditionalAbortDiagnosticDelegateErrorFilters::
UsdUtilsConditionalAbortDiagnosticDelegateErrorFilters(
        const std::vector<std::string>& stringFilters,
        const std::vector<std::string>& codePathFilters) : 
    _stringFilters(stringFilters), _codePathFilters(codePathFilters)  {}

void 
UsdUtilsConditionalAbortDiagnosticDelegateErrorFilters::SetStringFilters(
        const std::vector<std::string>& stringFilters) 
{
    _stringFilters = stringFilters;
}

void 
UsdUtilsConditionalAbortDiagnosticDelegateErrorFilters::SetCodePathFilters(
        const std::vector<std::string>& codePathFilters)
{
    _codePathFilters = codePathFilters;
}

UsdUtilsConditionalAbortDiagnosticDelegate::
UsdUtilsConditionalAbortDiagnosticDelegate(
    const UsdUtilsConditionalAbortDiagnosticDelegateErrorFilters& 
    includeFilters, 
    const UsdUtilsConditionalAbortDiagnosticDelegateErrorFilters& 
    excludeFilters) :
    _includePatternStringFilters(constructPatternFilters(
            includeFilters.GetStringFilters())),
    _includePatternCodePathFilters(constructPatternFilters(
            includeFilters.GetCodePathFilters())),
    _excludePatternStringFilters(constructPatternFilters(
            excludeFilters.GetStringFilters())),
    _excludePatternCodePathFilters(constructPatternFilters(
            excludeFilters.GetCodePathFilters()))
{
    TfDiagnosticMgr::GetInstance().AddDelegate(this);
}

UsdUtilsConditionalAbortDiagnosticDelegate::
~UsdUtilsConditionalAbortDiagnosticDelegate()
{
    TfDiagnosticMgr::GetInstance().RemoveDelegate(this);
}

bool
UsdUtilsConditionalAbortDiagnosticDelegate::_RuleMatcher(
        const TfDiagnosticBase& err,
        const std::vector<TfPatternMatcher>& stringPatternFilters,
        const std::vector<TfPatternMatcher>& codePathPatternFilters)
{
    const std::string& sourceFileName = err.GetSourceFileName();
    if (!sourceFileName.empty()) {
        for (const TfPatternMatcher& codePathPattern : codePathPatternFilters) {
            if (codePathPattern.Match(sourceFileName)) {
                return true;
            }
        }
    }
    const std::string& errorString = err.GetCommentary();
    if (!errorString.empty()) {
        for (const TfPatternMatcher& stringPattern : stringPatternFilters) {
            if (stringPattern.Match(errorString)) {
                return true;
            }
        }
    }
    return false;
}

void
UsdUtilsConditionalAbortDiagnosticDelegate::IssueError(const TfError& err)
{
    // if matching in include rules and NOT in exclude rules then abort 
    if (_RuleMatcher(err, _includePatternStringFilters, 
                _includePatternCodePathFilters) &&
        !_RuleMatcher(err, _excludePatternStringFilters, 
            _excludePatternCodePathFilters)) {
        TfLogCrash("Aborted by UsdUtilsConditionalAbortDiagnosticDelegate On "
                "Error", err.GetCommentary(), std::string(), 
                err.GetContext(), true);
        ArchAbort(false);
    } else if (!err.GetQuiet()) {
        _PrintDiagnostic(err.GetDiagnosticCode(), err.GetContext(), 
                err.GetCommentary(), err.GetInfo<TfError>());
    }
}

void
UsdUtilsConditionalAbortDiagnosticDelegate::IssueFatalError(
    const TfCallContext &ctx, 
    const std::string &msg)
{
    TfLogCrash("FATAL ERROR", msg, std::string() /*additionalInfo*/,
               ctx, true /*logToDB*/);
    ArchAbort(/*logging=*/ false);    
}

void
UsdUtilsConditionalAbortDiagnosticDelegate::IssueStatus(const TfStatus& status)
{
    _PrintDiagnostic(status.GetDiagnosticCode(), status.GetContext(),
            status.GetCommentary(), status.GetInfo<TfStatus>());
}

void
UsdUtilsConditionalAbortDiagnosticDelegate::IssueWarning(const TfWarning& 
        warning)
{
    // if matching in include rules and NOT in exclude rules then abort 
    if (_RuleMatcher(warning, _includePatternStringFilters, 
                _includePatternCodePathFilters) &&
        !_RuleMatcher(warning, _excludePatternStringFilters, 
            _excludePatternCodePathFilters)) {
        TfLogCrash("Aborted by UsdUtilsConditionalAbortDiagnosticDelegate On "
                "Warning", warning.GetCommentary(), std::string(), 
                warning.GetContext(), true);
        ArchAbort(false);
    } else if (!warning.GetQuiet()) {
        _PrintDiagnostic(warning.GetDiagnosticCode(), warning.GetContext(),
                warning.GetCommentary(), warning.GetInfo<TfWarning>());
    }
}

void _PrintDiagnostic(const TfEnum &code, const TfCallContext &ctx,
                 const std::string& msg, const TfDiagnosticInfo &info) {
    std::fprintf(stderr, "%s",
            TfDiagnosticMgr::FormatDiagnostic(code, ctx, msg, info).c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE

