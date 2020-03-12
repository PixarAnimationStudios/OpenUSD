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
#ifndef PXR_BASE_TF_DIAGNOSTIC_MGR_H
#define PXR_BASE_TF_DIAGNOSTIC_MGR_H

/// \file tf/diagnosticMgr.h

#include "pxr/pxr.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/status.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/warning.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/api.h"

#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/functionLite.h"

#include <tbb/enumerable_thread_specific.h>
#include <tbb/spin_rw_mutex.h>
#include <tbb/atomic.h>

#include <cstdarg>
#include <list>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
    TF_LOG_STACK_TRACE_ON_ERROR,
    TF_LOG_STACK_TRACE_ON_WARNING,
    TF_ERROR_MARK_TRACKING,
    TF_PRINT_ALL_POSTED_ERRORS_TO_STDERR
    );

class TfError;
class TfErrorMark;

/// \class TfDiagnosticMgr
/// \ingroup group_tf_Diagnostic
///
/// Singleton class through which all errors and diagnostics pass.
class TfDiagnosticMgr : public TfWeakBase {
public:

    typedef TfDiagnosticMgr This;

    typedef std::list<TfError> ErrorList;

    /// Synonym for standard STL iterator to traverse the error list.
    ///
    /// The error list for a thread is an STL list.  The \c ErrorIterator type
    /// is an STL iterator and can be used without restriction in any way that
    /// it is legal to use an STL iterator.
    ///
    /// Given an iterator, one accesses the error in the standard STL fashion:
    /// \code
    ///     TfErrorMark m;
    ///
    ///     ... ;
    ///     if (!m.IsClean()) {
    ///         TfErrorMark::Iterator i;
    ///         for (i = m.GetBegin(); i != m.GetEnd(); ++i) {
    ///            cout << "file = " << i->GetSourceFileName()
    ///                 << "line = " << i->GetSourceLineNumber() << "\n";
    ///         }
    /// \endcode
    typedef ErrorList::iterator ErrorIterator;

    /// Returns the name of the given diagnostic code.
    TF_API
    static std::string GetCodeName(const TfEnum &code);

    /// Return a human-readable diagnostic message. The TfDiagnosticMgr uses 
    /// this function to print diagnostics when no diagnostic delegates are 
    /// installed. Diagnostic delegate implementations can call this to produce 
    /// messages in the same format, if desired.
    TF_API
    static std::string FormatDiagnostic(const TfEnum &code, 
            const TfCallContext &context, const std::string &msg, 
            const TfDiagnosticInfo &info);

    /// \class Delegate
    /// One may set a delegate with the \c TfDiagnosticMgr which will be
    /// called to respond to errors and diagnostics.
    ///
    /// \note None of the methods in \c TfDiagnosticMgr::Delegate can be
    /// reentrant.
    ///
    /// Practically speaking, this means they cannot invoke:
    ///
    /// - TF_ERROR
    /// - TF_RUNTIME_ERROR
    /// - TF_CODING_ERROR
    /// - TF_WARN
    /// - TF_STATUS
    ///
    /// For a more complete list, see diagnostic.h 
    ///
    class Delegate {
      public:
        TF_API 
        virtual ~Delegate() = 0;

        /// Called when a \c TfError is posted.
        virtual void IssueError(TfError const &err) = 0;

        /// Called when a \c TF_FATAL_ERROR is issued (or a failed
        /// \c TF_AXIOM).
        virtual void IssueFatalError(TfCallContext const &context,
                                     std::string const &msg) = 0;

        /// Called when a \c TF_STATUS() is issued.
        virtual void IssueStatus(TfStatus const &status) = 0;

        /// Called when a \c TF_WARNING() is issued.
        virtual void IssueWarning(TfWarning const &warning) = 0;

    protected:
        /// Abort the program, but avoid the session logging mechanism. This
        /// is intended to be used for fatal error cases where any information
        /// has already been logged.
        TF_API
        void _UnhandledAbort() const;
    };

    /// Return the singleton instance.
    TF_API static This &GetInstance() {
        return TfSingleton<This>::GetInstance();
    }

    /// Add the delegate \p delegate to the list of current delegates.
    ///
    /// This will add the delegate even if it already exists in the list.
    ///
    /// Each delegate will be called when diagnostics and errors are invoked
    ///
    /// This function is thread safe.
    TF_API
    void AddDelegate(Delegate* delegate);

    /// Removes all delegates equal to \p delegate from the current delegates.
    ///
    /// This function is thread safe.
    TF_API
    void RemoveDelegate(Delegate* delegate);

    /// Set whether errors, warnings and status messages should be printed out
    /// to the terminal.
    TF_API
    void SetQuiet(bool quiet) { _quiet = quiet; }
    
