#ifndef TF_SEMAPHORE_H
#define TF_SEMAPHORE_H


#include "pxr/base/arch/threads.h"

#if defined(ARCH_SUPPORTS_PTHREAD_SEMAPHORE)
#include <errno.h>
#include <semaphore.h>
#else
#include "pxr/base/tf/auto.h"
#include "pxr/base/tf/mutex.h"
#include "pxr/base/tf/condVar.h"
#endif

#include <boost/noncopyable.hpp>

/*!
 * \file semaphore.h
 * \ingroup group_tf_Multithreading
 */



/*!
 * \class TfSemaphore Semaphore.h pxr/base/tf/semaphore.h
 * \ingroup group_tf_Multithreading
 * \brief Semaphore datatype
 *
 * A \c TfSemaphore is used to indicate when a quantity of some resource
 * is available.  The \c TfSemaphore class is a direct embodiment of the
 * matching POSIX thread datatype on systems that support
 * sem_init()/sem_post().
 */

#if defined(doxygen) || defined(ARCH_SUPPORTS_PTHREAD_SEMAPHORE)

class TfSemaphore : boost::noncopyable {
public:
    //! Construct a semaphore, with an initial value of \p count.
    TfSemaphore(size_t count = 0);

    //! Destructor (error if run while some is waiting on this semaphore).
    ~TfSemaphore();

    //! Block until the semaphore count is positive, then decrement the count.
    void Wait() {
        while (sem_wait(&_sem) == -1 && errno == EINTR)
            ;
    }

    /*! 
     * \brief Non-blocking version of \c Wait().
     *
     * If the semaphore count is positive, \c TryWait() decrements the count
     * and returns true.  Otherwise, the function returns false.
     */
    bool TryWait() {
        return sem_trywait(&_sem) == 0;
    }
    
    //! Increment semaphore count, waking a thread waiting on the semaphore.
    void Post() {
        sem_post(&_sem);
    }

private:
    sem_t _sem;
};

#else

/*
 * Implementation using TfMutex and TfCondVar.
 */
class TfSemaphore : boost::noncopyable {
public:
    //! Construct a semaphore, with an initial value of \p count.
    TfSemaphore(size_t count = 0)
        : _count(count)
    {
    }

    //! Destructor (error if run while some is waiting on this semaphore).
    ~TfSemaphore() {
    }

    //! Block until the semaphore count is positive, then decrement the count.
    void Wait() {
        TF_SCOPED_AUTO(_m);
        while (!_count)
            _cv.Wait(_m);

        _count--;
    }

    /*! 
     * \brief Non-blocking version of \c Wait().
     *
     * If the semaphore count is positive, \c TryWait() decrements the count
     * and returns true.  Otherwise, the function returns false.
     */
    bool TryWait() {
        TF_SCOPED_AUTO(_m);
        return _count ? (--_count, true) : false;
    }
    
    //! Increment semaphore count, waking a thread waiting on the semaphore.
    void Post() {
        TF_SCOPED_AUTO(_m);
        _count++;
        _cv.Signal();
    }

private:
    TfCondVar _cv;
    TfMutex   _m;
    size_t    _count;
};

#endif





#endif
