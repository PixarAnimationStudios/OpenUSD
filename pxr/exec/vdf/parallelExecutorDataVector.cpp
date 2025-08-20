//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/parallelExecutorDataVector.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"

#include "pxr/base/arch/threads.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

#include <algorithm>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

template < typename T >
static void
_ResizeAtomicArray(
    std::unique_ptr<std::atomic<T>[]> *array,
    const size_t oldSize,
    const size_t newSize,
    const T init)
{
    // Allocate a new array
    std::unique_ptr<std::atomic<T>[]> newArray(new std::atomic<T>[newSize]);

    // Copy all the existing values into the new array.
    for (size_t i = 0; i < oldSize; ++i) {
        newArray[i].store(
            (*array)[i].load(std::memory_order_relaxed),
            std::memory_order_relaxed);
    }

    // Set all the new values in the array to the initial value.
    for (size_t i = oldSize; i < newSize; ++i) {
        newArray[i].store(init, std::memory_order_relaxed);
    }

    // Set the array pointer to the new array
    array->swap(newArray);
}

template < typename T >
static void
_ResetConcurrentVector(T *vector, const size_t capacity)
{
    // If the vector has not yet reached its capacity, simply clear it, if 
    // we can. Note that if the memory underpinning the vector storage is
    // zero-initialized, we cannot simply clear the vector. In that case we
    // have to make sure to reset the memory back to zero.
    typedef typename T::allocator_type Allocator;
    typedef WorkZeroAllocator<typename T::value_type> ZeroAllocator;
    if (!std::is_same<Allocator, ZeroAllocator>::value &&
        vector->capacity() <= capacity) {
        vector->clear();
    }

    // If the vector is beyond the initial capacity, shrink it down to its
    // initial capacity on order to reduce its memory footprint.
    else {
        T newVector;
        newVector.reserve(capacity);
        vector->swap(newVector);
    }
}

template < typename Container, typename ... T >
static size_t
_EmplaceEntry(Container *data, T ... args)
{
    const typename Container::iterator it = data->emplace_back(args...);
    const size_t idx = std::distance(data->begin(), it);
    TF_DEV_AXIOM(data->size() > idx);
    return idx;
}

Vdf_ParallelExecutorDataVector::~Vdf_ParallelExecutorDataVector()
{
    // Delete all the locations segments.
    for (size_t i = 0; i < _numSegments; ++i) {
        delete[] _locations[i].load(std::memory_order_acquire);
    }
}

void
Vdf_ParallelExecutorDataVector::Resize(const VdfNetwork &network)
{
    // What's our new size?
    const size_t newSize = network.GetOutputCapacity();

    // Already appropriately sized?
    if (newSize <= (_numSegments * _SegmentSize)) {
        return;
    }

    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Vdf", "Vdf_ParallelExecutorDataVector::Resize");

    // Resize the locations array.
    const size_t newNumSegments = (newSize / _SegmentSize) + 1;
    _ResizeAtomicArray<_LocationsSegment>(
        &_locations, _numSegments, newNumSegments, nullptr);
    _numSegments = newNumSegments;

    // Resize the touched array, but only do so if an array has previously
    // been allocated. This is so that we don't do the redundant work of
    // allocating touched bits on transient data vectors (e.g. for temporary
    // executors).
    if (_numTouched.load(std::memory_order_acquire) != 0) {
        _GrowTouched();
    }
    
    // Go ahead and reserve some storage for executor data. If this is the first
    // time storage is being allocated for the concurrent_vector, the size will
    // dictate the sizes of the individual allocations the vector will make as
    // it grows.
    _outputData.reserve(_InitialDataNum);
    for (size_t i = 0; i < _NumBuffers; ++i) {
        _bufferData[i].reserve(_InitialDataNum);
    }
    _inboxes.reserve(_InitialDataNum);
    _invalidationData.reserve(_InitialDataNum);
}

void
Vdf_ParallelExecutorDataVector::Reset(
    const DataHandle handle,
    const VdfId outputId) const
{
    // Reset the data entries at the given location.
    for (size_t i = 0; i < _NumBuffers; ++i) {
        _bufferData[i][handle].Reset();
    }
    _inboxes[handle].Reset();
    _invalidationData[handle].Reset();

    // Note we reset the output data last, in order to synchronize dependent
    // non-atomic and atomic writes on the outputId field.
    _outputData[handle].Reset(outputId);
}

