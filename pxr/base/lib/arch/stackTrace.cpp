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
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/env.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/symbols.h"
#include "pxr/base/arch/vsnprintf.h"
#if defined(ARCH_OS_WINDOWS)
#include <io.h>
#include <process.h>
#include <Winsock2.h>
#include <DbgHelp.h>
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif
#else
#include <dlfcn.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/wait.h>
#endif
#include <algorithm>
#include <atomic>
#include <fstream>
#include <ostream>
#include <iterator>
#include <limits>
#include <cstdlib>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>

/* Darwin/ppc did not do stack traces.  Darwin/i386 still 
   needs some work, this has been stubbed out for now.  */

#if defined(ARCH_OS_LINUX)
#include <ucontext.h>
#endif

#if defined(ARCH_OS_LINUX) && defined(ARCH_BITS_64)
#include <unwind.h>
#endif

#if defined(ARCH_OS_DARWIN)
#include <execinfo.h>
#endif

#if defined(ARCH_OS_WINDOWS)
#define getpid() _getpid()
#define write(fd_, data_, size_) _write(fd_, data_, size_)
#define strdup(str_) _strdup(str_)
#endif

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <time.h>

PXR_NAMESPACE_OPEN_SCOPE

using namespace std;

#define MAX_STACK_DEPTH 4096

#if !defined(ARCH_OS_WINDOWS)
// XXX Darwin
// total hack -- no idea if this will work if we die in malloc...
typedef int (*ForkFunc)(void);
ForkFunc Arch_nonLockingFork =
#if defined(ARCH_OS_LINUX)
    (ForkFunc)dlsym(RTLD_NEXT, "__libc_fork");
#elif defined(ARCH_OS_DARWIN)
    NULL;
#else
#error Unknown architecture.
#endif
#endif

/*** Stack Logging Global Variables ***/

// Stores the application's launch time
static time_t _appLaunchTime;

// This bool determines whether a stack trace should be
// logged upon catching a crash. Use ArchSetFatalStackLogging
// to set this value.
static bool _shouldLogStackToDb = false;

// This string holds the path the script used to log sessions
// to a database.
static const char * _logStackToDbCmd = nullptr;

// Arguments to _logStackToDbCmd for non-crash and crash reports, respectively.
static const char* const* _sessionLogArgv = nullptr;
static const char* const* _sessionCrashLogArgv = nullptr;

// This string stores the program name to be used when
// displaying error information.  Initialized in
// Arch_InitConfig() to ArchGetExecutablePath()
static char * _progNameForErrors = NULL;

// Key-value map for program info. Stores additional
// program info to be used when displaying error information.
typedef std::map<std::string, std::string> Arch_ProgInfoMap;
static Arch_ProgInfoMap _progInfoMap;

// Printed version of _progInfo map, since we can't
// traverse it during an error. 
static char *_progInfoForErrors = NULL;
// Mutex for above:
static std::mutex _progInfoForErrorsMutex;

// Key-value map for extra log info.  Stores unowned pointers to text to be
// emitted in stack trace logs in case of fatal errors or crashes.
typedef std::map<std::string, std::vector<std::string> const *> Arch_LogInfoMap;
static Arch_LogInfoMap _logInfoForErrors;
// Mutex for above:
static std::mutex _logInfoForErrorsMutex;

static void
_EmitAnyExtraLogInfo(FILE* outFile, size_t max = 0)
{
    // This function can't cause any heap allocation, be careful.
    // XXX -- std::string::c_str and fprintf can do allocations.
    std::lock_guard<std::mutex> lock(_logInfoForErrorsMutex);
    size_t n = 0;
    for (Arch_LogInfoMap::const_iterator i = _logInfoForErrors.begin(),
             end = _logInfoForErrors.end(); i != end; ++i) {
        fputs("\n", outFile);
        fputs(i->first.c_str(), outFile);
        fputs(":\n", outFile);
        for (std::string const &line: *i->second) {
            if (max && n++ >= max) {
                fputs("... full diagnostics reported in the stack trace "
                      "file.\n", outFile);
                return;
            }
            fputs(line.c_str(), outFile);
        }
    }
}

static void
_atexitCallback()
{
    ArchLogSessionInfo();
}

void
ArchEnableSessionLogging()
{
    static int unused = atexit(_atexitCallback);
    (void)unused;
}

static const char* const stackTracePrefix = "st";
static const char* stackTraceCmd = nullptr;
static const char* const* stackTraceArgv = nullptr;

static long _GetAppElapsedTime();

