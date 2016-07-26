#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/typeFunctions.h"
#include "pxr/base/arch/threads.h"

#include <boost/function.hpp>

#include <algorithm>
#include <cstdio>
#include <iostream>

using boost::function;

using std::vector;
using std::deque;



/*
 * We keep track of which threads are still active using TfThreadStateVar's;
 * this lets us Wait() or TimedWait() on any individual thread.  To check
 * if they're all done, there is a TfCondVar _allThreadsDoneCond we
 * can wait on.
 */

int TfThreadDispatcher::_nTotalThreadsPending = 0;
int TfThreadDispatcher::_nExtraPhysicalThreadsAllowed = 0;
int TfThreadDispatcher::_nExtraPhysicalThreadsAvailable = 0;

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define< TfStopBackgroundThreadsNotice,
                    TfType::Bases<TfNotice> >();
}

TfStopBackgroundThreadsNotice::~TfStopBackgroundThreadsNotice() { }

TfThreadDispatcher::TfThreadDispatcher(size_t maxPoolThreads, size_t stackSize)
    : _stackSize(stackSize)
{
    /*
     * Ensure that thread-specific data is setup and ready.
     */
    (void) TfThreadInfo::Find();

    _singleThreaded = false;
    
    _poolMode = false;
    _pool._maxThreads = maxPoolThreads;
    _pool._nBlockedTicks = 0;
    _pool._lifoMode = false;
    _pool._shuttingDown = false;
    _pool._dispatcher = this;
    
    if (pthread_attr_init(&_detachedAttr) != 0 ||
        pthread_attr_init(&_joinableAttr) != 0) {
        // CODE_COVERAGE_OFF - this should not happen
        TF_FATAL_ERROR("error initializing attributes");
        // CODE_COVERAGE_ON
    }
    
    if (pthread_attr_setstacksize(&_detachedAttr, _stackSize) != 0 ||
        pthread_attr_setstacksize(&_joinableAttr, _stackSize) != 0) {
        // CODE_COVERAGE_OFF - this should not happen
        TF_FATAL_ERROR("error setting stacksize");
        // CODE_COVERAGE_ON
    }

    if (pthread_attr_setdetachstate(&_detachedAttr, PTHREAD_CREATE_DETACHED) != 0 ||
        pthread_attr_setdetachstate(&_joinableAttr, PTHREAD_CREATE_JOINABLE) != 0) {
        // CODE_COVERAGE_OFF - this should not happen
        TF_FATAL_ERROR("error setting detatch state");
        // CODE_COVERAGE_ON
    }

    _pool._nIdleWorkers.Set(0);
    _allThreadsDone = true;
    _nThreadsPending.Set(0);
}

TfThreadDispatcher::~TfThreadDispatcher()
{
    if (!IsDone())
        TF_FATAL_ERROR("destructor called with running threads");

    _pool._shuttingDown = true;
    
    for (size_t i = 0; i < _pool._workerIds.size(); i++) {
        _pool._threadListSemaphore.Post();
    }
    
    for (size_t i = 0; i < _pool._workerIds.size(); i++) {
        void* ignored;
        if (pthread_join(_pool._workerIds[i], &ignored) != 0) {
            // post an error?
        }
    }

    TF_VERIFY(pthread_attr_destroy(&_detachedAttr) == 0 and
              pthread_attr_destroy(&_joinableAttr) == 0);
}

/*
 * This is deliberately not thread-safe: only the main thread of the application
 * should be calling this anyway...
 */

void
TfThreadDispatcher::SetPhysicalThreadLimit(size_t num)
{
    if (num == 0) {
        TF_CODING_ERROR("cannot set number of physical threads to zero");
        return;
    }

    ArchSetThreadConcurrency(num);

    int previous = _nExtraPhysicalThreadsAllowed;

    _nExtraPhysicalThreadsAllowed = int(num - 1);

    // The reason that we compute the delta instead of just setting the
    // new value, is that _nExtraPhysicalThreadsAvailable may have already
    // been modified by code that has allocated some physical threads before
    // this call was made.  In that case we want to increase (or decrease)
    // the correct number of available threads based on the value passed
    // in to this function.  Note that the delta may be negative, and it
    // may make the number of available physical threads negative.  This
    // should be handled correctly elsewhere.
    _nExtraPhysicalThreadsAvailable += 
        (_nExtraPhysicalThreadsAllowed - previous);

}

