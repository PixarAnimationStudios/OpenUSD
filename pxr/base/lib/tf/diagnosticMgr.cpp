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
#include "pxr/base/tf/diagnosticMgr.h"

#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/tf/stringUtils.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyExceptionState.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/function.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/threads.h"

#include <boost/utility.hpp>

#include <signal.h>
#include <stdlib.h>

#include <thread>
#include <memory>

using std::list;
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

namespace {
// Helper RAII struct for ensuring we protect functions
// that we wish to not have reentrant behaviors from delegates
// that we call out to.
struct _ReentrancyGuard {
    public:
        _ReentrancyGuard(bool* reentrancyGuardValue) :
            _reentrancyGuardValue(reentrancyGuardValue),
            _scopeWasReentered(false)
        {
            if (!*_reentrancyGuardValue) {
                *_reentrancyGuardValue = true;
            } else {
                _scopeWasReentered = true; 
            }
        }

        bool ScopeWasReentered() {
            return _scopeWasReentered;
        }
        
        ~_ReentrancyGuard() {
            if (!_scopeWasReentered) {
                *_reentrancyGuardValue = false;
            }
        } 

    private:
        bool* _reentrancyGuardValue;
        bool _scopeWasReentered;
};
} // end anonymous namespace


// Helper function for printing a diagnostic message. This is used in non-main
// threads and when a delegate is not available.
//
// If \p info contains a TfPyExceptionState, that will be printed too.
//
static void
_PrintDiagnostic(FILE *fout, const TfEnum &code, const TfCallContext &context,
                 const std::string& msg, const TfDiagnosticInfo &info);

static std::string
_FormatDiagnostic(const TfEnum &code, const TfCallContext &context,
                  const std::string &msg, const TfDiagnosticInfo &info);

static std::string
_FormatDiagnostic(const TfDiagnosticBase &d, const TfDiagnosticInfo &info);


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        TF_LOG_STACK_TRACE_ON_ERROR,
        "issue stack traces for all errors");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        TF_ERROR_MARK_TRACKING,
        "capture stack traces at TfErrorMark ctor/dtor, enable "
        "TfReportActiveMarks debugging API.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(TF_PRINT_ALL_POSTED_ERRORS_TO_STDERR,
        "print all posted errors immediately, meaning that even errors that "
        "are expected and handled will be printed, producing possibly "
        "confusing output");
}


// Abort without logging.  This is meant for use by things like TF_FATAL_ERROR,
// which already log (more extensive) session information before doing the
// abort.
static
void
Tf_UnhandledAbort()
{
    constexpr bool logging = true;
    ArchAbort(!logging);
}

TF_INSTANTIATE_SINGLETON(TfDiagnosticMgr);


TfDiagnosticMgr::Delegate::~Delegate() {}

void
TfDiagnosticMgr::Delegate::_UnhandledAbort() const
{
    Tf_UnhandledAbort();
}

TfDiagnosticMgr::TfDiagnosticMgr() :
    _errorMarkCounts(static_cast<size_t>(0)),
    _quiet(false)
{
    _nextSerial = 0;
    TfSingleton<This>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<TfDiagnosticMgr>();
}

TfDiagnosticMgr::~TfDiagnosticMgr()
{
}

void
TfDiagnosticMgr::AddDelegate(Delegate* delegate)
{
    if (delegate == nullptr) { 
        return; 
    } 

    tbb::spin_rw_mutex::scoped_lock lock(_delegatesMutex, /*writer=*/true);
    _delegates.push_back(delegate);
}

void
TfDiagnosticMgr::RemoveDelegate(Delegate* delegate)
{
    if (delegate == nullptr) {
        return;
    }

    tbb::spin_rw_mutex::scoped_lock lock(_delegatesMutex, /*writer=*/true);
    _delegates.erase(std::remove(_delegates.begin(),
                                 _delegates.end(),
                                 delegate),
                     _delegates.end());
}

