//
// Copyright 2018 Pixar
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

#ifndef PXR_BASE_TRACE_DATA_BUFFER_H
#define PXR_BASE_TRACE_DATA_BUFFER_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"

#include "pxr/base/arch/hints.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <memory>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
/// \class TraceDataBuffer
///
/// This class stores copies of data that are associated with TraceEvent
/// instances. 
/// Data stored in the buffer must be copy constructible and trivially
/// destructible.
///
class TraceDataBuffer {
public:
    constexpr static size_t DefaultAllocSize = 1024;

    /// Constructor. The buffer will make allocations of \p allocSize.
    ///
    TraceDataBuffer(size_t allocSize = DefaultAllocSize) : _alloc(allocSize) {}

    /// Makes a copy of \p value and returns a pointer to it.
    ///
    template <typename T>
    const T* StoreData(const T& value)
    {
        static_assert(std::is_copy_constructible<T>::value,
            "Must by copy constructible");
        static_assert(std::is_trivially_destructible<T>::value,
            "No destructors will be called");
        return new(_alloc.Allocate(alignof(T), sizeof(T))) T(value);
    }

    /// Makes a copy of \p str and returns a pointer to it.
    /// Specialization for c strings.
    const char* StoreData(const char* str) {
        const size_t strLen = std::strlen(str) + 1;
        void* mem = _alloc.Allocate(alignof(char), strLen);
        char* cstr = reinterpret_cast<char*>(mem);
        std::memcpy(cstr, str, strLen);
        return cstr;
    }

private:
    // Simple Allocator that only supports allocations, but not frees. 
    // Allocated memory is tied to the lifetime of the allocator object.
    class Allocator {
    public:
        Allocator(size_t blockSize) 
            : _desiredBlockSize(blockSize) {}
        Allocator(Allocator&&) = default;
        Allocator& operator=(Allocator&&) = default;

        Allocator(const Allocator&) = delete;
        Allocator& operator=(const Allocator&) = delete;

        void* Allocate(const size_t align, const size_t size) {
            Byte* alignedNext = AlignPointer(_next, align);
            Byte* end = alignedNext + size;
            if (ARCH_UNLIKELY(end > _blockEnd)) {
                AllocateBlock(align, size);
                alignedNext = AlignPointer(_next, align);
                end = _next + size;
            }
            _next = end;
            return alignedNext;
        }

    private:
        using Byte = std::uint8_t;

        static Byte* AlignPointer(Byte* ptr, const size_t align) {
            const size_t alignMask = align - 1;
            return reinterpret_cast<Byte*>(
                reinterpret_cast<uintptr_t>(ptr + alignMask) & ~alignMask);
        }

        TRACE_API void AllocateBlock(const size_t align, const size_t desiredSize);

        Byte* _blockEnd = nullptr;
        Byte* _next = nullptr;
        using BlockPtr = std::unique_ptr<Byte[]>;
        std::deque<BlockPtr> _blocks;
        size_t _desiredBlockSize;
    };

    Allocator _alloc;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_DATA_BUFFER_H