    /// Return an iterator to the beginning of this thread's error list.
    ErrorIterator GetErrorBegin() { return _errorList.local().begin(); }

    /// Return an iterator to the end of this thread's error list.
    ErrorIterator GetErrorEnd() { return _errorList.local().end(); }

    /// Remove error specified by iterator \p i.
    /// \deprecated Use TfErrorMark instead.
    TF_API
    ErrorIterator EraseError(ErrorIterator i);

    /// Remove all the errors in [first, last) from this thread's error
    /// stream. This should generally not be invoked directly. Use TfErrorMark
    /// instead.
    TF_API
    ErrorIterator EraseRange(ErrorIterator first, ErrorIterator last);

    /// Append an error to the list of active errors.  This is generally not
    /// meant to be called by user code.  It is public so that the system
    /// which translates tf errors to and from python exceptions can manage
    /// errors.
    TF_API
    void AppendError(TfError const &e);
    
    /// This method will create a TfError, append it to the error list, and
    /// pass it to all delegates.
    ///
    /// If no delegates have been registered and no error mark is active, this
    /// method will print the error to stderr.
    TF_API
    void PostError(TfEnum errorCode, const char* errorCodeString,
        TfCallContext const &context,  
        const std::string& commentary, TfDiagnosticInfo info,
        bool quiet);
    
    /// This method will create a TfError, append it to the error list, and
    /// pass it to all delegates.
    ///
    /// If no delegates have been registered and no error mark is active, this
    /// method will print the error to stderr.
    TF_API
    void PostError(const TfDiagnosticBase& diagnostic);

    /// This method will create a TfWarning and pass it to all delegates.
    ///
    /// If no delegates have been registered, this method will print the
    /// warning msg to stderr.
    TF_API
    void PostWarning(TfEnum warningCode, const char *warningCodeString,
        TfCallContext const &context, std::string const &commentary,
        TfDiagnosticInfo info, bool quiet) const;

    /// This method will create a TfWarning and pass it to all delegates.
    ///
    /// If no delegates have been registered, this method will print the
    /// warning msg to stderr.
    TF_API
    void PostWarning(const TfDiagnosticBase& diagnostic) const;

    /// This method will create a TfStatus and pass it to all delegates.
    ///
    /// If no delegates have been registered, this method will print the
    /// status msg to stderr.
    TF_API
    void PostStatus(TfEnum statusCode, const char *statusCodeString,
        TfCallContext const &context, std::string const &commentary,
        TfDiagnosticInfo info, bool quiet) const;

    /// This method will create a TfStatus and pass it to all delegates.
    ///
    /// If no delegates have been registered, this method will print the
    /// status msg to stderr.
    TF_API
    void PostStatus(const TfDiagnosticBase& diagnostic) const;

    /// This method will issue a fatal error to all delegates.
    ///
    /// If no delegates have been registered, this method will print the error
    /// msg and abort the process.
    TF_API
    void PostFatal(TfCallContext const &context, TfEnum statusCode,
                   std::string const &msg) const;

    /// Return true if an instance of TfErrorMark exists in the current thread
    /// of execution, false otherwise.
    bool HasActiveErrorMark() { return _errorMarkCounts.local() > 0; }

#if !defined(doxygen)
    //
    // Public, but *only* meant to be used by the TF_ERROR() macro.
    //
    /// \private
    class ErrorHelper {
      public:
        ErrorHelper(TfCallContext const &context, TfEnum errorCode,
                    const char* errorCodeString)
            : _context(context), _errorCode(errorCode),
              _errorCodeString(errorCodeString)
        {
        }

        TF_API
        void Post(const char* fmt, ...) const
            ARCH_PRINTF_FUNCTION(2,3);

        TF_API
        void PostQuietly(const char* fmt, ...) const
            ARCH_PRINTF_FUNCTION(2,3);

        TF_API
        void Post(const std::string& msg) const;

        TF_API
        void PostWithInfo(
                const std::string& msg,
                TfDiagnosticInfo info = TfDiagnosticInfo()) const;

        TF_API
        void PostQuietly(const std::string& msg,
                         TfDiagnosticInfo info = TfDiagnosticInfo()) const;

      private:
        TfCallContext _context;
        TfEnum _errorCode;
        const char *_errorCodeString;
    };

    struct WarningHelper {
        WarningHelper(TfCallContext const &context, TfEnum warningCode,
                      const char *warningCodeString)
            : _context(context), _warningCode(warningCode),
              _warningCodeString(warningCodeString)
        {
        }

        TF_API
        void Post(const char* fmt, ...) const
            ARCH_PRINTF_FUNCTION(2,3);

