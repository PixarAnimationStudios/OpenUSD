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
#ifndef TF_POINTERANDBITS_H
#define TF_POINTERANDBITS_H

#include "pxr/pxr.h"
#include "pxr/base/arch/pragmas.h"

#include <cstdint>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// Return true if \p val is a power of two.
constexpr bool Tf_IsPow2(uintptr_t val) {
    return val && !(val & (val - 1));
}

/// \class TfPointerAndBits
///
/// This class stores a T * and a small integer in the space of a T *. The
/// number of bits possible to store depends on the alignment of T.  The
/// number of distinct values representable by the bits and the maximal value
/// are exposed via the compile time constants \a NumBitsValues and \a
/// MaxValue, respectively.
///
/// The bits may be set and retrieved as any integral type.  The pointer value
/// and the bits value may be set and retrieved independently.
///
template <class T>
class TfPointerAndBits
{
    // Microsoft Visual Studio doesn't like alignof(<abstract-type>).
    // We'll assume that such an object has a pointer in it (the vtbl
    // pointer) and use void* for alignment in that case.
    template <typename U, bool = false>
    struct _AlignOf {
        static constexpr uintptr_t value = alignof(U);
    };
    template <typename U>
    struct _AlignOf<U, true> {
        static constexpr uintptr_t value = alignof(void*);
    };

    // Microsoft Visual Studio doesn't like alignof(<abstract-type>).
    // We'll assume that such an object has a pointer in it (the vtbl
    // pointer) and use void* for alignment in that case.
    static constexpr uintptr_t _GetAlign() {
        return _AlignOf<T, std::is_abstract<T>::value>::value;
    }

    static constexpr bool _SupportsAtLeastOneBit() {
        return _GetAlign() > 1 && Tf_IsPow2(_GetAlign());
    }

public:
    /// Constructor.  Pointer is initialized to null, bits are initialized to
    /// zero.
    constexpr TfPointerAndBits() : _ptrAndBits(0) {
        static_assert(_SupportsAtLeastOneBit(),
                      "T's alignment does not support any bits");
    }

    /// Constructor.  Set the pointer to \a p, and the bits to \a bits.
    constexpr explicit TfPointerAndBits(T *p, uintptr_t bits = 0)
        : _ptrAndBits(_Combine(p, bits))
    {
        static_assert(_SupportsAtLeastOneBit(),
                      "T's alignment does not support any bits");
    }

    constexpr uintptr_t GetMaxValue() const {
        return _GetAlign() - 1;
    }

    constexpr uintptr_t GetNumBitsValues() const {
        return _GetAlign();
    }

    /// Assignment.  Leaves bits unmodified.
    TfPointerAndBits &operator=(T *ptr) {
        _SetPtr(ptr);
        return *this;
    }

    /// Indirection.
    T *operator->() const {
        return _GetPtr();
    }

    /// Dereference.
    T &operator *() const {
        return *_GetPtr();
    }

    /// Retrieve the stored bits as the integral type \a Integral.
    template <class Integral>
    Integral BitsAs() const {
        ARCH_PRAGMA_PUSH
        ARCH_PRAGMA_FORCING_TO_BOOL
        return static_cast<Integral>(_GetBits());
        ARCH_PRAGMA_POP
    }

    /// Set the stored bits.  No static range checking is performed.
    template <class Integral>
    void SetBits(Integral val) {
        _SetBits(static_cast<uintptr_t>(val));
    }

    /// Set the pointer value to \a ptr.
    void Set(T *ptr) {
        _SetPtr(ptr);
    }

    /// Set the pointer value to \a ptr and the bits to \a val.
    template <class Integral>
    void Set(T *ptr, Integral val) {
        _ptrAndBits = _Combine(ptr, val);
    }

    /// Retrieve the pointer.
    T *Get() const {
        return _GetPtr();
    }

    /// Retrieve the raw underlying value.  This can be useful for doing literal
    /// equality checks between two instances.  The only guarantees are that
    /// this has the same bit pattern as the pointer value if the bits are 0,
    /// and will compare equal to another instance when both have identical
    /// pointer and bits values.
    constexpr uintptr_t GetLiteral() const {
        return _AsInt(_ptrAndBits);
    }

    /// Swap this PointerAndBits with \a other.
    void Swap(TfPointerAndBits &other) {
        ::std::swap(_ptrAndBits, other._ptrAndBits);
    }

private:
    constexpr uintptr_t _GetBitMask() const {
        return GetMaxValue();
    }

    // Combine \a p and \a bits into a single pointer value.
    constexpr T *_Combine(T *p, uintptr_t bits) const {
        return _AsPtr(_AsInt(p) | (bits & _GetBitMask()));
    }

    // Cast the pointer \a p to an integral type.  This function and _AsPtr are
    // the only ones that do the dubious compiler-specific casting.
    constexpr uintptr_t _AsInt(T *p) const {
        return (uintptr_t)p;
    }

    // Cast the integral \a i to the pointer type.  This function and _AsInt are
    // the only ones that do the dubious compiler-specific casting.
    constexpr T *_AsPtr(uintptr_t i) const {
        return (T *)i;
    }

    // Retrieve the held pointer value.
    inline T *_GetPtr() const {
        return _AsPtr(_AsInt(_ptrAndBits) & ~_GetBitMask());
    }

    // Set the held pointer value.
    inline void _SetPtr(T *p) {
        _ptrAndBits = _Combine(p, _GetBits());
    }

    // Retrieve the held bits value.
    inline uintptr_t _GetBits() const {
        return _AsInt(_ptrAndBits) & _GetBitMask();
    }

    // Set the held bits value.
    inline void _SetBits(uintptr_t bits) {
        _ptrAndBits = _Combine(_GetPtr(), bits);
    }

    // Single pointer member stores pointer value and bits.
    T *_ptrAndBits;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_POINTERANDBITS_H