size_t
TfThreadDispatcher::GetPhysicalThreadLimit()
{
    return _nExtraPhysicalThreadsAllowed + 1;
}

size_t
TfThreadDispatcher::RequestExtraPhysicalThreads(size_t n)
{
    int previous = ArchAtomicIntegerFetchAndAdd(
        &_nExtraPhysicalThreadsAvailable, -int(n));
    if (previous >= int(n))
        return n;
    else if (previous >= 0) {
        int shortFall = int(n) - previous;
        ArchAtomicIntegerFetchAndAdd(&_nExtraPhysicalThreadsAvailable,
                                     shortFall);
        /*
         * At this point, we changed the count by exactly
         *     -n + shortFall = -n + (n - previous) = previous.
         * which means we're entitled to say we took previous physical
         * threads away.
         */
        return previous;
    }
    else {
        /*
         * Previous was negative when we started.
         * So undo the change we just made, and signal there's nothing
         * available.
         */
        ArchAtomicIntegerFetchAndAdd(&_nExtraPhysicalThreadsAvailable, int(n));
        return 0;
    }
}

void
TfThreadDispatcher::ReleaseExtraPhysicalThreads(size_t n)
{
    int previous = ArchAtomicIntegerFetchAndAdd(
        &_nExtraPhysicalThreadsAvailable, int(n));
    if (previous + int(n) > _nExtraPhysicalThreadsAllowed) {
        TF_CODING_ERROR(
            "released %zd physical threads + available %d physical threads "
            "> total %d physical threads",
            n, previous, _nExtraPhysicalThreadsAllowed);
        
        _nExtraPhysicalThreadsAvailable = _nExtraPhysicalThreadsAllowed;
    }
}

size_t
TfThreadDispatcher::GetTotalPendingThreads()
{
    return _nTotalThreadsPending;
}

size_t
TfThreadDispatcher::ParallelRequestAndWait(const function<void ()>& cl)
{
    return ParallelRequestAndWait(_nExtraPhysicalThreadsAllowed + 1, cl);
}

size_t
TfThreadDispatcher::ParallelRequestAndWait(size_t nThreads,
                                           const function<void ()>& fn)
{
    if (nThreads == 0)
        return 0;
    
    /*
     * For the case of one thread, launched from a thread which is itself
     * part of a single thread (this includes the main thread) just run the
     * function directly.  Otherwise, just use regular ParallelStart() and
     * wait on each returned thread.
     */
    if (nThreads == 1 && TfThreadInfo::Find()->GetNumThreads() == 1) {
        fn();
        return 1;
    }
    else {
        /*
         * For now, just create a new dispatcher, run the threads,
         * and be done.  What would be better though would be to keep
         * a dispatcher with pool threads around, and use that.  With
         * glibc-2.2.4, that's too dangerous -- programs can hang on exit
         * if there are still threads that are alive.  Once we get past
         * glibc-2.2.4, though, we should instead keep a single dispatcher
         * in pool-mode around.  Additionally, we'll need to play the game
         * of running n-1 threads in the dispatcher, and the then using this
         * thread to execute the function (i.e. make a new TfThreadInfo,
         * switch it in, run the function, switch it back, etc.).
         */
        TfThreadDispatcher d;
        size_t nExtra =
            TfThreadDispatcher::RequestExtraPhysicalThreads(nThreads - 1);

        vector<TfThread<void>::Ptr> threads = d.ParallelStart(nExtra + 1, fn);
        for (size_t i = 0; i < threads.size(); i++)
            threads[i]->Wait();

        TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra);
        return threads.size();
    }
}
        
/*
 * Global thread dispatcher, for "fire-and-forget" threads.
 */

TfThreadDispatcher&
TfThreadDispatcher::_GetAnonymousDispatcher()
{
    static TfThreadDispatcher* anonymous = NULL;
    TF_EXECUTE_ONCE({
        size_t stackSize = ArchGetDefaultThreadStackSize();
        anonymous = new TfThreadDispatcher(INT_MAX,stackSize);
    });
    return *anonymous;
}

