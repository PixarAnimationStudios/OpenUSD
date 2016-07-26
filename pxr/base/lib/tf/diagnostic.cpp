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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/vsnprintf.h"
#include "pxr/base/arch/stackTrace.h"

#include <cstdio>
#include <stdexcept>
#include <csignal>

using std::string;

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(TF_DIAGNOSTIC_CODING_ERROR_TYPE,"Coding Error");
    TF_ADD_ENUM_NAME(TF_DIAGNOSTIC_FATAL_CODING_ERROR_TYPE,"Fatal Coding Error");
    TF_ADD_ENUM_NAME(TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE,"Runtime Error");
    TF_ADD_ENUM_NAME(TF_DIAGNOSTIC_FATAL_ERROR_TYPE,"Fatal Error");
    TF_ADD_ENUM_NAME(TF_DIAGNOSTIC_NONFATAL_ERROR_TYPE,"Error");
    TF_ADD_ENUM_NAME(TF_DIAGNOSTIC_WARNING_TYPE, "Warning");
    TF_ADD_ENUM_NAME(TF_DIAGNOSTIC_STATUS_TYPE, "Status");
    TF_ADD_ENUM_NAME(TF_APPLICATION_EXIT_TYPE, "Application Exit");
};

std::string
Tf_DiagnosticStringPrintf()
{
    return std::string();
}

std::string
Tf_DiagnosticStringPrintf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    string s = TfVStringPrintf(format, ap);
    va_end(ap);
    return s;
}

bool
Tf_FailedVerifyHelper(const TfCallContext &context,
                      char const *condition,
                      const std::string &msg)
{
    std::string errorMsg =
        std::string("Failed verification: ' ") + condition + " '";

    if (not msg.empty())
        errorMsg += " -- " + msg;

    if (TfGetenvBool("TF_FATAL_VERIFY", false)) {
        Tf_DiagnosticHelper(context, TF_DIAGNOSTIC_FATAL_ERROR_TYPE).
            IssueFatalError(errorMsg);
    } else {
        Tf_PostErrorHelper(context, TF_DIAGNOSTIC_CODING_ERROR_TYPE, errorMsg);
    }

    return false;
}


/*
 * This is called when std::terminate is invoked but there is no current
 * pending exception.  According to the C++ standard (15.5.1), this occurs
 *
 * - when a throw-expression with no operand attempts to rethrow an exception
 *   and no exception is being handled (15.1)
 *
 * - when the destructor or the copy assignment operator is invoked on an
 *   object of type std::thread that refers to a joinable thread
 *   (30.3.1.3, 30.3.1.4). [C++ 11]
 *
 * Additionally, libstdc++ may invoke std::terminate
 *
 * - when a pure virtual method is called (__cxa_pure_virtual)
 * - when a deleted virtual method is called (__cxa_deleted_virtual). [C++ 11]
 *
 * though this is not behavior guaranteed to exist in future versions of
 * libstdc++ or in other implementations of the standard library.
 *
 * Finally, a program may also call std::terminate directly.
 */
static void
_BadThrowHandler()
{
    TF_FATAL_ERROR("std::terminate() called without a current exception");
}

void
Tf_TerminateHandler()
{
    string reason;
    string type;

    try {
        /*
         * If there's no exception, we'll end up in the handler above.
         */
        std::set_terminate(::_BadThrowHandler);
        throw;
    }
    catch (std::bad_alloc& exc) {
        std::set_terminate(Tf_TerminateHandler);
        reason = "allocation failed (you've run out of memory)";
        type = "bad_alloc";
    }
    catch (std::exception& exc) {
        std::set_terminate(Tf_TerminateHandler);
        reason = exc.what();
        type = typeid(exc).name();
    }
    catch (...) {
        std::set_terminate(Tf_TerminateHandler);
        /*
         * Unknown exception type.
         * NB: with g++, there is an ABI function to tell us the type
         * of the exception.  Add this to arch at some point, and then make
         * use of it here.
         */
        reason = "reason unknown";
        type = "";
    }

    TF_FATAL_ERROR("%s : uncaught exception! : '%s'", 
        reason.empty() ? "<empty>" : reason.c_str(),
        type.empty() ? "<empty>" : type.c_str() ) ;
}

void TfSetProgramNameForErrors(string const& programName)
{
    ArchSetProgramNameForErrors(programName.c_str());
}

std::string TfGetProgramNameForErrors()
{
    return ArchGetProgramNameForErrors();
}

// Called when we get a fatal signal.
static void
_fatalSignalHandler(int signo, siginfo_t*, void* uctx)
{
    const char* msg = "unknown signal";
    switch (signo) {
    case SIGSEGV:
        msg = "received SIGSEGV";
        break;

    case SIGBUS:
        msg = "received SIGBUS";
        break;

    case SIGFPE:
        msg = "received SIGFPE";
        break;

    case SIGABRT:
        msg = "received SIGABRT";
        break;

#if defined(_GNU_SOURCE)
    default:
        msg = strsignal(signo);
        break;
#endif
    }
    ArchLogPostMortem(msg);

    // Fatal signal handlers should not return. If they do and the
    // signal is SIGSEGV, SIGBUS, and possibly others, the signal will
    // be immediately re-raised when the instruction is re-executed.
    // Avoid atexit handlers and destructors but flush stdout and
    // stderr in case there might be any useful information lingering
    // in their buffers.
    //
    fflush(stdout);
    fflush(stderr);

    // Simulate the exit status of being killed by signal signo
    _exit(128 + signo);
}


void
TfInstallTerminateAndCrashHandlers()
{
    std::set_terminate(Tf_TerminateHandler);

    // Catch segvs and bus violations
    struct sigaction act;
    act.sa_sigaction = _fatalSignalHandler;
    act.sa_flags     = SA_SIGINFO;

    // The signal handler (more specifically ArchLogPostMortem) has a
    // flag to prevent it from running concurrently. If it is invoked
    // concurrently, it will simply spin until the other thread is
    // done. But if it is the same thread, then it will deadlock. This
    // only happens if we get one of the below signals in this thread
    // while handling another one of the below signals. We can prevent
    // the deadlock by simply blocking all of the synchronous signals
    // during the handling of any of them. If a synchronous signal
    // occurs while blocked, the process behaves as if SIG_DFL was in
    // effect.
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGSEGV);
    sigaddset(&act.sa_mask, SIGBUS);
    sigaddset(&act.sa_mask, SIGFPE);

    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGBUS,  &act, NULL);
    sigaction(SIGFPE,  &act, NULL);
    sigaction(SIGABRT, &act, NULL);
}
