//
// Copyright 2024 Pixar
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

#ifndef PXR_BASE_TF_DELEGATED_COUNT_PTR_H
#define PXR_BASE_TF_DELEGATED_COUNT_PTR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/api.h"

#include "pxr/base/tf/diagnosticLite.h"

#include <memory>
#include <type_traits>

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

/// Stores a pointer to a `ValueType` which uses `TfDelegatedCountIncrement`
/// and `TfDelegatedCountDecrement` to bookkeep.
///
/// The `TfDelegatedCountPtr` is responsible for calling
/// `TfDelegatedCountIncrement` and `TfDelegatedCountDecrement` as needed in
/// construction, assignment, and destruction operations.
///
/// The increment and decrement methods can assume pointers are non-null.
/// Releasing resources (e.g. freeing memory) is delegated to the user defined
/// `TfDelegatedCountDecrement` overloads.
///
/// This may be created from a user constructed raw pointer or via
/// `TfMakeDelegatedCountPtr`.
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

    /// Creates a new pointer without retaining
    /// \sa TfDelegatedCountDoNotIncrementTag
    TfDelegatedCountPtr(TfDelegatedCountDoNotIncrementTagType,
                        RawPtrType rawPointer) noexcept :
        _pointer{rawPointer} {
    }

    /// Creates a new retained pointer
    ///
    /// Retains only if the pointer is valid.
    /// \sa TfDelegatedCountIncrementTag
    TfDelegatedCountPtr(TfDelegatedCountIncrementTagType,
                        RawPtrType rawPointer)
    noexcept(IncrementIsNoExcept()) :
        _pointer{rawPointer} {
        _IncrementIfValid();
    }

    /// Constructs by copying the pointer from `ptr`.
    ///
    /// Retains only if the pointer is valid.
    TfDelegatedCountPtr(const TfDelegatedCountPtr& ptr)
    noexcept(IncrementIsNoExcept()) :
        _pointer{ptr.get()} {
        _IncrementIfValid();
    }

    /// Constructs by copying a convertible pointer from `ptr`.
    ///
    /// This is necessary in part to allow a `const` `ValueType` to be
    /// constructed from from a non-`const` `ValueType`.
    ///
    /// Retains only if the pointer is valid.
    template <typename OtherType>
    explicit TfDelegatedCountPtr(
        const TfDelegatedCountPtr<OtherType>& ptr,
        std::enable_if_t<_IsPtrConvertible<OtherType>::value, int> = 0)
    noexcept(IncrementIsNoExcept()) :
        _pointer(ptr.get()) {
        _IncrementIfValid();
    }

    /// Constructs by moving the pointer from `ptr`.
    ///
    /// `ptr` will be reset to its default state (ie. `nullptr`).
    TfDelegatedCountPtr(TfDelegatedCountPtr&& ptr) noexcept :
        _pointer(ptr.get()) {
        ptr._pointer = nullptr;
    }

    /// Assigns by copying the pointer from `ptr`.
    ///
    /// Retains only if the pointer is valid.
    TfDelegatedCountPtr& operator=(const TfDelegatedCountPtr& ptr)
    noexcept(IncrementAndDecrementAreNoExcept()) {
        // Implement copy assigment in terms of move assignment
        return (*this = TfDelegatedCountPtr{ptr});
    }

    /// Assigns by copying a convertible pointer from `ptr`.
    ///
    /// This is necessary in part to allow a `const` `ValueType` to be
    /// assigned from from a non-`const` `ValueType`.
    ///
    /// Retains only if the pointer is valid.
    template <typename OtherType>
    TfDelegatedCountPtr& operator=(
        const TfDelegatedCountPtr<OtherType>& ptr)
    noexcept(IncrementAndDecrementAreNoExcept()) {
        static_assert(_IsPtrConvertible<OtherType>::value);
        // Implement copy assigment in terms of move assignment
        return (*this = TfDelegatedCountPtr{ptr});
    }

    /// Assigns by moving the pointer from `ptr`.
    ///
    /// `ptr` will be reset to its default state (ie. `nullptr`).
    TfDelegatedCountPtr& operator=(TfDelegatedCountPtr&& ptr)
    noexcept(DecrementIsNoExcept()) {
        _DecrementIfValid();
        _pointer = ptr.get();
        ptr._pointer = nullptr;
        return *this;
    }

    /// Resets the pointer to its default state (ie. `nullptr`)
    TfDelegatedCountPtr& operator=(std::nullptr_t)
    noexcept(DecrementIsNoExcept()) {
        reset();
        return *this;
    }

    /// Releases if the underlying pointer is valid 
    ~TfDelegatedCountPtr() noexcept(DecrementIsNoExcept()) {
        _DecrementIfValid();
    }

    /// Dereference the underlying pointer
    ReferenceType operator*() const noexcept(DereferenceIsNoExcept()) {
        return *get();
    }

    /// Arrow operator dispatch for the underlying pointer
    RawPtrType operator->() const noexcept {
        return get();
    }

    /// Returns true if the underlying pointer is non-null.
    explicit operator bool() const noexcept { return get(); }

    /// Returns true if the underlying pointers are equivalent.
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

    /// Reset the pointer to its default state (`nullptr`), releasing if
    /// necessary.
    void reset() noexcept(DecrementIsNoExcept()) {
        _DecrementIfValid();
        _pointer = nullptr;
    }

    /// Swaps the pointer between two delegated count pointers
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

/// Constructs a `TfDelegatedCountPtr` using `args`.
///
/// This will necessarily call TfDelegatedCountIncrement once
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