namespace {

// Return the length of s.
size_t asstrlen(const char* s)
{
    size_t result = 0;
    if (s) {
        while (*s++) {
            ++result;
        }
    }
    return result;
}

// Copy the string at src to dst, returning a pointer to the NUL terminator
// in dst (NOT a pointer to dst).
//
// ARCH_NOINLINE because old clang versions generated incorrect optimized
// code.
char* asstrcpy(char* dst, const char* src) ARCH_NOINLINE;
char* asstrcpy(char* dst, const char* src)
{
    while ((*dst++ = *src++)) {
        // Do nothing
    }
    return dst - 1;
}

// Compare the strings for equality.
bool asstreq(const char* dst, const char* src)
{
    if (!dst || !src) {
        return dst == src;
    }
    while (*dst || *src) {
        if (*dst++ != *src++) {
            return false;
        }
    }
    return true;
}

// Compare the strings for equality up to n characters.
bool asstrneq(const char* dst, const char* src, size_t n)
{
    if (!dst || !src) {
        return dst == src;
    }
    while ((*dst || *src) && n) {
        if (*dst++ != *src++) {
            return false;
        }
        --n;
    }
    return true;
}

// Returns the environment variable named name, or NULL if it doesn't exist.
const char* asgetenv(const char* name)
{
    if (name) {
        const size_t len = asstrlen(name);
        for (char** i = ArchEnviron(); *i; ++i) {
            const char* var = *i;
            if (asstrneq(var, name, len)) {
                if (var[len] == '=') {
                    return var + len + 1;
                }
            }
        }
    }
    return nullptr;
}

// Minimum safe size for a buffer to hold a long converted to decimal ASCII.
static constexpr int numericBufferSize =
    std::numeric_limits<long>::digits10
    + 1     // sign
    + 1     // overflow (digits10 doesn't necessarily count the high digit)
    + 1     // trailing NUL
    + 1;    // paranoia

// Return the number of digits in the decimal string representation of x.
size_t asNumDigits(long x)
{
    size_t result = 1;
    if (x < 0) {
        x = -x;
        ++result;
    }
    while (x >= 10) {
        ++result;
        x /= 10;
    }
    return result;
}

// Write the decimal string representation of x to s, which must have
// sufficient space available.
char* asitoa(char* s, long x)
{
    // Write the minus sign.
    if (x < 0) {
        x = -x;
        *s = '-';
    }

    // Skip to the end and write the terminating NUL.
    char* end = s += asNumDigits(x);
    *s = '\0';

    // Write each digit, starting with the 1's column, working backwards.
    if (x == 0) {
        *--s = '0';
    }
    else {
        static const char digit[] = "0123456789";
        while (x) {
            *--s = digit[x % 10];
            x /= 10;
        }
    }
    return end;
}

// Write a string to a file descriptor.
void aswrite(int fd, const char* msg)
{
    int saved = errno;
    write(fd, msg, asstrlen(msg));
    errno = saved;
}

int _GetStackTraceName(char* buf, size_t len)
{
    // Take care to avoid non-async-safe functions.
    // NOTE: This doesn't protect against other threads changing the
    //       temporary directory or program name for errors.

    // Count the string length required.
    size_t required =
        asstrlen(ArchGetTmpDir()) +
        1 +     // "/"
        asstrlen(stackTracePrefix) +
        1 +     // "_"
        asstrlen(ArchGetProgramNameForErrors()) +
        1 +     // "."
        asNumDigits(getpid()) +
        1;      // "\0"

    // Fill in buf with the default name.
    char* end = buf;
    if (len < required) {
        // No space.  Not quite an accurate error code.
        errno = ENOMEM;
        return -1;
    }
    else {
        end = asstrcpy(end, ArchGetTmpDir());
        end = asstrcpy(end, "/");
        end = asstrcpy(end, stackTracePrefix);
        end = asstrcpy(end, "_");
        end = asstrcpy(end, ArchGetProgramNameForErrors());
        end = asstrcpy(end, ".");
        end = asitoa(end, getpid());
    }

    // Return a name that isn't currently in use.  Simultaneously create
    // the empty file.
    int suffix = 0;
#if defined(ARCH_OS_WINDOWS)
    int fd = _open(buf, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, 
                   _S_IREAD | _S_IWRITE);
#else
    int fd =  open(buf, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, 0640);
#endif

    while (fd == -1 && errno == EEXIST) {
        // File exists.  Try a new suffix if there's space.
        ++suffix;
        if (len < required + 1 + asNumDigits(suffix)) {
            // No space.  Not quite an accurate error code.
            errno = ENOMEM;
            return -1;
        }
        asstrcpy(end, ".");
        asitoa(end + 1, suffix);
#if defined(ARCH_OS_WINDOWS)
        fd = _open(buf, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, 
                   _S_IREAD | _S_IWRITE);
#else
        fd =  open(buf, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, 0640);
#endif
    }
    if (fd != -1) {
        ArchCloseFile(fd);
        fd = 0;
    }
    return fd;
}

// Build an argument list (async-safe).
static bool
_MakeArgv(
    const char* dstArgv[],
    size_t maxDstArgs,
    const char* cmd,
    const char* const srcArgv[],
    const char* const substitutions[][2],
    size_t numSubstitutions)
{
    if (!cmd || !srcArgv) {
        return false;
    }

    // Count the maximum number of arguments needed.
    size_t n = 1;
    for (const char *const* i = srcArgv; *i; ++n, ++i) {
        // Do nothing
    }

    // Make sure we don't have too many arguments.
    if (n >= maxDstArgs) {
        return false;
    }

    // Build the command line.
    size_t j = 0;
    for (size_t i = 0; i != n; ++i) {
        if (asstreq(srcArgv[i], "$cmd")) {
            dstArgv[j++] = cmd;
        }
        else {
            dstArgv[j] = srcArgv[i];
            for (size_t k = 0; k != numSubstitutions; ++k) {
                if (asstreq(srcArgv[i], substitutions[k][0])) {
                    dstArgv[j] = substitutions[k][1];
                    break;
                }
            }
            ++j;
        }
    }
    dstArgv[j] = nullptr;

    return true;
}

#if !defined(ARCH_OS_WINDOWS)
/* We use a 'non-locking' fork so that we won't get hung up if we've
 * had malloc corruption when we crash.  The crash recovery behavior
 * can be tested with ArchTestCrash(), which should crash with this
 * malloc corruption.
 */
static int
nonLockingFork()
{
    if (Arch_nonLockingFork != NULL) {
        return (Arch_nonLockingFork)();
    }
    return fork();
}
#endif

#if defined(ARCH_OS_LINUX)
static int
nonLockingLinux__execve (const char *file,
                         char *const argv[],
                         char *const envp[])
{
#if defined(ARCH_BITS_64)
    /*
     * We make a direct system call here, because we can't find an
     * execve which corresponds with the non-locking fork we call
     * (__libc_fork().)
     *
     * This code doesn't mess with other threads, and avoids the bug
     * that calling regular execv after the nonLockingFork() causes
     * hangs in a threaded app.  (We use the non-locking fork to get
     * around problems with forking when we have had memory
     * corruption.)  whew.
     *
     * %rdi, %rsi, %rdx, %rcx, %r8, %r9 are args 0-5
     * syscall clobbers %rcx and %r11
     *
     * why do we put args 1, 2 into cx, dx and then move them?
     * because it doesn't work if you directly specify them as
     * constraints to gcc.
     */

    unsigned long result;
    __asm__ __volatile__ (
        "mov    %0, %%rdi    \n\t"
        "mov    %%rcx, %%rsi \n\t"
        "mov    %%rdx, %%rdx \n\t"
        "mov    $0x3b, %%rax \n\t"
        "syscall             \n\t"
        : "=a" (result)
        : "0" (file), "c" (argv), "d" (envp)
        : "memory", "cc", "r11"
    );

    if (result >= 0xfffffffffffff000) {
        errno = -result;
        result = (unsigned int)-1;
    }

    return result;
#else
#error Unknown architecture
#endif
}

#endif

#if !defined(ARCH_OS_WINDOWS)
/* This is the corresponding execv which works with nonLockingFork().
 * currently, it's only different from execv for linux.  The crash
 * recovery behavior can be tested with ArchTestCrash().
 */
static int
nonLockingExecv(const char *path, char *const argv[])
{
#if defined(ARCH_OS_LINUX)
     return nonLockingLinux__execve (path, argv, __environ);
#else
     return execv(path, argv);
#endif
}
#endif

/*
 * Return the base of a filename.
 */

static std::string
getBase(const char* path)
{
#if defined(ARCH_OS_WINDOWS)
    const std::string tmp = path;
    std::string::size_type i = tmp.find_last_of("/\\");
    if (i != std::string::npos) {
        std::string::size_type j = tmp.find(".exe");
        if (j != std::string::npos) {
            return tmp.substr(i + 1, j - i - 1);
        }
        return tmp.substr(i + 1);
    }
    return tmp;
#else
    const char* base = strrchr(path, '/');
    if (!base)
        return path;

    base++;
    return strlen(base) > 0 ? base : path;
#endif
}

} // anonymous namespace

