#ifndef TF_THREADDISPATCHER_H
#define TF_THREADDISPATCHER_H
 

#include "pxr/base/tf/mutex.h"
#include "pxr/base/tf/fastMutex.h"
#include "pxr/base/tf/threadInfo.h"
#include "pxr/base/tf/thread.h"
#include "pxr/base/tf/condVar.h"
#include "pxr/base/tf/semaphore.h"
#include "pxr/base/tf/atomicInteger.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/threads.h"
#include <vector>
#include <deque>
#include <limits.h>

/*!
 * \file threadDispatcher.h
 * \ingroup group_tf_Multithreading
 */

class TfThreadBase;

///
/// \class TfThreadDispatcher ThreadDispatcher.h pxr/base/tf/threadDispatcher.h
/// \ingroup group_tf_Multithreading
/// \brief Launches one or more threads. This class is mainly used in C++. You 
/// cannot use it to launch threads in Python.
///
/// In Python, this class provides minimal support for querying and configuring 
/// the dispatching system. 
///
/// \section cppcode_TfThreadDispatcher C++-Specific Information
///
/// A \c TfThreadDispatcher is used to execute a particular function with
/// specified arguments in a separate thread.  Both member and non-member
/// functions can be executed; there is no restruction on the return type
/// of the functions.  The information about the function to be called,
/// possible class object (for member functions) and arguments is encapsulated
/// in a \c boost::function object; see this documentation for further details.
/// In particular, pay attention to the necessary persistence of objects
/// passed into the \c boost::function object.
///
///
/// Here is the basic use:
/// \code
///
/// void TaskFunction(const string& taskName) {
///     ...
/// }
///
/// struct Simulator {
///      double Compute(TfVec3d);
///      ...
/// };
///
/// void doStuff(Simulator& s, TfVec3d vec) {
///    boost::function<double ()> c1 = boost::bind(s, Simulator::Compute, vec);
///    boost::function<void ()>   c2 = boost::bind(TaskFunction, "example");
///    TfThreadDispatcher td;
///    TfThread<void>::Ptr t1 = td.Start(c1);
///    TfThread<double>::Ptr t2 = td.Start(c2);
///
///    t1->Wait();
///    double result = t2->GetResult();
///
/// }
/// \endcode
///
/// The above example launches threads to run \c TaskFunction() and
/// \c Simulator::Compute().  The return value from the invocations of the
/// functions may be retrieved later; note that the first thread has no return
/// value since \c TaskFunction() returns \c void.
///
/// Additionally, note that the value returned from the functions need not
/// be limited; both return-by-value and return-by-reference are supported
/// and work correctly.
///
/// Note that the \c boost::function object need not be named:
/// \code
///     t1 = td.Start(boost::bind(TaskFunction, "example"));
/// \endcode
/// is perfectly acceptable.
///
/// <H2> Thread Launching </H2>
///
/// Functions to be executed can either run in newly created threads,
/// or from an available persistent thread pool maintained by the
/// dispatcher.  If more threads are submitted to the thread pool than
/// the pool can handle at once, threads block until a thread is
/// available to handle them.  Functions submitted to run in newly
/// created threads are always run immediately.
///
/// <H2> Ordering </H2>
///
/// By default, pool mode threads are dispatched in FIFO (first in,
/// first out) order.  Users may call SetLIFOMode(true) to set LIFO
/// (last in, first out) ordering.  See \c SetLIFOMode() for more details.
///
/// <H2> Restrictions </H2>
///
/// A thread dispatcher may not be destroyed while there are still
/// unfinished threads.  Should this happen, a run-time abort will
/// occur.  (The alternative is for the thread functor's destructor to
/// block, which is potentially mysterious).
///
/// <H2> Persistence </H2>
///
/// Data referenced in executing the function (in particular, the class object,
/// for member functions) and any arguments that are not copied, but passed by
/// reference, must remain persistent while the thread runs.
///
/// \anchor TfThreadDispatcher_tsdData
/// <H2> Thread-Specific Data </H2>
///
/// It is often useful for a thread to be told (a) how many other
/// threads are working on that task (see \c ParallelStart()),
/// and (b) a unique index (in the range 0 to nthreads-1).  The \c
/// TfThreadDispatcher class assists the user by creating a \c
/// TfThreadInfo structure that is unique to each thread.  A thread can
/// access its own \c TfThreadInfo structure by calling the static
/// member function \c TfThreadInfo::Find().  The user can add
/// additional thread-specific data by deriving their own
/// TfThreadDispatcher::CreateThreadInfo().  Here is an example that
/// makes available a common \c TfMutex and \c TfCondVar to the running
/// threads:
/// \code
///
/// struct My_ThreadInfo : public TfThreadInfo {
///     My_ThreadInfo(size_t index, size_t nThreads,
///                   TfMutex* mutex, TfCondVar* var)
///       : TfThreadInfo(index, nThreads, TfThreadInfo::Find())
///     {
///         _mutex = mutex;
///         _var = var;
///     }
/// };
///
///
/// struct My_Dispatcher : public TfThreadDispatcher {
///     virtual TfThreadInfo* CreateThreadInfo(size_t index, size_t nThreads);
///     TfMutex _commonMutex;
///     TfCondVar _commonVar;
/// };
///
///
/// TfThreadInfo*
/// MyDispatcher::CreateThreadInfo(size_t index, size_t nThreads) {
///     return new My_ThreadInfo(index, nThreads, &_commonMutex, &_commonVar);
/// }
///
/// \endcode
///
/// Each thread launched from a \c My_Dispatcher dispatcher can
/// dynamically cast the return value of \c TfThreadInfo::Store() to a
/// \c My_ThreadInfo*.  The mutex and condition variable pointers of
/// all the \c My_ThreadInfo structures all point to \c _commonMutex of
/// the dispatcher the thread was launched from and similarly for \c
/// _commonVar.
///
/// \anchor threadDispatcher_allocation
/// <H2> Physical Thread Allocation </H2>
///
/// A process may start a number of threads during its lifetime.  Sometimes,
/// these threads may be largely inactive.  However, the number of 
/// computationally active threads at any time may need to be limited if 
/// multiple processes on the same machine are to coexist peacefully.  To 
/// achieve that, the \c TfThreadDispatcher class has a number of static 
/// functions for limiting, allocating, and freeing some number of physical
/// threads.
///
/// First, a process sets the number of physical threads available by a call to
/// \c TfThreadDispatcher::SetPhysicalThreadLimit().  This call should only be
/// made from "application-level" code i.e. in response to a command-line
/// option or other suitable high-level configuration.  In particular, a
/// library should \e never make this decision on its own, since a machine with
/// \p P physical threads may be running a number of processes, each of which
/// need to use only some subset of the physical threads.
///
/// The simplest use of multi-threading is to launch a number of threads in
/// parallel.  If the number of threads doesn't need to be known in advance,
/// and return values from launched threads are unneeded, this is best written
/// as:
/// \code
///     boost::function<void ()> cl = boost::bind(...);
///     TfThreadDispatcher::ParallelRequestAndWait(cl);
/// \endcode
/// This will attempt to use the maximum number of physical threads.  Whenever
/// feasible, try to use the above form for launching threads.
///
/// Suppose however that you need to know the number of threads launched
/// in advance.  In this case, you could write:
/// \code
///     boost::function<void ()> cl = boost::bind(...);
///     size_t nExtra = TfThreadDispatcher::RequestExtraPhysicalThreads(...);
///
///     // use nExtra+1 for whatever you need
///
///     dispatcher.ParallelStart(nExtra + 1, cl);
///     dispatcher.Wait();
///
///     TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra);
/// \endcode
///
/// In the case of a dispatcher where the threads are launched one-at-a-time
/// in pool mode, one would write:
/// \code
///    size_t nExtra = TfThreadDispatcher::RequestExtraPhysicalThreads(...);
///    TfThreadDispatcher dispatcher(nExtra + 1);
///    dispatcher.SetPoolMode(true);
///
///    // add any number of threads
///
///    dispatcher.Wait();
///
///    TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra);
/// \endcode
///
/// Note that all of the above cases require waiting for the dispatcher to 
/// finish all its threads.  If you don't wait for the dispatcher to finish, 
/// then you must give the dispatcher one less thread, because the thread that 
/// adds jobs to the dispatcher is itself a computationally active thread if it 
/// does not wait for the dispatcher to finish.
///

