#ifndef TF_MUTEX_H
#define TF_MUTEX_H

/*!
 * \file mutex.h
 * \brief Mutual exclusion datatype for multithreaded programs.
 * \ingroup group_tf_Multithreading
 */


#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/arch/atomicOperations.h"
#include "pxr/base/arch/threads.h"
#include <boost/noncopyable.hpp>
#include <pthread.h>



class TfCondVar;

/*!
 * \class TfMutex Mutex.h pxr/base/tf/mutex.h
 * \ingroup group_tf_Multithreading
 * \brief Mutual exclusion datatype
 *
 * A \c TfMutex is used to lock and unlock around a critical section
 * for thread safe behavior.  The \c TfMutex class
 * is a direct embodiment of the matching POSIX thread datatype.
 * \b Note: whenever possible, use a \c TfMutex in conjunction with
 * \c TfAuto.
 *
 * A \c TfMutex can be initialized to be either recursive or non-recursive.
 * A recursive lock means that a given thread can relock a \c TfMutex
 * multiple times; if the thread has locked a recursive \c TfMutex n times
 * then it takes n unlocks to relinquish the lock.
 *
 * A non-recursive \c TfMutex, however, will deadlock if a thread tries
 * to relock a \c TfMutex it has already locked.  (For debug releases,
 * i.e. when the preprocessor directive \c TF_DEBUG is defined,
 * a non-recursive \c TfMutex will detect the deadlock and exit with
 * an appropriate error message/trace, as will attempts to unlock a \c TfMutex
 * by a thread which is not the owner of the lock.)
 *
 * A non-recursive \c TfMutex will lock and unlock somewhat faster than
 * a recursive \c TfMutex.
 *
 * \remark
 * \c TfFastMutex implements an even faster lock that avoids function
 * calls, at the cost of spinning; however,
 * a \c TfMutex should always be your first choice.
 */

class TfMutex : boost::noncopyable {
public:
    //! Mutex behavior types.
    enum Type {
        NON_RECURSIVE = 0,  //!< fastest; non-recursive locking behavior
        RECURSIVE           //!< recursive locking behavior
    };
    
    //! Initializes the class for locking and unlocking.
    TfMutex(Type = NON_RECURSIVE);

    //! Blocks until the lock is acquired.
    void Lock() {
#if !defined(ARCH_SUPPORTS_PTHREAD_RECURSIVE_MUTEX)
        if (_isRecursive) {
            _RecursiveLock();
            return;
        }
#endif

        if (TF_DEV_BUILD)
            _DebugModeLock();
        else
            (void) pthread_mutex_lock(&_mutex);
    }

    //! Releases the already acquired lock.
    void Unlock() {
#if !defined(ARCH_SUPPORTS_PTHREAD_RECURSIVE_MUTEX)
        if (_isRecursive) {
            _RecursiveUnlock();
            return;
        }
#endif
        if (TF_DEV_BUILD)
            _DebugModeUnlock();
        else
            (void) pthread_mutex_unlock(&_mutex);
    }

    /*!
     * \brief Non-blocking lock acquisition.
     *
     * If no one else is holding the lock, the function returns \c true
     * and the lock is acquired.  Otherwise, the function returns \c false
     * and the lock is not acquired.  In neither case does the function
     * block.
     */

    bool TryLock() {
#if !defined(ARCH_SUPPORTS_PTHREAD_RECURSIVE_MUTEX)
        if (_isRecursive) {
            return _RecursiveTryLock();
        }
#endif  
        return !pthread_mutex_trylock(&_mutex);
    }
    
    //! Equivalent to \c Lock().
    void Start() {
        Lock();
    }

    //! Equivalent to \c Unlock().
    void Stop() {
        Unlock();
    }
    
private:
    pthread_mutex_t _mutex;

#if !defined(ARCH_SUPPORTS_PTHREAD_RECURSIVE_MUTEX)
    pthread_t _owner;
    size_t _lockCount;
    bool _isRecursive;

    void _RecursiveLock();
    void _RecursiveUnlock();
    bool _RecursiveTryLock();
#endif    

    void _DebugModeLock();
    void _DebugModeUnlock();

    pthread_mutex_t& _GetRawMutex() {
        return _mutex;
    }
    
    friend class TfCondVar;
};

/*!
 * \hideinitializer
 * \brief Provide a once-only locking facility for initializations
 * \ingroup group_tf_Multithreading
 *
 * A common pattern to making static available data is the following:
 * \code
 *     Resource* GetResource() {
 *         static Resource* r = NULL;
 *         if (!r) {
 *             r = new Resource;
 *         }
 *         return r;
 *     }
 * \endcode
 *
 * Every caller of GetResource() is given the same pointer; furthermore,
 * initialization is performed as needed.
 *
 * Unfortunately, the above is not threadsafe.  If GetResource() is not
 * called until the program is multithreaded, two or more threads might
 * race to set the static variable \c r.
 *
 * The solution is to use a \c TfMutex; the problem is making sure that
 * the \c TfMutex itself has been constructed.
 *
 * The \c TF_EXECUTE_ONCE() macro solves this problem.
 * However, every instance of this macro uses the same global lock.
 * Therefore, it should only be used to set static data, which ensures
 * that the number of time the macro actually needs to lock is limited
 * to the actual number of initializations required by the code.
 * Additionally, the static data being set must remain set, to ensure
 * that the global lock not become a bottleneck.
 *
 * Here is the indicated use:
 * \code
 *     Resource* GetResource() {
 *         static Resource* r = NULL;
 *         TF_EXECUTE_ONCE(r = new Resource);
 *         return r;
 *     }
 * \endcode
 *
 * The argument to this macro can be an expression, or even a number
 * of statements if brackets are used:
 * \code
 *     Resource* GetResource() {
 *         static Resource* r = NULL;
 *         TF_EXECUTE_ONCE({ r = new Resource;
 *                           r->Setup(); });
 *         return r;
 *     }
 * \endcode
 */
 
#define TF_EXECUTE_ONCE(code)                                           \
    ARCH_EXECUTE_ONCE_WITH_ERRMSG(code,                                 \
          TF_FATAL_ERROR("attempted to recursively "                    \
                         "enter a TF_EXECUTE_ONCE block"))






#endif