/*
 * Run an external program to write post-mortem information to logfile for
 * process pid.  This waits until the program completes.
 *
 * This is an internal function used by ArchLogPostMortem().  It must call
 * only async-safe functions.
 */

static
int _LogStackTraceForPid(const char *logfile)
{
    // Get the command to run.
    const char* cmd = asgetenv("ARCH_POSTMORTEM");
    if (!cmd) {
        cmd = stackTraceCmd;
    }
    if (!cmd || !stackTraceArgv) {
        // Silently do nothing.
        return 0;
    }

    // Construct the substitutions.
    char pidBuffer[numericBufferSize], timeBuffer[numericBufferSize];
    asitoa(pidBuffer, getpid());
    asitoa(timeBuffer, _GetAppElapsedTime());
    const char* const substitutions[3][2] = {
        { "$pid", pidBuffer }, { "$log", logfile }, { "$time", timeBuffer }
    };

    // Build the argument list.
    static constexpr size_t maxArgs = 32;
    const char* argv[maxArgs];
    if (!_MakeArgv(argv, maxArgs, cmd, stackTraceArgv, substitutions, 2)) {
        static const char msg[] = "Too many arguments to postmortem command\n";
        aswrite(2, msg);
        return 0;
    }

    // Invoke the command.
    ArchCrashHandlerSystemv(argv[0], (char *const*)argv,
                            300 /* wait up to 300 seconds */ , NULL, NULL);
    return 1;
}

void
ArchSetPostMortem(const char* command, const char *const argv[] )
{
    stackTraceCmd  = command;
    stackTraceArgv = argv;
}

/*
 * Arch_SetAppLaunchTime()
 * -------------------------------
 * Stores the current time as the application's launch time.
 * This function is internal.
 */
ARCH_HIDDEN
void
Arch_SetAppLaunchTime()
{
    _appLaunchTime = time(NULL);
}

/*
 * ArchGetAppLaunchTime()
 * -------------------------------
 * Returns the application's launch time, or NULL if a timestamp hasn't
 * been created with AchSetAppLaunchTime().  
 */
time_t
ArchGetAppLaunchTime()
{
    // Defaults to NULL
    return _appLaunchTime;
}
 
