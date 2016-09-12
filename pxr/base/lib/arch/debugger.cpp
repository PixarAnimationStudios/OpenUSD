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
/// \file debugger.cpp

#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/daemon.h"
#include "pxr/base/arch/env.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/systemInfo.h"
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
#include "pxr/base/arch/inttypes.h"
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#elif defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <ciso646>
#endif
#include <atomic>

// We don't want this inlined so ArchDebuggerTrap() is as clean as
// possible.  The fewer instructions in that function, the more likely
// we don't confuse the debugger's stack unwinding.
static void Arch_DebuggerInit() ARCH_NOINLINE;

static bool _archDebuggerInitialized = false;
static bool _archDebuggerEnabled = false;
static std::atomic<bool> _archDebuggerWait(false);

static char** _archDebuggerAttachArgs = 0;

#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
static
void
Arch_DebuggerTrapHandler(int)
{
    // If we're not configured to wait then do nothing.  Otherwise
    // reconfigure to not wait the next time then wait for the
    // debugger to continue us.
    bool oldVal = true;
    if (_archDebuggerWait.compare_exchange_strong(oldVal, false)) {
        raise(SIGSTOP);
    }
}
#endif

#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
static
void
Arch_DebuggerInitPosix()
{
    _archDebuggerInitialized = true;

    // Handle the SIGTRAP signal so if no debugger is attached then
    // nothing happens when ArchDebuggerTrap() is called.  If we
    // didn't handle this signal then the app would die.
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags   = SA_NODEFER;
    act.sa_handler = Arch_DebuggerTrapHandler;
    if (sigaction(SIGTRAP, &act, 0)) {
        ARCH_WARNING("Failed to set SIGTRAP handler;  "
                     "debug trap not enabled");
        _archDebuggerEnabled = false;
    }
    else {
        _archDebuggerEnabled = true;
    }
}
namespace {
struct InitPosix {
    InitPosix() { Arch_DebuggerInitPosix(); }
};
}
#endif

static
void
Arch_DebuggerInit()
{
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
#if defined(ARCH_CPU_INTEL) && defined(ARCH_BITS_64)
    // Save some registers that normally don't have to be preserved.  We
    // do this so the caller of ArchDebuggerTrap() can see its arguments
    // that were passed in registers.  If that function doesn't use those
    // arguments after calling ArchDebuggerTrap() then the compiler is
    // free to not store them anywhere and, if we clobber their values
    // here, there's nowhere the debugger can look to get them.
    uint64_t rdi, rsi, rdx, rcx;
    asm volatile(
        "movq %%rdi, %[rdi];\n"
        "movq %%rsi, %[rsi];\n"
        "movq %%rdx, %[rdx];\n"
        "movq %%rcx, %[rcx];\n"
        : [rdi] "=m" (rdi), [rsi] "=m" (rsi), [rdx] "=m" (rdx), [rcx] "=m" (rcx)
        : // input
        : // clobbered
        );
#endif

    // Initialize once.
    static InitPosix initPosix;

#if defined(ARCH_CPU_INTEL) && defined(ARCH_BITS_64)
    // Restore the saved registers.
    asm volatile(
        "movq %[rdi], %%rdi;\n"
        "movq %[rsi], %%rsi;\n"
        "movq %[rdx], %%rdx;\n"
        "movq %[rcx], %%rcx;\n"
        : // output
        : [rdi] "m" (rdi), [rsi] "m" (rsi), [rdx] "m" (rdx), [rcx] "m" (rcx)
        : // clobbered
        );
#endif

#elif defined(ARCH_OS_WINDOWS)

    _archDebuggerInitialized = true;
    _archDebuggerEnabled = true;

#endif
}


#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
// Use a 'non-locking' fork so that we won't get hung up if we've
// had malloc corruption.  We can't prevent fork() from using the
// heap, unfortunately, since fork handlers can do whatever they
// want.  May want to use clone() on Linux or something similar.
static int
nonLockingFork()
{
    typedef int (*ForkFunc)(void);
    extern ForkFunc Arch_nonLockingFork;

    if (Arch_nonLockingFork != NULL) {
	return (Arch_nonLockingFork)();
    } else {
	return fork();
    }
}

