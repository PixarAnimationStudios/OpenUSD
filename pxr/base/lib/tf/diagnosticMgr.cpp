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
#include "pxr/base/tf/diagnosticMgr.h"

#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/diagnosticNotice.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/pyExceptionState.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/function.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/threads.h"

#include <boost/foreach.hpp>
#include <boost/utility.hpp>

#include <signal.h>
#include <stdlib.h>

#include <thread>

using std::list;
using std::string;

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
}


// Abort, but first remove Tf's signal handler so we don't invoke the session
// logging mechanism.  This is meant for use by things like TF_FATAL_ERROR,
// which already log (more extensive) session information before doing the
// abort.
static
void
Tf_UnhandledAbort()
{
    // Remove signal handler.
    struct sigaction act;
    act.sa_handler = SIG_DFL;
    act.sa_flags   = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGABRT, &act, NULL);
    abort();
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
TfDiagnosticMgr::SetDelegate( DelegateWeakPtr const &delegate )
{
    bool hadDelegate = _delegate;

    _delegate = delegate;

    if(hadDelegate) {
        TF_WARN("Overwriting existing TfError delegate. This will not be "
            "allowed in the future.");
    }
}

void
TfDiagnosticMgr::AppendError(TfError const &e) {
    if (not HasActiveErrorMark()) {
        _ReportError(e);
    } else {
        e._data->_serial = _nextSerial.fetch_and_increment();
        ErrorList &errorList = _errorList.local();
        errorList.push_back(e);
        _AppendErrorsToLogText(boost::prior(errorList.end())); 
    }
}