void
Vdf_ParallelExecutorDataVector::Clear()
{
    TRACE_FUNCTION();

    // Reset the locations segments.
    for (size_t i = 0; i < _numSegments; ++i) {
        // Only reset the segments that have already been allocated.
        if (const _LocationsSegment segment =
                _locations[i].load(std::memory_order_acquire)) {
            for (size_t j = 0; j < _SegmentSize; ++j) {
                segment[j].store(_LocationInvalid, std::memory_order_relaxed);
            }
        }
    }

    // Reset the touched array by clearing all bits. We can do this in parallel.
    const size_t numTouched = _numTouched.load(std::memory_order_acquire);
    WorkParallelForN(
        numTouched,
        [&touched = _touched](size_t begin, size_t end){
            for (size_t i = begin; i != end; ++i) {
                touched[i].store(0, std::memory_order_relaxed);
            }
        });

    // Clear all the executor data. This may be expensive, since it will
    // invoke destructors as necessary. Also drop the memory allocated for
    // executor data, and revert to the storage space used for the initial
    // reservation.
    _ResetConcurrentVector(&_outputData, _InitialDataNum);
    for (size_t i = 0; i < _NumBuffers; ++i) {
        _ResetConcurrentVector(&_bufferData[i], _InitialDataNum);
    }
    _ResetConcurrentVector(&_inboxes, _InitialDataNum);
    _ResetConcurrentVector(&_invalidationData, _InitialDataNum);
}

void
Vdf_ParallelExecutorDataVector::_GrowTouched()
{
    // We are going to grow the bitset to the full size of the network.
    const size_t networkSize = _numSegments * _SegmentSize;
    const size_t numTouched = (networkSize / _TouchedWordBits) + 1;

    // If the bitset is already appropriately sized, we can bail out now.
    if (_numTouched.load(std::memory_order_acquire) >= numTouched) {
        return;
    }

    TRACE_FUNCTION_SCOPE("growing");

    // During lazy initialization of this bitset from Touch(), multiple threads
    // may get here concurrently. We will use the lock to make sure that only
    // one of them gets to allocate and construct the new array.
    tbb::spin_mutex::scoped_lock lock(_touchedMutex);
    
    // We may not be the first thread that got here, in which case the work
    // has already happened. We can safely bail out if the array is now large
    // enough.
    const size_t oldNumTouched = _numTouched.load(std::memory_order_acquire);
    if (oldNumTouched >= numTouched) {
        return;
    }

    // Resize the touched array to fit all of the outputs in the network.
    _ResizeAtomicArray(&_touched, oldNumTouched, numTouched, UINT64_C(0));

    // Atomically set the size of the touched bitset. Note, that we take
    // advantage of the fact that all threads will attempt to resize the vector
    // to the same length (all outputs are part of the same network). If this
    // were not the case, this would have to be a CAS loop or similar concept.
    // The release semantics here make sure that the dependent non-atomic writes
    // above are retired before _numTouched is acquired elsewhere (e.g. Touch(),
    // IsTouched() and Untouch()).
    _numTouched.store(numTouched, std::memory_order_release);
}

Vdf_ParallelExecutorDataVector::_LocationsSegment
Vdf_ParallelExecutorDataVector::_CreateSegment(size_t segmentIndex) const
{
    TRACE_FUNCTION();

    // Allocate and initialize the new segment.
    _LocationsSegment newSegment = new std::atomic<int>[_SegmentSize];
    for (size_t i = 0; i < _SegmentSize; ++i) {
        newSegment[i].store(_LocationInvalid, std::memory_order_release);
    }

    // Attempt to swap in the new segment, and return a pointer to the new
    // segment if the operation succeeds.
    _LocationsSegment segment = nullptr;
    if (_locations[segmentIndex].compare_exchange_strong(segment, newSegment)) {
        return newSegment;
    }

    // If the CAS operation fails, another thread was able to get to swapping in
    // the segment before this one. In that case, delete the newly allocated
    // segment and return a pointer to the segment that was created by the other
    // thread.
    delete[] newSegment;
    return segment;
}

int
Vdf_ParallelExecutorDataVector::_CreateLocation(
    const VdfId outputId,
    int currentLocation,
    std::atomic<int> *newLocation) const
{
    // If the location index is invalid and not currently pending (i.e. being
    // inserted from another thread), create new data and store the location
    // index.
    if (currentLocation == _LocationInvalid &&
        newLocation->compare_exchange_strong(
            currentLocation, _LocationPending)) {
        const int location = _CreateData(outputId);
        newLocation->store(location, std::memory_order_release);
        return location;
    }

    // If the location index is currently pending, busy wait until it becomes
    // available.
    return _WaitForLocation(currentLocation, newLocation);
}

