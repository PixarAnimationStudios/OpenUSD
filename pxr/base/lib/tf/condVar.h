#ifndef TF_CONDVAR_H
#define TF_CONDVAR_H


#include "pxr/base/tf/mutex.h"

#include <boost/noncopyable.hpp>

#include <pthread.h>
#include <time.h>

/*!
 * \file condVar.h
 * \ingroup group_tf_Multithreading
 */



/*!
 * \class TfCondVar CondVar.h pxr/base/tf/condVar.h
 * \ingroup group_tf_Multithreading
 * \brief Condition variable datatype
 *
 * A \c TfCondVar is used to wait for a particular predicate to become true.
 * (But consider using a \c TfThreadStateVar if it provides the functionality
 * you require.) Here is the prototypical use of a \c TfCondVar:
 *
 * \code
 *     TfCondVar cv;    // assume these are accessible
 *     TfMutex   m;     // to all running threads
 *
 *     ...
 *
 *     // wait for <predicate> to become true
 *     m.Lock();
 *
 *     // ALWAYS do this in a loop
 *     while (!<predicate>) {
 *         cv.Wait(m);
 *     }
 *
 *     // execute code that required <predicate> to be true
 * 
 *     m.Unlock();
 *
 * \endcode
 *
 * The call to \c Wait() requires that \c m be locked; \c Wait() releases
 * the lock on \c m.  When \c Wait() returns, the lock is reacquired.
 * The above code was the "waiting" code.  The other half is the code that
 * "signals" when it has made the predicate true:
 *
 * \code
 *     m.Lock();
 *     // execute code that leaves <predicate> true
 * 
 *     cv.Broadcast();  // tell "waiter" <predicate> has been changed
 *
 *     m.Unlock();
 * \endcode
 *
 * (Note that both of the above examples could be more safely
 * written using \c TfAuto with enclosing scopes.)
 */ 

class TfCondVar : boost::noncopyable {
public:
    //! Constructs a condition variable.
    TfCondVar();

    /*!
     * \brief Relinquishes \c mutex and blocks until \c Broadcast() is called.
     *
     * This call requires that the thread first acquire a lock for \c mutex.
     * The call to \c Wait() blocks until another thread calls \c Broadcast()
     * on this \c TfCondVar \e and the lock on \c mutex can be reestablished.
     * When both have occured, \c Wait() returns with \c mutex locked by
     * the calling thread.
     */
    void Wait(TfMutex& mutex) {
        pthread_cond_wait(&_cond, &mutex._GetRawMutex());
    }

    /*!
     * \brief Specifies a time limit for calls to \c TimedWait().
     *
     * If this call is made at time \e t then the time limit is
     * set to \e t + \c duration, where \c duration is measured in seconds.
     */
    void SetTimeLimit(double duration);

    /*!
     * \brief Same as \c Wait() but with a limit on the time waited.
     *
     * A call to \c SetTimeLimit(duration) at time \e t sets an internal
     * alarm time of \e t + \c duration in the condition variable.
     * A subsequent call to \c TimedWait() has the same behavior as
     * a call to \c Wait(), except that once time t + \c duration is reached,
     * the condition variable attempts to relock the \c mutex and return
     * to the calling thread.  The function returns \c true if the time limit
     * was not reached before another thread called \c Broadcast(), and \c
     * false otherwise.  Because of the need to relock \c mutex
     * before returning (even if the time limit is reached) may take
     * arbitrarily longer than expected.
     *
     * Note that the time limit specified by a call to \c SetTimeLimit()
     * stays in effect until reset by another such call.  If \c SetTimeLimit()
     * has never been called, \c TimedWait() returns \c false immediately.
     */
    bool TimedWait(TfMutex& mutex);

    /*!
     * \brief Sends a message to any threads invoking \c Wait()
     * or \c TimedWait().
     *
     * The caller should acquire a lock on the appropriate mutex
     * variable before calling \c Broadcast() and then relinquish the
     * lock afterward.
     */
    void Broadcast() {
        pthread_cond_broadcast(&_cond);
    }

    /*!
     * \brief Send a message to one thread invoking \c Wait()
     * or \c TimedWait().
     *
     * Similar to \c Broadcast(), except only one thread is woken.  This
     * can avoid problems in the traditional producer-consumer setup
     * where only one item is produced, but all waiting threads wake up 
     * and try to access it, only one succeeding.  Using \c Signal avoids 
     * that thrashing.
     *
     */
    void Signal() {
        pthread_cond_signal(&_cond);
    }

private:
    pthread_cond_t _cond;
    timespec _timeLimit;
    bool     _timeLimitSet;
};





#endif
