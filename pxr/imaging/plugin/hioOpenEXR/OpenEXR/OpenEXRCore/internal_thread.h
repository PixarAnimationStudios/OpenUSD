/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_THREAD_H
#define OPENEXR_PRIVATE_THREAD_H

#include "openexr_config.h"

#if ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
/*
 * On Windows, use native InitOnceExecuteOnce, which avoids MSVC's
 * problematic <threads.h>
 */
#        include <windows.h>
#        define ONCE_FLAG_INIT INIT_ONCE_STATIC_INIT
typedef INIT_ONCE once_flag;
static BOOL CALLBACK
once_init_fn (PINIT_ONCE once, PVOID param, PVOID* ctx)
{
    void (*fn) (void) = (void (*) (void)) param;
    fn ();
    return TRUE;
}
static inline void
call_once (once_flag* flag, void (*func) (void))
{
    InitOnceExecuteOnce (flag, once_init_fn, (PVOID) func, NULL);
}
#    elif __has_include(<threads.h>) && !defined(__FreeBSD__)
/*
 * On Linux (glibc 2.28+), use standard <threads.h>.
 * FreeBSD requires -lstdthreads for <threads.h>; use pthread fallback instead.
 */
#        include <threads.h>

#    else
/*
 * No <threads.h> on macOS and older Linux distros: fall back to pthreads
 */
#        include <pthread.h>
#        define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
typedef pthread_once_t once_flag;
static inline void
call_once (once_flag* flag, void (*func) (void))
{
    (void) pthread_once (flag, func);
}

#    endif

#endif /* ILMTHREAD_THREADING_ENABLED */

/*
 * If threading is disabled, or call_once/ONCE_FLAG_INIT wasn't declared
 * above, declare a default implementation.
 */
#ifndef ONCE_FLAG_INIT
#  define ONCE_FLAG_INIT 0
typedef int once_flag;
static inline void
call_once (once_flag* flag, void (*func) (void))
{
    if (!*flag) {
        *flag = 1;
        func ();
    }
}
#endif


#endif /* OPENEXR_PRIVATE_THREAD_H */
