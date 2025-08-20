//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_FIXED_SIZE_HOLDER_H
#define PXR_EXEC_VDF_FIXED_SIZE_HOLDER_H

#include "pxr/pxr.h"

#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/mallocTag.h"

#include <memory>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// Storage class used when T is too big to fit in Vdf_FixedSizeHolder's Size.
// It is mutable and therefore it makes deep copies.
template <typename T>
class Vdf_FixedSizeHolderRemoteStorage
{
public:
    template <typename U>
    explicit Vdf_FixedSizeHolderRemoteStorage(U &&value) {
        TfAutoMallocTag2 tag2("Vdf", "Vdf_FixedSizeHolder::ctor");
        TfAutoMallocTag tag(__ARCH_PRETTY_FUNCTION__);
        _pointer.reset(new T(std::forward<U>(value)));
    }

    Vdf_FixedSizeHolderRemoteStorage(
        Vdf_FixedSizeHolderRemoteStorage const &other) {
        TfAutoMallocTag2 tag2("Vdf", "Vdf_FixedSizeHolder::copy ctor");
        TfAutoMallocTag tag(__ARCH_PRETTY_FUNCTION__);
        _pointer.reset(new T(other.Get()));
    }
    Vdf_FixedSizeHolderRemoteStorage &operator=(
        Vdf_FixedSizeHolderRemoteStorage const &other) {
        if (this != &other) {
            TfAutoMallocTag2 tag2("Vdf", "Vdf_FixedSizeHolder::assignment");
            TfAutoMallocTag tag(__ARCH_PRETTY_FUNCTION__);
            _pointer.reset(new T(other.Get()));
        }
        return *this;
    }

    Vdf_FixedSizeHolderRemoteStorage(
        Vdf_FixedSizeHolderRemoteStorage &&) = default;
    Vdf_FixedSizeHolderRemoteStorage &operator=(
        Vdf_FixedSizeHolderRemoteStorage &&) = default;

    inline T const &Get() const {
        return *_pointer;
    }
    inline T &GetMutable() {
        return *_pointer;
    }
private:
    std::unique_ptr<T> _pointer;
};

// Local storage class used when T is small enough to fit in
// Vdf_FixedSizeHolder's Size.
template <typename T>
class Vdf_FixedSizeHolderLocalStorage
{
public:
    template <typename U>
    explicit Vdf_FixedSizeHolderLocalStorage(U &&value)
        : _value(std::forward<U>(value))
    {}
    inline T const &Get() const {
        return _value;
    }
    inline T &GetMutable() {
        return _value;
    }
private:
    T _value;
};

///
/// \class Vdf_FixedSizeHolder
///
/// Vdf_FixedSizeHolder holds an object of type \a T of any size, but the
/// sizeof(Vdf_FixedSizeHolder<T>) is always exactly \a Size.  If \a T's size is
/// less than or equal to \a Size, it is stored directly in member data.  If
/// instead it is greater than \a Size, it is stored on the heap.
/// 
/// The remote storage policy allows mutation of the object held in the
/// Vdf_FixedSizeHolder, but does this by deep-copying the held object whenever
/// the holder is copied.
///
template <class T, size_t Size>
class Vdf_FixedSizeHolder
{
    // Ensure that Size is large enough to hold remote storage even if this
    // particular T fits into local storage.
    static_assert(sizeof(Vdf_FixedSizeHolderRemoteStorage<int>) <= Size,
                  "Size too small to allow remote storage");

    // Choose storage type based on the size of T.  Local if small enough,
    // heap if too big.
    typedef typename std::conditional<
        sizeof(T) <= Size,
        Vdf_FixedSizeHolderLocalStorage<T>,
        Vdf_FixedSizeHolderRemoteStorage<T>
    >::type _StorageType;

public:

    //! Construct a fixed size holder holding \a obj.
    explicit Vdf_FixedSizeHolder(T const &obj) : _storage(obj)
    {
    }

    //! Construct a fixed size holder holding \a obj.
    explicit Vdf_FixedSizeHolder(T &&obj) : _storage(std::move(obj))
    {
    }

    Vdf_FixedSizeHolder(const Vdf_FixedSizeHolder &other)
        : _storage(other._storage)
    {
    }

    Vdf_FixedSizeHolder(Vdf_FixedSizeHolder &&other)
        : _storage(std::move(other._storage))
    {
    }

    ~Vdf_FixedSizeHolder() {
        _storage.~_StorageType();
    }

    Vdf_FixedSizeHolder& operator=(const Vdf_FixedSizeHolder &other) {
        _storage = other._storage;
        return *this;
    }

    Vdf_FixedSizeHolder& operator=(Vdf_FixedSizeHolder &&other) {
        _storage = std::move(other._storage);
        return *this;
    }

    inline T const &Get() const {
        return _storage.Get();
    }

    inline T &GetMutable() {
        return _storage.GetMutable();
    }

    inline void Set(T const &value) {
        _storage.GetMutable() = value;
    }

    inline void Set(T &&value) {
        _storage.GetMutable() = std::move(value);
    }

private:
    union {
        _StorageType _storage;
        char _padding[Size];
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_EXEC_VDF_FIXED_SIZE_HOLDER_H