// This is like fork() except the new process will have the init process as
// its parent and it will not and cannot have a controlling terminal.  We
// go through a fairly typical daemonize process except we don't exit the
// original process and we wait for the callback in the new process to exec
// or return.
bool
Arch_DebuggerRunUnrelatedProcessPosix(bool (*cb)(void*), void* data)
{
    // Do *not* use the heap in here.  Avoid using any functions except
    // system calls.

    // Open a pipe for communicating success failure from descendant
    // processes.
    int ready[2];
    if (pipe(ready) == -1) {
        return false;
    }

    // Fork so the child is not a process group leader and is in the
    // background.
    pid_t pid = nonLockingFork();
    if (pid == -1) {
	// fork failed!
        close(ready[0]);
        close(ready[1]);
	return false;
    }

    if (pid > 0) {
	// This is the parent.
        close(ready[1]);

	// Wait for the descendant to report status. We could collect the
        // entire result but we only care if we get any data or not.
        int result;
        ssize_t n = read(ready[0], &result, 1);
        while (n == -1) {
            n = read(ready[0], &result, 1);
        }

        // Done with pipe.
        close(ready[0]);

        // Success if descendant sent no data at all.
	return n == 0;
    }

    // This is the child.  Do *not* call exit() from here down.  We must
    // call _exit() to avoid running any atexit handlers.
    close(ready[0]);

    // Ensure that we cannot be stopped by ^Z
#if defined(SIGTTOU)
    signal(SIGTTOU, SIG_IGN);
#endif /* SIGTTOU */
#if defined(SIGTTIN)
    signal(SIGTTIN, SIG_IGN);
#endif /* SIGTTIN */
#if defined(SIGTTSTP)
    signal(SIGTTSTP, SIG_IGN);
#endif /* SIGTTSTP */

    // Create a new session and set this process as the process group
    // leader.  In addition, the process has no controlling terminal.
    if (setsid() == -1) {
        int result = errno;
        write(ready[1], &result, sizeof(result));
	_exit(1);
    }

    // Now we need to ensure that this process does not reacquire a
    // controlling terminal.  Typically, the first terminal opened by
    // a process that is the process group leader will become the
    // controlling terminal and will be inherited by all other processes
    // in the process group.  So we fork yet again to ensure that the
    // new child process is not the group leader.  Now it can never
    // acquire a controlling terminal.
    //
    // When the process group leader process exits, all other processes
    // in the group get a SIGHUP.  Since we don't want to die when our
    // newly created process group leader exits, we have to ignore SIGHUP.
    //
    // Note: BSD systems use ioctl(fd, TIOCNOTTY, NULL) to disable
    // controlling terminals, but Mac OSX appears to have a working
    // setsid() call so I've used it.
    //
    signal(SIGHUP, SIG_IGN);

    pid = nonLockingFork();
    if (pid == -1) {
	// fork failed!
        int result = errno;
        write(ready[1], &result, sizeof(result));
	_exit(2);
    }
    if (pid > 0) {
	// This is the parent.
	_exit(0);
    }

    // Now we are in the grandchild process.  We are not a process group
    // leader, a session leader, and we have no controlling terminal.

    // Close all open file descriptors
    int result = ArchCloseAllFiles(1, &ready[1]);
    if (result == -1) {
        write(ready[1], &result, sizeof(result));
	_exit(3);
    }

    // Change directory to root to make sure that we are not on
    // a mounted file system.  We especially want to avoid being
    // in automounted file systems which may try to go away.
    //
    result = chdir("/");
    if (result == -1) {
        write(ready[1], &result, sizeof(result));
	_exit(4);
    }

    // Clear any inherited umask.
    umask(0);

    // Open stdin, stdout, stderr.
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);

    // Arrange for the ready pipe to close on exec.
    long arg = FD_CLOEXEC;
    if (fcntl(ready[1], F_SETFD, arg) == -1) {
        // We can't close on exec so we can't indicate success of exec.
        int result = errno;
        write(ready[1], &result, sizeof(result));
        _exit(5);
    }

    // Invoke callback.  If this calls execve() then ready[1] will close
    // automatically without us writing to it, indicating success.
    if (not cb(data)) {
        result = errno;
        write(ready[1], &result, sizeof(result));
	_exit(6);
    }

    // Success
    _exit(0);
}


extern char** environ;


static
bool
Arch_DebuggerAttachExecPosix(void* data)
{
    char** args = (char**)data;
    execve(args[0], args, environ);
    return false;
}

static
bool
Arch_DebuggerIsAttachedPosix()
{
#if defined(ARCH_OS_DARWIN)
    const int PTRACE_ATTACH = PT_ATTACHEXC;
    const int PTRACE_DETACH = PT_DETACH;
#endif

    // Check for a ptrace based debugger by trying to ptrace.
    pid_t parent = getpid();
    pid_t pid = nonLockingFork();
    if (pid < 0) {
        // fork failed.  We'll guess there's no debugger.
        return false;
    }

    // Child process.
    if (pid == 0) {
        // Attach to the parent with ptrace() this will fail if the
        // parent is already being traced.
        if (ptrace(PTRACE_ATTACH, parent, NULL, NULL) == -1) {
            // A debugger is probably attached if the error is EPERM.
            _exit(errno == EPERM ? 1 : 0);
        }

        // Wait for the parent to stop as a result of the attach.
        int status;
        while (waitpid(parent, &status, 0) == -1 and errno == EINTR) {
            // Do nothing
        }

        // Detach and continue the parent.
        ptrace(PTRACE_DETACH, parent, 0, SIGCONT);

        // A debugger was not attached.
        _exit(0);
    }

    // Parent process
    int status;
    while (waitpid(pid, &status, 0) == -1 and errno == EINTR) {
        // Do nothing
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status) != 0;
    }
    return false;

}

#endif // defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)