        TF_API
        void PostQuietly(const char* fmt, ...) const
            ARCH_PRINTF_FUNCTION(2,3);

        TF_API
        void Post(const std::string &str) const;

        TF_API
        void PostWithInfo(
                const std::string& msg,
                TfDiagnosticInfo info = TfDiagnosticInfo()) const;

        TF_API
        void PostQuietly(const std::string& msg) const;

      private:
        TfCallContext _context;
        TfEnum _warningCode;
        const char *_warningCodeString;
    };

    struct StatusHelper {
        StatusHelper(TfCallContext const &context, TfEnum statusCode,
                     const char *statusCodeString)
            : _context(context), _statusCode(statusCode),
              _statusCodeString(statusCodeString)
        {
        }

        TF_API
        void Post(const char* fmt, ...) const
            ARCH_PRINTF_FUNCTION(2,3);

        TF_API
        void PostQuietly(const char* fmt, ...) const
            ARCH_PRINTF_FUNCTION(2,3);

        TF_API
        void Post(const std::string &str) const;

        TF_API
        void PostWithInfo(
                const std::string& msg,
                TfDiagnosticInfo info = TfDiagnosticInfo()) const;

        TF_API
        void PostQuietly(const std::string& msg) const;

      private:
        TfCallContext _context;
        TfEnum _statusCode;
        const char *_statusCodeString;
    };

    struct FatalHelper {
        FatalHelper(TfCallContext const &context, TfEnum statusCode)
            : _context(context),
              _statusCode(statusCode)
        {
        }
        
        void Post(const std::string &str) const {
            This::GetInstance().PostFatal(_context, _statusCode, str);
        }
      private:
        TfCallContext _context;
        TfEnum _statusCode;
    };
        
#endif
    
private:

    TfDiagnosticMgr();
    virtual ~TfDiagnosticMgr();
    friend class TfSingleton<This>;
    
    // Return an iterator to the first error with serial number >= mark, or the
    // past-the-end iterator, if no such errors exist.
    TF_API
    ErrorIterator _GetErrorMarkBegin(size_t mark, size_t *nErrors);

    // Invoked by ErrorMark ctor.
    inline void _CreateErrorMark() { ++_errorMarkCounts.local(); }

    // Invoked by ErrorMark dtor.
    inline bool _DestroyErrorMark() { return --_errorMarkCounts.local() == 0; }

    // Report an error, either via delegate or print to stderr, and issue a
    // notice if this thread of execution is the main thread.
    void _ReportError(const TfError &err);

    // Splice the errors in src into this thread's local list.  Also reassign
    // serial numbers to all the spliced errors to ensure they work correctly
    // with local error marks.
    void _SpliceErrors(ErrorList &src);

    // Helper to append pending error messages to the crash log.
    void _AppendErrorsToLogText(ErrorIterator i);

    // Helper to fully rebuild the crash log error text when errors are erased
    // from the middle.
    void _RebuildErrorLogText();

    // Helper to actually publish log text into the Arch crash handler.
    void _SetLogInfoForErrors(std::vector<std::string> const &logText) const;

    // A guard used to protect reentrency when adding/removing
    // delegates as well as posting errors/warnings/statuses
    mutable tbb::enumerable_thread_specific<bool> _reentrantGuard;

    // The registered delegates.
    std::vector<Delegate*> _delegates;

    mutable tbb::spin_rw_mutex _delegatesMutex;

    // Global serial number for sorting.
    tbb::atomic<size_t> _nextSerial;

    // Thread-specific error list.
    tbb::enumerable_thread_specific<ErrorList> _errorList;

    // Thread-specific diagnostic log text for pending errors.
    struct _LogText {
        void AppendAndPublish(ErrorIterator i, ErrorIterator end);
        void RebuildAndPublish(ErrorIterator i, ErrorIterator end);
        
        std::pair<std::vector<std::string>,
                  std::vector<std::string>> texts;
        bool parity = false;
    private:
        void _AppendAndPublishImpl(bool clear,
                                   ErrorIterator i, ErrorIterator end);
    };
    tbb::enumerable_thread_specific<_LogText> _logText;

    // Thread-specific error mark counts.  Use a native key for best performance
    // here.
    tbb::enumerable_thread_specific<
        size_t, tbb::cache_aligned_allocator<size_t>,
        tbb::ets_key_per_instance> _errorMarkCounts;

    bool _quiet;

    friend class TfError;
    friend class TfErrorTransport;
    friend class TfErrorMark;
};

TF_API_TEMPLATE_CLASS(TfSingleton<TfDiagnosticMgr>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_DIAGNOSTIC_MGR_H