class TfThreadDispatcher {
public:
    /*!
     * \brief Constructor.
     *
     * Threads submitted to the dispatcher are either run immediately
     * in a newly created thread, or submitted to the pool, to wait
     * for the next available pool thread. The maximum number of
     * threads available to handle pool-threads is specified by \c
     * maxPoolThreads.  Note that pool threads are created lazily, as
     * needed, and then remain for some indefinite time, waiting to
     * handle submitted pool threads. 
     *
     * The default behavior is not to run threads using the pool.
     * Also, if \p maxPoolThreads is zero, then threads submitted
     * to the (non-existent pool) are simply run immediately,
     * in the submitter's thread.
     *
     * If your object is to run an arbitrary number of threads
     * immediately, they should not be submitted using the pool.
     *
     * Threads run from the constructed dispatcher have their stack
     * set (hopefully!) to \c stackSize bytes.  Whether the requested
     * amount of stack can be accomodated is, of course, ridiculously
     * architecture dependent.  However, the default stack size is
     * believed to be suitable for most applications, and should not
     * be overridden unless you are positive you know better.
     */

    TfThreadDispatcher(size_t maxPoolThreads = INT_MAX,
                       size_t stackSize = ArchGetDefaultThreadStackSize());

    virtual ~TfThreadDispatcher();

