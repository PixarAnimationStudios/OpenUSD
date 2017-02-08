//
// Copyright 2016 Pixar
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
#ifndef TF_POOLALLOCATOR_H
#define TF_POOLALLOCATOR_H

/// \file tf/poolAllocator.h
/// \ingroup group_tf_Memory

#include "pxr/pxr.h"

#include <tbb/spin_mutex.h>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfPoolAllocator
/// \ingroup group_tf_Memory
///
/// Fast pool based allocator.
///
/// A \c TfPoolAllocator is a very fast allocator for requests for a single
/// size.  It has the advantage that it does not waste any space, as \c
/// malloc() does, storing the size of each memory request for reclamation
/// purposes.  Additionally, the allocation and deallocation routines are
/// inlined, and extremely short.
///
/// To use, one constructs an allocator which specifies the size of each
/// element and the total number of bytes to allocate per chunk.  The bytes
/// per chunk should be a multiple of the element size although this is
/// neither checked nor enforced.
///
/// \par Thread-Safety This class is thread safe.  One can disable thread
/// safety via a call to \c SetThreadSafety().  This is useful when the client
/// will absolutely be running in a single thread and wishes to not pay the
/// price of the locking behavior.
///
class TfPoolAllocator {
public:

    /// \param elementSize   Size of the element that this allocator will
    ///                      maintain.
    /// \param bytesPerChunk The pool in the allocator will request these many
    ///                      bytes at a time when obtaining more memory.
    ///
    TfPoolAllocator(size_t elementSize, size_t bytesPerChunk);

    /// Destructor.  The pool allocator will return all memory in its pool on
    /// destruction regardless of whether or not the individual data created
    /// by this pool allocator was previously returned.
    ~TfPoolAllocator();

    /// Enable or disable "lock on Alloc/Free"
    ///
    /// Allows one to override the locking semantics and to make this
    /// allocator thread-safe or not thread-safe as desired.  The default
    /// state is with the lock enabled.
    void SetThreadSafety(bool threadSafety) { _threadSafety = threadSafety; }

    /// Returns memory out of the pool allocator.
    ///
    /// The memory returned by this routine is suitably aligned for all
    /// purposes.
    void* Alloc() {
        _PoolNode* node;
        if (_threadSafety) {
            tbb::spin_mutex::scoped_lock lock(_fastMutex);
            node = _freeList ? _freeList : _Refill();
            _freeList = node->next;
            _freeListLength--;
        }
        else {
             node = _freeList ? _freeList : _Refill();
            _freeList = node->next;
            _freeListLength--;
        }

        return node;
    }
    
    /// Frees up the memory previously granted by the allocator.
    ///
    /// The location \c ptr must have been obtained by a previous call to \c
    /// Alloc(); if not, chaos will quickly ensue.
    void Free(const void* ptr) {
        void *nonConstPtr = const_cast<void *>(ptr);
        if (_threadSafety) {
            tbb::spin_mutex::scoped_lock lock(_fastMutex);

            reinterpret_cast<_PoolNode*>(nonConstPtr)->next = _freeList;
            _freeList = reinterpret_cast<_PoolNode*>(nonConstPtr);
            _freeListLength++;
        }
        else {
            reinterpret_cast<_PoolNode*>(nonConstPtr)->next = _freeList;
            _freeList = reinterpret_cast<_PoolNode*>(nonConstPtr);
            _freeListLength++;
        }
        _freeCalled = true;
    }

    /// Returns completely unused memory blocks held by the allocator to the
    /// process' global dynamic memory space.
    ///
    /// The return value is the number of bytes of space returned to the
    /// system by freeing completely unused chunks of memory currently held by
    /// the allocator.  Note that the reclaim operation is relatively
    /// expensive (i.e. considerably more expensive than an allocation or
    /// deallocation); thus, this operation should be used somewhat sparingly.
    /// The complexity of this function is n (log n) where n is proportional
    /// to the amount of memory currently held by the allocator (either used
    /// or unused). Also, note that large chunks of memory may be
    /// unreclaimable if even a small amount of each chunk is currently in
    /// use. Thus, the sucess of the reclamation is critically dependent on
    /// how fragmented the memory state is.
    size_t Reclaim();

    /// Return number of bytes in use for this pool, and unused space in
    /// bytesUnallocated.
    size_t GetBytesInUse(size_t* bytesUnallocated) const;

    /// Returns the address of element \p index fort this pool, ONLY if
    /// bytesPerChunk % elementSize == 0 and ONLY if Free() has not been used.
    /// Note that those conditions are asserted against.
    /// 
    /// This avoids storing pointers to elements in the pool which can be
    /// costly when elementSize is small. It is an 100% overhead to store an
    /// eight bytes pointer somewhere else when elementSize is eight bytes.
    void *GetElement(size_t index) const;

private:
    struct _PoolNode {
        _PoolNode* next;
    };

    _PoolNode* _Refill();
    
    tbb::spin_mutex          _fastMutex;
    _PoolNode *              _freeList;
    size_t                   _freeListLength;
    const size_t             _elementSize;
    const size_t             _bytesPerChunk;
    std::vector<_PoolNode *> _chunks;
    bool                     _threadSafety;
    bool                     _freeCalled;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
