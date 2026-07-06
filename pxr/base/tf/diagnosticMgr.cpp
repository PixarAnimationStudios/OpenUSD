//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticMgr.h"

#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/diagnosticTransport.h"
#include "pxr/base/tf/diagnosticTrap.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorTransport.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyExceptionState.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/function.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/threads.h"

#include <signal.h>
#include <stdlib.h>

#include <any>
#include <thread>
#include <variant>
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
    _ReentrancyGuard(bool *reentrancyGuardValue) :
        _reentrancyGuardValue(reentrancyGuardValue),
        _scopeWasReentered(false) {
        _scopeWasReentered = std::exchange(*_reentrancyGuardValue, true);
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
    bool *_reentrancyGuardValue;
    bool _scopeWasReentered;
};
} // end anonymous namespace


// Helper function for printing a diagnostic message when a delegate is not
// available.
//
// If \p info contains a TfPyExceptionState, that will be printed too.
//
static void
_PrintDiagnostic(FILE *fout, const TfEnum &code, const TfCallContext &context,
                 const std::string& msg, const TfDiagnosticInfo &info);

static std::string
_FormatDiagnostic(const TfDiagnosticBase &d, const TfDiagnosticInfo &info);


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        TF_LOG_STACK_TRACE_ON_ERROR,
        "log stack traces for all errors");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        TF_LOG_STACK_TRACE_ON_WARNING,
        "log stack traces for all warnings");
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
[[noreturn]]
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

TfDiagnosticMgr::_LogTextPin::_LogTextPin(
    std::string &&key, std::unique_ptr<std::vector<std::string>> &&lines)
    : _key(std::move(key))
    , _lines(std::move(lines))
{
    ArchSetExtraLogInfoForErrors(_key, _lines.get());
}

TfDiagnosticMgr::_LogTextPin::~_LogTextPin()
{
    if (_lines) {
        ArchSetExtraLogInfoForErrors(_key, nullptr);
    }
}

TfDiagnosticMgr::_LogTextPin &
TfDiagnosticMgr::_LogTextPin::_LogTextPin::operator=(_LogTextPin &&other)
{
    if (this != &other) {
        if (_lines) {
            ArchSetExtraLogInfoForErrors(_key, nullptr);
        }
        _key   = std::move(other._key);
        _lines = std::move(other._lines);
    }
    return *this;
}

// _LogTextBuffer implementation
//
// The requirement at the Arch level for ArchSetExtraLogInfoForErrors is that
// the pointer we hand it must remain valid, and we can't mutate the structure
// it points to since if another thread crashes, Arch will read it to generate
// the crash report.  So instead we maintain two copies.  We update one copy to
// the new text and then publish it to Arch, effectively swapping out the old
// copy.  Then we copy it to the other buffer so they are in sync.  Next time
// through we do it again but with the data structures swapped, tracked by
// _parity.

TfDiagnosticMgr::_LogTextBuffer::_LogTextBuffer(std::string &&label)
    : _label(std::move(label))
{
}

TfDiagnosticMgr::_LogTextBuffer::~_LogTextBuffer()
{
    if (!_label.empty()) {
        ArchSetExtraLogInfoForErrors(_label, nullptr);
    }
}

template <class Fn>
void
TfDiagnosticMgr::_LogTextBuffer::Update(Fn &&fn)
{
    auto *first  = &_texts.first;
    auto *second = &_texts.second;

    if (std::exchange(_parity, !_parity)) {
        std::swap(first, second);
    }

    // Fill the text in the first buffer.
    std::forward<Fn>(fn)(*first);

    // Publish first, then fill the second so both are in sync.  Passing nullptr
    // in case the text is empty removes the entry for our _label.
    ArchSetExtraLogInfoForErrors(_label, first->empty() ? nullptr : first);

    // Fill the text in the second buffer to keep in-sync.  We don't copy
    // because some updates (like appending) are algorithmically cheaper than
    // copying the whole buffer.
    std::forward<Fn>(fn)(*second);

    TF_DEV_AXIOM(*first == *second);
}

std::string
TfDiagnosticMgr::_ThreadState::_MakeLabel(const char *suffix)
{
    return TfStringPrintf(
        "Thread %s %s", TfStringify(std::this_thread::get_id()).c_str(),
        suffix);
}

