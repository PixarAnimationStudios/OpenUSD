#ifndef TF_THREADINFO_H
#define TF_THREADINFO_H


#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/barrier.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/arch/atomicOperations.h"
#include <pthread.h>
#include "pxr/base/tf/hashmap.h"

/*!
 * \file threadInfo.h
 * \ingroup group_tf_Multithreading
 */

class TfThreadBase;
class TfThreadDispatcher;
template <typename T> class TfThreadData;

/*!
 * \class TfThreadInfo ThreadInfo.h pxr/base/tf/threadInfo.h
 * \brief Thread-specific data for threads launched by \c TfThreadDispatcher.
 * \ingroup group_tf_Multithreading
 * 
 * When launching several threads on the same task, it is often useful
 * for each thread to be told (a) how many other threads are working on that
 * task, and (b) a unique index (in the range 0 to nthreads-1).
 * The \c TfThreadDispatcher class assists the user by creating a
 * \c TfThreadInfo structure that is unique to each thread; a thread
 * retrieves its own \c TfThreadInfo by calling the static function
 * \c TfThreadInfo::Store().  See \ref TfThreadDispatcher_tsdData for
 * an example of how thread-specific data can be augmented.
 */
class TfThreadInfo {
public:
    //! Construct info for the \p index th of \p n thread.
    TfThreadInfo(size_t index, size_t n, TfThreadInfo* parent);

    //! Destructor.
    virtual ~TfThreadInfo();
    
    /*!
     * \brief Return the \c pthread_t identifier of this thread.
     *
     * This returns the same value as \c pthread_self(); given that
     * you already have a pointer to your \c TfThreadInfo object,
     * a call to \c GetSelf() is slightly cheaper than calling
     * \c pthread_self().
     */
    pthread_t GetSelf() const {
        return _threadId;
    }
    
    /*!
     * \brief Return the index of this thread.
     *
     * The index is a number in the range 0 to n-1 (inclusive)
     * where \p n is the number of threads launched in parallel on this
     * particular task.
     */
    size_t GetIndex() const {
        return _threadIndex;
    }

    /*!
     * \brief Return the globally unique non-recycled id for this thread.
     */
    int GetUniqueId() const {
        return _uniqueThreadId;
    }

    //! Return the total number of threads launched in parallel for this task.
    size_t GetNumThreads() const {
        return _nThreads;
    }
    
    /*!
     * \brief Get the index bounds for a thread when dividing up a loop.
     *
     * An extremely common multithreading technique is to carve a loop
     * from 0 to n into equal sized pieces, by launching multiple threads
     * using either \c TfThreadDispatcher::ParallelRequestAndWait() or
     * \c TfThreadDispatcher::ParallelStart().  In this case, each launched
     * thread should iterate on only a subset of the indices between 0 and n.
     * \c GetIndexBounds() returns the portion of the loop a thread is
     * responsible for.  As an example,
     * \code
     * void ComputeResult(vector<double>* result) {
     *     size_t i0, i1;
     *     TfThreadInfo::Find()->GetIndexBounds(result->size(), &i0, &i1);
     *
     *     for (size_t i = i0; i < i1; i++)
     *         (*result)[i] = DoComputation(i);
     * }
     * \endcode
     *
     * If only one thread is launched on this task (or \c ComputeResult()
     * is simply called without multithreading) then \c i0 and \c i1 are
     * set to 0 and \c result->size() respectively.  For two threads, the
     * first thread would run with \c i0 and \c i1 set to 0
     * and \c result->size()/2, while the second
     * thread would find that \c i0 was \c result->size()/2+1, and
     * \c i1 was \c result->size().
     */
    void GetIndexBounds(size_t n, size_t* i0, size_t* i1) const {
        size_t perTask = n / _nThreads;
        *i0 = _threadIndex * perTask;
        *i1 = (_threadIndex == _nThreads-1) ? n : perTask * (_threadIndex + 1);
    }
     
    //! \overload
    void GetIndexBounds(int n, int* i0, int* i1) const {
        int perTask = n / _nThreads;
        *i0 = _threadIndex * perTask;
        *i1 = (_threadIndex == _nThreads-1) ? n : perTask * (_threadIndex + 1);
    }

    /*!
     * \brief Return the \c TfBarrier associated with this thread.
     *
     * Threads launched by \c ParallelStart() or \c ParallelRequestAndWait()
     * share a \c TfBarrier, whose size is set to the number of threads
     * started in a group.  Otherwise, the \c TfBarrier has size set to one.
     */
    TfBarrier& GetBarrier() {
        return _sharedBarrierPtr->GetBarrier();
    }
    
    //! Return the parent that created this thread, or NULL (if not created)
    // through \c TfThreadDispatcher.
    TfThreadInfo* GetParent() const {
        return _parent;
    }
    
