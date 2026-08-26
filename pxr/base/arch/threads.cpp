//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/threads.h"

#include <cstring>
#include <thread>

#if defined(ARCH_OS_LINUX)
#  include <cerrno>
#  include <pthread.h>
#  include <sched.h>
#  include <sys/resource.h>
#endif

#if defined(ARCH_OS_DARWIN)
#  include <pthread.h>
#  include <sys/qos.h>
#endif

#if defined(ARCH_OS_WINDOWS)
#  include <Windows.h>
#endif

#if defined(ARCH_OS_WASM_VM)
#  include <emscripten/threading.h>
#  include <pthread.h>
#endif

// emscripten_set_thread_name() records a name only when the program is built
// with --threadprofiler, which defines PTHREADS_PROFILING; it is documented as
// a no-op otherwise.  Gate naming on that so we never report success for a call
// that nothing records.
#if defined(ARCH_OS_WASM_VM) && \
    defined(PTHREADS_PROFILING) && PTHREADS_PROFILING
#define ARCH_WASM_THREAD_NAMING
#endif

PXR_NAMESPACE_OPEN_SCOPE

// Static initializer to get the main thread id.  We want this to run as early
// as possible, so we actually capture the main thread's id.  We assume that
// we're not starting threads before main().

namespace {

const std::thread::id _mainThreadId = std::this_thread::get_id();

#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN) || \
    defined(ARCH_WASM_THREAD_NAMING)

// Every naming API except Windows imposes a byte limit on the name.  Linux and
// MacOS reject an over-long name outright; Emscripten cuts one on a byte
// boundary, which can split a multi-byte character.  In all three cases we
// truncate first, on a character boundary, so the result stays valid UTF-8.
#  if defined(ARCH_OS_LINUX)
// The kernel enforces TASK_COMM_LEN (16), including the null terminator.
constexpr size_t _maxThreadNameLen = 15;
#  elif defined(ARCH_OS_DARWIN)
// The kernel enforces MAXTHREADNAMESIZE (64), including the null terminator.
constexpr size_t _maxThreadNameLen = 63;
#  else
// emscripten_set_thread_name() documents a 32-byte limit but not whether that
// counts the null terminator.  Use 31, which is within the limit on either
// reading: handing it 32 bytes only for it to keep 31 could split the very
// multi-byte character we took care to preserve.  Losing a byte off an advisory
// label costs nothing by comparison.
constexpr size_t _maxThreadNameLen = 31;
#  endif

// Truncate 'name' to at most maxBytes bytes, respecting UTF-8 character
// boundaries.
//
// Writes the result into buf (which must be at least maxBytes+1 bytes) and
// returns buf.  If the name fits, returns name directly without copying.
//
// If 'name' is not valid UTF-8 this can cut more than strictly necessary: up to
// three bytes, or the entire name given a long enough run of continuation
// bytes.  An advisory label does not warrant more care than that.
char const *
_TruncateUtf8(char const *name, char *buf, size_t maxBytes)
{
    size_t len = strlen(name);
    if (len <= maxBytes) {
        return name;
    }
    // Walk backwards from maxBytes until we find a byte that is not a
    // UTF-8 continuation byte (10xxxxxx), so we don't split a multi-byte
    // sequence.
    size_t cut = maxBytes;
    while (cut > 0 && (static_cast<unsigned char>(name[cut]) & 0xC0) == 0x80) {
        --cut;
    }
    memcpy(buf, name, cut);
    buf[cut] = '\0';
    return buf;
}
#endif

#if defined(ARCH_OS_WINDOWS)
// Convert a non-empty UTF-8 string to a wide string for the Win32
// thread-naming API.  Returns an empty wstring on conversion failure -- callers
// must handle an empty input themselves, since this cannot distinguish the two
// cases.
std::wstring
_Utf8ToWide(char const *name)
{
    if (!name || name[0] == '\0') {
        return {};
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    if (len <= 0) {
        return {};
    }
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, name, -1, result.data(), len);
    return result;
}

// Convert a null-terminated wide string to UTF-8.
std::string
_WideToUtf8(PCWSTR wname)
{
    if (!wname || wname[0] == L'\0') {
        return {};
    }
    int len = WideCharToMultiByte(
        CP_UTF8, 0, wname, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return {};
    }
    std::string result(len - 1, '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, wname, -1, result.data(), len, nullptr, nullptr);
    return result;
}
#endif

} // anonymous namespace

bool ArchIsMainThread()
{
    return std::this_thread::get_id() == _mainThreadId;
}

std::thread::id
ArchGetMainThreadId()
{
    return _mainThreadId;
}

bool
ArchSetThisThreadName(char const *name)
{
    if (!name) {
        return false;
    }

#if defined(ARCH_OS_LINUX)
    char buf[_maxThreadNameLen + 1];
    return pthread_setname_np(
        pthread_self(), _TruncateUtf8(name, buf, _maxThreadNameLen)) == 0;

#elif defined(ARCH_OS_DARWIN)
    char buf[_maxThreadNameLen + 1];
    return pthread_setname_np(_TruncateUtf8(name, buf, _maxThreadNameLen)) == 0;

#elif defined(ARCH_OS_WINDOWS)
    // An empty name is valid -- it clears the thread description -- but
    // _Utf8ToWide() cannot represent it distinctly from a conversion failure,
    // so handle it here.
    if (name[0] == '\0') {
        return SUCCEEDED(SetThreadDescription(GetCurrentThread(), L""));
    }
    std::wstring wname = _Utf8ToWide(name);
    if (wname.empty()) {
        // Conversion failed.
        return false;
    }
    return SUCCEEDED(SetThreadDescription(GetCurrentThread(), wname.c_str()));

#elif defined(ARCH_WASM_THREAD_NAMING)
    // Returns void, and the surrounding #if guarantees the thread profiler is
    // present to record the name, so there is nothing to report but success.
    char buf[_maxThreadNameLen + 1];
    emscripten_set_thread_name(
        pthread_self(), _TruncateUtf8(name, buf, _maxThreadNameLen));
    return true;

#else
    // No thread naming facility.
    return false;
#endif
}

std::string
ArchGetThisThreadName()
{
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    // ArchSetThisThreadName() truncates to _maxThreadNameLen, and so does the
    // kernel, so this buffer cannot be too small.
    char buf[_maxThreadNameLen + 1] = {};
    if (pthread_getname_np(pthread_self(), buf, sizeof(buf)) != 0) {
        return {};
    }
    return buf;

#elif defined(ARCH_OS_WINDOWS)
    PWSTR wname = nullptr;
    if (FAILED(GetThreadDescription(GetCurrentThread(), &wname)) ||
            wname == nullptr) {
        return {};
    }
    std::string result = _WideToUtf8(wname);
    LocalFree(wname);
    return result;

#else
    // No way to read a thread name back.  WASM is deliberately not handled:
    // Emscripten offers emscripten_set_thread_name() but no corresponding get,
    // so a name set there cannot be retrieved even when ArchSetThisThreadName()
    // succeeded.
    return {};
#endif
}

bool
ArchSetThisThreadPriority(ArchThreadPriority priority)
{
#if defined(ARCH_OS_LINUX)

    int policy;
    struct sched_param param;
    if ((pthread_getschedparam(pthread_self(), &policy, &param) != 0) ||
        (policy != SCHED_OTHER)) {
        // Error, or we have an exotic scheduling policy.  The nice value has no
        // effect under the real-time policies (SCHED_FIFO, SCHED_RR).
        return false;
    }

    // nice=10 for low priority follows the Linux 'nice' utility's default.
    int targetNice = (priority == ArchThreadPriorityLowest) ? 19 : 10;

    // NOTE: this deliberately relies on Linux-specific behavior.  POSIX
    // specifies the nice value as a per-process attribute, but under Linux/NPTL
    // it is per-thread, and PRIO_PROCESS with who=0 means the calling thread
    // rather than the whole process.  See NOTES in setpriority(2).  Do not
    // "correct" this to a per-process reading -- de-prioritizing the caller's
    // entire process is not what this API promises.

    // getpriority() returns -1 on error, but -1 is also a valid nice value, so
    // check errno explicitly.
    errno = 0;
    int nice = getpriority(PRIO_PROCESS, 0);
    if ((nice == -1 && errno != 0) || nice >= targetNice) {
        // Error, or priority already lower than target.
        return false;
    }
    return setpriority(PRIO_PROCESS, 0, targetNice) == 0;

#elif defined(ARCH_OS_DARWIN)
    qos_class_t targetQos = (priority == ArchThreadPriorityLowest)
        ? QOS_CLASS_BACKGROUND : QOS_CLASS_UTILITY;

    // QOS_CLASS_UNSPECIFIED (0) means either that no QoS has been set or that
    // the query failed and left our initializer in place.  Treat both as
    // "unknown, proceed with the set".
    qos_class_t currentQos = QOS_CLASS_UNSPECIFIED;
    int relative = 0;
    pthread_get_qos_class_np(pthread_self(), &currentQos, &relative);

    if (currentQos != QOS_CLASS_UNSPECIFIED && currentQos <= targetQos) {
        return false;
    }
    return pthread_set_qos_class_self_np(targetQos, 0) == 0;

#elif defined(ARCH_OS_WINDOWS)
    int target = (priority == ArchThreadPriorityLowest)
        ? THREAD_PRIORITY_IDLE
        : THREAD_PRIORITY_BELOW_NORMAL;

    int current = GetThreadPriority(GetCurrentThread());
    if (current == THREAD_PRIORITY_ERROR_RETURN) {
        return false;
    }
    if (current <= target) {
        return false;
    }
    return SetThreadPriority(GetCurrentThread(), target) != 0;

#else
    // No thread priority facility.  On WASM in particular, threads are Web
    // Workers, which expose no priority control at all.
    (void)priority;
    return false;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
