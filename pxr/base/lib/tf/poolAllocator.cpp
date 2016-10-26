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
#include "pxr/base/tf/poolAllocator.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"
#include <new>
#include <algorithm>



TfPoolAllocator::TfPoolAllocator(size_t elementSize, size_t bytesPerChunk)
    : _elementSize(elementSize),
      _bytesPerChunk(bytesPerChunk),
      _freeCalled(false)
{
    TF_AXIOM(sizeof(_PoolNode) <= _elementSize);
    TF_AXIOM(_elementSize < _bytesPerChunk);

    _freeList = NULL;
    _freeListLength = 0;
    _threadSafety = true;
}

TfPoolAllocator::~TfPoolAllocator()
{
    for (size_t i = _chunks.size(); i--; )
        free(_chunks[i]);
}    

size_t
TfPoolAllocator::GetBytesInUse(size_t* unallocated) const
{
    size_t wasted = (_bytesPerChunk % _elementSize) * _chunks.size();
    *unallocated = _freeListLength * _elementSize + wasted;
    return (_bytesPerChunk * _chunks.size()) - *unallocated;
}
    
TfPoolAllocator::_PoolNode*
TfPoolAllocator::_Refill()
{
    TfAutoMallocTag2 tag("Tf", "TfPoolAllocator::_Refill");
    TF_AXIOM(_freeListLength == 0);
    
    size_t nelem = _bytesPerChunk / _elementSize;
    char* start = reinterpret_cast<char *>(malloc(_bytesPerChunk));
    if (!start)
        throw std::bad_alloc();

    char* last = &start[(nelem - 1) * _elementSize];
    _PoolNode* poolStart = reinterpret_cast<_PoolNode*>(start);
    
    _chunks.push_back(poolStart);
    
    // thread the chunk into _elementSize pieces
    for (char *ptr = start; ptr < last; ptr += _elementSize)
        reinterpret_cast<_PoolNode*>(ptr)->next =
            reinterpret_cast<_PoolNode*>(ptr + _elementSize);

    _freeListLength = nelem;
    reinterpret_cast<_PoolNode*>(last)->next = NULL;
    return reinterpret_cast<_PoolNode*>(poolStart);
}

void *
TfPoolAllocator::GetElement(size_t index) const
{
    // Verify assumption that there is no waste per chunk.
    TF_AXIOM(_bytesPerChunk % _elementSize == 0);
    TF_AXIOM(not _freeCalled);
    size_t nelem = _bytesPerChunk / _elementSize;
    size_t chunk = index / nelem;
    TF_AXIOM(chunk < _chunks.size());
    size_t indexInChunk = index % nelem;
    TF_AXIOM(indexInChunk < nelem);

    char *start = reinterpret_cast<char *>(_chunks[chunk]);

    return &start[indexInChunk * _elementSize];
}

/*
 * The function TfPoolAllocator::Reclaim() tries
 * to reclaim the memory on the freelist, by checking the vector
 * _chunks for those chunks that are completely unallocated.  This is
 * done by scanning the freelist; if a given chunk "owns"---as defined
 * by addressInChunk() below---(_bytesPerChunk/_elementSize) items on
 * the freelist, then no allocations have been made from that chunk,
 * and the chunk may be freed.  We can check for all such chunks in
 * O(n log n) time, because the determination of which chunk an item
 * on the freelist belongs to can be made in O (log n) time, using
 * binary search on the ordered list of chunks.
 */

typedef std::pair<void*, size_t> _ChunkRecord;

/*
 * Used by std::sort
 */
static bool
_RecordComparison (::_ChunkRecord a, _ChunkRecord b)
{
    return a.first < b.first;
}

/*
 * True if start <= addr < start + bytesPerChunk
 */
