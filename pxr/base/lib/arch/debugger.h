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
#ifndef ARCH_DEBUGGER_H
#define ARCH_DEBUGGER_H

/// \file arch/debugger.h
/// Routines for interacting with a debugger.

#include "pxr/base/arch/attributes.h"

/// Stop in a debugger.
///
/// This function will do one of the following:  start a debugger
/// attached to this process stopped on this function;  stop in an
/// already attached debugger;  stop and wait for a debugger to
/// attach, or nothing.
///
/// On Linux this will start a debugger using \c ArchDebuggerAttach()
/// if no debugger is attached.  If a debugger is (or was) attached it
/// will stop on this function due to \c SIGTRAP.  Alternatively, users
/// can configure the debugger to not stop on \c SIGTRAP and instead
/// break on \c ArchDebuggerTrap().
///
/// If a debugger is not attached, \c ArchDebuggerAttach() does not
/// attach one, and \c ArchDebuggerWait() has been most recently
/// called with \c true then this will wait for a debugger to attach,
/// otherwise it does nothing and the process does not stop.  The user
/// can continue the process from the debugger simply by issuing the
/// continue command.  The user can also continue the process from an
/// attached terminal by putting the process into the foreground or
/// background.
/// 
void ArchDebuggerTrap() ARCH_NOINLINE;

/// Cause debug traps to wait for the debugger or not.
///
/// When \p wait is \c true the next call to \c ArchDebuggerTrap()
/// will cause the process to wait for a signal.  The user can attach
/// a debugger to continue the process.  The process will not wait
/// again until another call to this function with \p wait \c true.
/// 
void ArchDebuggerWait(bool wait);

/// Attach a debugger.
///
/// Attaches the debugger by running the contents of the enviroment variable
/// ARCH_DEBUGGER using /bin/sh.  Any '%p' in the contents of this variable
/// will be replaced with the process id of the process launching the debugger.
/// Any '%e' will be replaced with the path to the executable for the process.
///
/// Returns true if ARCH_DEBUGGER is set and the debugger was successfully
/// launched, otherwise returns false.
bool ArchDebuggerAttach() ARCH_NOINLINE;

/// Test if a debugger is attached
///
/// Attempts to detect if a debugger is currently attached to the process.
bool ArchDebuggerIsAttached() ARCH_NOINLINE;

/// Stop in the debugger.
///
/// This macro expands to \c ArchDebuggerTrap() and, if necessary and
/// possible, code to prevent optimization so the caller appears in the
/// debugger's stack trace.  The calling functions should also use the
/// \c ARCH_NOINLINE function attribute.
#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)
#define ARCH_DEBUGGER_TRAP do { ArchDebuggerTrap(); asm(""); } while (0)
#else
#define ARCH_DEBUGGER_TRAP do { ArchDebuggerTrap(); } while (0)
#endif

#endif // ARCH_DEBUGGER_H
