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
#ifndef ARCH_DAEMON_H
#define ARCH_DAEMON_H

/// \file arch/daemon.h
/// \ingroup group_arch_Multithreading
/// Create background or daemon processes.

/// Close all file descriptors (with possible exceptions)
///
/// \c ArchCloseAllFiles will close all file descriptors open in the
/// current process.  Occasionally you'd like to close all files except
/// for some small subset (like 0, 1, and 2).  The \p nExcept and \p
/// exceptFds arguments can be used to provide the list of exceptions.
/// \c ArchDaemonizeProcess uses this method to close all unwanted file
/// descriptors in the daemon process.
///
/// \p nExcept should be the number of elements in the \p exceptFds array.
/// Invalid file descriptors in exceptFds are ignored.
///
/// \note Be \b very careful when using this routine.  It is intended
/// to be used after a \c fork(2) call to close \b all unwanted file
/// descriptors.  However, it does not flush stdio buffers, wait for
/// processes opened with popen, shut down the X11 display connection,
/// or anything.  It just slams closed all the file descriptors.  This
/// is appropriate following a \c fork(2) call as all these file
/// descriptors are duplicates of the ones in the parent process and
/// shutting down the X11 display connection would mess up the parent's
/// X11 display.  But you shouldn't use \c ArchCloseAllFiles unless you
/// know what you are doing.
///
/// \return -1 on error and \c errno will be set to an appropriate
/// value.  Returns 0 on success.
///
/// \ingroup group_arch_Multithreading
int ArchCloseAllFiles(int nExcept, const int* exceptFds);

#endif // ARCH_DAEMON_H 
