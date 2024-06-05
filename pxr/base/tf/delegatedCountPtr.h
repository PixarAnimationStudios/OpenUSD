//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TF_DELEGATED_COUNT_PTR_H
#define PXR_BASE_TF_DELEGATED_COUNT_PTR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/api.h"

#include "pxr/base/tf/diagnosticLite.h"

#include <memory>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// When constructing a `TfDelegatedCountPtr` from a raw pointer, use the
/// `TfDelegatedCountIncrementTag` to explicitly signal that the pointer's
/// delegated count should be incremented on construction. This is the most
/// common tag.
constexpr struct TfDelegatedCountIncrementTagType{}
    TfDelegatedCountIncrementTag{};

/// When constructing a `TfDelegatedCountPtr` from a raw pointer, use the
/// `TfDelegatedCountDoNotIncrementTag` to avoid incrementing the delegated
/// count on construction. This must be carefully used to avoid memory errors.
constexpr struct TfDelegatedCountDoNotIncrementTagType{}
    TfDelegatedCountDoNotIncrementTag{};

/// Stores a pointer to a `ValueType` which uses `TfDelegatedCountIncrement` and
/// `TfDelegatedCountDecrement` to bookkeep.  This class is most useful to adapt
/// existing types that have their own bespoke reference counting logic to a
/// common C++-style "smart pointer" interface.
///
/// The `TfDelegatedCountPtr` calls `TfDelegatedCountIncrement` and
/// `TfDelegatedCountDecrement` as needed in construction, assignment, and
/// destruction operations.  These functions are expected to be provided by
/// client code to do the specific resource management related to the pointed-to
/// object.  These functions must have the following signatures:
///
/// \code
/// void TfDelegatedCountIncrement(MyObject *obj);
/// void TfDelegatedCountDecrement(MyObject *obj);
/// \endcode
///
/// For example if `MyObject` has a reference count member variable, the
/// overload `TfDelegatedCountIncrement(MyObject *obj)` could simply increment
/// that count.  The `TfDelegatedCountDecrement(MyObject *obj)` might decrement
/// the count and check to see if it has gone to zero.  If so, it can clean up
/// resources related to the object such as deleting it or freeing memory.
///
/// These increment and decrement functions are never passed null pointers.
///
/// A `TfDelegatedCountPtr` can be created by construction with a raw pointer,
/// or by `TfMakeDelegatedCountPtr` to create and manage an object on the heap.
template <typename ValueType>
class TfDelegatedCountPtr {
public:
    using RawPtrType = std::add_pointer_t<ValueType>;
    using ReferenceType = std::add_lvalue_reference_t<ValueType>;
    using element_type = ValueType;

    static_assert(
        std::is_same_v<
            void,
            decltype(TfDelegatedCountIncrement(std::declval<RawPtrType>()))>);
    static_assert(
        std::is_same_v<
            void,
            decltype(TfDelegatedCountDecrement(std::declval<RawPtrType>()))>);

    using IncrementIsNoExcept =
        std::integral_constant<
            bool,
            noexcept(TfDelegatedCountIncrement(std::declval<RawPtrType>()))>;
    using DecrementIsNoExcept =
        std::integral_constant<
            bool,
            noexcept(TfDelegatedCountDecrement(std::declval<RawPtrType>()))>;
    using IncrementAndDecrementAreNoExcept =
        std::integral_constant<
            bool, IncrementIsNoExcept() && DecrementIsNoExcept()>;
    using DereferenceIsNoExcept =
        std::integral_constant<bool, noexcept(*std::declval<RawPtrType>())>;

private:
    template <typename ConvertibleType>
    using _IsPtrConvertible = std::is_convertible<
        std::add_pointer_t<ConvertibleType>, RawPtrType>;

public:
    /// Create a pointer storing `nullptr`
    TfDelegatedCountPtr() noexcept = default;

    /// Create a new pointer storing `rawPointer` without calling
    /// `TfDelegatedCountIncrement`.
    /// \sa TfDelegatedCountDoNotIncrementTag
    TfDelegatedCountPtr(TfDelegatedCountDoNotIncrementTagType,
                        RawPtrType rawPointer) noexcept :
        _pointer{rawPointer} {
    }

    /// Create a new pointer storing `rawPointer` and call
    /// `TfDelegatedCountIncrement` on it if it is not `nullptr`.
    /// \sa TfDelegatedCountIncrementTag
    TfDelegatedCountPtr(TfDelegatedCountIncrementTagType,
                        RawPtrType rawPointer)
    noexcept(IncrementIsNoExcept()) :
        _pointer{rawPointer} {
        _IncrementIfValid();
    }

    /// Copy construct from `ptr`, calling `TfDelegatedCountIncrement` on the
    /// held pointer if it is not `nullptr`.
    TfDelegatedCountPtr(const TfDelegatedCountPtr& ptr)
    noexcept(IncrementIsNoExcept()) :
        _pointer{ptr.get()} {
        _IncrementIfValid();
    }

