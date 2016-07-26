#ifndef TF_BARRIER_H
#define TF_BARRIER_H

#include "pxr/base/tf/condVar.h"
#include "pxr/base/tf/fastMutex.h"
#include "pxr/base/tf/mutex.h"
#include <boost/noncopyable.hpp>

/*!
 * \file barrier.h
 * \ingroup group_tf_Multithreading
 */



/*!
 * \class TfBarrier Barrier.h pxr/base/tf/barrier.h
 * \ingroup group_tf_Multithreading
 * \brief Thread synchronization primitive
 *
 * A \c TfBarrier is used to synchronize threads; all the threads
 * pause until all the threads reach the barrier.
 */

class TfBarrier : boost::noncopyable {
public:
    //! Initializes the barrier for \p nThreads threads.
    TfBarrier(size_t nThreads = 1)
        : _nThreads(nThreads)
    {
        _ctr = nThreads;
        _cycle = 0;
        _canSpin = false;
    }


    //! It is a run-time error to destroy a barrier that is still active.
    ~TfBarrier();
    
    /*!
     * \brief Set the barrier to synchronize \p nThreads threads.
     *
     * It is a run-time error to call \c SetSize() while the barrier
     * is still active (i.e. while threads are waiting on the barrier).
     */
    void SetSize(size_t nThreads);

    //! Return the number of threads the barrier is set to synchronize.
    size_t GetSize() {
        return _nThreads;
    }
    
    /*!
     * \brief Set whether or not the barrier can spin-lock.
     *
     * The default behavior for a barrier is spin-locking disabled.
     * In this mode, a thread that calls \c Wait() actually waits on
     * a condition variable (\c TfCondVar) until enough threads have
     * called \c Wait().  With spin-locking enabled, a thread spins
     * waiting for other threads.
     *
     * Do not enable spin mode unless you know what you're doing and you
     * have a strong guarantee that threads will tend to arrive at a barrier
     * pretty much in lock-step; note that this implies you have a processor
     * per thread!
     */
    void SetSpinMode(bool arg) {
        _canSpin = arg;
    }

    //! Get the current spin-mode state (\c true for enabled).
    bool GetSpinMode() {
        return _canSpin;
    }
    
    //! Return true if any thread is waiting on the barrier.
    bool IsWaitActive() {
        return _ctr != _nThreads;
    }

    //! Block until \p GetSize() threads have called \c Wait().
    void Wait();
    
private:
    TfMutex         _mutex;
    TfFastMutex     _fastMutex;
    TfCondVar       _cond;
    size_t          _nThreads,
                    _ctr;
    volatile size_t _cycle;
    bool            _canSpin;
};





#endif
