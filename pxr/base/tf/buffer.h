//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_BUFFER_H
#define PXR_BASE_TF_BUFFER_H

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnosticLite.h"

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// Tag type for constructing a Tf_Buffer without allocating the underlying
// array.  Use with tf_BufferDeferAllocation
struct Tf_BufferDeferAllocation {};

// Tag value for deferred-allocation Tf_Buffer construction.
static constexpr Tf_BufferDeferAllocation tf_BufferDeferAllocation {};

// A move-only, heap-allocated buffer with compile-time capacity and dynamic
// size. Intended for trivially-destructible aggregate types in
// performance-sensitive producer/consumer patterns.  This class was originally
// written as a support piece for TfMallocTag, but lives in a private header for
// modularity.
//
// Because Capacity is fixed and the backing array is allocated exactly once,
// elements never move.  Pointers, references and iterators to elements remain
// valid across emplace_back() and pop_back() for as long as the buffer owns its
// array.
//
// T must be trivially destructible and default constructible. Capacity is fixed
// at compile time. Size is dynamic, bounded by Capacity.
template <typename T, size_t Capacity>
class Tf_Buffer
{
    static_assert(std::is_trivially_destructible_v<T>,
                  "Tf_Buffer requires trivially destructible T");
    static_assert(std::is_default_constructible_v<T>,
                  "Tf_Buffer requires default constructible T");

public:
    using value_type      = T;
    using size_type       = size_t;
    using pointer         = T*;
    using reference       = T&;
    using iterator        = T*;
    using const_pointer   = const T*;
    using const_reference = const T&;
    using const_iterator  = const T*;

    // Tag type for constructing a Tf_Buffer without allocating the underlying
    // array.  Use with Tf_Buffer::deferAllocation
    using DeferAllocation = Tf_BufferDeferAllocation;

    // Tag value for deferred-allocation Tf_Buffer construction.
    static constexpr DeferAllocation deferAllocation {};
    
    // Construction & assignment

    // Construct a new empty buffer by allocating a Capacity-sized array on the
    // heap.  Elements are default-initialized, so for trivially
    // default-constructible T no per-element work is done and construction
    // costs a single allocation.  For T with a nontrivial default constructor
    // that constructor runs Capacity times.
    Tf_Buffer() : _data(new T[Capacity]) {}

    // Construct a new buffer with no backing array.  This leaves the buffer in
    // exactly the state a moved-from buffer is left in: empty(), size() == 0,
    // data() == nullptr, begin() == end().  Call allocate() or move-assign from
    // another buffer to bring it into service.
    explicit Tf_Buffer(DeferAllocation) {}

    // Move-only.  A moved-from buffer is left empty and unallocated, the same
    // state as a DeferAllocation-constructed buffer.
    Tf_Buffer(Tf_Buffer&& other) noexcept
        : _data(std::move(other._data))
        , _size(std::exchange(other._size, 0)) {}

    Tf_Buffer& operator=(Tf_Buffer&& other) noexcept {
        if (this != &other) {
            _data = std::move(other._data);
            _size = std::exchange(other._size, 0);
        }
        return *this;
    }

    Tf_Buffer(const Tf_Buffer&) = delete;
    Tf_Buffer& operator=(const Tf_Buffer&) = delete;

    // Return the buffer's fixed Capacity.
    static constexpr size_t capacity() { return Capacity; }

    // Return the buffer's current size.
    size_t size() const { return _size; }

    // Return true if the buffer's size is zero.
    bool empty() const { return _size == 0; }

    // Return true if the buffer's size is Capacity.
    bool full() const { return _size == Capacity; }

    // Access element at index \p i.
    reference operator[](size_type i) {
        TF_DEV_AXIOM(i < _size);
        return _data[i];
    }

    // Access element at index \p i.
    const_reference operator[](size_type i) const {
        TF_DEV_AXIOM(i < _size);
        return _data[i];
    }

    // Access the first element.  The buffer must not be empty().
    reference front() {
        TF_DEV_AXIOM(!empty());
        return _data[0];
    }

    // Access the first element.  The buffer must not be empty().
    const_reference front() const {
        TF_DEV_AXIOM(!empty());
        return _data[0];
    }

    // Access the last element.  The buffer must not be empty().
    reference back() {
        TF_DEV_AXIOM(!empty());
        return _data[_size - 1];
    }

    // Access the last element.  The buffer must not be empty().
    const_reference back() const {
        TF_DEV_AXIOM(!empty());
        return _data[_size - 1];
    }

    // Return an iterator to the first element in the buffer.
    iterator begin() { return _data.get(); }
    // Return the past-the-end iterator.
    iterator end()   { return _data.get() + _size; }

    // Return an iterator to the first element in the buffer.
    const_iterator begin() const { return _data.get(); }
    // Return the past-the-end iterator.
    const_iterator end()   const { return _data.get() + _size; }

    // Return an iterator to the first element in the buffer.
    const_iterator cbegin() const { return _data.get(); }
    // Return the past-the-end iterator.
    const_iterator cend()   const { return _data.get() + _size; }

    // Return a pointer to the buffer's data.
    pointer data() { return _data.get(); }

    // Return a pointer to the buffer's data.
    const_pointer data() const { return _data.get(); }

    // Return a pointer to the buffer's data.
    const_pointer cdata() const { return _data.get(); }

    // Construct an element in-place at the end. The buffer must not be full(),
    // and must be allocated -- see allocate().
    template <typename... Args>
    reference emplace_back(Args&&... args) {
        TF_DEV_AXIOM(_data);
        TF_DEV_AXIOM(_size < Capacity);
        T* slot = _data.get() + _size;
        new (slot) T { std::forward<Args>(args)... };
        ++_size;
        return *slot;
    }

    // Copy an element onto the end. Delegates to emplace_back.
    void push_back(const T& val) {
        emplace_back(val);
    }

    // Remove the last element. Asserts buffer is not empty.
    void pop_back() {
        TF_DEV_AXIOM(!empty());
        --_size;
        // No destructor call - T is trivially destructible.
    }

    // Reset size to zero.  Does not deallocate; the existing array is retained
    // for reuse.  Does not allocate either, so an unallocated buffer stays
    // unallocated -- use allocate() to bring one into service.
    void clear() noexcept {
        _size = 0;
    }

    // Allocate the backing array if this buffer does not already have one,
    // bringing a deferred-allocation or moved-from buffer into service as an
    // empty buffer.  A no-op if already allocated, so it is safe to call
    // unconditionally.  Unlike clear(), this may throw std::bad_alloc.
    void allocate() {
        if (!_data) {
            TF_DEV_AXIOM(_size == 0);
            _data.reset(new T[Capacity]);
        }
    }

private:
    std::unique_ptr<T[]> _data;
    size_t               _size = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_BUFFER_H