    /*!
     * \name Configuration functions
     *
     * These functions control a thread dispatcher's behavior.
     * @{
     */

    /*!
     * \brief Set whether submitted threads should use the pool or not.
     *
     * If \p mode is true then threads are submitted to the pool and
     * may not run immediately, if the pool is busy with other
     * threads.  Otherwise, the threads are run in newly created
     * threads which terminate when their thread is done.  There is no
     * a priori limit on how many such threads will be created.
     */

    void SetPoolMode(bool mode) {
        _poolMode = mode;
    }
    
    //! Return the current setting of pool mode.
    bool GetPoolMode() const {
        return _poolMode;
    }


    /*!
     * \brief If in pool mode, maximum number of concurrent threads.
     *
     * Defaults to INT_MAX, only used if GetPoolMode() == true
     */
    size_t GetMaxNumPoolThreads() const {
        return _pool._maxThreads;
    }    
    

    /*!
     * \brief Force a thread-dispatcher not to create threads.
     *
     * Setting a dispatcher to be single threaded causes
     * submitted threads (either pool-mode or non-pool-mode) to be immediately
     * executed without thread creation.
     *
     * Note that if the pool-size is zero, then threads submitted in pool-mode
     * are run immediately without thread-creation (i.e. in the submitter's thread),
     * but non-pool-mode threads are run without thread-creation only if
     * \c SetSingleThreaded(true) was called.
     */
    void SetSingleThreaded(bool arg) {
        _singleThreaded = arg;
    }

    //! Return \c true if in single threaded mode.
    bool GetSingleThreaded() const {
        return _singleThreaded;
    }

    /*!
     * \brief Set whether pool-mode threads should use LIFO ordering.
     *
     * If \p mode is true then pool-modethreads will be dispatched
     * in LIFO (last in, first out) order.  If \p mode is false then
     * pool-mode threads will be dispatched in FIFO (first in, first
     * out) order.
     *
     * It is safe (as far as the dispatcher is concerned) to call
     * this method at any time, but if this method is called while
     * there are still pending jobs in the dispatcher, the order of
     * dispatch for those pending jobs is indeterminate.  Jobs
     * submitted after this method is called will follow the
     * specified ordering.
     *
     * Note that the dispatch ordering policy determines the order
     * in which threads are <i>started</i>, but says nothing about
     * the order in which they will finish.  In particular, in the
     * multithreaded case, the dispatcher does not guarantee that
     * threads will finish in a specific order.
     */

    void SetLIFOMode(bool mode) {
        TF_SCOPED_AUTO(_pool._threadListMutex);
        _pool._lifoMode = mode;
    }

    //! Return \c true if the dispatcher is set to LIFO ordering.
    bool GetLIFOMode(void) const {
        TF_SCOPED_AUTO(_pool._threadListMutex);
        return _pool._lifoMode;
    }

