#ifndef TF_THREADBASE_H
#define TF_THREADBASE_H


#include "pxr/base/tf/auto.h"
#include "pxr/base/tf/condVar.h"
#include "pxr/base/tf/mutex.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/threadInfo.h"
#include <atomic>

/*!
 * \file threadBase.h
 * \ingroup group_tf_Multithreading
 */



template <class T> class TfThread;
class TfThreadDispatcher;

/*!
 * \class TfThreadBase ThreadBase.h pxr/base/tf/ThreadBase.h
 * \ingroup group_tf_Multithreading
 * \brief Typeless base class for \c TfThread.
 *
 * This is the base class for \c TfThread; it lacks the knowledge
 * of the return type for the function being run in a thread.
 */
class TfThreadBase : public TfSimpleRefBase {
public:
    /*!
     * \brief Handle \c TfThread via \c TfRefPtr mechanism.
     *
     * When threads are launched from a \c TfThreadDispatcher,
     * the dispatcher returns a \c TfThread by means of a \c TfRefPtr.
     * When the last reference to a \c TfThread disappears, memory is
     * reclaimed.
     */
    typedef TfRefPtr<TfThreadBase> Ptr;

    /*!
     * \brief Block until the thread is completed.
     *
     * This call will not return until the thread being waited on finishes.
     *
     * Finally, you cannot \c Wait() on a non-immediate thread from
     * another non-immediate thread if both threads are run from the
     * same dispatcher (because of the potential for deadlock, given that
     * the thread-pool is of finite size).  Attempting such a wait results
     * in run-time termination of the program.
     */
    void Wait() {
        if (_inDispatcherPool)
            _PossiblyRunPendingThread();
        TF_SCOPED_AUTO(_finishedMutex);
        while (not _finished) {
            _finishedCondVar.Wait(_finishedMutex);
        }
    }
    
    /*!
     * \brief Query thread completion status.
     *
     * This call returns true if the thread has completed; if not,
     * and \c duration is positive, the call blocks for up to \c duration
     * seconds and returns true if the thread has completed by that time.
     * Otherwise, false is returned.
     */
    bool IsDone(double duration = 0.0) {
        TF_SCOPED_AUTO(_finishedMutex);
        if (duration <= 0.0) {
            return _finished;
        }
        _finishedCondVar.SetTimeLimit(duration);
        while (not _finished) {
            if (not _finishedCondVar.TimedWait(_finishedMutex)) {
                return false;
            }
        }
        return true;
    }
    
    /*!
     * \brief Cancel a thread without waiting for termination.
     *
     * This call sends a cancellation notice to a thread, and returns
     * immediately.  The thread may or may not honor the cancellation notice,
     * and if it does, it may be some indeterminate time in the future.
     * Use \c Wait() or \c IsDone() to determine when a thread actually
     * finishes, if necessary.
     *
     * Note that only threads launched in immediate mode can be
     * canceled; attempts to cancel a pool-mode thread are silently
     * ignored.
     */
     
    void Cancel() {
        if (!_inDispatcherPool) {
            pthread_cancel(_id);
        }
    }
        
    //! Indicate if a thread was canceled.
    bool IsCanceled() {
        return _canceled;
    }

    //! Return the dispatcher that created this thread.
    TfThreadDispatcher* GetThreadDispatcher() {
        return _dispatcher;
    }

    virtual ~TfThreadBase();

private:
    TfThreadBase(TfThreadInfo* threadInfo);
    
    void    _PossiblyRunPendingThread();
    virtual void _ExecuteFunc() = 0;

    void _StoreThreadInfo() {
        _threadInfo->_Store();
    }

    TfMutex             _finishedMutex;
    TfCondVar           _finishedCondVar;
    bool                _finished;          // message upon completion
    TfThreadDispatcher* _dispatcher;        // launcher
    TfThreadInfo*       _threadInfo;
    pthread_t           _id;                // only set for immediate-mode
    bool                _launchedSingleThreaded;
    bool                _inDispatcherPool;  // non-immediate launch
    bool                _canceled,
                        _finishedFunc;
    Ptr                 _self;              // prevent premature destruction!
    
    friend class TfThreadDispatcher;
    template <class T> friend class TfThread;
};





#endif