TfDiagnosticMgr::TfDiagnosticMgr()
    : _quiet(false)
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

    TfSpinRWMutex::ScopedLock lock(_delegatesMutex, /*writer=*/true);
    _delegates.push_back(delegate);
}

void
TfDiagnosticMgr::RemoveDelegate(Delegate* delegate)
{
    if (delegate == nullptr) {
        return;
    }

    TfSpinRWMutex::ScopedLock lock(_delegatesMutex, /*writer=*/true);
    _delegates.erase(
        std::remove(_delegates.begin(), _delegates.end(), delegate),
        _delegates.end());
}

void
TfDiagnosticMgr::AppendError(TfError const &e) {
    if (!HasActiveErrorMark()) {
        _ReportError(e);
    } else {
        ErrorList &errorList = _threadState.local()._errorList;
        errorList.push_back(e);
        errorList.back()._serial = _nextSerial.fetch_add(1);
        _AppendPendingErrorsLogText(std::prev(errorList.end())); 
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
        size_t serial = _nextSerial.fetch_add(src.size());
        for (auto& error : src) {
            error._serial = serial++;
        }
        // Now splice them into the main list.
        ErrorList &errorList = _threadState.local()._errorList;
        // We store the begin iterator from the new list.  This iterator remains
        // valid *after the splice*, and iterates the spliced elements from src
        // in errorList.
        ErrorList::iterator newErrorsBegin = src.begin();
        errorList.splice(errorList.end(), src);
        _AppendPendingErrorsLogText(newErrorsBegin);
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

template <class Fn>
bool
TfDiagnosticMgr::_ForEachDelegate(Fn const &fn) const
{
    // We intentionally hold the read lock on _delegatesMutex while invoking
    // them because this lets them synchronize their destruction and removal
    // safely.  If we dropped the lock, we could call out to a deleted delegate.
    TfSpinRWMutex::ScopedLock lock(_delegatesMutex, /*write=*/false);
    if (_delegates.empty()) {
        return false;
    }
    for (Delegate *delegate: _delegates) {
        if (delegate) {
            fn(delegate);
        }
    }
    return true;
}

void *
TfDiagnosticMgr::_PushTrap(TfDiagnosticTrap *trap)
{
    _TrapStack &stack = _markCountsAndTrapStacks.local().trapStack;
    stack.push_back(trap);
    return &stack;
}

void
TfDiagnosticMgr::_PopTrap(TfDiagnosticTrap *trap, void *key)
{
    const bool wasClean = trap->IsClean();
    auto &stack = *static_cast<_TrapStack *>(key);
    if (TF_VERIFY(!stack.empty() && stack.back() == trap)) {
        stack.pop_back();
    }

    // If there's an enclosing trap the diagnostics will be re-posted into it.
    // We have to rebuild the log text to remove the existing entries -- they'll
    // be repopulated as appropriate on re-post.  We can't just leave the log
    // text alone since if a TfErrorMark has arrived in the meantime, it could
    // snag a subset, for example.
    if (!wasClean) {
        _RebuildTrappedDiagnosticsLogText(trap->_logStart);
        if (stack.empty()) {
            TF_VERIFY(trap->_logStart == 0);
        }
    }
}

// Return the active TfDiagnosticTrap on this thread, or nullptr.
inline TfDiagnosticTrap *
TfDiagnosticMgr::_GetActiveTrap()
{
    auto &stack = _markCountsAndTrapStacks.local().trapStack;
    return stack.empty() ? nullptr : stack.back();
}

void
TfDiagnosticMgr::_ReportError(const TfError &err)
{
    _ReentrancyGuard guard(&_reentrantGuard.local());
    if (guard.ScopeWasReentered()) {
        return;
    }

    if (TfDiagnosticTrap *trap = _GetActiveTrap()) {
        trap->_container.Append(err);
        _AppendTrappedDiagnosticsLogText(err, trap);
        return;
    }

    const bool dispatchedToDelegate =
        _ForEachDelegate([&err](Delegate *delegate) {
            delegate->IssueError(err);
        });
    
    if (!dispatchedToDelegate && !err.GetQuiet()) {
        _PrintDiagnostic(
            stderr, err.GetDiagnosticCode(), err.GetContext(),
            err.GetCommentary(), err._info);
    }
}

void
TfDiagnosticMgr::PostWarning(
    TfEnum warningCode, const char *warningCodeString,
    TfCallContext const &context, std::string const &commentary,
    TfDiagnosticInfo info, bool quiet)
{
    _ReentrancyGuard guard(&_reentrantGuard.local());
    if (guard.ScopeWasReentered()) {
        return;
    }

    if (TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_WARNING)) {
        ArchDebuggerTrap();
    }

    const bool logStackTraceOnWarning =
        TfDebug::IsEnabled(TF_LOG_STACK_TRACE_ON_WARNING);

    if (logStackTraceOnWarning) {
        _PrintDiagnostic(stderr, warningCode, context, commentary, info);
        TfLogStackTrace("WARNING", /* logToDb */ false);
    }

    quiet |= _quiet;

    TfWarning warning(
        warningCode, warningCodeString, context, commentary, info, quiet);

    if (TfDiagnosticTrap *trap = _GetActiveTrap()) {
        trap->_container.Append(warning);
        _AppendTrappedDiagnosticsLogText(warning, trap);
        return;
    }

    const bool dispatchedToDelegate =
        _ForEachDelegate([&warning](Delegate *delegate) {
            delegate->IssueWarning(warning);
        });
    
    if (!logStackTraceOnWarning && !dispatchedToDelegate && !quiet) {
        _PrintDiagnostic(stderr, warningCode, context, commentary, info);
    }
}

void
TfDiagnosticMgr::PostWarning(const TfDiagnosticBase& diagnostic)
{
    PostWarning(diagnostic.GetDiagnosticCode(),
                diagnostic.GetDiagnosticCodeAsString().c_str(),
                diagnostic.GetContext(), diagnostic.GetCommentary(),
                diagnostic._info, diagnostic.GetQuiet());
}

void TfDiagnosticMgr::PostStatus(
    TfEnum statusCode, const char *statusCodeString,
    TfCallContext const &context, std::string const &commentary,
    TfDiagnosticInfo info, bool quiet)
{
    _ReentrancyGuard guard(&_reentrantGuard.local());
    if (guard.ScopeWasReentered()) {
        return;
    }

    quiet |= _quiet;

    TfStatus status(
        statusCode, statusCodeString, context, commentary, info, quiet);

    if (TfDiagnosticTrap *trap = _GetActiveTrap()) {
        trap->_container.Append(status);
        _AppendTrappedDiagnosticsLogText(status, trap);
        return;
    }

    const bool dispatchedToDelegate =
        _ForEachDelegate([&status](Delegate *delegate) {
            delegate->IssueStatus(status);
        });

    if (!dispatchedToDelegate && !quiet) {
        _PrintDiagnostic(stderr, statusCode, context, commentary, info);
    }
}

void
TfDiagnosticMgr::PostStatus(const TfDiagnosticBase& diagnostic)
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
        TfLogCrash("RECURSIVE FATAL ERROR",
                   msg, std::string() /*additionalInfo*/,
                   context, true /*logToDB*/);
    }

    if (TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_ERROR) ||
        TfDebug::IsEnabled(TF_ATTACH_DEBUGGER_ON_FATAL_ERROR)) {
        ArchDebuggerTrap();
    }

    _ForEachDelegate([&context, &msg](Delegate *delegate) {
        delegate->IssueFatalError(context, msg);
    });

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

TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::EraseError(ErrorIterator i)
{
    ErrorList &errorList = _threadState.local()._errorList;

    return i == errorList.end() ? i : errorList.erase(i);
}

TfDiagnosticMgr::ErrorIterator
TfDiagnosticMgr::_GetErrorMarkBegin(size_t mark, size_t *nErrors)
{
    ErrorList &errorList = _threadState.local()._errorList;

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

    // Capture the serial before erasing so the partial rebuild can locate
    // the dirty boundary via the existing backward scan.
    const size_t startSerial = first->_serial;
    ErrorIterator result = _threadState.local()._errorList.erase(first, last);
    _RebuildPendingErrorLogText(startSerial);
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

/* static */
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
TfDiagnosticMgr::_AppendPendingErrorsLogText(ErrorIterator i)
{
    auto &ts = _threadState.local();
    // Need to be careful -- Update() calls the passed function twice to update
    // the text double-buffer.
    ts._pendingErrorsLogText.Update([&](std::vector<std::string> &log) {
        for (ErrorIterator iter = i, end = ts._errorList.end();
             iter != end; ++iter) {
            log.push_back(_FormatDiagnostic(*iter, iter->_info));
        }
    });
}

void
TfDiagnosticMgr::_RebuildPendingErrorLogText(size_t startSerial)
{
    // Partial rebuild: keep log entries for errors with serial < startSerial
    // and re-format only those with serial >= startSerial.  startSerial = 0
    // performs a full rebuild (re-scans all errors).
    //
    // _GetErrorMarkBegin does a backward scan -- O(tail), not O(total) -- and
    // returns the iterator directly, avoiding a separate forward traversal.
    // std::list::size() is O(1) in C++11.
    auto &ts = _threadState.local();
    size_t nErrors;
    ErrorIterator markBegin = _GetErrorMarkBegin(startSerial, &nErrors);
    const size_t validLogEnd = ts._errorList.size() - nErrors;

    ts._pendingErrorsLogText.Update([&](std::vector<std::string> &log) {
        if (log.size() > validLogEnd) {
            log.resize(validLogEnd);
        }
        for (ErrorIterator i = markBegin, end = ts._errorList.end();
             i != end; ++i) {
            log.push_back(_FormatDiagnostic(*i, i->_info));
        }
    });
}

void
TfDiagnosticMgr::_AppendTrappedDiagnosticsLogText(TfDiagnosticBase const &d,
                                                  TfDiagnosticTrap *trap)
{
    _LogTextBuffer &logText = _threadState.local()._trappedDiagnosticsLogText;
    // Establish trap's _logStart if not already established.
    if (trap->_logStart == size_t(-1)) {
        trap->_logStart = logText.GetSize();
    }
    logText.Update([&](std::vector<std::string> &log) {
        log.push_back(_FormatDiagnostic(d, d._info));
    });
}

void
TfDiagnosticMgr::_RebuildTrappedDiagnosticsLogText(size_t validLogEnd)
{
    // Partial rebuild: keep entries [0, validLogEnd) intact and re-scan only
    // the portion of each trap's container that falls at or after validLogEnd.
    // validLogEnd = 0 performs a full rebuild (clear + rescan all traps).
    //
    // pos tracks where each trap's entries should start in the correct log,
    // computed from current container sizes rather than _logStart so that the
    // algorithm stays correct even if _logStart is momentarily stale.  After
    // the update, _logStart is resynced for all traps.
    auto &stack = _markCountsAndTrapStacks.local().trapStack;
    _threadState.local()._trappedDiagnosticsLogText.Update(
        [&](std::vector<std::string> &log) {
            if (log.size() > validLogEnd) {
                log.resize(validLogEnd);
            }
            size_t pos = 0;
            for (TfDiagnosticTrap const *trap : stack) {
                const size_t trapSize = trap->_container.size();
                const size_t trapEnd  = pos + trapSize;
                if (trapEnd <= log.size()) {
                    // This trap's entries are entirely within the clean
                    // prefix -- already in the log, nothing to do.
                    pos = trapEnd;
                    continue;
                }
                // Scan past any entries already in the log for this trap,
                // then format and append the remainder.
                const size_t skip =
                    (log.size() > pos) ? log.size() - pos : 0;
                auto it = trap->_container.GetIterator(skip);
                while (it.Next([&](TfDiagnosticBase const &d) {
                    log.push_back(_FormatDiagnostic(d, d._info));
                })) {}
                pos = trapEnd;
            }
        });
    // Resync each trap's _logStart to match its position in the rebuilt log.
    size_t pos = 0;
    for (TfDiagnosticTrap *trap : stack) {
        trap->_logStart = pos;
        pos += trap->_container.size();
    }
}

TfDiagnosticMgr::_LogTextPin
TfDiagnosticMgr::_PinErrorLogText(
    ErrorIterator first, ErrorIterator last) const
{
    auto lines = std::make_unique<std::vector<std::string>>();
    lines->reserve(std::distance(first, last));
    for (auto i = first; i != last; ++i) {
        lines->push_back(_FormatDiagnostic(*i, i->_info));
    }
    std::string key = _ThreadState::_MakeLabel(
        TfStringPrintf("Transient Errors %p", lines.get()).c_str());
    return _LogTextPin(std::move(key), std::move(lines));
}

TfDiagnosticMgr::_LogTextPin
TfDiagnosticMgr::_PinDiagnosticsLogText(
    Tf_DiagnosticContainer const &container) const
{
    auto lines = std::make_unique<std::vector<std::string>>();
    lines->reserve(container.size());
    auto it = container.GetIterator();
    while (it.Next([&](TfDiagnosticBase const &d) {
        lines->push_back(_FormatDiagnostic(d, d._info));
    }));
    std::string key = _ThreadState::_MakeLabel(
        TfStringPrintf("Transient Diagnostics %p", lines.get()).c_str());
    return _LogTextPin(std::move(key), std::move(lines));
}

std::string
TfDiagnosticMgr::FormatDiagnostic(const TfEnum &code, 
        const TfCallContext &context, const std::string &msg, 
        const TfDiagnosticInfo &info)
{
    string output;
    string codeName = TfDiagnosticMgr::GetCodeName(code);
    if (!context || context.IsHidden()) {
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
            std::any_cast<TfPyExceptionState>(&info)) {
        output += TfStringPrintf("%s\n", exc->GetExceptionString().c_str());
    }
#endif // PXR_PYTHON_SUPPORT_ENABLED

    return output;
}

static std::string
_FormatDiagnostic(const TfDiagnosticBase &d, const TfDiagnosticInfo &info) {
    return TfDiagnosticMgr::FormatDiagnostic(d.GetDiagnosticCode(), 
            d.GetContext(), d.GetCommentary(), info);
}

static void
_PrintDiagnostic(FILE *fout, const TfEnum &code, const TfCallContext &context,
    const std::string& msg, const TfDiagnosticInfo &info)
{
    fprintf(fout, "%s", TfDiagnosticMgr::FormatDiagnostic(code, context, msg, 
                info).c_str());
}

////////////////////////////////////////////////////////////////////////
// Test hooks -- not declared in any public header; tests declare them locally.

class Tf_DiagnosticMgrTestAccess
{
public:
    static std::vector<std::string>
    GetPendingErrorsLogText() {
        TfDiagnosticMgr &mgr = TfDiagnosticMgr::GetInstance();
        return mgr._threadState.local()._pendingErrorsLogText._texts.first;
    }
    static std::vector<std::string>
    GetTrappedDiagnosticsLogText() {
        TfDiagnosticMgr &mgr = TfDiagnosticMgr::GetInstance();
        return mgr._threadState.local()._trappedDiagnosticsLogText._texts.first;
    }
    static std::vector<std::string>
    GetTransportLogTextForTesting(TfErrorTransport const &transport) {
        if (transport._pin._lines) {
            return *transport._pin._lines;
        }
        return {};
    }
    static std::vector<std::string>
    GetTransportLogTextForTesting(TfDiagnosticTransport const &transport) {
        if (transport._pin._lines) {
            return *transport._pin._lines;
        }
        return {};
    }
};

TF_API
std::vector<std::string>
Tf_GetPendingErrorsLogTextForTesting()
{
    return Tf_DiagnosticMgrTestAccess::GetPendingErrorsLogText();
}

TF_API
std::vector<std::string>
Tf_GetTrappedDiagnosticsLogTextForTesting()
{
    return Tf_DiagnosticMgrTestAccess::GetTrappedDiagnosticsLogText();
}

TF_API
std::vector<std::string>
Tf_GetTransportLogTextForTesting(TfErrorTransport const &transport)
{
    return Tf_DiagnosticMgrTestAccess::GetTransportLogTextForTesting(transport);
}

TF_API
std::vector<std::string>
Tf_GetTransportLogTextForTesting(TfDiagnosticTransport const &transport)
{
    return Tf_DiagnosticMgrTestAccess::GetTransportLogTextForTesting(transport);
}

PXR_NAMESPACE_CLOSE_SCOPE