/*
 * ArchSetFatalStackLogging()
 * -------------------------------
 * This enables the logging of the stack trace and other build
 * information upon intercepting a crash.
 *
 * This function can be called from python.
 */
void
ArchSetFatalStackLogging( bool flag )
{
    _shouldLogStackToDb = flag;   
}

/*
 * ArchGetFatalStackLogging()
 * ---------------------------
 * Returns the current value of the logging flag.
 *
 * This function can be called from python.
 */
bool
ArchGetFatalStackLogging()
{
    return _shouldLogStackToDb;
}

void
ArchSetProgramInfoForErrors(const std::string& key,
                            const std::string& value)
{
    std::lock_guard<std::mutex> lock(_progInfoForErrorsMutex);

    if (value.empty()) {
        _progInfoMap.erase(key);
    } else {
        _progInfoMap[key] = value;
    }

    std::ostringstream ss;

    // update the error info string
    for(Arch_ProgInfoMap::iterator iter = _progInfoMap.begin();
        iter != _progInfoMap.end(); ++iter) {

        ss << iter->first << ": " << iter->second << '\n';
    }

    if (_progInfoForErrors)
        free(_progInfoForErrors);

    _progInfoForErrors = strdup(ss.str().c_str());
}

std::string
ArchGetProgramInfoForErrors(const std::string& key) {
    
    std::lock_guard<std::mutex> lock(_progInfoForErrorsMutex);

    Arch_ProgInfoMap::iterator iter = _progInfoMap.find(key);
    std::string result;
    if (iter != _progInfoMap.end())
        result = iter->second;

    return result;
} 

void
ArchSetExtraLogInfoForErrors(const std::string &key,
                             std::vector<std::string> const *lines)
{
    std::lock_guard<std::mutex> lock(_logInfoForErrorsMutex);
    if (!lines || lines->empty()) {
        _logInfoForErrors.erase(key);
    } else {
        _logInfoForErrors[key] = lines;
    }
}

/*
 * ArchSetProgramNameForErrors
 * ---------------------------
 * Set's the program name that is to be used for diagnostic output.  
 */
void
ArchSetProgramNameForErrors( const char *progName )
{
     
    if (_progNameForErrors)
        free(_progNameForErrors);
    
    if (progName)
        _progNameForErrors = strdup(getBase(progName).c_str());
    else
        _progNameForErrors = NULL;
}

/*
 * ArchGetProgramNameForErrors
 * ----------------------------
 * Returns the currently set program name used for
 * reporting error information.  Returns "libArch"
 * if a value hasn't been set.
 */
const char *
ArchGetProgramNameForErrors()
{
    if (_progNameForErrors)
        return _progNameForErrors;

    return "libArch";
}

#if defined(ARCH_OS_WINDOWS)
static long
_GetAppElapsedTime()
{
    FILETIME       starttime;
    FILETIME       exittime;
    FILETIME       kerneltime;
    FILETIME       usertime;
    ULARGE_INTEGER li;

    if (::GetProcessTimes(GetCurrentProcess(),
            &starttime, &exittime, &kerneltime, &usertime) == 0) {
        ARCH_WARNING("_GetAppElapsedTime failed");
        return 0L;
    }
    memcpy(&li, &usertime, sizeof(FILETIME));
    return static_cast<long>(li.QuadPart / 10000000ULL);
}
#else
static long
_GetAppElapsedTime()
{
    rusage ru;

    // We only record the amount of time spent in user instructions,
    // so as to discount idle time when logging up time.
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        return long(ru.ru_utime.tv_sec);
    }

    // Fallback to logging the entire session time, if we could
    // not get the user time from the resource usage.

    // Note: Total time measurement will be a little off because this
    // calculation happens after the stack trace is generated which can
    // take a long time.
    //
    return long(time(0) - _appLaunchTime);
}
#endif

static void
_InvokeSessionLogger(const char* progname, const char *stackTrace)
{
    // Get the command to run.
    const char* cmd = asgetenv("ARCH_LOGSESSION");
    const char* const* srcArgv =
        stackTrace ? _sessionCrashLogArgv : _sessionLogArgv;
    if (!cmd) {
        cmd = _logStackToDbCmd;
    }
    if (!cmd || !srcArgv) {
        // Silently do nothing.
        return;
    }

    // Construct the substitutions.
    char pidBuffer[numericBufferSize], timeBuffer[numericBufferSize];
    asitoa(pidBuffer, getpid());
    asitoa(timeBuffer, _GetAppElapsedTime());
    const char* const substitutions[4][2] = {
        {"$pid", pidBuffer}, {"$time", timeBuffer},
        {"$prog", progname}, {"$stack", stackTrace}
    };

    // Build the argument list.
    static constexpr size_t maxArgs = 32;
    const char* argv[maxArgs];
    if (!_MakeArgv(argv, maxArgs, cmd, srcArgv, substitutions, 4)) {
        static const char msg[] = "Too many arguments to log session command\n";
        aswrite(2, msg);
        return;
    }

    // Invoke the command.
    ArchCrashHandlerSystemv(argv[0], (char *const*)argv, 
                            60 /* wait up to 60 seconds */, NULL, NULL);
}

/*
 * '_FinishLoggingFatalStackTrace' appends the sessionLog
 * to the stackTrace, and then calls an external program to add it
 * to the stack_trace database table.
 */
