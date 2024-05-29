//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_DAEMON_H
#define PXR_BASE_ARCH_DAEMON_H

/// \file arch/daemon.h
/// \ingroup group_arch_Multithreading
/// Create background or daemon processes.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"

PXR_NAMESPACE_OPEN_SCOPE

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
ARCH_API 
int ArchCloseAllFiles(int nExcept, const int* exceptFds);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_DAEMON_H 
