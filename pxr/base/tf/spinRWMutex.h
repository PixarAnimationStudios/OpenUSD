//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SPIN_RW_MUTEX_H
#define PXR_BASE_TF_SPIN_RW_MUTEX_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfSpinRWMutex
///
/// This class implements a readers-writer spin lock that emphasizes throughput
/// when there is light contention or moderate contention dominated by readers.
/// Like all spin locks, significant contention performs poorly; consider a
/// different algorithm design or synchronization strategy in that case.
///
/// In the best case, acquiring a read lock is an atomic add followed by a
/// conditional branch, and acquiring a write lock is an atomic bitwise-or
/// followed by a conditional branch.
///
/// When contended by only readers, acquiring a read lock is the same: an atomic
/// add followed by a conditional branch.  Of course the shared cache line being
/// concurrently read and modified will affect performance.
///
/// In the worst case, acquiring a read lock does the atomic add and conditional
/// branch, but the condition shows writer activity, so the add must be undone
/// by a subtraction, and then the thread must wait to see no writer activity
/// before trying again.
///
/// Similarly in the worst case for acquiring a write lock, the thread does the
/// atomic bitwise-or, but sees another active writer, and then must wait to see
/// no writer activity before trying again.  Once the exclusive-or is done
/// successfully, then the writer must wait for any pending readers to clear out
/// before it can proceed.
///
/// This class provides a nested TfSpinRWMutex::ScopedLock that makes it easy to
/// acquire locks, upgrade reader to writer, downgrade writer to reader, and
/// have those locks automatically release when the ScopedLock is destroyed.
/// 
class TfSpinRWMutex
{
    static constexpr int OneReader = 2;
    static constexpr int WriterFlag = 1;
    
public:

    /// Construct a mutex, initially unlocked.
    TfSpinRWMutex() : _lockState(0) {}

    /// Scoped lock utility class.  API modeled roughly after
    /// tbb::spin_rw_mutex::scoped_lock.
    struct ScopedLock {

        // Acquisition states.
        static constexpr int NotAcquired = 0;
        static constexpr int ReadAcquired = 1;
        static constexpr int WriteAcquired = 2;

