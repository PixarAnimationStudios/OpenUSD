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
#include "pxr/base/arch/daemon.h"
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

// Fork the current process and close all undesired file descriptors.
//
int
ArchCloseAllFiles(int nExcept, const int* exceptFds)
{
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)

    int status, retStatus, retErrno;
    int i, j, maxfd, maxExcept = -1;
    struct rlimit limits;

    // Figure out how many file descriptors there are.
    //
    status = getrlimit(RLIMIT_NOFILE, &limits);

    if (limits.rlim_cur == RLIM_INFINITY)
    {
        maxfd = NOFILE;
    }
    else
    {
        maxfd = (int)limits.rlim_cur;
    }

    // Figure out the largest file descriptor in exceptFds.
    for (i = 0; i < nExcept; ++i) {
        if (maxExcept < exceptFds[i]) {
            maxExcept = exceptFds[i];
        }
    }

    retStatus = 0;
    retErrno  = 0;

    for (i = 0; i < maxfd; ++i)
    {
        // Check if we should skip this file descriptor.
        // XXX -- This is slow for large maxfd and nExcept but nExcept is
        //        never large in our use cases.  We could copy and sort
        //        exceptFds if we think it might get big but we should
        //        avoid using the heap because we might get called from
        //        precarious situations, e.g. signal handlers.
        if (i <= maxExcept) {
            for (j = 0; j != nExcept; ++j) {
                if (exceptFds[j] == i) {
                    break;
                }
            }
            if (j != nExcept) {
                // File descriptor is in exceptFds
                continue;
            }
        }

        do {
            // Close the file, repeat if interrupted.
            //
            errno = 0;
            status = close(i);
        } while (status != 0 && errno == EINTR);

        if (status != 0 &&
            errno  != EBADF)
        {
            // We got some real error.  Remember it but keep going.
            //
            retStatus = status;
            retErrno  = errno;
        }
    }

    // Restore errno to the last encountered real error.  In
    // particular this will clear out any EBADF value left over from
    // the loop above.
    //
    errno = retErrno;

    return retStatus;

#else

    // Not supported
    errno = EINVAL;
    return -1;

#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