static
bool
Arch_DebuggerAttach()
{
    // Be very careful here to avoid using the heap.  We're not even sure
    // the stack is available but there's only so much we can do about that.

#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)

    // To attach to gdb under Unix/Linux and Gnome:
    //   ARCH_DEBUGGER="gnome-terminal -e 'gdb -p %p'"
    //
    // To attach to TotalView:
    //   ARCH_DEBUGGER="totalview -pid %p %e"
    //
    // You can alternatively use:
    //   ARCH_DEBUGGER="totalview -e 'dset TV::dll_read_loader_symbols_only *' -pid %p %e"
    // to prevent TotalView from loading all debug symbols immediately.
    // You can achieve the same in gdb using the .gdbinit file or another
    // file with the -x option.
    //
    // Note that, as of this writing, TotalView does not notice stops due to
    // ArchDebuggerTrap() and there appears to be no way to fix that.  The
    // debugged program, however, does stop so users simply need to click
    // TotalView's pause button to see the program state.  Unfortunately,
    // there's no obvious indication that the program has stopped.

    if (_archDebuggerAttachArgs) {
        // We need to start a process unrelated to this process so the
        // debugger's parent process is init, not this process (you can't
        // debug an ancestor process).
        if (Arch_DebuggerRunUnrelatedProcessPosix(
                    Arch_DebuggerAttachExecPosix, _archDebuggerAttachArgs)) {
            // Give the debugger a chance to attach.  We have no way of
            // blocking to wait for that and we can't be sure the client
            // is even going to start a debugger so we simply sleep for
            // what we hope is long enough.
            sleep(5);
            return true;
        }
    }

#elif defined(ARCH_OS_WINDOWS)
    DebugBreak();
#endif

    return false;
}


// Do initialization now that would require heap/stack when attaching.
ARCH_HIDDEN
void
Arch_InitDebuggerAttach()
{
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    // Maximum length of a pid written as a decimal.  It's okay for this
    // to be greater than that.
    static const size_t _decimalPidLength = 20;

    // Parse the contents of the ARCH_DEBUGGER environment variable and
    // store the result.  This way we can avoid using the heap or tricky
    // stack allocations when launching the debugger.
    const char* e = ArchGetEnv("ARCH_DEBUGGER").c_str();
    if (e and e[0]) {
        std::string link;

        // Compute the length of the string.
        size_t n = 0;
        for (char* i = e; *i; ++i) {
            if (i[0] == '%' and i[1] == 'p') {
                n += _decimalPidLength;
                ++i;
            }
            else if (i[0] == '%' and i[1] == 'e') {
                // Get the symlink in the proc filesystem if we haven't
                // yet.
                if (link.empty()) {
                    link = ArchGetExecutablePath();
                }

                n += link.size();
                ++i;
            }
            else {
                ++n;
            }
        }

        // Build the argv array.
        _archDebuggerAttachArgs = (char**)malloc(4 * sizeof(char*));
        _archDebuggerAttachArgs[0] = strdup("/bin/sh");
        _archDebuggerAttachArgs[1] = strdup("-c");
        _archDebuggerAttachArgs[2] = (char*)malloc(n + 1);
        _archDebuggerAttachArgs[3] = NULL;

        // Build the command string.
        char* a = _archDebuggerAttachArgs[2];
        for (char* i = e; *i; ++i) {
            if (i[0] == '%' and i[1] == 'p') {
                // Write the process id.
                sprintf(a, "%d", (int)getpid());

                // Skip past the written process id.
                while (*a) {
                    ++a;
                }

                // Skip over the '%p'.
                ++i;
            }
            else if (i[0] == '%' and i[1] == 'e') {
                // Write the process id.
                strcat(a, link.c_str());

                // Skip past the written process path.
                a += link.size();

                // Skip over the '%e'.
                ++i;
            }
            else {
                // Copy the character.
                *a++ = *i;
            }
        }

        // Terminate the command string.
        *a = '\0';
    }
#endif
}

void
ArchDebuggerTrap()
{
    Arch_DebuggerInit();
    if (_archDebuggerEnabled) {
#if defined(ARCH_OS_WINDOWS)
        if (IsDebuggerPresent()) {
            DebugBreak();
        }
#elif defined(ARCH_CPU_INTEL) && \
      (defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG))
        // Send a trap if a debugger is attached or we fail to start one.
        // If we start one we assume it will automatically stop this process.
        if (Arch_DebuggerIsAttachedPosix() or not Arch_DebuggerAttach()) {
            asm("int $3");
        }
#endif
    }
}

void
ArchDebuggerWait(bool wait)
{
    _archDebuggerWait = wait;
}

bool
ArchDebuggerAttach()
{
    return 
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
		Arch_DebuggerIsAttachedPosix() or 
#endif
		Arch_DebuggerAttach();
}

bool
ArchDebuggerIsAttached()
{
    Arch_DebuggerInit();
#if defined(ARCH_OS_WINDOWS)
    return IsDebuggerPresent() == TRUE;
#elif defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    return Arch_DebuggerIsAttachedPosix();
#endif
    return false;
}