void
TfDiagnosticMgr::AppendError(TfError const &e) {
    if (!HasActiveErrorMark()) {
        _ReportError(e);
    } else {
        ErrorList &errorList = _errorList.local();
        errorList.push_back(e);
        errorList.back()._serial = _nextSerial.fetch_and_increment();
        _AppendErrorsToLogText(std::prev(errorList.end())); 
    }
}

void
TfDiagnosticMgr::_SpliceErrors(ErrorList &src)
{
    if (!HasActiveErrorMark()) {
        for (ErrorList::const_iterator
                 i = src.begin(), end = src.end(); i != end; ++i) {
            _ReportError(*i);
        }
    } else {
        // Reassign new serial numbers to the errors.
        size_t serial = _nextSerial.fetch_and_add(src.size());
        for (auto& error : src) {
            error._serial = serial++;
        }
        // Now splice them into the main list.
        ErrorList &errorList = _errorList.local();
        // We store the begin iterator from the new list.  This iterator remains
        // valid *after the splice*, and iterates the spliced elements from src
        // in errorList.
        ErrorList::iterator newErrorsBegin = src.begin();
        errorList.splice(errorList.end(), src);
        _AppendErrorsToLogText(newErrorsBegin);
    }
}

void
TfDiagnosticMgr::PostError(TfEnum errorCode, const char* errorCodeString,
                           TfCallContext const &context,
                           const string& commentary,
                           TfDiagnosticInfo info, bool quiet)
{
    if (TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_ERROR))
        ArchDebuggerTrap();

    const bool logStackTraceOnError =
        TfDebug::IsEnabled(TF_LOG_STACK_TRACE_ON_ERROR);

    if (logStackTraceOnError ||
        TfDebug::IsEnabled(TF_PRINT_ALL_POSTED_ERRORS_TO_STDERR)) {
    
        _PrintDiagnostic(stderr, errorCode, context, commentary, info);
    }

    if (logStackTraceOnError) {
        TfLogStackTrace("ERROR", /* logToDb */ false);
    }

    quiet |= _quiet;

    TfError err(errorCode, errorCodeString, context, commentary, info, quiet);
    AppendError(err);
}

void
TfDiagnosticMgr::PostError(const TfDiagnosticBase& diagnostic)
{
    PostError(diagnostic.GetDiagnosticCode(),
              diagnostic.GetDiagnosticCodeAsString().c_str(),
              diagnostic.GetContext(), diagnostic.GetCommentary(),
              diagnostic._info, diagnostic.GetQuiet());
}

void
TfDiagnosticMgr::_ReportError(const TfError &err)
{
    _ReentrancyGuard guard(&_reentrantGuard.local());
    if (guard.ScopeWasReentered()) {
        return;
    }

    bool dispatchedToDelegate = false;
    {
        tbb::spin_rw_mutex::scoped_lock lock(_delegatesMutex, /*writer=*/false);
        for (auto const& delegate : _delegates) {
            if (delegate) {
                delegate->IssueError(err);
            }
        }
        dispatchedToDelegate = !_delegates.empty();
    }
    
    if (!dispatchedToDelegate && !err.GetQuiet()) {
        _PrintDiagnostic(stderr,
                         err.GetDiagnosticCode(),
                         err.GetContext(),
                         err.GetCommentary(),
                         err._info);
    }
}

void
TfDiagnosticMgr::PostWarning(
    TfEnum warningCode, const char *warningCodeString,
    TfCallContext const &context, std::string const &commentary,
    TfDiagnosticInfo info, bool quiet) const
{
    _ReentrancyGuard guard(&_reentrantGuard.local());
    if (guard.ScopeWasReentered()) {
        return;
    }

    if (TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_WARNING))
        ArchDebuggerTrap();

    quiet |= _quiet;

    TfWarning warning(warningCode, warningCodeString, context, commentary, info,
                      quiet);

    bool dispatchedToDelegate = false;
    {
        tbb::spin_rw_mutex::scoped_lock lock(_delegatesMutex, /*writer=*/false);
        for (auto const& delegate : _delegates) {
            if (delegate) {
                delegate->IssueWarning(warning);
            }
        }
        dispatchedToDelegate = !_delegates.empty();
    }
    
    if (!dispatchedToDelegate && !quiet) {
        _PrintDiagnostic(stderr, warningCode, context, commentary, info);
    }
}

