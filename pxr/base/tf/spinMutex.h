//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SPIN_MUTEX_H
#define PXR_BASE_TF_SPIN_MUTEX_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfSpinMutex
///
/// This class implements a simple spin lock that emphasizes throughput when
/// there is little to no contention.  Like all spin locks, any contention
/// performs poorly; consider a different algorithm design or synchronization
/// strategy in that case.
///
/// This class provides a nested TfSpinMutex::ScopedLock that makes it easy to
/// acquire locks and have those locks automatically release when the ScopedLock
/// is destroyed.
///
/// TfSpinMutex is observed to compile to the same instruction sequence as
/// tbb::spin_mutex on x86-64 for uncontended lock/unlock.  The main difference
/// between TfSpinMutex and tbb:spin_mutex is that, for contended lock
/// operations, TfSpinMutex calls an out-of-line function to handle spinning &
/// backoff, while the tbb::spin_mutex inlines that code.  This translates to 4
/// instructions inlined to take a TfSpinMutex lock, compared to 28 instructions
/// inlined for tbb:spin_mutex at the time of this writing.  Correspondingly
/// tbb::spin_mutex offers ~2% better throughput under high contention.  But
/// again, avoid spin locks if you have contention.
/// 
class TfSpinMutex
{
public:

    /// Construct a mutex, initially unlocked.
    TfSpinMutex() : _lockState(false) {}

    /// Scoped lock utility class.  API modeled roughly after
    /// tbb::spin_rw_mutex::scoped_lock.
    struct ScopedLock {

        /// Construct a scoped lock for mutex \p m and acquire a lock.
        explicit ScopedLock(TfSpinMutex &m) : _mutex(&m) {
            Acquire();
        }

        /// Construct a scoped lock not associated with a \p mutex.
        ScopedLock() = default;

        /// If this scoped lock is acquired, Release() it.
        ~ScopedLock() {
            Release();
        }

        /// If the current scoped lock is acquired, Release() it, then associate
        /// this lock with \p m and acquire a lock.
        void Acquire(TfSpinMutex &m) {
            Release();
            _mutex = &m;
            Acquire();
        }

        /// Release the currently required lock on the associated mutex.  If
        /// this lock is not currently acquired, do nothing.
        void Release() {
            if (_acquired) {
                _Release();
            }
        }

        /// Acquire a lock on this lock's associated mutex.  This lock must not
        /// already be acquired when calling \p Acquire().
        void Acquire() {
            TF_DEV_AXIOM(_mutex);
            TF_DEV_AXIOM(!_acquired);
            _mutex->Acquire();
            _acquired = true;
        }

        /// If the current scoped lock is acquired, Release() it, then associate
        /// this lock with \p m and try to acquire a lock.  Return true if the
        /// lock was successfully acquired, false if not.
        bool TryAcquire(TfSpinMutex &m) {
            Release();
            _mutex = &m;
            return TryAcquire();
        }

        /// Try to acquire a lock on this lock's associated mutex.  The lock
        /// must not already be acquired when calling \p TryAcquire().  Return
        /// true if the lock was successfully acquired, false if not.
        bool TryAcquire() {
            TF_DEV_AXIOM(_mutex);
            TF_DEV_AXIOM(!_acquired);
            _acquired = _mutex->TryAcquire();
            return _acquired;
        }
        
    private:

        void _Release() {
            TF_DEV_AXIOM(_acquired);
            _mutex->Release();
            _acquired = false;
        }

        TfSpinMutex *_mutex = nullptr;
        bool _acquired = false;
    };

    /// Acquire a lock on this mutex if it is not currently held by another
    /// thread. Return true if the lock was acquired, or false if it was not
    /// because another thread held the lock.  This thread must not already hold
    /// a lock on this mutex.
    inline bool TryAcquire() {
        return _lockState.exchange(true, std::memory_order_acquire) == false;
    }

    /// Acquire a lock on this mutex.  If another thread holds a lock on this
    /// mutex, wait until it is released and this thread successfully acquires
    /// it.  This thread must not already hold a lock on this mutex.
    void Acquire() {
        if (ARCH_LIKELY(TryAcquire())) {
            return;
        }
        _AcquireContended();
    }

    /// Release this thread's lock on this mutex.
    inline void Release() {
        _lockState.store(false, std::memory_order_release);
    }

private:
    
    TF_API void _AcquireContended();
    
    std::atomic<bool> _lockState;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SPIN_MUTEX_H
