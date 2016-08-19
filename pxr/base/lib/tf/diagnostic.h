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
#ifndef TF_DIAGNOSTIC_H
#define TF_DIAGNOSTIC_H

/// \file tf/diagnostic.h
/// \ingroup group_tf_Diagnostic
/// Low-level utilities for informing users of various internal and external
/// diagnostic conditions.
///
/// lib/tf supports a range of error-reporting routines.
///
/// For a more detailed explanation of when each of the facilities described
/// in this file is appropriate, (and more importantly, when they're not!)
/// see \ref page_tf_Diagnostic.

#include "pxr/base/arch/function.h"
#include "pxr/base/tf/diagnosticLite.h"

#if defined(__cplusplus) || defined (doxygen)

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnosticHelper.h"

#include <stddef.h>
#include <stdarg.h>
#include <string>

// Note: diagnosticLite.h defines the various macros, but we'll document
// them here.  The following block is only for doxygen, not seen by a real
// compile.  To see the actual macro definition, look in diagnosticLite.h.

#if defined(doxygen)

/// \addtogroup group_tf_Diagnostic
///@{

/// Issue an internal programming error, but continue execution.
///
/// Please see \ref page_tf_TfError for more information about how to use
/// TF_ERROR().
///
/// This is safe to call in secondary threads, but the error will be downgraded
/// to a warning.
///
/// \hideinitializer
#define TF_ERROR(...)

/// Issue an internal programming error, but continue execution.
///
/// This macro is a convenience.  It produces a TF_ERROR() with an error code
/// indicating a coding error.  It takes a printf-like format specification or a
/// std::string.  Generally, an error handilng delegate will take action to turn
/// this error into a python exception, and if it remains unhandled at the end of
/// an application iteration will roll-back the undo stack to a last-known-good
/// state.
///
/// This is safe to call in secondary threads, but the error will be downgraded
/// to a warning.
///
/// \hideinitializer
#define TF_CODING_ERROR(fmt, args)

/// Issue a generic runtime error, but continue execution.
///
/// This macro is a convenience.  It produces a TF_ERROR() with an error code
/// indicating a generic runtime error.  It is preferred over TF_ERROR(0),
/// but using a specific error code is preferred over this.  It takes a
/// printf-like format specification or a std::string.  Generally, an error
/// handilng delegate will take action to turn this error into a python
/// exception, and if it remains unhandled at the end of an application iteration
/// will roll-back the undo stack to a last-known-good state.
///
/// This is safe to call in secondary threads, but the error will be downgraded
/// to a warning.
///
/// \hideinitializer
#define TF_RUNTIME_ERROR(fmt, args)

/// Issue a fatal error and end the program.
///
/// This macro takes a printf-like format specification or a std::string.  The
/// program will generally terminate upon a fatal error.
///
/// \hideinitializer
#define TF_FATAL_ERROR(fmt, args)

/// Issue a warning, but continue execution.
///
/// This macro works with a variety of argument sets. It supports simple
/// printf-like format specification or a std::string. It also supports
/// specification of a diagnostic code and a piece of arbitrary information in
/// the form of a TfDiagnosticInfo. The following is a full list of supported
/// argument lists:
///
/// \code
/// TF_WARN(const char *)           // plain old string
/// TF_WARN(const char *, ...)      // printf like formatting
/// TF_WARN(std::string)            // stl string
/// \endcode
///
/// A diagnostic code can be passed in along with the warning message. See
/// \ref DiagnosticEnumConventions for an example of registering an enum type
/// and it's values as diagnostic codes.
///
/// \code
/// TF_WARN(DIAGNOSTIC_ENUM, const char *)
/// TF_WARN(DIAGNOSTIC_ENUM, const char *, ...)
/// TF_WARN(DIAGNOSTIC_ENUM, std::string)
/// \endcode
///
/// A piece of arbitrary data can also be passed in along with the diagnostic
/// code and warning message as follows:
///
/// \code
/// TF_WARN(info, DIAGNOSTIC_ENUM, const char *)
/// TF_WARN(info, DIAGNOSTIC_ENUM, const char *, ...)
/// TF_WARN(info, DIAGNOSTIC_ENUM, std::string)
/// \endcode
///
/// Generally, no adjustment to program state should occur as the result of
/// this macro. This is in contrast with errors as mentioned above.
///
/// This is safe to call in secondary threads, but the warning will be printed
/// to \c stderr rather than being handled by the diagnostic delegate.
///
/// \hideinitializer
#define TF_WARN(...)

/// Issue a status message, but continue execution.
///
/// This macro works with a variety of argument sets. It supports simple
/// printf-like format specification or a std::string. It also supports
/// specification of a diagnostic code and a piece of arbitrary information in
/// the form of a TfDiagnosticInfo. The following is a full list of supported
/// argument lists:
///
/// \code
/// TF_STATUS(const char *)           // plain old string
/// TF_STATUS(const char *, ...)      // printf like formatting
/// TF_STATUS(std::string)            // stl string
/// \endcode
///
/// A diagnostic code can be passed in along with the status message. See
/// \ref DiagnosticEnumConventions for an example of registering an enum type
/// and it's values as diagnostic codes.
///
/// \code
/// TF_STATUS(DIAGNOSTIC_ENUM, const char *)
/// TF_STATUS(DIAGNOSTIC_ENUM, const char *, ...)
/// TF_STATUS(DIAGNOSTIC_ENUM, std::string)
/// \endcode
///
/// A piece of arbitrary data can also be passed in along with the diagnostic
/// code and status message as follows:
///
/// \code
/// TF_STATUS(info, DIAGNOSTIC_ENUM, const char *)
/// TF_STATUS(info, DIAGNOSTIC_ENUM, const char *, ...)
/// TF_STATUS(info, DIAGNOSTIC_ENUM, std::string)
/// \endcode
///
/// Generally, no adjustment to program state should occur as the result of
/// this macro. This is in contrast with errors as mentioned above.
///
/// This is safe to call in secondary threads, but the message will be printed
/// to \c stderr rather than being handled by the diagnostic delegate.
///
/// \hideinitializer
#define TF_STATUS(...)

/// Aborts if the condition \c cond is not met.
///
/// \param cond is any expression convertible to bool; if the condition evaluates
/// to \c false, program execution ends with this call.
///
/// Note that the diagnostic message sent is the code \c cond, in the form of
/// a string.  Unless the condition expression is self-explanatory, use
/// \c TF_FATAL_ERROR().  See \ref DiagnosticTF_FATAL_ERROR for further
/// discussion.
///
/// Currently, a \c TF_AXIOM() statement is not made a no-op in optimized
/// builds; however, it always possible that either (a) the axiom statement
/// might be removed at some point if the code is deemed correct or (b) in the
/// future, some flavor of build might choose to make axioms be no-ops.  Thus,
/// programmers must make \e certain that the code in \p cond is entirely free
/// of side effects.
///
/// \hideinitializer
#define TF_AXIOM(cond)

/// The same as TF_AXIOM, but compiled only in dev builds.
///
/// \param cond is any expression convertible to bool; if the condition evaluates
/// to \c false, program execution ends with this call.
///
/// This macro has the same behavior as TF_AXIOM, but it is compiled only
/// in dev builds. This version should only be used in code that is
/// known (not just suspected!) to be performance critical.
///
/// \hideinitializer
#define TF_DEV_AXIOM(cond)

/// Checks a condition and reports an error if it evaluates false.
///
/// This can be thought of as something like a softer, recoverable TF_AXIOM.
///
/// The macro expands to an expression whose value is either true or false
/// depending on \c cond. If \c cond evaluates to false, issues a coding error
/// indicating the failure.
///
/// \param cond is any expression convertible to bool.
///
/// Usage generally follows patterns like these:
/// \code
/// // Simple check.  This is like a non-fatal TF_AXIOM.
/// TF_VERIFY(condition);
///
/// // Avoiding code that requires the condition be met.
/// if (TF_VERIFY(condition)) {
///     // code requiring condition be met.
/// }
///
/// // Executing recovery code in case the condition is not met.
/// if (not TF_VERIFY(condition)) {
///     // recovery code to execute since condition was not met.
/// }
/// \endcode
/// 
/// Here are some examples:
/// \code
/// // List should be empty.  If not, issue an error, clear it out and continue.
/// if (not TF_VERIFY(list.empty()) {
///     // The list was unexpectedly not empty.  TF_VERIFY will have
///     // issued a coding error with details.  We clear the list and continue.
///     list.clear();
/// }
///
/// // Only add to string if ptr is valid.
/// string result = ...; 
/// if (TF_VERIFY(ptr != NULL)) {
///     result += ptr->Method();
/// }
/// \endcode
///
/// The macro also optionally accepts printf-style arguments to generate a
/// message emitted in case the condition is not met.  For example:
/// \code
/// if (not TF_VERIFY(index < size,
///                   "Index out of bounds (%zu >= %zu)", index, size)) {
///     // Recovery code...
/// }
/// \endcode
///
/// Unmet conditions generate TF_CODING_ERRORs by default, but setting the
/// environment variable TF_FATAL_VERIFY to 1 will make unmet conditions
/// generate TF_FATAL_ERRORs instead and abort the program.  This is intended for
/// testing.
///
/// This is safe to call in secondary threads, but the error will be downgraded
/// to a warning.
///
/// \hideinitializer
#define TF_VERIFY(cond [, format, ...])

#endif  /* defined(doxygen) */
           
//
// The rest of this is seen by a regular compile (or doxygen).
//

#if defined(__cplusplus) || defined(doxygen)

/// Get the name of the current function as a \c std::string.
///
/// This macro will return the name of the current function, nicely
/// formatted, as an \c std::string.  This is meant primarily for
/// diagnostics.  Code should not rely on a specific format, because it
/// may change in the future or vary across architectures.  For example,
/// \code
/// void YourClass::SomeMethod(int x) {
///     cout << "Debugging info about function " << TF_FUNC_NAME() << "." << endl;
///     ...
/// }
/// \endcode
/// Should display something like:
/// "Debugging info about function YourClass::SomeMethod."
///
/// \hideinitializer
#define TF_FUNC_NAME()                                 \
    ArchGetPrettierFunctionName(__ARCH_FUNCTION__, __ARCH_PRETTY_FUNCTION__)

void Tf_TerminateHandler();

#if !defined(doxygen)

// Redefine these macros from DiagnosticLite to versions that will accept
// either string or printf-like args.

#ifdef TF_CODING_ERROR
#undef TF_CODING_ERROR
#endif
#define TF_CODING_ERROR(...)                            \
    Tf_PostErrorHelper(TF_CALL_CONTEXT,                 \
        TF_DIAGNOSTIC_CODING_ERROR_TYPE, __VA_ARGS__)

#ifdef TF_FATAL_CODING_ERROR
#undef TF_FATAL_CODING_ERROR
#endif
#define TF_FATAL_CODING_ERROR                                \
    Tf_DiagnosticHelper(TF_CALL_CONTEXT,                \
        TF_DIAGNOSTIC_CODING_ERROR_TYPE).IssueFatalError


#ifdef TF_CODING_WARNING
#undef TF_CODING_WARNING
#endif
#define TF_CODING_WARNING(...)                   \
    Tf_PostWarningHelper(TF_CALL_CONTEXT,               \
        TF_DIAGNOSTIC_CODING_ERROR_TYPE, __VA_ARGS__)

#ifdef TF_DIAGNOSTIC_WARNING
#undef TF_DIAGNOSTIC_WARNING
#endif
#define TF_DIAGNOSTIC_WARNING                                \
    Tf_DiagnosticHelper(TF_CALL_CONTEXT.Hide(),                \
        TF_DIAGNOSTIC_WARNING_TYPE).IssueWarning

#ifdef TF_RUNTIME_ERROR
#undef TF_RUNTIME_ERROR
#endif // TF_RUNTIME_ERROR
#define TF_RUNTIME_ERROR(...)                           \
    Tf_PostErrorHelper(TF_CALL_CONTEXT,                 \
        TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE, __VA_ARGS__)

#ifdef TF_FATAL_ERROR
#undef TF_FATAL_ERROR
#endif // TF_FATAL_ERROR
#define TF_FATAL_ERROR                                  \
    Tf_DiagnosticHelper(TF_CALL_CONTEXT,                \
        TF_DIAGNOSTIC_FATAL_ERROR_TYPE).IssueFatalError

#ifdef TF_DIAGNOSTIC_FATAL_ERROR
#undef TF_DIAGNOSTIC_FATAL_ERROR
#endif // TF_DIAGNOSTIC_FATAL_ERROR
#define TF_DIAGNOSTIC_FATAL_ERROR                       \
    Tf_DiagnosticHelper(TF_CALL_CONTEXT,                \
        TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE).IssueFatalError

#ifdef TF_DIAGNOSTIC_NONFATAL_ERROR
#undef TF_DIAGNOSTIC_NONFATAL_ERROR
#endif // TF_DIAGNOSTIC_NONFATAL_ERROR
#define TF_DIAGNOSTIC_NONFATAL_ERROR                        \
    Tf_DiagnosticHelper(TF_CALL_CONTEXT,                 \
        TF_DIAGNOSTIC_WARNING_TYPE).IssueWarning

// Redefine the following three macros from DiagnosticLite to versions that will
// accept the following sets of arguments:
// * MACRO(const char *, ...)
// * MACRO(const std::string &msg)
// * MACRO(ENUM, const char *, ...)
// * MACRO(ENUM, const std::string *msg)
// * MACRO(TfDiagnosticInfo, ENUM, const char *, ...)
// * MACRO(TfDiagnosticInfo, ENUM, const std::string *msg)

#ifdef TF_WARN
#undef TF_WARN
#endif // TF_WARN
#define TF_WARN(...)                                \
    Tf_PostWarningHelper(TF_CALL_CONTEXT, __VA_ARGS__)

#ifdef TF_STATUS
#undef TF_STATUS
#endif // TF_STATUS
#define TF_STATUS(...)                              \
    Tf_PostStatusHelper(TF_CALL_CONTEXT, __VA_ARGS__)

#ifdef TF_ERROR
#undef TF_ERROR
#endif // TF_ERROR
#define TF_ERROR(...)                              \
    Tf_PostErrorHelper(TF_CALL_CONTEXT, __VA_ARGS__)

#ifdef TF_QUIET_ERROR
#undef TF_QUIET_ERROR
#endif // TF_ERROR
#define TF_QUIET_ERROR(...)                              \
    Tf_PostQuietlyErrorHelper(TF_CALL_CONTEXT, __VA_ARGS__)

// See documentation above.
#define TF_VERIFY(cond, ...)                                                   \
    (ARCH_LIKELY(cond) ? true :                                                \
     Tf_FailedVerifyHelper(TF_CALL_CONTEXT, # cond,                            \
                           Tf_DiagnosticStringPrintf(__VA_ARGS__)))

// Helpers for TF_VERIFY
bool
Tf_FailedVerifyHelper(TfCallContext const &context,
                      char const *condition,
                      std::string const &msg);

// Helpers for TF_VERIFY.
std::string Tf_DiagnosticStringPrintf();
std::string Tf_DiagnosticStringPrintf(const char *format, ...)
    ARCH_PRINTF_FUNCTION(1, 2)
    ;

#endif // !doxygen

#endif // __cplusplus || doxygen

/// Sets program name for reporting errors.
///
/// This function simply calls to ArchSetProgramNameForErrors().
void TfSetProgramNameForErrors(std::string const& programName);

/// Returns currently set program info. 
///
/// This function simply calls to ArchGetProgramNameForErrors().
std::string TfGetProgramNameForErrors();

/// \private
struct Tf_DiagnosticHelper {
    Tf_DiagnosticHelper(TfCallContext const &context,
                        TfDiagnosticType type) :
        _context(context),
        _type(type)
    {
    }
    
    TfCallContext const &GetContext() const { return _context; }
    TfDiagnosticType GetType() const { return _type; }

    void IssueError(std::string const &msg) const;
    void IssueError(char const *fmt, ...) const ARCH_PRINTF_FUNCTION(2,3);
    void IssueFatalError(std::string const &msg) const;
    void IssueFatalError(char const *fmt, ...) const ARCH_PRINTF_FUNCTION(2,3);
    void IssueWarning(std::string const &msg) const;
    void IssueWarning(char const *fmt, ...) const ARCH_PRINTF_FUNCTION(2,3);
    void IssueStatus(std::string const &msg) const;
    void IssueStatus(char const *fmt, ...) const ARCH_PRINTF_FUNCTION(2,3);

  private:
    TfCallContext _context;
    TfDiagnosticType _type;
};

#endif

/// (Re)install Tf's crash handler. This should not generally need to be
/// called since Tf does this itself when loaded.  However, when run in 3rd
/// party environments that install their own signal handlers, possibly
/// overriding Tf's, this provides a way to reinstall them, in hopes that
/// they'll stick.
///
/// This calls std::set_terminate() and installs signal handlers for SIGSEGV,
/// SIGBUS, SIGFPE, and SIGABRT.
void TfInstallTerminateAndCrashHandlers();

///@}

#endif // TF_DIAGNOSTIC_H