TfThreadInfo*
TfThreadDispatcher::CreateThreadInfo(size_t index, size_t nThreads)
{
    return new TfThreadInfo(index, nThreads, TfThreadInfo::Find());
}
    
void
TfThreadDispatcher::_Pool::_Add(TfThreadBase* thread, TfThreadDispatcher* d)
{
    {
        TF_SCOPED_AUTO(_threadListMutex);
        _waitingThreads.push_back(thread);

        if (size_t(_nIdleWorkers.Get()) < _waitingThreads.size() &&
            _workerIds.size() < _maxThreads) {
            pthread_t id;

            _nIdleWorkers.Increment();
            if (ARCH_UNLIKELY(pthread_create(&id, &d->_joinableAttr,
                                             TfThreadDispatcher::_PoolTask,
                                             this) < 0)) {
                TF_FATAL_ERROR("pthread_create failed");
            }
            _workerIds.push_back(id);
        }
    }
    
    _threadListSemaphore.Post();
}

/*
 * Register a new thread to be run.  If not in pool mode, launch in its
 * own thread; otherwise, add to the pool's queue of waiting threads.
 */
void
TfThreadDispatcher::_SubmitThread(TfThreadBase* thread)
{
    {
        TF_SCOPED_AUTO(_allThreadsDoneMutex);
        _allThreadsDone = false;
        
        _nThreadsPending.Increment();
        ArchAtomicIntegerIncrement(&_nTotalThreadsPending);
    }
    thread->_dispatcher = this;
    
    // prevent thread structure from destructing even if it is immediately
    // forgotten about by dispatching agent
    thread->_self = TfRefPtr<TfThreadBase>(thread);
    
    if (_singleThreaded || (_poolMode && _pool._maxThreads == 0)) {
        thread->_inDispatcherPool = false;
        thread->_id = pthread_self();
        TfThreadDispatcher::_ImmediateTask(thread);
    }
    else if (_poolMode) {
        thread->_inDispatcherPool = true;
        _pool._Add(thread, this);
    }
    else {
        thread->_inDispatcherPool = false;

        if (ARCH_UNLIKELY(pthread_create(&thread->_id, &_detachedAttr,
                                         TfThreadDispatcher::_ImmediateTask,
                                         thread) < 0)) {
            TF_FATAL_ERROR("pthread_create failed");
        }
    }
}

void
TfThreadDispatcher::FlushPendingPoolThreads()
{
    TF_SCOPED_AUTO(_pool._threadListMutex);

    while (!_pool._waitingThreads.empty()) {
        TfThreadBase* thread = _pool._waitingThreads.front();
        _pool._waitingThreads.pop_front();
        thread->_finishedFunc = false;
        _TaskCleanupHandler(thread);
    }

    TF_AXIOM(_pool._waitingThreads.empty());
}

bool
TfThreadDispatcher::_FlushWaitingPoolThread(TfThreadBase::Ptr tPtr)
{
    TfThreadBase* thread = TfTypeFunctions<TfThreadBase::Ptr>::GetRawPtr(tPtr);
    TF_SCOPED_AUTO(_pool._threadListMutex);

    /*
     * If it's on the list, do the remove/erase thing:
     */

    deque<TfThreadBase*>& wl = _pool._waitingThreads;
    deque<TfThreadBase*>::iterator it = std::remove(wl.begin(), wl.end(), thread);

    if (it != wl.end()) {
        wl.erase(it, wl.end());
        thread->_finishedFunc = false;
        _TaskCleanupHandler(thread);
        return true;
    }
    else
        return false;
}

/*
 * Called when a thread terminates, either by cancellation or normally.
 */