int
Vdf_ParallelExecutorDataVector::_CreateData(const VdfId outputId) const
{
    // Emplace a new output data entry.
    const _OutputDataVector::iterator outputDataIt =
        _outputData.emplace_back(outputId);

    // Note that the data entries are always created in lock step. However, if
    // this happens concurrently, another thread may get to inserting entries
    // into either one of the arrays, first. This is okay, we just have to
    // ensure that if this happens, we wait until the entries have been fully
    // constructed. We increment a checksum in the output data at the index of
    // the data entry insertion, and once this checksum has reached a certain
    // count we know that all individual entries have been inserted at the
    // output data index, and are guaranteed to be properly constructed.
    uint8_t finalChecksum = 0;

    for (size_t i = 0; i < _NumBuffers; ++i) {
        const size_t idx = _EmplaceEntry(&_bufferData[i]);
        _outputData[idx].constructionChecksum.fetch_add(
            1, std::memory_order_release);
        ++finalChecksum;
    }

    const size_t mbIdx = _EmplaceEntry(&_inboxes);
    _outputData[mbIdx].constructionChecksum.fetch_add(
        1, std::memory_order_release);
    ++finalChecksum;

    const size_t invIdx = _EmplaceEntry(&_invalidationData);
    _outputData[invIdx].constructionChecksum.fetch_add(
        1, std::memory_order_release);
    ++finalChecksum;

    // The data entries that were just inserted above may not be the ones
    // corresponding to our newly inserted output data (if another thread got to
    // inserting those first). So, we need to wait until ours become available.
    // That should happen very soon, so let's just spin wait.
    const std::atomic<uint8_t> *checksum = &outputDataIt->constructionChecksum;
    while (checksum->load(std::memory_order_acquire) != finalChecksum) {
        ARCH_SPIN_PAUSE();
    }

    // Return the index of the newly inserted output data entry.
    return std::distance(_outputData.begin(), outputDataIt);
}

void
Vdf_ParallelExecutorDataVector::_ResetLocation(
    const VdfId outputId,
    int currentLocation,
    std::atomic<int> *newLocation) const
{
    // Set the location pending, and if that succeeds reset the actual data.
    // This prevents multiple threads from resetting the data at the same time.
    if (newLocation->compare_exchange_strong(
            currentLocation, _LocationPending)) {
        TF_DEV_AXIOM(currentLocation >= 0);
        // There is an ABA situation here where one thread successfully 
        // performs the exchange while another thread instead of
        // doing the exchange gets pre-empted and goes to sleep. The working 
        // thread does the work and then puts currentLocation back into the
        // atomic, so when the pre-empted thread wakes up it sees that the 
        // atomic's value is equal to currentLocation and enters this
        // if block. To prevent us from resetting our vectors twice,
        // we have to redo this check that we did in GetOrCreateDataHandle
        if (outputId != _outputData[currentLocation].outputId.load(
            std::memory_order_acquire)) {
            Reset(currentLocation, outputId);
        }
        newLocation->store(currentLocation, std::memory_order_release);
        return;
    }
    
    // If we were not able to set the location pending, some other thread
    // might have done so. In that case, busy wait until the data has been
    // reset by this other thread.
    TF_DEV_AXIOM(currentLocation == _LocationPending);
    _WaitForLocation(currentLocation, newLocation);
}

int
Vdf_ParallelExecutorDataVector::_WaitForLocation(
    int currentLocation,
    std::atomic<int> *newLocation) const
{
    while (currentLocation == _LocationPending) {
        ARCH_SPIN_PAUSE();
        currentLocation = newLocation->load(std::memory_order_acquire);
    }
    return currentLocation;
}

bool
Vdf_ParallelExecutorDataVector::_Inbox::Take(
    VdfVector *value,
    const VdfMask &mask)
{
    // Bail out if there is already a value assigned to this inbox. Note that
    // we cannot mutate (including deallocate) any previously stored buffer
    // since clients may hold pointers to it.
    VdfExecutorBufferData *previousBuffer =
        _buffer.load(std::memory_order_acquire);
    if (previousBuffer) {
        return false;
    }

    // Allocate a new buffer, which will assume ownership of the value.
    VdfExecutorBufferData *newBuffer = new VdfExecutorBufferData();
    newBuffer->TakeOwnership(value);
    newBuffer->SetExecutorCacheMask(mask);

    // Attempt to atomically exchange the current buffer with this newly
    // allocated buffer. If this operation fails, yield ownership of the value
    // and deallocate the newly constructed buffer.
    if (!_buffer.compare_exchange_strong(previousBuffer, newBuffer)) {
        newBuffer->YieldOwnership();
        delete newBuffer;
        return false;
    }

    // We were able to transfer ownership of the value into the inbox.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
