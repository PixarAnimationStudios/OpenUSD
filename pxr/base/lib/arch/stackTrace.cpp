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
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/symbols.h"
#include "pxr/base/arch/vsnprintf.h"
#include <atomic>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <dlfcn.h>
#include <cstdlib>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>
#include <cstring>

/* Darwin/ppc did not do stack traces.  Darwin/i386 still 
   needs some work, this has been stubbed out for now.  */

#if defined(ARCH_OS_LINUX)
#include <ucontext.h>
#endif

#if defined(ARCH_OS_LINUX) && defined(ARCH_BITS_64)
#include <unwind.h>
#endif

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <time.h>

using namespace std;

#define MAX_STACK_DEPTH		4096

// XXX Darwin
// total hack -- no idea if this will work if we die in malloc...

typedef int (*ForkFunc)(void);
ForkFunc Arch_nonLockingFork =

#if defined(ARCH_OS_LINUX)
    (ForkFunc)dlsym(RTLD_NEXT, "__libc_fork");
#elif defined(ARCH_OS_DARWIN)
    fork;			/* XXX -- this is not necessarily correct */
#else
#error Unknown architecture.
#endif

/*** Stack Logging Global Varaibles ***/

// Stores the application's launch time
static time_t _appLaunchTime;

// This bool determines whether a stack trace should be
// logged upon catching a crash. Use ArchSetFatalStackLogging
// to set this value.
static bool _shouldLogStackToDb = false;

// This string holds the path the the script used to log sessions
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
static pthread_mutex_t _progInfoForErrorsMutex = PTHREAD_MUTEX_INITIALIZER;

// Key-value map for extra log info.  Stores unowned pointers to text to be
// emitted in stack trace logs in case of fatal errors or crashes.
typedef std::map<std::string, char const *> Arch_LogInfoMap;
static Arch_LogInfoMap _logInfoForErrors;
// Mutex for above:
static pthread_mutex_t _logInfoForErrorsMutex = PTHREAD_MUTEX_INITIALIZER;

namespace {
// Private auto lock/unlock RAII class for pthread_mutex_t.
struct Locker {
    explicit Locker(pthread_mutex_t &mutex)
        : _mutex(&mutex) { pthread_mutex_lock(_mutex); }
    ~Locker() { pthread_mutex_unlock(_mutex); }
private:
    pthread_mutex_t *_mutex;
};
}

static void
_EmitAnyExtraLogInfo(FILE* outFile)
{
    // This function can't cause any heap allocation, be careful.
    Locker lock(_logInfoForErrorsMutex);
    for (Arch_LogInfoMap::const_iterator i = _logInfoForErrors.begin(),
             end = _logInfoForErrors.end(); i != end; ++i) {
        fprintf(outFile, "\n%s:\n%s", i->first.c_str(), i->second);
    }
}

static void
_EmitAnyExtraLogInfo(FILE* outFile, size_t max)
{
    size_t n = 0;
    // This function can't cause any heap allocation, be careful.
    Locker lock(_logInfoForErrorsMutex);
    for (Arch_LogInfoMap::const_iterator i = _logInfoForErrors.begin(),
             end = _logInfoForErrors.end(); i != end; ++i) {
        // We limit the # of errors printed to avoid spam.
        if (n++ >= max) {
            fprintf(
                outFile,
                "...more diagnostic information is in the stack trace file.\n");
            break;
        }
        fprintf(outFile, "%s:\n%s", i->first.c_str(), i->second);
    }
}