    /*!
     * \brief Create a \c TfThreadInfo* structure for each thread.
     *
     * The default version of \c CreateThreadInfo() is called by
     * \c Start() to create a new TfThreadInfo object for each thread.
     * The function is virtual so that users can create structures
     * derived from \c TfThreadInfo that contain additional thread-specific
     * data.  See \ref TfThreadDispatcher_tsdData for an example.
     */
    virtual TfThreadInfo* CreateThreadInfo(size_t index, size_t nThreads);    

    /*!
     * @}
     */
    
    /*!
     * \name Thread launching functions
     * 
     * These functions launch and monitor threads.
     * Before using any of the functions in this group, you should
     * be familiar with the policies
     * for \ref threadDispatcher_allocation "allocating physical threads".
     *
     * @{
     */
    
    /*!
     * \brief Execute a bound function in a thread.
     *
     * The object \c func is queued to run in a separate thread.
     * The return value from calling \c func can be retrieved
     * via the \c GetResult() call from the returned \c TfThread structure.
     */
    template <class RET>
    typename TfThread<RET>::Ptr
    Start(const boost::function<RET ()>& func) {
        return _Launch(func, TfNullPtr);
    }

    /*!
     * \brief Execute a bound function in a thread.
     *
     * The object \c func is queued to run in a separate thread.  The return
     * value from calling \c func can be retrieved via the \c GetResult() call
     * from the returned \c TfThread structure.
     *
     * \a func can be anything that's convertible to a boost function of the
     * appropriate type and exposes a result_type typedef.
     */
    template <class FuncObj>
    typename TfThread<typename FuncObj::result_type>::Ptr
    Start(const FuncObj &func) {
        return Start(boost::function<typename FuncObj::result_type ()>(func));
    }

    /*!
     * \brief Launch threads in parallel.
     *
     * \c ParallelStart() launches \c nThreads separate threads
     * to execute \c func.  The \c TfThreadInfo structures for
     * these threads will return \c nThreads when \c GetNumThreads() is
     * called, and each \c TfThreadInfo structure will return a unique
     * value for \c GetIndex() in the range zero to \c nThreads-1
     * (inclusive).
     *
     * If you plan to launch multiple threads and wait for them to return,
     * see if \c ParallelRequestAndWait() is a better choice.
     *
     * Before using this call, you should be familiar with the policies
     * for \ref threadDispatcher_allocation "allocating physical threads".
     */

    template <class RET>
    std::vector<typename TfThread<RET>::Ptr>
    ParallelStart(size_t nThreads, const boost::function<RET ()>& func) {
        return _PLaunch(nThreads, func);
    }

    /*!
     * \brief Launch threads in parallel.
     *
     * \c ParallelStart() launches \c nThreads separate threads
     * to execute \c func.  The \c TfThreadInfo structures for
     * these threads will return \c nThreads when \c GetNumThreads() is
     * called, and each \c TfThreadInfo structure will return a unique
     * value for \c GetIndex() in the range zero to \c nThreads-1
     * (inclusive).
     *
     * If you plan to launch multiple threads and wait for them to return,
     * see if \c ParallelRequestAndWait() is a better choice.
     *
     * Before using this call, you should be familiar with the policies
     * for \ref threadDispatcher_allocation "allocating physical threads".
     */
    template <class FuncObj>
    std::vector<typename TfThread<typename FuncObj::result_type>::Ptr>
    ParallelStart(size_t nThreads, const FuncObj &func) {
        return ParallelStart(nThreads, boost::function<typename
                             FuncObj::result_type ()>(func));
    }


    //! Block until all threads launched by \c Start() have completed.
    void Wait() const {
        TF_SCOPED_AUTO(_allThreadsDoneMutex);
        while (!_allThreadsDone)
            _allThreadsDoneCond.Wait(_allThreadsDoneMutex);
    }