inline bool
_AddressInChunk(void* addr, void* start, size_t bytesPerChunk)
{
    char* cAddr = reinterpret_cast<char*>(addr);
    char* cStart = reinterpret_cast<char*>(start);

    return (cStart <= cAddr) && (cAddr < cStart + bytesPerChunk);
}
        
/*
 * Return v[i] owning addr.
 */
static _ChunkRecord*
_LocateOwner(::_ChunkRecord v[],
             size_t n, void* addr, size_t bytesPerChunk)
{
    TF_AXIOM(n > 0);
    _ChunkRecord* owner = NULL;
    
    if (n < 5) {
        for (size_t i = 0; i < n; i++) {
            if (::_AddressInChunk(addr, v[i].first, bytesPerChunk)) {
                owner = &v[i];
                break;
            }
        }
    }
    else {    
        size_t low = 0,
               high = n - 1;

        // ordinary binary search, checking middle key (gets early hits)

        while (low < high) {
            size_t middle = (low + high) / 2;

            if (::_AddressInChunk(addr, v[middle].first, bytesPerChunk)) {
                owner = &v[middle];
                break;
            }
            else if (addr < v[middle].first)
                high = middle - 1;
            else
                low = middle + 1;
        }

        if (::_AddressInChunk(addr, v[low].first, bytesPerChunk))
            owner = &v[low];
    }
    
    TF_AXIOM(owner);
    return owner;
}

/*
 * TfPoolAllocator::Reclaim() frees chunks
 * that are completely unused (i.e. no allocations made against them),
 * as follows:
 *
 * (1) Make an list of pairs (chunk-address, ctr), ordered by chunk-address,
 *     with ctr = _bytesPerChunk/_elementSize for each chunk address.
 *
 * (2) For each item on the free list, search the ordered list to find
 *     the chunk that "owns" the item; decrement ctr for that chunk.
 *     If ctr reaches zero, that chunk can be freed.
 *
 * (3) Rethread the free list, keeping only those items that belong to
 *     chunks whose ctr has not reached zero.
 *
 * (4) Clear the _chunks vector, and free all chunks whose ctr is zero.
 *     Chunks whose ctr is zero are put back onto the _chunks vector.
 */

size_t
TfPoolAllocator::Reclaim()
{
    TfAutoMallocTag2 tag("Tf", "TfPoolAllocator::Reclaim");
    tbb::spin_mutex::scoped_lock lock(_fastMutex);
    
    if (_chunks.empty() || !_freeList)
        return 0;
    
    size_t itemsPerChunk = _bytesPerChunk / _elementSize,
           chunksFreed = 0,
           nChunks = _chunks.size();

    _ChunkRecord* v = new _ChunkRecord[nChunks];
    for (size_t i = 0; i < nChunks; i++)
        v[i] = _ChunkRecord(_chunks[i], itemsPerChunk);
    std::sort(&v[0], &v[nChunks], &::_RecordComparison);
    
    for (_PoolNode* node = _freeList; node; node = node->next)  // (2)
        _LocateOwner(v, nChunks, node, _bytesPerChunk)->second--;

                                                                
    _PoolNode  dummyNode;                                       // (3)
    _PoolNode** tailPtr = &dummyNode.next;  // tail of new free list

    for (_freeListLength = 0; _freeList ; _freeList = _freeList->next) {
        if (::_LocateOwner(v, nChunks, _freeList, _bytesPerChunk)->second > 0) {
            *tailPtr = _freeList;
            tailPtr = &_freeList->next;
            _freeListLength++;
        }
    }

    *tailPtr = NULL;
    _freeList = dummyNode.next;

    _chunks.clear();                                            // (4)
    for (size_t i = 0; i < nChunks; i++) {
        if (v[i].second == 0) {
             delete [] reinterpret_cast<char*>(v[i].first);
            chunksFreed++;
        }
        else
            _chunks.push_back(reinterpret_cast<_PoolNode*>(v[i].first));
    }

    
    delete [] v;
    return chunksFreed * _bytesPerChunk;
}


