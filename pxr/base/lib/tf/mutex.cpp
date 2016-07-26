#include "pxr/base/tf/mutex.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"
#include <stddef.h>
#include <errno.h>

using namespace std;



static pthread_mutexattr_t Tf_recursiveMutexAttr,
                           Tf_fastMutexAttr,
                           Tf_errorCheckingMutexAttr;


TfMutex::TfMutex(Type type)
{
    /*
     * It is pretty much guaranteed that a TfMutex will be created very
     * early on in a program's lifetime, if a program links with lib/tf.
     * That makes this constructor a good place to do any general
     * "boot-strapping tasks" we want done, just by virtue of having lib/tf
     * in the picture.
     *
     * An even better place would be down in arch/atomicOperations.cpp,
     * but we can't do any tf-centric calls down there, so this seems like
     * the next best place.
     */
     
#if defined(ARCH_SUPPORTS_PTHREAD_RECURSIVE_MUTEX)
    TF_EXECUTE_ONCE({
        pthread_mutexattr_init(&Tf_recursiveMutexAttr);
        pthread_mutexattr_init(&Tf_fastMutexAttr);
        pthread_mutexattr_init(&Tf_errorCheckingMutexAttr);
        pthread_mutexattr_settype(&Tf_recursiveMutexAttr,
                                  PTHREAD_MUTEX_RECURSIVE);
        pthread_mutexattr_settype(&Tf_fastMutexAttr,
                                  PTHREAD_MUTEX_NORMAL);
        pthread_mutexattr_settype(&Tf_errorCheckingMutexAttr,
                                  PTHREAD_MUTEX_ERRORCHECK);
    });
#else
    _isRecursive = (type == RECURSIVE);
    TF_EXECUTE_ONCE({
        pthread_mutexattr_init(&Tf_recursiveMutexAttr);
        pthread_mutexattr_init(&Tf_fastMutexAttr);
        pthread_mutexattr_init(&Tf_errorCheckingMutexAttr);
    });
#endif
    
    if (type == NON_RECURSIVE) {
        if (TF_DEV_BUILD)
            pthread_mutex_init(&_mutex, &Tf_errorCheckingMutexAttr);
        else
            pthread_mutex_init(&_mutex, &Tf_fastMutexAttr);
    }
    else {
        pthread_mutex_init(&_mutex, &Tf_recursiveMutexAttr);
    }
}

void
TfMutex::_DebugModeLock()
{
    int err;
    if (ARCH_UNLIKELY((err=pthread_mutex_lock(&_mutex)) != 0)) {
        // CODE_COVERAGE_OFF - error conditions due to insufficient resources
        switch(err) {
          case EAGAIN:
            TF_FATAL_ERROR("pthread_mutex_lock failed with 'EAGAIN':\n  "
                           "Insufficient system resources available to lock "
                           "the mutex");
            break;
          case EDEADLK:
            TF_FATAL_ERROR("pthread_mutex_lock failed with 'EDEADLK':\n"
                           "  Calling thread already owns mutex and the mutex "
                           "doesn't allow recursive behavior.");
            break;
          case EINVAL:
            TF_FATAL_ERROR("pthread_mutex_lock failed with 'EINVAL': "
                           "Invalid mutex.");
            break;
          default:
            TF_FATAL_ERROR( "pthread_mutex_lock failed with "
                            "unrecognized error %d", err);
        // CODE_COVERAGE_ON
        }
    }
}

void
TfMutex::_DebugModeUnlock()
{
    if (ARCH_UNLIKELY(pthread_mutex_unlock(&_mutex)))
        TF_FATAL_ERROR("pthread_mutex_unlock failed");
}

#if !defined(ARCH_SUPPORTS_PTHREAD_RECURSIVE_MUTEX)
void
TfMutex::_RecursiveLock()
{
    pthread_t self = pthread_self();
    
    if (_lockCount && self == _owner)
        _lockCount++;
    else {
        (void) pthread_mutex_lock(&_mutex);
        _lockCount = 1;
        _owner = self;
    }
}

void
TfMutex::_RecursiveUnlock()
{
    if (--_lockCount == 0)
        (void) pthread_mutex_unlock(&_mutex);
}

bool
TfMutex::_RecursiveTryLock()
{
    pthread_t self = pthread_self();
    
    if (_lockCount && self == _owner) {
        _lockCount++;
        return true;
    }
    else if (pthread_mutex_trylock(&_mutex) == 0) {
        _lockCount = 1;
        _owner = self;
        return true;
    }
    else
        return false;
}

#endif