    /// Copy construct from `ptr` if it is convertible to this class's
    /// `RawPtrType`. Call `TfDelegatedCountIncrement` on the held pointer if it
    /// is not `nullptr`.
    template <typename OtherType>
    explicit TfDelegatedCountPtr(
        const TfDelegatedCountPtr<OtherType>& ptr,
        std::enable_if_t<_IsPtrConvertible<OtherType>::value, int> = 0)
    noexcept(IncrementIsNoExcept()) :
        _pointer(ptr.get()) {
        _IncrementIfValid();
    }

    /// Construct by moving from `ptr`.
    ///
    /// `ptr` is left in its default state (i.e. `nullptr`).
    TfDelegatedCountPtr(TfDelegatedCountPtr&& ptr) noexcept :
        _pointer(ptr.get()) {
        ptr._pointer = nullptr;
    }

    /// Assign by copying from `ptr`.
    ///
    /// Call `TfDelegatedCountIncrement` on the held pointer if it is not
    /// `nullptr`.
    TfDelegatedCountPtr& operator=(const TfDelegatedCountPtr& ptr)
    noexcept(IncrementAndDecrementAreNoExcept()) {
        // Implement copy assigment in terms of move assignment
        return (*this = TfDelegatedCountPtr{ptr});
    }

    /// Assign by copying from `ptr` if it is convertible to this class's
    /// `RawPtrType`. Call `TfDelegatedCountIncrement` on the held pointer if it
    /// is not `nullptr`.
    template <typename OtherType>
    TfDelegatedCountPtr& operator=(
        const TfDelegatedCountPtr<OtherType>& ptr)
    noexcept(IncrementAndDecrementAreNoExcept()) {
        static_assert(_IsPtrConvertible<OtherType>::value);
        // Implement copy assigment in terms of move assignment
        return (*this = TfDelegatedCountPtr{ptr});
    }

    /// Assign by moving from `ptr`.
    ///
    /// `ptr` is left in its default state (i.e. `nullptr`).
    TfDelegatedCountPtr& operator=(TfDelegatedCountPtr&& ptr)
    noexcept(DecrementIsNoExcept()) {
        _DecrementIfValid();
        _pointer = ptr.get();
        ptr._pointer = nullptr;
        return *this;
    }

    /// Reset this pointer to its default state (i.e. `nullptr`)
    TfDelegatedCountPtr& operator=(std::nullptr_t)
    noexcept(DecrementIsNoExcept()) {
        reset();
        return *this;
    }

    /// Call `TfDelegatedCountDecrement` on the held pointer if it is not
    /// nullptr`. A bug occurs in VS2017 where calling DecrementIsNoExcept() 
    /// may return void. The bug is possibly related to an issue with using
    /// noexcept expressions in destructors.
    ~TfDelegatedCountPtr() noexcept(DecrementIsNoExcept::value) {
        _DecrementIfValid();
    }

    /// Dereference the underlying pointer.
    ReferenceType operator*() const noexcept(DereferenceIsNoExcept()) {
        return *get();
    }

    /// Arrow operator dispatch for the underlying pointer.
    RawPtrType operator->() const noexcept {
        return get();
    }

    /// Return true if the underlying pointer is non-null, false otherwise.
    explicit operator bool() const noexcept { return get(); }

    /// Return true if the underlying pointers are equivalent.
    template <typename OtherType>
    bool operator==(
        const TfDelegatedCountPtr<OtherType>& other) const noexcept {
        return get() == other.get();
    }

    /// Returns false if the underlying pointers are equivalent.
    template <typename OtherType>
    bool operator!=(
        const TfDelegatedCountPtr<OtherType>& other) const noexcept {
        return get() != other.get();
    }

    /// Orders based on the underlying pointer.
    template <typename OtherType>
    bool operator<(
        const TfDelegatedCountPtr<OtherType>& other) const noexcept {
        return get() < other.get();
    }

    /// Return the underlying pointer.
    RawPtrType get() const noexcept { return _pointer; }

    /// Reset the pointer to its default state (`nullptr`), calling
    /// `TfDelegatedCountDecrement` if the held pointer is not null.
    void reset() noexcept(DecrementIsNoExcept()) {
        _DecrementIfValid();
        _pointer = nullptr;
    }

    /// Swap this object's held pointer with other's.
    void swap(TfDelegatedCountPtr& other) noexcept {
        std::swap(other._pointer, _pointer);
    }

private:
    void _IncrementIfValid() noexcept(IncrementIsNoExcept()) {
        if (_pointer) {
             TfDelegatedCountIncrement(_pointer);
        }
    }

    void _DecrementIfValid() noexcept(DecrementIsNoExcept()) {
        if (_pointer) {
            TfDelegatedCountDecrement(_pointer);
        }
    }

    ValueType* _pointer{nullptr};    
};

/// Construct a `ValueType` instance on the heap via `new`, passing `args`. Call
/// `TfDelegatedCountIncrement` on the resulting pointer and return a
/// `TfDelegatedCountPtr` holding that pointer.
template <typename ValueType, typename... Args>
TfDelegatedCountPtr<ValueType>
TfMakeDelegatedCountPtr(Args&&... args) {
    return TfDelegatedCountPtr<ValueType>(
        TfDelegatedCountIncrementTag,
        new ValueType(std::forward<Args>(args)...)
    );
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