    /*!
     * \brief Return the dispatcher that created this thread.
     *
     * If this thread was not created by \c TfThreadDispatcher,
     * NULL is returned.
     */

    // code for this is in threadDispatcher.h
    inline TfThreadDispatcher* GetThreadDispatcher();

    /*!
     * \brief Return the thread-specific \c TfThreadInfo* for this thread.
     *
     * Each thread has a \c TfThreadInfo object assigned to it.  \c Find()
     * returns a pointer to the \c TfThreadInfo object assigned to a thread.
     * A thread does not need to have been launched by \c TfThreadDispatcher
     * to call this function.
     */
    static TfThreadInfo* Find() {
        if (!TfThreadInfo::_initialized)
            _InitializeTsdKey();

        if (void *value = pthread_getspecific(*_tsdKey))
            return reinterpret_cast<TfThreadInfo*>(value);
        else
            return _CreateFree();
    }

    //! Enum type for \c TfThreadData constructor
    enum DataLifetime {
        SHORT_TERM,     //!< (default) create this data as a short-term object
        LONG_TERM       //!< create this data as a long-term object
    };

private:
    class _SharedBarrier : public TfRefBase {
    public:
        static TfRefPtr<_SharedBarrier> New(size_t n) {
            return TfCreateRefPtr(new _SharedBarrier(n));
        }

        TfBarrier& GetBarrier() {
            return _barrier;
        }
    private:
        _SharedBarrier(size_t n)
            : _barrier(n) {
        }

        TfBarrier _barrier;
    };

    static pthread_key_t* _tsdKey;
    static volatile bool _initialized;

    void _Store() {
        _threadId = pthread_self();
        pthread_setspecific(*_tsdKey, this);
    }

    void _SetSharedBarrier(const TfRefPtr<_SharedBarrier>& ptr) {
        _sharedBarrierPtr = ptr;
    }
    
    static void _InitializeTsdKey();
    static void _AutoDtor(void* key);

    static TfThreadInfo* _CreateFree();

    pthread_t            _threadId;     // caches pthread_self()
    size_t               _threadIndex,  // for parallel tasks
                         _nThreads;
    int                  _uniqueThreadId;
    TfThreadBase*        _thread;
    TfThreadInfo*        _parent;
    TfRefPtr<_SharedBarrier> _sharedBarrierPtr;
    
    /*
     * These classes are used to implement TfThreadData.
     * Basically, a TfThreadData holds an integer key, which is
     * a lookup into a map of _ThreadData<T> pointers.
     */
    class _UntypedThreadData {
    protected:
        _UntypedThreadData(void* data)
            : _data(data)
        {
        }

        virtual ~_UntypedThreadData();
    private:
        void* _Get() {
            return _data;
        }
            
        void* _data;

        template <typename T> friend class TfThreadData;
        friend class TfThreadInfo;
	friend class TfThreadDispatcher;
    };

    template <typename T>
    class _ThreadData : public _UntypedThreadData {
        _ThreadData(const T& defaultValue)
            : _UntypedThreadData(&_data),
              _data(defaultValue)
        {
        }
        
        T _data;
        friend class TfThreadInfo;
        template <typename U> friend class TfThreadData;
        template <typename U> friend class TfFastThreadData;
    };

    /*
     * The map *_shortTermThreadDataTable is destroyed when *this
     * is destroyed.  In contrast, if *this was created for a pool-mode thread,
     * the next pool-mode thread that occupies the same physical pthread as
     * *this has the map pointed to by _longTermThreadDataTable transferred to it.
     * Eventually, when the pool-mode physical pthread is destroyed, the long
     * term data map is destroyed.
     */

    /*
     * This counter is used to generate a unique key for every TfThreadData
     * object.
     */
    static int _threadDataKeyCount;     

    static int _GetNextThreadDataKey() {
        return ArchAtomicIntegerFetchAndAdd(
                                  &TfThreadInfo::_threadDataKeyCount, 1);
    }

    typedef TfHashMap<int, _UntypedThreadData*> _ThreadDataTable;
    _ThreadDataTable* _shortTermThreadDataTable;
    _ThreadDataTable* _longTermThreadDataTable;

    void _SetLongTermThreadDataTable(_ThreadDataTable* t) {
        _longTermThreadDataTable = t;
    }
    
    _ThreadDataTable& _GetThreadDataTable(bool shortTerm) {
        return shortTerm ? *_shortTermThreadDataTable : *_longTermThreadDataTable;
    }
    
    friend class TfThreadDispatcher;
    friend class TfThreadBase;
    template <typename T> friend class TfThreadData;
    template <typename T> friend class TfFastThreadData;
};                              





#endif
