//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executorDataVector.h"

#include "pxr/exec/vdf/network.h"

#include "pxr/base/tf/stl.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE


/// Issue a magic instruction sequence which Valgrind will intercept and mark
/// the specified memory defined for memcheck.
///
/// XXX: We should replace this with the VALGRIND_MAKE_MEM_DEFINED macro
///      from memcheck.h, once we can establish a build dependency on
///      valgrind-devel.
///
static inline void
Vdf_ExecutorDataVector_ValgrindMakeDefined(void *ptr, size_t size)
{
#if defined(ARCH_OS_LINUX)
    // Pass a result and a pointer to some arguments via registers
    volatile unsigned long long int result;
    volatile unsigned long long int args[6] = {
        0x4D430002,                  // Memcheck: MAKE_MEM_DEFINED
        (uintptr_t)(ptr),            // Memory address
        size,                        // Size in bytes
        0, 0, 0                      // Remaining unused arguments
    };

    // Magical instruction sequence which Valgrind will interpret as an
    // annotation (see valgrind.h)
    __asm__ volatile(
        "rolq $3,  %%rdi ; rolq $13, %%rdi\n\t"
        "rolq $61, %%rdi ; rolq $51, %%rdi\n\t"
        "xchgq %%rbx, %%rbx"
        : "=d" (result)
        : "a" (&args[0]), "0" (0)
        : "cc", "memory"
    );
#endif
}

Vdf_ExecutorDataVector::~Vdf_ExecutorDataVector()
{
    // Make sure to delete all previously allocated segments.
    for (const _LocationsSegment segment : _locations) {
        delete[] segment;
    }
}

void
Vdf_ExecutorDataVector::Resize(const VdfNetwork &network)
{
    // What's our new size?
    const size_t newSize = network.GetOutputCapacity();

    // Already appropriately sized?
    if (newSize <= (_locations.size() * _SegmentSize)) {
        return;
    }

    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Vdf", "Vdf_ExecutorDataVector::Resize");

    // Let's segment the array and only allocated segments as they are required.
    // This helps reduce the cost of executor data vector creation for transient
    // executors - especially if they compute data relatively sparsely.
    const size_t numSegments = (newSize / _SegmentSize) + 1;
    _locations.resize(numSegments, nullptr);

    // Go ahead and reserve some storage for executor data. If this is the first
    // time storage is being allocated for the concurrent_vector, the size will
    // dictate the sizes of the individual allocations the vector will make as
    // it grows.
    _outputData.reserve(_InitialExecutorDataNum);
    _bufferData.reserve(_InitialExecutorDataNum);
    _invalidationData.reserve(_InitialExecutorDataNum);
    _smblData.reserve(_InitialExecutorDataNum);
}

void
Vdf_ExecutorDataVector::Clear()
{
    if (GetNumData() == 0) {
        return;
    }

    TRACE_FUNCTION();

    // Clear all the executor data. This may be expensive, since it will
    // invoke destructors as necessary.
    _outputData.clear();
    _bufferData.clear();
    _invalidationData.clear();
    _smblData.clear();

    // Also drop the memory allocated for executor data, and revert to the
    // storage space used for the initial reservation.
    if (_outputData.capacity() > _InitialExecutorDataNum) {
        TfReset(_outputData);
        TfReset(_bufferData);
        TfReset(_invalidationData);
        TfReset(_smblData);

        _outputData.reserve(_InitialExecutorDataNum);
        _bufferData.reserve(_InitialExecutorDataNum);
        _invalidationData.reserve(_InitialExecutorDataNum);
        _smblData.reserve(_InitialExecutorDataNum);
    }
}

Vdf_ExecutorDataVector::_LocationsSegment
Vdf_ExecutorDataVector::_CreateSegment(size_t segmentIndex)
{
    TF_DEV_AXIOM(_locations[segmentIndex] == nullptr);

    // Note, that we will leave this array uninitialized after allocation. A
    // back pointer in the executor data will help us verify the validity of
    // the location: The random garbage in this array will not match the back
    // pointer, and it can therefore be identified as garbage. If the back
    // pointer happens to match the garbage, then it's already accidentally
    // pointing at the right array element so good for us.
    const _LocationsSegment segment = new uint32_t[_SegmentSize];
    _locations[segmentIndex] = segment;

    // Don't fret Mr. Valgrind! As an optimization we will be leaving the
    // memory in the locations array uninitialized.
    Vdf_ExecutorDataVector_ValgrindMakeDefined(
        segment, sizeof(uint32_t) * _SegmentSize);

    return segment;
}

PXR_NAMESPACE_CLOSE_SCOPE