void
TfDiagnosticMgr::PostWarning(const TfDiagnosticBase& diagnostic) const
{
    PostWarning(diagnostic.GetDiagnosticCode(),
                diagnostic.GetDiagnosticCodeAsString().c_str(),
                diagnostic.GetContext(), diagnostic.GetCommentary(),
                diagnostic._info, diagnostic.GetQuiet());
}

void TfDiagnosticMgr::PostStatus(
    TfEnum statusCode, const char *statusCodeString,
    TfCallContext const &context, std::string const &commentary,
    TfDiagnosticInfo info, bool quiet) const
{
    _ReentrancyGuard guard(&_reentrantGuard.local());
    if (guard.ScopeWasReentered()) {
        return;
    }

    quiet |= _quiet;

    TfStatus status(statusCode, statusCodeString, context, commentary, info,
                    quiet);

    bool dispatchedToDelegate = false;
    {
        tbb::spin_rw_mutex::scoped_lock lock(_delegatesMutex, /*writer=*/false);
        for (auto const& delegate : _delegates) {
            if (delegate) {
                delegate->IssueStatus(status);
            }
        }
        dispatchedToDelegate = !_delegates.empty();
    }

    if (!dispatchedToDelegate && !quiet) {
        _PrintDiagnostic(stderr, statusCode, context, commentary, info);
    }
}

void
TfDiagnosticMgr::PostStatus(const TfDiagnosticBase& diagnostic) const
{
    PostStatus(diagnostic.GetDiagnosticCode(),
               diagnostic.GetDiagnosticCodeAsString().c_str(),
               diagnostic.GetContext(), diagnostic.GetCommentary(),
               diagnostic._info, diagnostic.GetQuiet());
}

void TfDiagnosticMgr::PostFatal(TfCallContext const &context,
                                TfEnum statusCode,
                                std::string const &msg) const
{
    _ReentrancyGuard guard(&_reentrantGuard.local());
    if (guard.ScopeWasReentered()) {
        return;
    }

    if (TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_ERROR) ||
        TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_FATAL_ERROR))
        ArchDebuggerTrap();

    bool dispatchedToDelegate = false;
    {
        tbb::spin_rw_mutex::scoped_lock lock(_delegatesMutex, /*writer=*/false);
        for (auto const& delegate : _delegates) {
            if (delegate) {
                delegate->IssueFatalError(context, msg);
            }
        }
        dispatchedToDelegate = !_delegates.empty();
    }
    
    if (!dispatchedToDelegate) {
        if (statusCode == TF_DIAGNOSTIC_CODING_ERROR_TYPE) {
            fprintf(stderr, "Fatal coding error: %s [%s], in %s(), %s:%zu\n",
                    msg.c_str(), ArchGetProgramNameForErrors(),
                    context.GetFunction(), context.GetFile(), context.GetLine());
        }
        else if (statusCode == TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE) {
            fprintf(stderr, "Fatal error: %s [%s].\n",
                    msg.c_str(), ArchGetProgramNameForErrors());
            exit(1);
        }
        else {
            // Report and log information about the fatal error
            TfLogCrash("FATAL ERROR", msg, std::string() /*additionalInfo*/,
                       context, true /*logToDB*/);
        }

        // Abort, but avoid the signal handler, since we've already logged the
        // session info in TfLogStackTrace.
        Tf_UnhandledAbort();
    }
}

TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::EraseError(ErrorIterator i)
{
    ErrorList &errorList = _errorList.local();

    return i == errorList.end() ? i : errorList.erase(i);
}

TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::_GetErrorMarkBegin(size_t mark, size_t *nErrors)
{
    ErrorList &errorList = _errorList.local();

    if (mark >= _nextSerial || errorList.empty()) {
        if (nErrors)
            *nErrors = 0;
        return errorList.end();
    }

    // Search backward to find the the error with the smallest serial number
    // that's greater than or equal to mark.
    size_t count = 0;

    ErrorList::reverse_iterator i = errorList.rbegin(), end = errorList.rend();
    while (i != end && i->_serial >= mark) {
        ++i, ++count;
    }

    if (nErrors)
        *nErrors = count;
    return i.base();
}

TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::EraseRange(ErrorIterator first, ErrorIterator last)
{
    if (first == last)
        return last;

    ErrorIterator result = _errorList.local().erase(first, last);
    _RebuildErrorLogText();
    return result;
}

void
TfDiagnosticMgr::ErrorHelper::PostWithInfo(
    const string& msg, TfDiagnosticInfo info) const
{
    TfDiagnosticMgr::GetInstance().PostError(_errorCode, _errorCodeString,
        _context, msg, info, false);
}

void
TfDiagnosticMgr::ErrorHelper::Post(const string& msg) const
{
    TfDiagnosticMgr::GetInstance().PostError(_errorCode, _errorCodeString,
        _context, msg, TfDiagnosticInfo(), false);
}

void
TfDiagnosticMgr::ErrorHelper::PostQuietly(
    const string& msg, TfDiagnosticInfo info) const
{
    TfDiagnosticMgr::GetInstance().PostError(_errorCode, _errorCodeString,
        _context, msg, info, true);
}