        /// Construct a scoped lock for mutex \p m and acquire either a read or
        /// a write lock depending on \p write.
        explicit ScopedLock(TfSpinRWMutex &m, bool write=true)
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
        void Acquire(TfSpinRWMutex &m, bool write=true) {
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

        /// If the current scoped lock is acquired, Release() it, then associate
        /// this lock with \p m and try to acquire either a read or a write
        /// lock, depending on \p write.  Return true if successfully acquired,
        /// false if not.
        bool TryAcquire(TfSpinRWMutex &m, bool write=true) {
            Release();
            _mutex = &m;
            return TryAcquire(write);
        }

        /// Try to acquire either a read or a write lock on this lock's
        /// associated mutex.  The lock must not already be acquired when
        /// calling \p TryAcquire().  Return true if the lock was successfully
        /// acquired, false if not.
        bool TryAcquire(bool write=true) {
            return write ? TryAcquireWrite() : TryAcquireRead();
        }

        /// Release the currently required lock on the associated mutex.  If
        /// this lock is not currently acquired, silently do nothing.
        void Release() {
            switch (_acqState) {
            default:
            case NotAcquired:
                break;
            case ReadAcquired:
                _ReleaseRead();
                break;
            case WriteAcquired:
                _ReleaseWrite();
                break;
            };
        }

        /// Acquire a read lock on this lock's associated mutex.  This lock must
        /// not already be acquired when calling \p AcquireRead().
        void AcquireRead() {
            TF_DEV_AXIOM(_mutex);
            TF_DEV_AXIOM(_acqState == NotAcquired);
            _mutex->AcquireRead();
            _acqState = ReadAcquired;
        }

        /// Try to acquire a read lock on this lock's associated mutex.  The
        /// lock must not already be acquired when calling \p TryAcquireRead().
        /// Return true if the lock was successfully acquired, false if not.
        bool TryAcquireRead() {
            TF_DEV_AXIOM(_mutex);
            TF_DEV_AXIOM(_acqState == NotAcquired);
            if (_mutex->TryAcquireRead()) {
                _acqState = ReadAcquired;
                return true;
            }
            return false;
        }

        /// Acquire a write lock on this lock's associated mutex.  This lock
        /// must not already be acquired when calling \p AcquireWrite().
        void AcquireWrite() {
            TF_DEV_AXIOM(_mutex);
            TF_DEV_AXIOM(_acqState == NotAcquired);
            _mutex->AcquireWrite();
            _acqState = WriteAcquired;
        }

        /// Try to acquire a write lock on this lock's associated mutex.  The
        /// lock must not already be acquired when calling \p TryAcquireWrite().
        /// Return true if the lock was successfully acquired, false if not.
        bool TryAcquireWrite() {
            TF_DEV_AXIOM(_mutex);
            TF_DEV_AXIOM(_acqState == NotAcquired);
            if (_mutex->TryAcquireWrite()) {
                _acqState = WriteAcquired;
                return true;
            }
            return false;
        }

        /// Change this lock's acquisition state from a read lock to a write
        /// lock.  This lock must already be acquired for reading.  Return true
        /// if the upgrade occurred without releasing the read lock, false if it
        /// was released.
        bool UpgradeToWriter() {
            TF_DEV_AXIOM(_mutex);
            TF_DEV_AXIOM(_acqState == ReadAcquired);
            _acqState = WriteAcquired;
            return _mutex->UpgradeToWriter();
        }

        /// Change this lock's acquisition state from a write lock to a read
        /// lock.  This lock must already be acquired for writing.  Return true
        /// if the downgrade occurred without releasing the write in the
        /// interim, false if it was released and other writers may have
        /// intervened.
        bool DowngradeToReader() {
            TF_DEV_AXIOM(_mutex);
            TF_DEV_AXIOM(_acqState == WriteAcquired);
            _acqState = ReadAcquired;
            return _mutex->DowngradeToReader();
        }
        
    private:

        void _ReleaseRead() {
            TF_DEV_AXIOM(_acqState == ReadAcquired);
            _mutex->ReleaseRead();
            _acqState = NotAcquired;
        }

        void _ReleaseWrite() {
            TF_DEV_AXIOM(_acqState == WriteAcquired);
            _mutex->ReleaseWrite();
            _acqState = NotAcquired;
        }

        TfSpinRWMutex *_mutex;
        int _acqState; // NotAcquired (0), ReadAcquired (1), WriteAcquired (2)
    };

    /// Attempt to acquire a read lock on this mutex without waiting for
    /// writers.  This thread must not already hold a lock on this mutex (either
    /// read or write).  Return true if the lock is acquired, false otherwise.
    inline bool TryAcquireRead() {
        // Optimistically increment the reader count.
        if (ARCH_LIKELY(!(_lockState.fetch_add(OneReader) & WriterFlag))) {
            // We incremented the reader count and observed no writer activity,
            // we have a read lock.
            return true;
        }

        // Otherwise there's writer activity.  Undo the increment and return
        // false.
        _lockState -= OneReader;
        return false;
    }
    
    /// Acquire a read lock on this mutex.  This thread must not already hold a
    /// lock on this mutex (either read or write).  Consider calling
    /// DowngradeToReader() if this thread holds a write lock.
    inline void AcquireRead() {
        while (true) {
            if (TryAcquireRead()) {
                return;
            }
            // There's writer activity.  Wait to see no writer activity and
            // retry.
            _WaitForWriter();
        }
    }

    /// Release this thread's read lock on this mutex.
    inline void ReleaseRead() {
        // Just decrement the count.
        _lockState -= OneReader;
    }