static void
_FinishLoggingFatalStackTrace(const char *progname, const char *stackTrace,
                              const char *sessionLog, bool crashingHard)
{
    if (!crashingHard && sessionLog) {
        // If we were given a session log, cat it to the end of the stack.
        if (FILE* stackFd = ArchOpenFile(stackTrace, "a")) {
            if (FILE* sessionLogFd = ArchOpenFile(sessionLog, "r")) {
                fputs("\n\n********** Session Log **********\n\n", stackFd);
                // Cat the session log
                char line[4096];
                while (fgets(line, 4096, sessionLogFd)) {
                    fputs(line, stackFd);
                }
                fclose(sessionLogFd);
            }
            fclose(stackFd);
        }
    }

    // Add trace to database if _shouldLogStackToDb is true
    if (_shouldLogStackToDb)
    {
        _InvokeSessionLogger(progname, stackTrace);
    }

}

void
ArchLogSessionInfo(const char *crashStackTrace)
{
    if (_shouldLogStackToDb)
    {
        _InvokeSessionLogger(ArchGetProgramNameForErrors(), crashStackTrace);
    }
}

void
ArchSetLogSession(
    const char* command,
    const char* const argv[],
    const char* const crashArgv[])
{
    _logStackToDbCmd     = command;
    _sessionLogArgv      = argv;
    _sessionCrashLogArgv = crashArgv;
}

/*
 * Run an external program to make a report and tell the user where the report
 * file is.
 *
 * Use of char*'s is deliberate: only async-safe calls allowed past this point!
 */
void
ArchLogPostMortem(const char* reason,
                  const char* message /* = nullptr */,
                  const char* extraLogMsg /* = nullptr */)
{
    static std::atomic_flag busy = ATOMIC_FLAG_INIT;

    // Disallow recursion and allow only one thread at a time.
    while (busy.test_and_set(std::memory_order_acquire)) {
        // Spin!
        std::this_thread::yield();
    }

    const char* progname = ArchGetProgramNameForErrors();

    // If we can attach a debugger then just exit here.
    if (ArchDebuggerAttach()) {
        ARCH_DEBUGGER_TRAP;
        _exit(0);
    }

    /* Could use tmpnam but we're trying to be minimalist here. */
    char logfile[1024];
    if (_GetStackTraceName(logfile, sizeof(logfile)) == -1) {
        // Cannot create the logfile.
        static const char msg[] = "Cannot create a log file\n";
        aswrite(2, msg);
        busy.clear(std::memory_order_release);
        return;
    }

    // Write reason for stack trace to logfile.
    if (FILE* stackFd = ArchOpenFile(logfile, "a")) {
        if (reason) {
            fputs("This stack trace was requested because: ", stackFd);
            fputs(reason, stackFd);
            fputs("\n", stackFd);
        }
        if (message) {
            fputs(message, stackFd);
            fputs("\n", stackFd);
        }
        _EmitAnyExtraLogInfo(stackFd);
        if (extraLogMsg) {
            fputs(extraLogMsg, stackFd);
            fputs("\n", stackFd);
        }
        fputs("\nPostmortem Stack Trace\n", stackFd);
        fclose(stackFd);
    }

    /* get hostname for printing out in the error message only */
    char hostname[MAXHOSTNAMELEN];
    if (gethostname(hostname,MAXHOSTNAMELEN) != 0) {
        /* error getting hostname; don't try to print it */
        hostname[0] = '\0';
    }

    fputs("\n", stderr);
    fputs("------------------------ '", stderr);
    fputs(progname, stderr);
    fputs("' is dying ------------------------\n", stderr);

    // print out any registered program info
    {
        std::lock_guard<std::mutex> lock(_progInfoForErrorsMutex);
        if (_progInfoForErrors) {
            fprintf(stderr, "%s", _progInfoForErrors);
        }
    }

    if (reason) {
        fputs("This stack trace was requested because: ", stderr);
        fputs(reason, stderr);
        fputs("\n", stderr);
    }
    if (message) {
        fputs(message, stderr);
        fputs("\n", stderr);
    }
    fputs("The stack can be found in ", stderr);
    fputs(hostname, stderr);
    fputs(":", stderr);
    fputs(logfile, stderr);
    fputs("\n", stderr);

    int loggedStack = _LogStackTraceForPid(logfile);
    fputs("done.\n", stderr);
    // Additionally, print the first few lines of extra log information since
    // developers don't always think to look for it in the stack trace file.
    _EmitAnyExtraLogInfo(stderr, 3 /* max */);
    fputs("------------------------------------------------------------------\n",
          stderr);

    if (loggedStack) {
        _FinishLoggingFatalStackTrace(progname, logfile, NULL /*session log*/, 
                                      true /* crashing hard? */);
    }

    busy.clear(std::memory_order_release);
}

/*
 * Write a stack trace to a file, without forking.
 */
void
ArchLogStackTrace(const std::string& reason, bool fatal, 
                  const string &sessionLog)
{
    ArchLogStackTrace(ArchGetProgramNameForErrors(), reason, fatal,
                      sessionLog);
}

/*
 * Write a stack trace to a file, without forking.
 *
 * Note: use of mktemp is not threadsafe.
 */