    /*!
     * \brief Query thread completion status.
     *
     * This call returns true all the threads have been completed; if not,
     * and \c duration is positive, the call blocks for up to \c duration
     * seconds and returns true if all threads have completed by that time.
     * Otherwise, false is returned.
     */
    bool IsDone(double duration = 0.) const {
        TF_SCOPED_AUTO(_allThreadsDoneMutex);

        if (duration <= 0)
            return _allThreadsDone;
        
        _allThreadsDoneCond.SetTimeLimit(duration);
        
        while (!_allThreadsDone) {
            if (!_allThreadsDoneCond.TimedWait(_allThreadsDoneMutex))
                return false;
        }
        
        return true;
    }

    /*!
     * \brief Flush any pending pool-mode threads.
     *
     * Any threads waiting to run in pool-mode are canceled;
     * a subsequent query by \c ThreadBase::IsCanceled() on a flushed
     * thread will return \c true.  Note that you cannot call
     * a canceled thread's \c TfThread::Wait() function.
     */
    void FlushPendingPoolThreads();

    /*!
     * \brief Flush a specific pending pool-mode thread.
     *
     * Removes a thread launched in pool mode that has not yet begun its
     * execution.  The function returns true if the thread was actively
     * removed from the waiting thread list (which means it had not yet begun to run).
     * However, if the thread is not in pool-mode, has already begun running or has finished,
     * no action is taken and false is returned.
     * 
     * Note that if the thread has already begun its execution, you must
     * find another way to signal it stop, if that is your intention.
     */
    template <typename T>
    bool FlushWaitingPoolThread(typename TfThread<T>::Ptr tPtr) {
        return _FlushWaitingPoolThread(tPtr);
    }
    

    //! Return the number of threads that have not yet finished.
    size_t GetNumPendingThreads() const {
        return _nThreadsPending.Get();
    }


    
    /*!
     * @}
     */
    
    /*!
     * \brief Execute a function in a thread, in non-pool mode from a global
     * dispatcher.
     *
     * A dispatcher cannot be destroyed until all threads it has launched
     * have completed.  This presents a problem for threads that you launch,
     * might cancel, and don't want to wait for: the launching dispatcher
     * must remain in existence while these threads are outstanding.
     *
     * In this particular case, use \c AnonymousStart(), which is
     * similar to \c Start(), except that it uses a global dispatcher
     * which is never destroyed.  The function is always run in its own
     * thread.
     *
     * Before using this call, you should be familiar with the policies
     * for \ref threadDispatcher_allocation "allocating physical threads".
     */
    
    template <class RET>
    static
    typename TfThread<RET>::Ptr
    AnonymousStart(const boost::function<RET ()>& func) {
        return _GetAnonymousDispatcher()._Launch(func, TfNullPtr);
    }

    template <class FuncObj>
    static
    typename TfThread<typename FuncObj::result_type>::Ptr
    AnonymousStart(const FuncObj &func) {
        return AnonymousStart(boost::function<typename
                              FuncObj::result_type ()>(func));
    }

    /*!
     * \name Static functions for physical thread allocation.
     *
     * These (static) functions are used to centrally allocate
     * physical thread resources.  For detailed explanation of use, please
     * read the section on policies
     * for \ref threadDispatcher_allocation "allocating physical threads".
     * @{
     */

    /*!
     * \brief Set the maximum number of physical threads allowed by this
     * process.
     *
     * This call should invoked only from the "application level" i.e.
     * in response to a command-line argument or other high-level configuration
     * option.  In particular, it is inappropriate for library-level code
     * to think it knows how many physical threads a given process should have.
     *
     * By default, the number of physical threads allowed by a process is one.
     *
     * A process should never have more than \p num computationally active
     * threads at any given time.  (A thread which is sleeping, or waiting on
     * another thread is not deemed to be active.)
     *
     * In general, there is no (easy) means of automatically enforcing this
     * policy.  The best one can do is ensure that all code takes
     * takes care to call \c RequestExtraPhysicalThreads(),
     * \c ReleaseExtraPhysicalThreads(), \c Start() and
     * \c ParallelRequestAndWait() so as to never exceed this limit.
     */
    static void SetPhysicalThreadLimit(size_t num);

    /*!
     * \brief Return the number of physical threads allowed by this process.
     *
     * This simply returns the value set by a previous call to
     * \c SetPhysicalThreadLimit() (or one, if \c SetPhysicalThreadLimit() was
     * never called).  This call does \e not indicate how many physical threads
     * are currently reserved for use by other processes.
     */
    static size_t GetPhysicalThreadLimit();

