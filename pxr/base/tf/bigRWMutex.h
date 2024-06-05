//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_BIG_RW_MUTEX_H
#define PXR_BASE_TF_BIG_RW_MUTEX_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include "pxr/base/arch/align.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/spinRWMutex.h"

#include <atomic>
#include <memory>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfBigRWMutex
///
/// This class implements a readers-writer mutex and provides a scoped lock
/// utility.  Multiple clients may acquire a read lock simultaneously, but only
/// one client may hold a write lock, exclusive to all other locks.
///
/// This class emphasizes throughput for (and is thus best used in) the case
/// where there are many simultaneous reader clients all concurrently taking
/// read locks, with clients almost never taking write locks.  As such, taking a
/// read lock is a lightweight operation that usually does not imply much
/// hardware-level concurrency penalty (i.e. writes to shared cache lines).
/// This is done by allocating several cache-line-sized chunks of memory to
/// represent lock state, and readers typically only deal with a single lock
/// state (and therefore a single cache line).  On the other hand, taking a
/// write lock is very expensive from a hardware concurrency point of view; it
/// requires atomic memory operations on every cache-line.
///
/// To achieve good throughput under highly read-contended workloads, this class
/// allocates 10s of cachelines worth of state (~1 KB) to help minimize
/// hardware-level contention.  So it is probably not appropriate to use as
/// (e.g.) a member variable in an object that there are likely to be many of.
///
/// This class has been measured to show >10x throughput compared to
/// tbb::spin_rw_mutex, and >100x better throughput compared to
/// tbb::queuing_rw_mutex on reader-contention-heavy loads.  The tradeoff being
/// the relatively large size required compared to these other classes.
/// 
class TfBigRWMutex
{
public:
    // Number of different cache-line-sized lock states.
    static constexpr unsigned NumStates = 16;

    // Lock states -- 0 means not locked, -1 means locked for write, other
    // positive values count the number of readers locking this particular lock
    // state object.
    static constexpr int NotLocked = 0;
    static constexpr int WriteLocked = -1;

    /// Construct a mutex, initially unlocked.
    TF_API TfBigRWMutex();

    /// Scoped lock utility class.  API modeled after
    /// tbb::spin_rw_mutex::scoped_lock.
    struct ScopedLock {

        // Acquisition states: -1 means not acquired, -2 means acquired for
        // write (exclusive lock), >= 0 indicates locked for read, and the value
        // indicates which lock state index the reader has incremented.
        static constexpr int NotAcquired = -1;
        static constexpr int WriteAcquired = -2;

        /// Construct a scoped lock for mutex \p m and acquire either a read or
        /// a write lock depending on \p write.
        explicit ScopedLock(TfBigRWMutex &m, bool write=true)
            : _mutex(&m)
            , _acqState(NotAcquired) {
            Acquire(write);
        }

        /// Construct a scoped lock not associated with a \p mutex.
        ScopedLock() : _mutex(nullptr), _acqState(NotAcquired) {}

        /// If this scoped lock is acquired for either read or write, Release()
        /// it.
        ~ScopedLock() {
            Release();
        }

        /// If the current scoped lock is acquired, Release() it, then associate
        /// this lock with \p m and acquire either a read or a write lock,
        /// depending on \p write.
        void Acquire(TfBigRWMutex &m, bool write=true) {
            Release();
            _mutex = &m;
            Acquire(write);
        }            

        /// Acquire either a read or write lock on this lock's associated mutex
        /// depending on \p write.  This lock must be associated with a mutex
        /// (typically by construction or by a call to Acquire() that takes a
        /// mutex).  This lock must not already be acquired when calling
        /// Acquire().
        void Acquire(bool write=true) {
            if (write) {
                AcquireWrite();
            }
            else {
                AcquireRead();
            }
        }

        /// Release the currently required lock on the associated mutex.  If
        /// this lock is not currently acquired, silently do nothing.
        void Release() {
            switch (_acqState) {
            case NotAcquired:
                break;
            case WriteAcquired:
                _ReleaseWrite();
                break;
            default:
                _ReleaseRead();
                break;
            };
        }

        /// Acquire a read lock on this lock's associated mutex.  This lock must
        /// not already be acquired when calling \p AcquireRead().
        void AcquireRead() {
            TF_AXIOM(_acqState == NotAcquired);
            _acqState = _mutex->_AcquireRead(_GetSeed());
        }

        /// Acquire a write lock on this lock's associated mutex.  This lock
        /// must not already be acquired when calling \p AcquireWrite().
        void AcquireWrite() {
            TF_AXIOM(_acqState == NotAcquired);
            _mutex->_AcquireWrite();
            _acqState = WriteAcquired;
        }

        /// Change this lock's acquisition state from a read lock to a write
        /// lock.  This lock must already be acquired for reading.  For
        /// consistency with tbb, this function returns a bool indicating
        /// whether the upgrade was done atomically, without releasing the
        /// read-lock.  However the current implementation always releases the
        /// read lock so this function always returns false.
        bool UpgradeToWriter() {
            TF_AXIOM(_acqState >= 0);
            Release();
            AcquireWrite();
            return false;
        }
        
    private:

        void _ReleaseRead() {
            TF_AXIOM(_acqState >= 0);
            _mutex->_ReleaseRead(_acqState);
            _acqState = NotAcquired;
        }

        void _ReleaseWrite() {
            TF_AXIOM(_acqState == WriteAcquired);
            _mutex->_ReleaseWrite();
            _acqState = NotAcquired;
        }

        // Helper for returning a seed value associated with this lock object.
        // This helps determine which lock state a read-lock should use.
        inline int _GetSeed() const {
            return static_cast<int>(
                static_cast<unsigned>(TfHash()(this)) >> 8);
        }

        TfBigRWMutex *_mutex;
        int _acqState; // NotAcquired (-1), WriteAcquired (-2), otherwise
                       // acquired for read, and index indicates which lock
                       // state we are associated with.
    };

private:

    // Optimistic read-lock case inlined.
    inline int _AcquireRead(int seed) {
        // Determine a lock state index to use.
        int stateIndex = seed % NumStates;
        if (ARCH_UNLIKELY(_writerActive) ||
            !_states[stateIndex].mutex.TryAcquireRead()) {
            _AcquireReadContended(stateIndex);
        }
        return stateIndex;
    }

    // Contended read-lock helper.
    TF_API void _AcquireReadContended(int stateIndex);

    void _ReleaseRead(int stateIndex) {
        _states[stateIndex].mutex.ReleaseRead();
    }

    TF_API void _AcquireWrite();
    TF_API void _ReleaseWrite();
    
    struct _LockState {
        TfSpinRWMutex mutex;
        // This padding ensures that \p state instances sit on different cache
        // lines.
        char _unused_padding[
            ARCH_CACHE_LINE_SIZE-(sizeof(mutex) % ARCH_CACHE_LINE_SIZE)];
    };

    std::unique_ptr<_LockState []> _states;
    std::atomic<bool> _writerActive;
    
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_BIG_RW_MUTEX_H
