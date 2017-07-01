//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef _GUSD_USD_STAGEHOLDER_H_
#define _GUSD_USD_STAGEHOLDER_H_


#include "gusd/UT_Assert.h"

#include <pxr/pxr.h>
#include "pxr/usd/usd/stage.h"

#include <UT/UT_IntrusivePtr.h>

#include <tbb/spin_rw_mutex.h>

PXR_NAMESPACE_OPEN_SCOPE

class GusdUSD_StageLock;
typedef UT_IntrusivePtr<GusdUSD_StageLock> GusdUSD_StageLockHandle;


/** Read-write lock for a stage that can be shared.*/
class GusdUSD_StageLock : public UT_IntrusiveRefCounter<GusdUSD_StageLock>,
                          UT_NonCopyable
{
public:
    /* XXX: Using a spin mutex for the reader-writer lock because
            read-only locks are overwhelmingly the common case.
            Write locks are only acquired during initial load phases,
            and many code paths pool locks up in a way that prevents
            any contention within a single node procedure.
            With packed prims, deferred prim access is less controlled
            in regards to what threads invoke locking, so this may
            lead to high contention when loading packed USD
            prims from disk. If that occurs, consider switching to
            a tbb::queuing_rw_mutex.*/
    typedef tbb::spin_rw_mutex rwlock;

    class ScopedLock
    {
    public:
        ScopedLock() {}
        ScopedLock(const GusdUSD_StageLockHandle& lock, bool write)
            : _lock(GusdUTverify_val(lock)->_mutex) {}

        void    Acquire(const GusdUSD_StageLockHandle& lock, bool write)
                { _lock.acquire(GusdUTverify_val(lock)->_mutex, write); }        
        void    Release()           { _lock.release(); }
        void    DowngradeToReader() { _lock.downgrade_to_reader(); }
        bool    UpgradeToWriter()   { return _lock.upgrade_to_writer(); }
        
    private:
        rwlock::scoped_lock _lock;
    };

private:
    mutable rwlock  _mutex;
};


typedef UT_IntrusivePtr<GusdUSD_StageLock> GusdUSD_StageLockHandle;


/** Holder that caches a value related to the stage.
    This enforces access to the value through scoped locks.*/
template <typename T>
class GusdUSD_HolderT
{
public:
    GusdUSD_HolderT() {}
    GusdUSD_HolderT(const T& val, const GusdUSD_StageLockHandle& lock)
        : _val(val), _lock(lock) {}

    explicit                        operator bool() const
                                    { return _lock.get(); }
    
    void                            Clear()
                                    {
                                        _val = T();
                                        _lock.reset();
                                    }

    const GusdUSD_StageLockHandle&  GetLock() const { return _lock; }

    /** Scoped lock for accessing the held value.
        Callers are required to not retain references to the value
        outside of the lifetime of the lock.*/
    class ScopedLock
    {
    public:
        ScopedLock() {}
        ScopedLock(const GusdUSD_HolderT<T>& holder, bool write)
            : _lock(holder._lock, write), _holder(&holder) {}

        SYS_FORCE_INLINE void       Acquire(GusdUSD_HolderT<T>& holder,
                                            bool write)
                                    {
                                        _lock.Acquire(holder._lock, write);
                                        _holder = &holder;
                                    }

        SYS_FORCE_INLINE void       Release()
                                    {
                                        if(_holder) {
                                            _lock.Release();
                                            _holder = NULL;
                                        }
                                    }

        SYS_FORCE_INLINE bool       DowngradeToReader()
                                    { return _lock.DowngradeToReader(); }

        SYS_FORCE_INLINE bool       UpgradeToWriter()
                                    { return _lock.UpgradeToWriter(); }
        
        SYS_FORCE_INLINE T&         operator*()
                                    { return GusdUTverify_ptr(_holder)->_val; }
        SYS_FORCE_INLINE const T&   operator*() const
                                    { return GusdUTverify_ptr(_holder)->_val; }

        SYS_FORCE_INLINE T&         operator->()        { return **this; }
        SYS_FORCE_INLINE const T&   operator->() const  { return **this; }

    private:
        GusdUSD_HolderT<T>*             _holder;
        GusdUSD_StageLock::ScopedLock   _lock;
    };

    /** ScopedLock that only provides read access to a holder.*/
    class ScopedReadLock
    {
    public:
        ScopedReadLock() {}
        ScopedReadLock(const GusdUSD_HolderT<T>& holder)
        : _lock(holder._lock, /*write*/ false), _holder(&holder) {}
        
        SYS_FORCE_INLINE void       Acquire(const GusdUSD_HolderT<T>& holder)
                                    {
                                        _lock.Acquire(holder._lock,
                                                      /*write*/ false);
                                        _holder = &holder;
                                    }

        SYS_FORCE_INLINE void       Release()
                                    {
                                        if(_holder) {
                                            _lock.Release();
                                            _holder = NULL;
                                        }
                                    }

        SYS_FORCE_INLINE const T&   operator*() const
                                    { return GusdUTverify_ptr(_holder)->_val; }

        SYS_FORCE_INLINE const T&   operator->() const  { return **this; }

    private:
        const GusdUSD_HolderT<T>*       _holder;
        GusdUSD_StageLock::ScopedLock   _lock;
    };

private:
    T                       _val;
    GusdUSD_StageLockHandle _lock;
};


typedef GusdUSD_HolderT<UsdStageRefPtr> GusdUSD_StageHolder;
typedef GusdUSD_HolderT<UsdPrim>        GusdUSD_PrimHolder;

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_STAGEHOLDER_H_*/