    /*!
     * \brief Launch threads in parallel and wait for return.
     *
     * This function is the same as \c ParallelStart() except that the
     * function waits until all launched threads have completed, and there is
     * no access to threads' return values.  Note that any function of any type
     * can be converted to a \c boost::function<void>.
     *
     * The number of launched (logical) threads depends on how many unused
     * physical threads are available; in particular, this routine first
     * calls \c RequestExtraPhysicalThreads() to reserve all unused physical
     * threads, starts threads, and then releases the extra physical threads.
     *
     * In the case that only one thread is launched, this function
     * might not start a new thread, but instead simply execute the
     * function \p cl in the caller's thread.
     *
     * Before using this call, you should be familiar with the policies
     * for \ref threadDispatcher_allocation "allocating physical threads".
     *
     * The return value is the number of functions executed to run this job.
     * If the return value is one, the single function may or may not have run
     * in a new thread.
     */
    static size_t ParallelRequestAndWait(const boost::function<void ()>& fn);

    /*!
     * \overload
     *
     * This function is the same one-argument version, except that the
     * of tasks run in parallel will not exceed \p maxThreads (though it may
     * be smaller).
     */
    static size_t ParallelRequestAndWait(size_t maxThreads,
                                         const boost::function<void ()>& fn);

    /*!
     * \brief Attempt to allocate some number of unused physical threads.
     *
     * This function attempts to allocate \p n unused physical threads; the
     * actual number obtained (which may be zero) is returned.  The caller is
     * responsible at some point for calling \c ReleaseExtraPhysicalThreads()
     * with the value returned by this call.  Until a matching
     * \c ReleaseExtraPhysicalThreads() call occurs, the allocated physical
     * threads are reserved exclusively for the caller.
     *
     * Note that after having obtained \p p extra physical threads, the caller
     * may start up to \p p+1 threads, because the caller implicitly already
     * owns one physical thread (namely the physical thread the logical thread
     * is already running on).  (However note that starting up \p p+1 threads
     * but not waiting for them to finish constitutes actually using \p p+2
     * threads.)
     */
    static size_t RequestExtraPhysicalThreads(size_t n);

    /*!
     * \brief Return previously allocated physical threads.
     *
     * The parameter \p n should be the value previously returned by a call
     * to \p RequestExtraPhysicalThreads(), and \e not the value passed to
     * that call.
     */
    static void ReleaseExtraPhysicalThreads(size_t n);

    //! Return the number of threads in all dispatchers that are not finished.
    static size_t GetTotalPendingThreads();
        
    /*!
     * @}
     */

    /*!
     * \name Static functions for cancelation.
     *
     * Functions to control thread cancelation are supplied, but in practice,
     * thread cancelation works so badly that you should explore almost any
     * other option that will let you avoid using the functions in this group!
     * @{
     */

    /*
     * \brief Set cancelation state.
     *
     * By default, threads are created with cancelation disabled.
     * \c SetCancelState() allows cancelation to be turned on and off.
     * A typical use would be
     * \code
     *    ThreadDispatcher::SetCancelState(true);
     *    ReadInput();  // this command could block for a long time
     *    ThreadDispatcher::SetCancelState(false);
     * \endcode
     *
     * Note that these calls should be used with utmost caution;
     * cancelation of threads is very delicate, because a canceled thread
     * does \e not typically get a chance to execute destructors.
     * Also, enabling cancelation still only allows threads to be
     * killed at cancelation points (as defined by the POSIX standard).
     *
     * Please consult an expert before adding cancelation to your code.
     */
    static void SetCancelState(bool state) {
        int unused;
        pthread_setcancelstate(state ? PTHREAD_CANCEL_ENABLE :
                                       PTHREAD_CANCEL_DISABLE, &unused);
    }

    /*!
     * \brief Check for and honor any cancelation requests.
     *
     * If a thread wants to honor a cancelation request, the thread
     * may call the static function \c HonorCancelation().  If a cancelation
     * request has been posted for the thread, this call will not return.
     * Otherwise, no cancelation notice has been made and the thread continues
     * executing.  It is not possible to test and see if a thread is going
     * to be canceled, prior to actually running this call.
     *
     * Note that pool-mode threads are not subject to cancelation.
     */