void
TfThreadDispatcher::_TaskCleanupHandler(void* data)
{
    TfThreadBase* thread = reinterpret_cast<TfThreadBase*>(data);    
    
    if (!thread->_finishedFunc)
        thread->_canceled = true;
    
    if (!thread->_launchedSingleThreaded) {
        /*
         * Do this before setting our "finished" bit.
         */
        delete thread->_threadInfo;
    }
    
    {    
        TF_SCOPED_AUTO(thread->_dispatcher->_allThreadsDoneMutex);
        
        thread->_threadInfo = NULL;
        thread->_dispatcher->_nThreadsPending.Decrement();
        ArchAtomicIntegerDecrement(&_nTotalThreadsPending);
        
        if (thread->_dispatcher->_nThreadsPending.Get() == 0) {
            thread->_dispatcher->_allThreadsDone = true;
            thread->_dispatcher->_allThreadsDoneCond.Broadcast();
        }
    }

    {
        TF_SCOPED_AUTO(thread->_finishedMutex);
        thread->_finished = true;
        thread->_finishedCondVar.Broadcast();
    }

    /*
     * It is now safe for thread to be destroyed; remove the circular
     * link that may be the only thing keeping it alive.  Note
     * that setting thread->_self to NULL may cause thread to point
     * to freed memory.  So we first need to completely finish up before we
     * do this:
     */

    if (!thread->_launchedSingleThreaded) {
        /*
         * Ensure the auto-destructor mechanism doesn't run on the tsd key,
         * since we just nuked it (above).
         */
        pthread_setspecific(*TfThreadInfo::_tsdKey, 0);
    }

    thread->_self.Reset();       // break reference-count cycle;
                                 // may invoke "delete thread".  
}

/*
 * This is the pthread_create callback routine for non-pool threads.
 * Threads are begun with cancellation disabled.
 */
void*
TfThreadDispatcher::_ImmediateTask(void* data)
{
    TfThreadBase* thread = reinterpret_cast<TfThreadBase*>(data);
    int unused;
    
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &unused);

    pthread_cleanup_push(TfThreadDispatcher::_TaskCleanupHandler, thread);
    thread->_finishedFunc = false;
    thread->_ExecuteFunc();
    thread->_finishedFunc = true;
    pthread_cleanup_pop(1);

    return NULL;
}

/*
 * This is the pthread_create callback routine for threads which
 * are part of the pool.  While waiting for a task, the thread is
 * cancellable, but once the task starts, the thread switches back
 * to cancellation disabled.  When the task ends, or if the thread
 * is cancelled, it calls the _TaskCleanupHandler for clean termination.
 */
void*
TfThreadDispatcher::_PoolTask(void* ptr)
{
    _Pool& pool = *(reinterpret_cast<_Pool*>(ptr));

    /*
     * For pool-mode threads, we create a long-term data table, which
     * we maintain during the lifetime of this function (which is the
     * lifetime of the pool thread).  Since a TfThreadInfo
     * creates/destroys both a short term and a long term table, we
     * play the game of substituting our long term table in, executing
     * the thread, and then restoring the long term table that the
     * TfThreadInfo structure originally allocated.  See threadData.h
     * for more details.
     */
     
    TfThreadInfo::_ThreadDataTable* longTerm =
                                new TfThreadInfo::_ThreadDataTable;
    
    for (;;) {
        int unused;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &unused);
        pool._threadListSemaphore.Wait();           // wait for some work
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &unused);

        if (pool._shuttingDown) {
	    TF_FOR_ALL(it, *longTerm)
		delete it->second;
	    delete longTerm;
            pthread_exit(0);
	}

        pool._nIdleWorkers.Decrement();
        pool._dispatcher->_RunThreadFromQueue(longTerm);
        pool._nIdleWorkers.Increment();
    }
}

void
TfThreadDispatcher::_RunThreadFromQueue(TfThreadInfo::_ThreadDataTable* longTerm)
{
    TfThreadBase* thread;
    
    {
        TF_SCOPED_AUTO(_pool._threadListMutex);
            
        if (_pool._waitingThreads.empty())
            return;
        
        if (_pool._lifoMode) {
            thread = _pool._waitingThreads.back();
            _pool._waitingThreads.pop_back();
        } else {
            thread = _pool._waitingThreads.front();
            _pool._waitingThreads.pop_front();
        }
    }

    pthread_cleanup_push(TfThreadDispatcher::_TaskCleanupHandler, thread);

    TfThreadInfo::_ThreadDataTable* save = thread->_threadInfo->_longTermThreadDataTable;
    thread->_threadInfo->_longTermThreadDataTable = longTerm;

    thread->_finishedFunc = false;
    thread->_ExecuteFunc();
    thread->_finishedFunc = true;

    thread->_threadInfo->_longTermThreadDataTable = save;
        
    pthread_cleanup_pop(1);
}

void
TfThreadDispatcher::StopBackgroundThreads()
{
    TfStopBackgroundThreadsNotice().Send();
}