    /// Attempt to acquire a write lock on this mutex without waiting for other
    /// writers.  This thread must not already hold a lock on this mutex (either
    /// read or write).  Return true if the lock is acquired, false otherwise.
    inline bool TryAcquireWrite() {
        int state = _lockState.fetch_or(WriterFlag);
        if (!(state & WriterFlag)) {
            // We set the flag, wait for readers.
            if (state != 0) {
                // Wait for pending readers.
                _WaitForReaders();
            }
            return true;
        }
        return false;
    }

    /// Acquire a write lock on this mutex.  This thread must not already hold a
    /// lock on this mutex (either read or write).  Consider calling
    /// UpgradeToWriter() if this thread holds a read lock.
    void AcquireWrite() {
        // Attempt to acquire -- if we fail then wait to see no other writer and
        // retry.
        while (true) {
            if (TryAcquireWrite()) {
                return;
            }
            _WaitForWriter();
        }
    }

    /// Release this thread's write lock on this mutex.
    inline void ReleaseWrite() {
        _lockState &= ~WriterFlag;
    }

    /// Upgrade this thread's lock on this mutex (which must be a read lock) to
    /// a write lock.  Return true if the upgrade is done "atomically" meaning
    /// that the read lock was not released (and thus no other writer could have
    /// acquired the lock in the interim).  Return false if this lock was
    /// released and thus another writer could have taken the lock in the
    /// interim.
    bool UpgradeToWriter() {
        // This thread owns a read lock, attempt to upgrade to write lock.  If
        // we do so without an intervening writer, return true, otherwise return
        // false.
        bool atomic = true;
        while (true) {
            int state = _lockState.fetch_or(WriterFlag);
            if (!(state & WriterFlag)) {
                // We set the flag, release our reader count and wait for any
                // other pending readers.
                if (_lockState.fetch_sub(
                        OneReader) != (OneReader | WriterFlag)) {
                    _WaitForReaders();
                }
                return atomic;
            }
            // There was other writer activity -- wait for it to clear, then
            // retry.
            atomic = false;
            _WaitForWriter();
        }
    }

    /// Downgrade this mutex, which must be locked for write by this thread, to
    /// being locked for read by this thread.  Return true if the downgrade
    /// happened "atomically", meaning that the write lock was not released (and
    /// thus possibly acquired by another thread).  This implementation
    /// currently always returns true.
    bool DowngradeToReader() {
        // Simultaneously add a reader count and clear the writer bit by adding
        // (OneReader-1).
        _lockState += (OneReader-1);
        return true;
    }

private:
    friend class TfBigRWMutex;
    
    // Helpers for staged-acquire-write that BigRWMutex uses.
    enum _StagedAcquireWriteState {
        _StageNotAcquired,
        _StageAcquiring,
        _StageAcquired
    };

    // This API lets TfBigRWMutex acquire a write lock step-by-step so that it
    // can begin acquiring write locks on several mutexes without waiting
    // serially for pending readers to complete.  Call _StagedAcquireWriteStep
    // with _StageNotAcquired initially, and save the returned value.  Continue
    // repeatedly calling _StagedAcquireWriteStep, passing the previously
    // returned value until this function returns _StageAcquired.  At this
    // point the write lock is acquired.
    _StagedAcquireWriteState
    _StagedAcquireWriteStep(_StagedAcquireWriteState curState) {
        int state;
        switch (curState) {
        case _StageNotAcquired:
            state = _lockState.fetch_or(WriterFlag);
            if (!(state & WriterFlag)) {
                // We set the flag. If there were no readers we're done,
                // otherwise we'll have to wait for them, next step.
                return state == 0 ? _StageAcquired : _StageAcquiring;
            }
            // Other writer activity, must retry next step.
            return _StageNotAcquired;
        case _StageAcquiring:
            // We have set the writer flag but must wait to see no readers.
            _WaitForReaders();
            return _StageAcquired;
        case _StageAcquired:
        default:
            return _StageAcquired;
        };
    }
    
    TF_API void _WaitForReaders() const;
    TF_API void _WaitForWriter() const;
    
    std::atomic<int> _lockState;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SPIN_RW_MUTEX_H