void
ArchLogStackTrace(const std::string& progname, const std::string& reason,
                  bool fatal, const string &sessionLog)
{
    string tmpFile;
    int fd = ArchMakeTmpFile(ArchStringPrintf("%s_%s",
                                              stackTracePrefix,
                                              ArchGetProgramNameForErrors()),
                             &tmpFile);

    /* get hostname for printing out in the error message only */
    char hostname[MAXHOSTNAMELEN];
    if (gethostname(hostname,MAXHOSTNAMELEN) != 0) {
        hostname[0]= '\0';
    }

    fprintf(stderr,
            "--------------------------------------------------------------\n"
            "A stack trace has been requested by %s because of %s\n",
            progname.c_str(), reason.c_str());

    // print out any registered program info
    {
        std::lock_guard<std::mutex> lock(_progInfoForErrorsMutex);
        if (_progInfoForErrors) {
            fprintf(stderr, "%s", _progInfoForErrors);
        }
    }

    if (fd != -1) {
        FILE* fout = ArchFdOpen(fd, "w");
        fprintf(stderr, "The stack can be found in %s:%s\n"
                "--------------------------------------------------------------"
                "\n", hostname, tmpFile.c_str());
        ArchPrintStackTrace(fout, progname, reason);
        /* If this is a fatal stack trace, attempt to add it to the db */
        if (fatal) {
            _EmitAnyExtraLogInfo(fout);
        }
        fclose(fout);
        if (fatal) {
            _FinishLoggingFatalStackTrace(progname.c_str(), tmpFile.c_str(),
                                          sessionLog.empty() ?  
                                          NULL : sessionLog.c_str(),
                                          false /* crashing hard? */);
        }
    }
    else {
        /* we couldn't open the tmp file, so write the stack trace to stderr */
        fprintf(stderr,
                "--------------------------------------------------------------"
                "\n");
        ArchPrintStackTrace(stderr, progname, reason);
        _EmitAnyExtraLogInfo(stderr);
    }
    fprintf(stderr,
            "--------------------------------------------------------------\n");
}

#if defined(ARCH_OS_DARWIN)

/*
 * This function will use _LogStackTraceForPid(const char*), which uses
 * the stacktrace script, to log the stack to a file.  Then it reads the lines
 * back in and puts them into an output iterator.
 */
template <class OutputIterator>
static void
_LogStackTraceToOutputIterator(OutputIterator oi, size_t maxDepth, bool addEndl)
{
    /* Could use tmpnam but we're trying to be minimalist here. */
    char logfile[1024];
    _GetStackTraceName(logfile, sizeof(logfile));

    _LogStackTraceForPid(logfile);

    ifstream inFile(logfile);
    string line;
    size_t currentDepth = 0;
    while(!inFile.eof() && currentDepth < maxDepth) {
        getline(inFile, line);
        if(addEndl && !inFile.eof())
            line += "\n";
        *oi++ = line;
        currentDepth ++;
    }

    inFile.close();
    ArchUnlinkFile(logfile);
}

#endif

/*
 * ArchPrintStackTrace
 *  print out a stack trace to the given FILE *.
 */
void
ArchPrintStackTrace(FILE *fout, const std::string& programName, const std::string& reason)
{
    ostringstream oss;

    ArchPrintStackTrace(oss, programName, reason);

    if (fout == NULL) {
        fout = stderr;
    }

    fprintf(fout, "%s", oss.str().c_str());
    fflush(fout);
}

void
ArchPrintStackTrace(FILE* fout, const std::string& reason)
{
    ArchPrintStackTrace(fout, ArchGetProgramNameForErrors(), reason);
}

void
ArchPrintStackTrace(std::ostream& out, const std::string& reason)
{
    ArchPrintStackTrace(out, ArchGetProgramNameForErrors(), reason);
}

/*
 * ArchPrintStackTrace
 *  print out a stack trace to the given ostream.
 * 
 * This function should probably not be called from a signal handler as 
 * it calls printf and other unsafe functions.
 */
void
ArchPrintStackTrace(ostream& oss,
                    const std::string& programName,
                    const std::string& reason)
{
    oss << "==============================================================\n"
        << " A stack trace has been requested by "
        << programName << " because: " << reason << endl;

#if defined(ARCH_OS_DARWIN)

    _LogStackTraceToOutputIterator(ostream_iterator<string>(oss), numeric_limits<size_t>::max(), true);

#else

    vector<uintptr_t> frames;
    ArchGetStackFrames(MAX_STACK_DEPTH, &frames);
    ArchPrintStackFrames(oss, frames);

#endif

    oss << "==============================================================\n";
}

void
ArchGetStackTrace(ostream& oss, const std::string& reason)
{
    ArchPrintStackTrace(oss, ArchGetProgramNameForErrors(), reason);
}

void
ArchGetStackFrames(size_t maxDepth, vector<uintptr_t> *frames)
{
    ArchGetStackFrames(maxDepth, /* skip = */ 0, frames);
}

#if defined(ARCH_OS_LINUX) && defined(ARCH_BITS_64)
struct Arch_UnwindContext {
public:
    Arch_UnwindContext(size_t inMaxdepth, size_t inSkip,
                       vector<uintptr_t>* inFrames) :
        maxdepth(inMaxdepth), skip(inSkip), frames(inFrames) { }

public:
    size_t maxdepth;
    size_t skip;
    vector<uintptr_t>* frames;
};