void
TfDiagnosticMgr::ErrorHelper::Post(const char* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
TfDiagnosticMgr::ErrorHelper::PostQuietly(const char* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    PostQuietly(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
TfDiagnosticMgr::WarningHelper::Post(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
TfDiagnosticMgr::WarningHelper::Post(const string& msg) const
{
    TfDiagnosticMgr::GetInstance().PostWarning(_warningCode, _warningCodeString,
        _context, msg, TfDiagnosticInfo(), false);
}

void
TfDiagnosticMgr::WarningHelper::PostWithInfo(
    const string& msg, TfDiagnosticInfo info) const
{
    TfDiagnosticMgr::GetInstance().PostWarning(_warningCode, _warningCodeString,
        _context, msg, info, false);
}

void
TfDiagnosticMgr::StatusHelper::Post(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
TfDiagnosticMgr::StatusHelper::Post(const string& msg) const
{
    TfDiagnosticMgr::GetInstance().PostStatus(_statusCode, _statusCodeString,
        _context, msg, TfDiagnosticInfo(), false);
}

void
TfDiagnosticMgr::StatusHelper::PostWithInfo(
    const string& msg, TfDiagnosticInfo info) const
{
    TfDiagnosticMgr::GetInstance().PostStatus(_statusCode, _statusCodeString,
        _context, msg, info, false);
}

/* statuc */
std::string
TfDiagnosticMgr::GetCodeName(const TfEnum &code)
{
    string codeName = TfEnum::GetDisplayName(code);
    if (codeName.empty()) {
        codeName = TfStringPrintf("(%s)%d",
            ArchGetDemangled(code.GetType()).c_str(),
            code.GetValueAsInt());
    }
    return codeName;
}

void
TfDiagnosticMgr::_SetLogInfoForErrors(
    std::vector<std::string> const &logText) const
{
    ArchSetExtraLogInfoForErrors(
        TfStringPrintf("Thread %s Pending Diagnostics", 
            TfStringify(std::this_thread::get_id()).c_str()),
        logText.empty() ? nullptr : &logText);
}

void
TfDiagnosticMgr::_LogText::AppendAndPublish(
    ErrorIterator begin, ErrorIterator end)
{
    return _AppendAndPublishImpl(/*clear=*/false, begin, end);
}

void
TfDiagnosticMgr::_LogText::RebuildAndPublish(
    ErrorIterator begin, ErrorIterator end)
{
    return _AppendAndPublishImpl(/*clear=*/true, begin, end);
}

void
TfDiagnosticMgr::_LogText::_AppendAndPublishImpl(
    bool clear, ErrorIterator begin, ErrorIterator end)
{
    // The requirement at the Arch level for ArchSetExtraLogInfoForErrors is
    // that the pointer we hand it must remain valid, and we can't mutate the
    // structure it points to since if another thread crashes, Arch will read it
    // to generate the crash report.  So instead we maintain two copies.  We
    // update one copy to the new text anpd then publish it to Arch, effectively
    // swapping out the old copy.  Then we update the old copy to match.  Next
    // time through we do it again but with the data structures swapped, which
    // is tracked by 'parity'.
    auto *first = &texts.first;
    auto *second = &texts.second;
    if (parity)
        std::swap(first, second);

    // Update first.
    if (clear)
        first->clear();
    for (ErrorIterator i = begin; i != end; ++i) {
        first->push_back(_FormatDiagnostic(*i, i->_info));
    }

    // Publish.
    ArchSetExtraLogInfoForErrors(
        TfStringPrintf("Thread %s Pending Diagnostics", 
                       TfStringify(std::this_thread::get_id()).c_str()),
        first->empty() ? nullptr : first);

    // Update second to match, arch is no longer looking at it.
    if (clear)
        second->clear();
    for (ErrorIterator i = begin; i != end; ++i) {
        second->push_back(_FormatDiagnostic(*i, i->_info));
    }

    // Switch parity.
    parity = !parity;
}

void
TfDiagnosticMgr::_AppendErrorsToLogText(ErrorIterator i)
{
    _logText.local().AppendAndPublish(i, GetErrorEnd());
}

void
TfDiagnosticMgr::_RebuildErrorLogText()
{
    _logText.local().RebuildAndPublish(GetErrorBegin(), GetErrorEnd());
}

static std::string
_FormatDiagnostic(const TfEnum &code, const TfCallContext &context,
                  const std::string &msg, const TfDiagnosticInfo &info)
{
    string output;
    string codeName = TfDiagnosticMgr::GetCodeName(code);
    if (context.IsHidden() ||
        !strcmp(context.GetFunction(), "") || !strcmp(context.GetFile(), "")) {
        output = TfStringPrintf("%s%s: %s [%s]\n",
                                codeName.c_str(),
                                ArchIsMainThread() ? "" : " (secondary thread)",
                                msg.c_str(),
                                ArchGetProgramNameForErrors());
    }
    else {
        output = TfStringPrintf("%s%s: in %s at line %zu of %s -- %s\n",
                                codeName.c_str(),
                                ArchIsMainThread() ? "" : " (secondary thread)",
                                context.GetFunction(),
                                context.GetLine(),
                                context.GetFile(),
                                msg.c_str());
    }

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    if (const TfPyExceptionState* exc =
            boost::any_cast<TfPyExceptionState>(&info)) {
        output += TfStringPrintf("%s\n", exc->GetExceptionString().c_str());
    }
#endif // PXR_PYTHON_SUPPORT_ENABLED

    return output;
}

static std::string
_FormatDiagnostic(const TfDiagnosticBase &d, const TfDiagnosticInfo &info) {
    return _FormatDiagnostic(d.GetDiagnosticCode(), d.GetContext(),
                             d.GetCommentary(), info);
}

static void
_PrintDiagnostic(FILE *fout, const TfEnum &code, const TfCallContext &context,
    const std::string& msg, const TfDiagnosticInfo &info)
{
    fprintf(fout, "%s", _FormatDiagnostic(code, context, msg, info).c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE
