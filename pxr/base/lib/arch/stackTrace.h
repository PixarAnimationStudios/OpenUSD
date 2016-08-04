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
#ifndef ARCH_STACKTRACE_H
#define ARCH_STACKTRACE_H

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/api.h"

#include <inttypes.h>
#include <stdio.h>
#include <functional>
#include <vector>
#include <string>
#include <iosfwd>

/*!
 * \file stackTrace.h
 * \brief Architecture-specific call-stack tracing routines.
 * \ingroup group_arch_Diagnostics
 */

/*!
 * \brief Dumps call-stack info to a file, and prints an informative
 * message.
 * \ingroup group_arch_Diagnostics
 *
 * The reason for the trace should be supplied in \p reason.  This routine
 * can be slow and is intended to be called for a fatal error, such as a
 * caught coredump signal, but may be called at any time.  An additional
 * message may be provided in \p message.  If \p reason is \c NULL then
 * this function only writes \p message to the banner (if any).
 *
 * This function is implemented by calling an external program.  This
 * is suitable for times where the current process may be corrupted.  In
 * other cases, using \c ArchPrintStackTrace() or other related functions
 * would be much faster.
 *
 * Note the use of \c char* as opposed to \c string: this is intentional,
 * because we are trying to use only async-safe function from here on and
 * malloc() is not async-safe.
 */
ARCH_API
void ArchLogPostMortem(const char* reason, const char* message = nullptr);

/*!
 * \brief Sets the command line that gathers call-stack info.
 * \ingroup group_arch_Diagnostics
 *
 * This function sets the command line to execute to gather and log call-stack
 * info.  \p argv must be NULL terminated.  \p command and/or \p argv may be
 * NULL to suppress execution.  Otherwise argv[0] must be the full path to the
 * program to execute, typically \p command or "$cmd" as described below.
 *
 * \p command and \p argv are not copied and must remain valid until the next
 * call to \c ArchSetPostMortem.
 *
 * Simple substitution is supported on argv elements:
 * \li $cmd:      Substitutes the command pathname, or $ARCH_POSTMORTEM if set
 * \li $pid:      Substitutes the process id
 * \li $log:      Substitutes the log pathname
 * \li $time:     Substitutes the user time (if available, else wall time)
 *
 * \sa ArchLogPostMortem
 */
ARCH_API
void ArchSetPostMortem(const char* command, const char *const argv[]);

/*!
 * \brief Log session info.
 * \ingroup group_arch_Diagnostics
 *
 * Optionally indicate that this is due to a crash by providing
 * the path to a file containing a stack trace in \p crashStackTrace.
 *
 */
ARCH_API
void ArchLogSessionInfo(const char *crashStackTrace=NULL);

/*!
 * \brief Sets the command line to log sessions.
 * \ingroup group_arch_Diagnostics
 *
 * This function sets the command line to execute to log session info.
 * \p argv is used if no crash stack trace is provided, otherwise
 * \p crashArgv is used.  Both must be NULL terminated.  If \p command
 * or \p argv is NULL then non-crashes are not logged;  if \p command
 * or \p crashArgv is NULL then crashes are not logged.  If not NULL then
 * argv[0] and crashArgv[0] must be full path to the program to execute,
 * typically \p command or "$cmd" as described below.
 *
 * \p command, \p argv, and \p crashArgv are not copied and must remain
 * valid until the next call to \c ArchSetLogSession.
 *
 * Simple substitution is supported on argv elements:
 * \li $cmd:      Substitutes the command pathname, or $ARCH_LOGSESSION if set
 * \li $prog      Substitutes the program name
 * \li $pid:      Substitutes the process id
 * \li $time:     Substitutes the user time (if available, else wall time)
 * \li $stack:    Substitutes the crash stack string (only in crashArgv)
 *
 * \sa ArchLogSessionInfo
 */
ARCH_API
void ArchSetLogSession(const char* command,
                       const char* const argv[],
                       const char* const crashArgv[]);

/*!
 * \brief Register the callback to invoke logging at
 * end of a successful session.
 * \ingroup group_arch_Diagnostics
 *
 * This function registers ArchLogSessionInfo()
 * and records the current timestamp, to send up-time
 * to the DB upon exiting.
 */
ARCH_API
void ArchEnableSessionLogging();

// Again, 2 versions, one for 2x and one for 3x.
/*!
 * \brief Print a stack trace to the given FILE *.
 * \ingroup group_arch_Diagnostics
 *
 * Prints out a stack trace to the given FILE *.  Will call 
 * Prints out a stack trace to the given stream (\c man \c stdio).  Will call 
 * ArchLoadSymbols() if symbols have not yet been loaded.
 */
ARCH_API
void ArchPrintStackTrace(FILE *fout, const std::string& programName,
			 const std::string& reason);

/*!
 * \brief Print a stack trace to the given FILE *.
 * \ingroup group_arch_Diagnostics
 *
 * Prints out a stack trace to the given FILE *.  Will call 
 * Prints out a stack trace to the given stream (\c man \c stdio).  Will call 
 * ArchLoadSymbols() if symbols have not yet been loaded.
 */
ARCH_API
void ArchPrintStackTrace(FILE *fout, const std::string& reason);


/*!
 * \brief Print a stack trace to the given iostream.
 * \ingroup group_arch_Diagnostics
 *
 * see above
 */
ARCH_API
void ArchPrintStackTrace(std::ostream& out, 
			 const std::string& programName,
			 const std::string& reason);

/*!
 * \brief Print a stack trace to the given iostream.
 * \ingroup group_arch_Diagnostics
 *
 * see above
 */
ARCH_API
void ArchPrintStackTrace(std::ostream& out, const std::string& reason);

/*!
 * \brief A callback to get a symbolic representation of an address.
 * \ingroup group_arch_Diagnostics
 */
typedef std::function<std::string(uintptr_t address)> ArchStackTraceCallback;

/*!
 * \brief Sets a callback to get a symbolic representation of an address.
 * \ingroup group_arch_Diagnostics
 *
 * The callback returns a string for an address in a stack trace,
 * typically including the name of the function containing the address.
 * \p cb may be \c NULL to use a default implementation.
 */
ARCH_API
void ArchSetStackTraceCallback(const ArchStackTraceCallback& cb);

/*!
 * \brief Returns the callback to get a symbolic representation of an address.
 * \ingroup group_arch_Diagnostics
 *
 * \sa ArchSetStackTraceCallback
 */
ARCH_API
void ArchGetStackTraceCallback(ArchStackTraceCallback* cb);

/*!
 * \brief Returns the set value for the application's launch time.
 * \ingroup group_arch_Diagnostics
 *
 * The timestamp for this value is set when the Arch library is initialized.
 */
ARCH_API
time_t ArchGetAppLaunchTime();

/*!
 * \brief Enables or disables the automatic logging of crash information.
 * \ingroup group_arch_Diagnostics
 *
 * This function controls whether the stack trace and build information is
 * automatically caught and stored to an internal database when a fatal
 * crash occurs. 
 */
ARCH_API
void ArchSetFatalStackLogging( bool flag );

/*!
 * \brief Returns whether automatic logging of fatal crashes is enabled
 * \ingroup group_arch_Diagnostics
 *
 * This is set to false by default.
 */
ARCH_API
bool ArchGetFatalStackLogging();

/*!
 * \brief Sets the program name to be used in diagnostic output
 * \ingroup group_arch_Diagnostics
 *
 * The default value is initialized to ArchGetExecutablePath().
 */
ARCH_API
void ArchSetProgramNameForErrors( const char * progName );

/*!
 * \brief Returns the currently set program name for reporting errors.
 * \ingroup group_arch_Diagnostics
 *
 * Defaults to ArchGetExecutablePath().
 */
ARCH_API
const char * ArchGetProgramNameForErrors();

/*!
 * \brief Sets additional program info to be reported to the terminal in case of
 * a fatal error.
 * \ingroup group_arch_Diagnostics
 *
 */
ARCH_API
void ArchSetProgramInfoForErrors( const std::string& key, const std::string& value );

/*!
 * \brief Returns currently set program info. 
 * \ingroup group_arch_Diagnostics
 *
 */
ARCH_API
std::string ArchGetProgramInfoForErrors(const std::string& key);


/*!
 * \brief Stores (or removes if \p text is NULL) a pointer to additional log
 * data that will be output in the stack trace log in case of a fatal error.
 * Note that the pointer \p text is copied, not the pointed-to text.  Thus it is
 * the caller's responsibility to ensure that \p text is valid until the caller
 * removes it by invoking this function again with text==NULL.
 * \ingroup group_arch_Diagnostics
 */
ARCH_API
void ArchSetExtraLogInfoForErrors(const std::string &key, char const *text);

/*!
 * \brief Logs a stack trace to a file in /var/tmp.
 * \ingroup group_arch_Diagnostics
 *
 * This function is similar to \c ArchLogPostMortem(), but will not fork an
 * external process and only reports a stack trace.  A file in /var/tmp is
 * created with the name \c st_APPNAME.XXXXXX, where \c mktemp is used to
 * make a unique extension for the file.  If \c sessionLog is specified,
 * then it will be appended to this file.  A message is printed to \c stderr
 * reporting that a stack trace has been taken and what file it has been
 * written to.  And if \c fatal is true, then the stack trace will  be added
 * to the stack_trace database table.
 */
ARCH_API
void ArchLogStackTrace(const std::string& progName, const std::string& reason,
		       bool fatal = false, 
		       const std::string& sessionLog = "");

/*!
 * \brief Logs a stack trace to a file in /var/tmp.
 * \ingroup group_arch_Diagnostics
 *
 * This function is similar to \c ArchLogPostMortem(), but will not fork an
 * external process and only reports a stack trace.  A file in /var/tmp is
 * created with the  name \c st_APPNAME.XXXXXX, where \c mktemp is used to
 * make a unique  extension for the file.  If \c sessionLog is specified,
 * then it will be appended to this file.  A message is printed to \c stderr
 * reporting that a stack trace has been taken and what file it has been
 * written to.  And if \c fatal is true, then the stack trace will  be added
 * to the stack_trace database table.
 */
ARCH_API
void ArchLogStackTrace(const std::string& reason,
                       bool fatal = false, 
                       const std::string& sessionLog = "");

/*!
 * \brief Return stack trace.
 * \ingroup group_arch_Diagnostics
 *
 * This function will return a vector of strings containing the
 * current stack.  The vector will be of maximum size \p maxDepth.
 */
ARCH_API
std::vector<std::string> ArchGetStackTrace(size_t maxDepth);


/*!
 * \brief Save frames of current stack
 * \ingroup group_arch_Diagnostics
 *
 * This function saves at maximum \c maxDepth frames of the current
 * stack into the vector \c frames.
 *
 */
ARCH_API
void ArchGetStackFrames(size_t maxDepth, std::vector<uintptr_t> *frames);

/*!
 * \brief Save frames of current stack.
 * \ingroup group_arch_Diagnostics
 *
 * This function saves at maximum \p maxDepth frames of the current
 * stack into the vector \p frames, skipping the first \p numFramesToSkipAtTop
 * frames.  The first frame will be at depth \p numFramesToSkipAtTop and the
 * last at depth \p numFramesToSkipAtTop + \p maxDepth - 1.
 *
 */
ARCH_API
void ArchGetStackFrames(size_t maxDepth, size_t numFramesToSkipAtTop,
                        std::vector<uintptr_t> *frames);

/*!
 * \brief Print stack frames to the given iostream.
 * \ingroup group_arch_Diagnostics
 *
 */
ARCH_API
void ArchPrintStackFrames(std::ostream& out,
                          const std::vector<uintptr_t> &frames);

typedef void (*ArchCrashHandlerSystemCB)(void* userData);
/*!
 * \brief Replacement for 'system' safe for a crash handler
 * \ingroup group_arch_Diagnostics
 *
 * This function is a substitute for system() which does not allocate
 * or free any data, and times out after \c timeout seconds if the 
 * operation in \c argv is not complete. Unlike system, it takes the 
 * full \c pathname of the program to run, and won't search the path.
 * Also unlike system, \c argv[] are the separated arguments, starting
 * with the program's name, as for execv. \c callback is called every
 * second. \c userData is passed to \c callback.  \c callback can be used, 
 * for example, to print a '.' repeatedly to show progress.  The alarm
 * used in this function could interfere with setitimer or other calls
 * to alarm, and this function uses non-locking fork and exec if available
 * so should not generally be used except following a catastrophe.
 */
ARCH_API
int ArchCrashHandlerSystemv(const char* pathname, char *const argv[],
			    int timeout, ArchCrashHandlerSystemCB callback, 
			    void* userData);

/*!
 * \brief Crash, to test crash behavior.
 * \ingroup group_arch_Diagnostics
 *
 * This function causes the calling program to crash by doing bad malloc 
 * and free things.  If \c spawnthread is true, it spawns a thread which 
 * remains alive during the crash.  It aborts if it fails to crash.
 *
 */
ARCH_API
void ArchTestCrash(bool spawnthread);

#if defined(ARCH_OS_DARWIN)
        /* XXX:---:20030825 Mac OS X has no ETIME.  ECANCELED seems
         * to have about the closest meaning to the actual error here.
         * The operation is timing out, not being explicitly canceled,
         * but it is canceled.
         */
#ifndef ETIME
#define ETIME ECANCELED
#endif  // end ETIME
#endif  // end ARCH_OS_DARWIN

#endif  // ARCH_STACKTRACE_H