static _Unwind_Reason_Code
Arch_unwindcb(struct _Unwind_Context *ctx, void *data)
{   
    Arch_UnwindContext* context = static_cast<Arch_UnwindContext*>(data);

    // never extend frames because it is unsafe to alloc inside a
    // signal handler, and this function is called sometimes (when
    // profiling) from a signal handler.
    if (context->frames->size() >= context->maxdepth) {
        return _URC_END_OF_STACK;
    }
    else {
        if (context->skip > 0) {
            --context->skip;
        }
        else {
            context->frames->push_back(_Unwind_GetIP(ctx));
        }
        return _URC_NO_REASON;
    }
}

/*
 * ArchGetStackFrames
 *  save some of stack into buffer.
 */
void
ArchGetStackFrames(size_t maxdepth, size_t skip, vector<uintptr_t> *frames)
{
    /* use the exception handling mechanism to unwind our stack.
     * note this is gcc >= 3.3.3 only.
     */
    Arch_UnwindContext context(maxdepth, skip, frames);
    _Unwind_Backtrace(Arch_unwindcb, (void*)&context);
}

#elif defined(ARCH_OS_WINDOWS)

void
ArchGetStackFrames(size_t maxdepth, size_t skip, vector<uintptr_t> *frames)
{
    void* stack[MAX_STACK_DEPTH];
    size_t frameCount = CaptureStackBackTrace(0, MAX_STACK_DEPTH, stack, NULL);
    frameCount = std::min(frameCount, maxdepth);
    frames->reserve(frameCount);
    for (size_t frame = skip; frame != frameCount; ++frame) {
        frames->push_back(reinterpret_cast<uintptr_t>(stack[frame]));
    }
}

#elif defined(ARCH_OS_DARWIN)

void
ArchGetStackFrames(size_t maxdepth, size_t skip, vector<uintptr_t> *frames)
{
    void* stack[MAX_STACK_DEPTH];
    const size_t frameCount =
        backtrace(stack, std::max((size_t)MAX_STACK_DEPTH, maxdepth));
    frames->reserve(frameCount);
    for (size_t frame = skip; frame != frameCount; ++frame) {
        frames->push_back(reinterpret_cast<uintptr_t>(stack[frame]));
    }
}

#else

void
ArchGetStackFrames(size_t, size_t, vector<uintptr_t> *)
{
}

#endif

static
std::string
Arch_DefaultStackTraceCallback(uintptr_t address)
{
    // Subtract one from the address before getting the info because
    // the stack frames have the addresses where we'll return to,
    // not where we called from.  We don't want the info for the
    // instruction after our calls, we want it for the call itself.
    // We don't need the exact address of the call because
    // ArchGetAddressInfo() will return the info for the closest
    // address is knows about that not after the given address.
    // (That's good because the address minus one is not the start
    // of the call instruction but there's no way to figure that out
    // here without decoding assembly instructions.)
    std::string objectPath, symbolName;
    void* baseAddress, *symbolAddress;
    if (ArchGetAddressInfo(reinterpret_cast<void*>(address - 1),
                   &objectPath, &baseAddress,
                   &symbolName, &symbolAddress) && symbolAddress) {
        Arch_DemangleFunctionName(&symbolName);
        const uintptr_t symbolOffset =
            (uint64_t)(address - (uintptr_t)symbolAddress);
        return ArchStringPrintf("%s+%#0lx", symbolName.c_str(), symbolOffset);
    }
    else {
        return ArchStringPrintf("%#016lx", address);
    }
}

static
vector<string>
Arch_GetStackTrace(const vector<uintptr_t> &frames);

/*
 * ArchPrintStackFrames
 *  print out stack frames to the given ostream.
 */
void
ArchPrintStackFrames(ostream& oss, const vector<uintptr_t> &frames)
{
    const vector<string> result = Arch_GetStackTrace(frames);
    for (size_t i = 0; i < result.size(); i++) {
        oss << result[i] << std::endl;
    }
}

/*
 * ArchGetStackTrace
 *  vector of strings
 */
vector<string>
ArchGetStackTrace(size_t maxDepth)
{
    vector<uintptr_t> frames;
    ArchGetStackFrames(maxDepth, &frames);
    return Arch_GetStackTrace(frames);
}


static
ArchStackTraceCallback*
Arch_GetStackTraceCallback()
{
    static ArchStackTraceCallback callback;
    return &callback;
}

vector<string>
Arch_GetStackTrace(const vector<uintptr_t> &frames)
{
    vector<string> rv;

    if (frames.empty()) {
        rv.push_back("No frames saved, stack traces probably not supported "
                     "on this architecture.");
        return rv;
    }

    ArchStackTraceCallback callback = *Arch_GetStackTraceCallback();
    if (!callback) {
        callback = Arch_DefaultStackTraceCallback;
    }
    for (size_t i = 0; i < frames.size(); i++) {
        const std::string symbolic = callback(frames[i]);
        rv.push_back(ArchStringPrintf(" #%-3i 0x%016lx in %s",
                                      (int)i, frames[i], symbolic.c_str()));
    }

    return rv;
}

void
ArchSetStackTraceCallback(const ArchStackTraceCallback& cb)
{
    *Arch_GetStackTraceCallback() = cb;
}

void
ArchGetStackTraceCallback(ArchStackTraceCallback* cb)
{
    if (cb) {
        *cb = *Arch_GetStackTraceCallback();
    }
}

static void
archAlarmHandler(int /*sig */)
{
    /* do nothing.  we just have to wake up. */
}

/*
 * Replacement for 'system' safe for a crash handler
 *
 * This function is a substitute for system() which does not allocate
 * or free any data, and times out after timeout seconds if the
 * operation in  argv is not complete.  callback is called every
 * second.  userData is passed to callback.  callback can be used,
 * for example, to print a '.' repeatedly to show progress.  The alarm
 * used in this function could interfere with setitimer or other calls
 * to alarm, and this function uses non-locking fork and exec if available
 * so should  not generally be used except following a catastrophe.
 */
int
ArchCrashHandlerSystemv(const char* pathname, char *const argv[],
                        int timeout, ArchCrashHandlerSystemCB callback,
                        void* userData)
{
#if defined(ARCH_OS_WINDOWS)
    fprintf(stderr, "ArchCrashHandlerSystemv unimplemented for Windows\n");
    return -1;
#else
    struct sigaction act, oldact;
    int retval = 0;
    int savedErrno;
    pid_t pid = nonLockingFork(); /* use non-locking fork */
    if (pid == -1) {
        /* fork() failed */
        char errBuffer[numericBufferSize];
        asitoa(errBuffer, errno);
        aswrite(2, "FAIL: Unable to fork() crash handler: errno=");
        aswrite(2, errBuffer);
        aswrite(2, "\n");
        return -1;
    }
    else if (pid == 0) {
        // Call setsid() in the child, which is intended to start a new
        // "session", and detach from the controlling tty.  We do this because
        // the stack tracing stuff invokes gdb, which wants to fiddle with the
        // tty, and if we're run in the background, that blocks, so we hang
        // trying to take the stacktrace.  This seems to fix that.
        //
        // If standard input is not a TTY then skip this.  This ensures
        // the child is part of the same process group as this process,
        // which is important on the renderfarm.
        if (isatty(0)) {
            setsid();
        }

        // Exec the handler.
        nonLockingExecv(pathname, argv);

        /* Exec failed */
        char errBuffer[numericBufferSize];
        asitoa(errBuffer, errno);
        aswrite(2, "FAIL: Unable to exec crash handler ");
        aswrite(2, pathname);
        aswrite(2, ": errno=");
        aswrite(2, errBuffer);
        aswrite(2, "\n");
        _exit(127);
    }
    else {
        int delta = 0;
        sigemptyset(&act.sa_mask);
# if defined(SA_INTERRUPT)
        act.sa_flags   = SA_INTERRUPT;
# else
        act.sa_flags   = 0;
# endif
        act.sa_handler = &archAlarmHandler;
        sigaction(SIGALRM, &act, &oldact);

        /* loop until timeout seconds have passed */
        do {
            int status;
            pid_t child;

            /* a timeout <= 0 means forever */
            if (timeout > 0) {
                delta = 1;  /* callback every delta seconds */
                alarm(delta);
            }

            /* see what the child is up to */
            child = waitpid(pid, &status, 0 /* forever, unless interrupted */);
            if (child == (pid_t)-1) {
                /* waitpid error.  return if not due to signal. */
                if (errno != EINTR) {
                    retval = -1;
                    char errBuffer[numericBufferSize];
                    asitoa(errBuffer, errno);
                    aswrite(2, "FAIL: Crash handler wait failed: errno=");
                    aswrite(2, errBuffer);
                    aswrite(2, "\n");
                    goto out;
                }
                /* continue below */
            }
            else if (child != 0) {
                /* child finished */
                if (WIFEXITED(status)) {
                    /* child exited successfully.  it returned 127
                     * if the exec() failed.  we'll set errno to
                     * ENOENT in that case though the actual error
                     * could be something else. */
                    retval = WEXITSTATUS(status);
                    if (retval == 127) {
                        errno = ENOENT;
                        aswrite(2, "FAIL: Crash handler failed to exec\n");
                    }
                    goto out;
                }

                if (WIFSIGNALED(status)) {
                    /* child died due to uncaught signal */
                    errno = EINTR;
                    retval = -1;
                    char sigBuffer[numericBufferSize];
                    asitoa(sigBuffer, WTERMSIG(status));
                    aswrite(2, "FAIL: Crash handler died: signal=");
                    aswrite(2, sigBuffer);
                    aswrite(2, "\n");
                    goto out;
                }
                /* child died for an unknown reason */
                errno = EINTR;
                retval = -1;
                char statusBuffer[numericBufferSize];
                asitoa(statusBuffer, status);
                aswrite(2, "FAIL: Crash handler unexpected wait status=");
                aswrite(2, statusBuffer);
                aswrite(2, "\n");
                goto out;
            }

            /* child is still going.  invoke callback, countdown, and
             * wait again for next interrupt. */
            if (callback)
                callback(userData);
            timeout -= delta;
        }  while (timeout > 0);

        /* timed out.  kill the child and wait for that. */
        alarm(0);  /* turn off alarm so it doesn't wake us during kill */
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);

        /*
         * Set the errno to 'EBUSY' to imply that some resource was busy
         * and hence we're 'timing out'.
         */
        errno = EBUSY;
        retval = -1;
        aswrite(2, "FAIL: Crash handler timed out\n");
    }

  out:
    savedErrno = errno;
    alarm(0);
    sigaction(SIGALRM, &oldact, NULL);

    errno = savedErrno;
    return retval;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
