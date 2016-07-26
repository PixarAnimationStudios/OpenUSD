#ifndef TF_FASTMUTEX_H
#define TF_FASTMUTEX_H


#include "pxr/base/arch/atomicOperations.h"
#include "pxr/base/tf/diagnosticLite.h"
#include <boost/noncopyable.hpp>

/*!
 * \file fastMutex.h
 * \ingroup group_tf_Multithreading
 */



/*!
 * \class TfFastMutex FastMutex.h pxr/base/tf/fastMutex.h
 * \ingroup group_tf_Multithreading
 * \brief Mutual exclusion datatype
 *
 * A \c TfFastMutex is used to lock and unlock around a critical section
 * for thread safe behavior.  \b Note: whenever possible, use \c TfFastMutex
 * in conjunction with \c TfAuto.  Attempts by a thread to relock a
 * \c TfFastMutex it already has locked will result in deadlock.
 *
 * \remark
 * The \c TfFastMutex class uses a spin-lock mechanism; your
 * first choice should be a \c TfMutex, unless you're positive about
 * what you're doing.
 *
 * A \c TfFastMutex should only be used in places where you know contention
 * to be unlikely.  If there is a doubt about that, use a \c TfMutex.  A
 * \c TfFastMutex is smaller than a \c TfMutex but may perform poorly
 * under heavy contention and does not support additional features like
 * recursive locking.
 *
 */

class TfFastMutex : boost::noncopyable {
public:
    /*! \brief Initializes the class for locking and unlocking. */
    TfFastMutex() {
        _value = 0;
    }

    /*! \brief Blocks until the lock is acquired. */
    void Lock() {
        if (!ArchAtomicCompareAndSwap(&_value, 0, 1))
            _WaitForLock();
    }

    /*! \brief Releases the already acquired lock. */
    void Unlock() {
        if (_value == 0)
            TF_FATAL_ERROR("unlocking an unlocked TfFastMutex");
        ArchAtomicIntegerSet(&_value, 0);
    }

    /*! \brief Equivalent to \c Lock(). */
    void Start() {
        Lock();
    }

    /*! \brief Equivalent to \c Unlock(). */
    void Stop() {
        Unlock();
    }
    
private:
    void _WaitForLock();
    volatile int _value;
};





#endif