    static void AllowCancelation() {
        int unused;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &unused); 
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &unused);
    }

    /*!
     * @}
     */

    uint64_t GetPoolBlockTime() const {
        return _pool._nBlockedTicks;
    }

    /// Sends a notice to request that background threads be shut down. This
    /// sends a TfStopBackgroundThreads notice.
    static void StopBackgroundThreads();
    
private:
    typedef TfThreadInfo::_SharedBarrier _SharedBarrier;
        
    template <class RET>
    typename TfThread<RET>::Ptr
    _Launch(const boost::function<RET ()>& func,
            const TfRefPtr<_SharedBarrier>& bPtr,
            int index = 0, int nThreads = 1) {
        TfThreadInfo* info = _singleThreaded ?
                                NULL : CreateThreadInfo(index, nThreads);
        if (info)
            info->_SetSharedBarrier(bPtr ? bPtr : _SharedBarrier::New(nThreads));

        // establish link to thread return data, so it won't be deleted
        // before we return, in the case that the thread runs quickly
        TfThread<RET>* thread = new TfThread<RET>(func, info);
        typename TfThread<RET>::Ptr threadPtr = TfCreateRefPtr(thread);

        _SubmitThread(thread);
        return threadPtr;
    }
    
    template <class RET>
    std::vector<typename TfThread<RET>::Ptr>
    _PLaunch(size_t n, const boost::function<RET ()>& func) {
        TfRefPtr<_SharedBarrier> bPtr = _SharedBarrier::New(n);
        std::vector<typename TfThread<RET>::Ptr> threads;
        for (size_t i = 0; i < n; i++)
            threads.push_back(_Launch(func, bPtr, i, n));
        return threads;
    }

    void _SubmitThread(TfThreadBase*);

    bool _FlushWaitingPoolThread(TfThreadBase::Ptr tPtr);

    static TfThreadDispatcher& _GetAnonymousDispatcher();

    static void  _TaskCleanupHandler(void* data);
    static void* _ImmediateTask(void*);     // pthread_create callback
    static void* _PoolTask(void*);          // pthread_create callback

    void _RunThreadFromQueue(TfThreadInfo::_ThreadDataTable* longTerm);

    const size_t    _stackSize;
    bool            _poolMode;
    pthread_attr_t  _detachedAttr, _joinableAttr;
    bool            _singleThreaded;        // disable threading
    
    struct _Pool {
        mutable TfFastMutex _threadListMutex;
        TfSemaphore     _threadListSemaphore;
        size_t          _maxThreads;
        TfAtomicInteger _nIdleWorkers;
        uint64_t        _nBlockedTicks;
        bool            _lifoMode;
        bool            _shuttingDown;
        TfThreadDispatcher* _dispatcher;
        
        std::vector<pthread_t> _workerIds;
        std::deque<TfThreadBase*> _waitingThreads;
        
        void _Add(TfThreadBase* thread, TfThreadDispatcher*);
        void _FlushPendingThreads();
    };

    static int          _nTotalThreadsPending,
                        _nExtraPhysicalThreadsAllowed,
                        _nExtraPhysicalThreadsAvailable;
    
    _Pool               _pool;

    mutable TfCondVar   _allThreadsDoneCond;
    mutable TfMutex     _allThreadsDoneMutex;
    bool                _allThreadsDone;

    TfAtomicInteger     _nThreadsPending;
    friend class TfThreadDispatcher::_Pool;
    friend class TfThreadBase;
};

inline TfThreadDispatcher*
TfThreadInfo::GetThreadDispatcher() {
    return _thread ? _thread->GetThreadDispatcher() : NULL;
}

/// \struct TfStopBackgroundThreadsNotice
///
/// Sent to request that background threads shut down. Any client that does
/// background computation should listen for and respond to this notice by
/// stopping background computation.
struct TfStopBackgroundThreadsNotice : public TfNotice {
    virtual ~TfStopBackgroundThreadsNotice();
};

#endif