void
TfDiagnosticMgr::_SpliceErrors(ErrorList &src)
{
    if (not HasActiveErrorMark()) {
        for (ErrorList::const_iterator
                 i = src.begin(), end = src.end(); i != end; ++i) {
            _ReportError(*i);
        }
    } else {
        // Reassign new serial numbers to the errors.
        size_t serial = _nextSerial.fetch_and_add(src.size());
        BOOST_FOREACH(TfError &error, src) {
            error._data->_serial = serial++;
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

    if (TfDebug::IsEnabled(TF_LOG_STACK_TRACE_ON_ERROR)) {
        _PrintDiagnostic(stderr, errorCode, context, commentary, info);
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
              diagnostic._data->_info, diagnostic.GetQuiet());
}

void
TfDiagnosticMgr::_ReportError(const TfError &err)
{
    const bool isMainThread = ArchIsMainThread();

    if (isMainThread and _delegate) {
        _delegate->IssueError(err);
    } else if (not err.GetQuiet()) {
        _PrintDiagnostic(stderr,
                         err.GetDiagnosticCode(),
                         err.GetContext(),
                         err.GetCommentary(),
                         err._data->_info);
    }

    if (isMainThread)
        TfDiagnosticNotice::IssuedError(err).Send(TfCreateWeakPtr(this));
}

void
TfDiagnosticMgr::PostWarning(
    TfEnum warningCode, const char *warningCodeString,
    TfCallContext const &context, std::string const &commentary,
    TfDiagnosticInfo info, bool quiet) const
{
    if (TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_WARNING))
        ArchDebuggerTrap();

    if (not ArchIsMainThread()) {
        _PrintDiagnostic(stderr, warningCode, context, commentary, info);
        return;
    }

    quiet |= _quiet;

    TfWarning warning(warningCode, warningCodeString, context, commentary, info,
                      quiet);

    if (_delegate)
        _delegate->IssueWarning(warning);
    else if (not quiet)
        _PrintDiagnostic(stderr, warningCode, context, commentary, info);

    TfDiagnosticNotice::IssuedWarning(warning).Send(TfCreateWeakPtr(this));
}

void
TfDiagnosticMgr::PostWarning(const TfDiagnosticBase& diagnostic) const
{
    PostWarning(diagnostic.GetDiagnosticCode(),
                diagnostic.GetDiagnosticCodeAsString().c_str(),
                diagnostic.GetContext(), diagnostic.GetCommentary(),
                diagnostic._data->_info, diagnostic.GetQuiet());
}

void TfDiagnosticMgr::PostStatus(
    TfEnum statusCode, const char *statusCodeString,
    TfCallContext const &context, std::string const &commentary,
    TfDiagnosticInfo info, bool quiet) const
{
    if (not ArchIsMainThread()) {
        _PrintDiagnostic(stdout, statusCode, context, commentary, info);
        return;
    }

    quiet |= _quiet;

    TfStatus status(statusCode, statusCodeString, context, commentary, info,
                    quiet);

    if (_delegate)
        _delegate->IssueStatus(status);
    else if (not quiet)
        _PrintDiagnostic(stderr, statusCode, context, commentary, info);

    TfDiagnosticNotice::IssuedStatus(status).Send(TfCreateWeakPtr(this));
}

void
TfDiagnosticMgr::PostStatus(const TfDiagnosticBase& diagnostic) const
{
    PostStatus(diagnostic.GetDiagnosticCode(),
               diagnostic.GetDiagnosticCodeAsString().c_str(),
               diagnostic.GetContext(), diagnostic.GetCommentary(),
               diagnostic._data->_info, diagnostic.GetQuiet());
}

void TfDiagnosticMgr::PostFatal(TfCallContext const &context,
                                TfEnum statusCode,
                                std::string const &msg) const
{
    if (TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_ERROR) or
        TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_FATAL_ERROR))
        ArchDebuggerTrap();

    bool isMainThread = ArchIsMainThread();

    // Send out the IssuedFatalError notice if we're in the main thread.
    if (isMainThread) {
        TfDiagnosticBase data(statusCode, "", context, msg,
                              TfDiagnosticInfo(), false /*quiet*/);
        TfDiagnosticNotice::IssuedFatalError fe(msg, context);
        fe.SetData(data);
        fe.Send(TfCreateWeakPtr(this));
    }

    if (isMainThread and _delegate) {
        _delegate->IssueFatalError(context, msg);
    } else {
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

    if (mark >= _nextSerial or errorList.empty()) {
        if (nErrors)
            *nErrors = 0;
        return errorList.end();
    }

    // Search backward to find the the error with the smallest serial number
    // that's greater than or equal to mark.
    size_t count = 0;

    ErrorList::reverse_iterator i = errorList.rbegin(), end = errorList.rend();
    while (i != end and i->_data->_serial >= mark) {
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

TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::ErrorHelper::PostWithInfo(
    const string& msg, TfDiagnosticInfo info) const
{
    TfDiagnosticMgr::GetInstance().PostError(_errorCode, _errorCodeString,
        _context, msg, info, false);
    return boost::prior(TfDiagnosticMgr::GetInstance().GetErrorEnd());
}

TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::ErrorHelper::Post(const string& msg) const
{
    TfDiagnosticMgr::GetInstance().PostError(_errorCode, _errorCodeString,
        _context, msg, TfDiagnosticInfo(), false);
    return boost::prior(TfDiagnosticMgr::GetInstance().GetErrorEnd());
}

TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::ErrorHelper::PostQuietly(
    const string& msg, TfDiagnosticInfo info) const
{
    TfDiagnosticMgr::GetInstance().PostError(_errorCode, _errorCodeString,
        _context, msg, info, true);
    return boost::prior(TfDiagnosticMgr::GetInstance().GetErrorEnd());
}


TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::ErrorHelper::Post(const char* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    Post(TfVStringPrintf(fmt, ap));
    va_end(ap);
    return boost::prior(TfDiagnosticMgr::GetInstance().GetErrorEnd());
}

TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::ErrorHelper::PostQuietly(const char* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    PostQuietly(TfVStringPrintf(fmt, ap));
    va_end(ap);
    return boost::prior(TfDiagnosticMgr::GetInstance().GetErrorEnd());
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
TfDiagnosticMgr::_SetLogInfoForErrors(const std::string &logText) const
{
    ArchSetExtraLogInfoForErrors(
        TfStringPrintf("Thread %s Pending Diagnostics", 
            TfStringify(std::this_thread::get_id()).c_str()),
        logText.empty() ? NULL : logText.c_str());
}

void
TfDiagnosticMgr
::_AppendErrorTextToString(ErrorIterator i, std::string &out)
{
    for (ErrorIterator end = GetErrorEnd(); i != end; ++i) {
        out += _FormatDiagnostic(*i, i->_data->_info);
    }
}

void
TfDiagnosticMgr::_AppendErrorsToLogText(ErrorIterator i)
{
    // Copy the string.
    boost::scoped_ptr<std::string> &oldStr = _logText.local();
    boost::scoped_ptr<std::string>
        newStr(oldStr ? new std::string(*oldStr) : new std::string());

    // Write the errors.
    _AppendErrorTextToString(i, *newStr);

    // Set the log info.
    _SetLogInfoForErrors(*newStr);

    // Delete the old string by swapping it out for the new in the TLS.
    oldStr.swap(newStr);
}

void
TfDiagnosticMgr::_RebuildErrorLogText()
{
    // Create a new string.
    boost::scoped_ptr<std::string> newStr(new std::string());

    // Write the errors.
    _AppendErrorTextToString(GetErrorBegin(), *newStr);

    // Set the log info.
    _SetLogInfoForErrors(*newStr);

    // Delete the old string by swapping it out for the new in the TLS.
    _logText.local().swap(newStr);
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

    if (const TfPyExceptionState* exc =
            boost::any_cast<TfPyExceptionState>(&info)) {
        output += TfStringPrintf("%s\n", exc->GetExceptionString().c_str());
    }

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
    if (!TfDiagnosticNotice::GetStderrOutputState())
        return;

    fprintf(fout, "%s", _FormatDiagnostic(code, context, msg, info).c_str());
}