static void
_EmitAnyExtraLogInfo(char const *fname)
{
    // This function can't cause any heap allocation, be careful.
    if (FILE* outFile = fopen(fname, "a")) {
        _EmitAnyExtraLogInfo(outFile);
        fclose(outFile);
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

static time_t _GetAppElapsedTime();


// asgetenv() want's this but we can't declare it inside the namespace.
extern char **environ;


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
    if (not dst or not src) {
        return dst == src;
    }
    while (*dst or *src) {
        if (*dst++ != *src++) {
            return false;
        }
    }
    return true;
}

// Compare the strings for equality up to n characters.
bool asstrneq(const char* dst, const char* src, size_t n)
{
    if (not dst or not src) {
        return dst == src;
    }
    while ((*dst or *src) and n) {
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
        for (char** i = environ; *i; ++i) {
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
    int fd = open(buf, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0640);
    while (fd == -1 and errno == EEXIST) {
        // File exists.  Try a new suffix if there's space.
        ++suffix;
        if (len < required + 1 + asNumDigits(suffix)) {
            // No space.  Not quite an accurate error code.
            errno = ENOMEM;
            return -1;
        }
        asstrcpy(end, ".");
        asitoa(end + 1, suffix);
        fd = open(buf, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0640);
    }
    if (fd != -1) {
        close(fd);
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
    if (not cmd or not srcArgv) {
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

/* We use a 'non-locking' fork so that we won't get hung up if we've
 * had malloc corrupton when we crash.  The crash recovery behavior
 * can be tested with ArchTestCrash(), which should crash with this
 * malloc corruption.
 */
static int
nonLockingFork()
{
    if (Arch_nonLockingFork != NULL) {
	return (Arch_nonLockingFork)();
    } else {
	return fork();
    }
}

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
        "mov    %0, %%rdi	\n\t"
        "mov    %%rcx, %%rsi	\n\t"
        "mov    %%rdx, %%rdx	\n\t"
        "mov    $0x3b, %%rax	\n\t"
        "syscall		\n\t"
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

/* This is the corresponding execv which works with nonLockingFork().
 * currently, it's only different from execv for linux.  The crash
 * recovery behavior can be tested with ArchTestCrash().
 */
static int
nonLockingExecv( const char *path, char *const argv[])
{
#if defined(ARCH_OS_LINUX)
     return nonLockingLinux__execve (path, argv, __environ);
#else
     return execv(path, argv);
#endif
}

/*
 * Return the base of a filename.
 */

static const char*
getBase(const char* path)
{
    const char* base = strrchr(path, '/');
    if (!base)
	return path;

    base++;
    return strlen(base) > 0 ? base : path;
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
    if (not cmd) {
        cmd = stackTraceCmd;
    }
    if (not cmd or not stackTraceArgv) {
        // Silently do nothing.
        return 0;
    }

    // Construct the substitutions.
    char pidBuffer[numericBufferSize], timeBuffer[numericBufferSize];
    asitoa(pidBuffer, getpid());
    asitoa(timeBuffer, _GetAppElapsedTime());
    const char* const substitutions[3][2] = {
        "$pid", pidBuffer, "$log", logfile, "$time", timeBuffer
    };

    // Build the argument list.
    static constexpr size_t maxArgs = 32;
    const char* argv[maxArgs];
    if (not _MakeArgv(argv, maxArgs, cmd, stackTraceArgv, substitutions, 2)) {
        static const char msg[] = "Too many arguments to postmortem command\n";
        write(2, msg, sizeof(msg) - 1);
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
    Locker lock(_progInfoForErrorsMutex);

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
    
    Locker lock(_progInfoForErrorsMutex);

    Arch_ProgInfoMap::iterator iter = _progInfoMap.find(key);
    std::string result;
    if (iter != _progInfoMap.end())
        result = iter->second;

    return result;
} 

void
ArchSetExtraLogInfoForErrors(const std::string &key, char const *text)
{
    Locker lock(_logInfoForErrorsMutex);
    if (not text or not strlen(text)) {
        _logInfoForErrors.erase(key);
    } else {
        _logInfoForErrors[key] = text;
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
        _progNameForErrors = strdup(getBase(progName));
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

static time_t
_GetAppElapsedTime()
{
    rusage ru;

    // We only record the amount of time spent in user instructions,
    // so as to discount idle time when logging up time.
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        return time_t(ru.ru_utime.tv_sec);
    }

    // Fallback to logging the entire session time, if we could
    // not get the user time from the resource usage.

    // Note: Total time measurement will be a little off because this
    // calculation happens after the stack trace is generated which can
    // take a long time.
    //
    return time(0) - _appLaunchTime;
}

static void
_InvokeSessionLogger(const char* progname, const char *stackTrace)
{
    // Get the command to run.
    const char* cmd = asgetenv("ARCH_LOGSESSION");
    const char* const* srcArgv =
        stackTrace ? _sessionCrashLogArgv : _sessionLogArgv;
    if (not cmd) {
        cmd = _logStackToDbCmd;
    }
    if (not cmd or not srcArgv) {
        // Silently do nothing.
        return;
    }

    // Construct the substitutions.
    char pidBuffer[numericBufferSize], timeBuffer[numericBufferSize];
    asitoa(pidBuffer, getpid());
    asitoa(timeBuffer, _GetAppElapsedTime());
    const char* const substitutions[4][2] = {
        "$pid", pidBuffer, "$time", timeBuffer,
        "$prog", progname, "$stack", stackTrace
    };

    // Build the argument list.
    static constexpr size_t maxArgs = 32;
    const char* argv[maxArgs];
    if (not _MakeArgv(argv, maxArgs, cmd, srcArgv, substitutions, 4)) {
        static const char msg[] = "Too many arguments to log session command\n";
        write(2, msg, sizeof(msg) - 1);
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
	if (FILE* stackFd = fopen(stackTrace, "a")) {
	    if (FILE* sessionLogFd = fopen(sessionLog, "r")) {
		fprintf(stackFd,"\n\n********** Session Log **********\n\n");
		// Cat the sesion log
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
    _InvokeSessionLogger(ArchGetProgramNameForErrors(), crashStackTrace);
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
ArchLogPostMortem(const char* reason, const char* message /* = nullptr */)
{
    static std::atomic_flag busy = ATOMIC_FLAG_INIT;

    // Disallow recursion and allow only one thread at a time.
    while (busy.test_and_set(std::memory_order_acquire)) {
        // Spin!
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
        write(2, msg, sizeof(msg) - 1);
        busy.clear(std::memory_order_release);
        return;
    }

    // Write reason for stack trace to logfile.
    if (FILE* stackFd = fopen(logfile, "a")) {
        if (reason) {
            fprintf(stackFd, "This stack trace was requested because: %s\n",
                    reason);
        }
        if (message) {
            fprintf(stackFd, "%s\n", message);
        }
        fclose(stackFd);
    }

    /* get hostname for printing out in the error message only */
    char hostname[MAXHOSTNAMELEN];
    if (gethostname(hostname,MAXHOSTNAMELEN) != 0) {
        /* error getting hostname; don't try to print it */
        hostname[0] = '\0';
    }

    fprintf(stderr, "\n");
    fprintf(stderr,
	    "------------------------ '%s' is dying ------------------------\n",
	    progname);

    // print out any registered program info
    {
        Locker lock(_progInfoForErrorsMutex);
        if (_progInfoForErrors) {
            fprintf(stderr, "%s", _progInfoForErrors);
        }
    }

    if (reason) {
        fprintf(stderr, "This stack trace was requested because: %s\n", reason);
    }
    if (message) {
        fprintf(stderr, "%s\n", message);
    }
    fprintf(stderr, "The stack can be found in %s:%s\n", hostname, logfile);
    int loggedStack = _LogStackTraceForPid(logfile);
    fprintf(stderr, "done.\n");
    // Additionally, print the first few lines of extra log information since
    // developers don't always think to look for it in the stack trace file.
    _EmitAnyExtraLogInfo(stderr, 3 /* max */);
    fprintf(stderr,
	"------------------------------------------------------------------\n");

    if (loggedStack) {
        _EmitAnyExtraLogInfo(logfile);
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
        Locker lock(_progInfoForErrorsMutex);
        if (_progInfoForErrors) {
            fprintf(stderr, "%s", _progInfoForErrors);
        }
    }

    if (fd != -1) {
        FILE* fout = fdopen(fd, "w");
        fprintf(stderr, "The stack can be found in %s:%s\n"
                "--------------------------------------------------------------"
                "\n", hostname, tmpFile.c_str());
        ArchPrintStackTrace(fout, progname, reason);
        fclose(fout);
        /* If this is a fatal stack trace, attempt to add it to the db */
        if (fatal) {
            _EmitAnyExtraLogInfo(tmpFile.c_str());
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
    unlink(logfile);
}

#endif

/*
 * ArchPrintStackTrace
 *  print out a stack trace to the given FILE *.
 */
void
ArchPrintStackTrace(FILE *fout, const std::string& programName, const std::string& reason)
{
    ostringstream	oss;

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
 *  print out a stack trace to the given iostream.
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
                   &symbolName, &symbolAddress) and symbolAddress) {
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
 *  print out stack frames to the given iostream.
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
    vector<string>    rv;

#if !defined(ARCH_OS_LINUX)

    rv.push_back("No frames saved, stack traces not supported on this "
                 "architecture.");
    return rv;

#else

    if (frames.empty()) {
	rv.push_back("No frames saved, stack traces probably not supported.");
	return rv;
    }

    ArchStackTraceCallback callback = *Arch_GetStackTraceCallback();
    if (not callback) {
        callback = Arch_DefaultStackTraceCallback;
    }
    for (size_t i = 0; i < frames.size(); i++) {
        const std::string symbolic = callback(frames[i]);
        rv.push_back(ArchStringPrintf(" #%-3i 0x%016lx in %s",
                                      (int)i, frames[i], symbolic.c_str()));
    }

#endif

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
    struct sigaction act, oldact;
    int retval = 0;
    int savedErrno;
    pid_t pid = nonLockingFork(); /* use non-locking fork */
    if (pid == -1) {
        /* fork() failed */
        fprintf(stderr, "FAIL: Unable to fork() crash handler\n");
        return -1;
    }
    else if (pid == 0) {
        // Call setsid() in the child, which is intended to start a new
        // "session", and detach from the controlling tty.  We do this because
        // the stack tracing stuff invokes gdb, which wants to fiddle with the
        // tty, and if we're run in the background, that blocks, so we hang
        // trying to take the stacktrace.  This seems to fix that.
        setsid();
        nonLockingExecv(pathname, argv);  /* use non-locking execv */
        /* exec() failed */
        fprintf(stderr, "FAIL: Unable to exec() crash handler %s: %s\n",
                pathname, strerror(errno));
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
                    if (WEXITSTATUS(status) == 127)
                        errno = ENOENT;
                    retval = WEXITSTATUS(status);
		    goto out;
                }

                if (WIFSIGNALED(status)) {
                    /* child died due to uncaught signal */
                    errno = EINTR;
		    retval = -1;
                    goto out;
                }
                /* child died for an unknown reason */
                errno = EINTR;
		retval = -1;
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
    }

  out:
    savedErrno = errno;
    alarm(0);
    sigaction(SIGALRM, &oldact, NULL);

    errno = savedErrno;
    return retval;
}

/* test thread for crashing (loops forever.) */
static void *
arch_athread(void *)
{
    while (1) {} /* loop forever */
    pthread_exit((void *) 0);
#if defined(ARCH_OS_DARWIN)
    return (void*)0;
#endif
}

/*
 * ArchTestCrash
 *     causes the calling program to crash by doing bad malloc things, so
 *     that crash handling behavior can be tested.  If 'spawnthread'
 *     is true, it spawns a thread which is alive during the crash.  If the
 *     program fails to crash, this aborts (since memory will be trashed.)
 */
void
ArchTestCrash(bool spawnthread)
{
    pthread_t tid;
    char *overwrite, *another;

    if (spawnthread) {
	pthread_create(&tid, NULL, arch_athread, NULL);
    }

    for (size_t i = 0; i < 15; ++i) {
	overwrite = (char *)malloc(2);
	another = (char *)malloc(7);

#define STRING "this is a long string, which will overwrite a lot of memory"
	for (size_t j = 0; j <= i; ++j)
	    strcpy(overwrite + (j * sizeof(STRING)), STRING);
	cerr << "succeeded in overwriting buffer with sprintf\n";

	free(another);
	cerr << "succeeded in freeing another allocated buffer\n";

	another = (char *)malloc(7);
	cerr << "succeeded in allocating another buffer after overwrite\n";

	another = (char *)malloc(13);
	cerr << "succeeded in allocating a second buffer after overwrite\n";

	another = (char *)malloc(7);
	cerr << "succeeded in allocating a third buffer after overwrite\n";

	free(overwrite);
	cerr << "succeeded in freeing overwritten buffer\n";
	free(overwrite);
	cerr << "succeeded in freeing overwrite AGAIN\n";
    }

    // Added this to get the test to crash with SmartHeap.  
    overwrite = (char *)malloc(1);
    for (size_t i = 0; i < 1000000; ++i)
	overwrite[i] = ' ';

    // Boy, darwin just doesn't want to crash: ok, handle *this*...
    for (size_t i = 0; i < 128000; i++) {
	char* ptr = (char*) malloc(i);
	free(ptr + i);
	free(ptr - i);
	free(ptr);
    }

    fprintf(stderr,"FAILED to crash! Aborting.\n");
    abort();
}
