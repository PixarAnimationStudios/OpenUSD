//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_INPUT_KEY_H
#define PXR_EXEC_EXEC_INPUT_KEY_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/providerResolution.h"

#include "pxr/base/tf/delegatedCountPtr.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// Data used to specify a computation input.
///
/// Exec compilation uses input keys to compile the input connections for that
/// provide input values to computations. The input key is expressed relative to
/// the scene object that owns the computation that reads from the input.
///
struct Exec_InputKey
{
    /// The name used to uniquely address the input value.
    TfToken inputName;

    /// The requested computation name.
    TfToken computationName;

    /// The requested computation result type.
    TfType resultType;

    /// Describes how we find the provider, starting from the object that owns
    /// the computation to which this key provides an input.
    ///
    ExecProviderResolution providerResolution;

    /// Indicates whether or not the input will fall back to looking for a
    /// dispatched computation if a local computation can't be found.
    ///
    bool fallsBackToDispatched:1;

    /// Indicates whether or not the input is optional.
    bool optional:1;
};

/// A vector of input keys.
///
/// This is chosen for efficient storage of input keys in
/// Exec_ComputationDefinition%s. The class wraps a TfSmallVector of
/// Exec_InputKey%s and an atomic reference counter so that vectors can be
/// shared by TfDelegatedCountPtr.
///
class Exec_InputKeyVector
{
public:
    /// Returns a TfDelegatedCountPtr to a new Exec_InputKeyVector.
    template <class... Args>
    static TfDelegatedCountPtr<Exec_InputKeyVector> MakeShared(Args &&...args) {
        return TfMakeDelegatedCountPtr<Exec_InputKeyVector>(
            std::forward<Args>(args)...);
    }

    /// Returns a TfDelegatedCountPtr to a common immutable empty vector.
    ///
    /// Computation definitions can return this pointer instead of allocating
    /// their own empty vectors.
    ///
    static TfDelegatedCountPtr<const Exec_InputKeyVector> GetEmptyVector() {
        static TfDelegatedCountPtr<const Exec_InputKeyVector> emptyVector =
            TfMakeDelegatedCountPtr<const Exec_InputKeyVector>();
        return emptyVector;
    }

    /// Gets the wrapped vector of input keys.
    TfSmallVector<Exec_InputKey, 1>& Get() {
        return _inputKeys;
    }

    /// Gets the wrapped vector of input keys.
    const TfSmallVector<Exec_InputKey, 1> &Get() const {
        return _inputKeys;
    }

private:
    // TfDelegatedCountPtr needs to call constructors.
    template <typename ValueType, typename... Args>
    friend TfDelegatedCountPtr<ValueType>
    TfMakeDelegatedCountPtr(Args&&... args);

    // Constructs an Exec_InputKeyVector by forwarding \p args to the
    // TfSmallVector constructor.
    //
    template <class... Args>
    explicit Exec_InputKeyVector(Args &&...args)
        : _inputKeys(std::forward<Args>(args)...)
        , _refCount(0)
    {}

    // Increments the refcount.
    friend void TfDelegatedCountIncrement(
        const Exec_InputKeyVector *const inputKeys) noexcept {
        inputKeys->_refCount.fetch_add(1, std::memory_order_relaxed);
    }

    // Decrements the refcount, and deletes the pointer if it reaches 0.
    friend void TfDelegatedCountDecrement(
        const Exec_InputKeyVector *const inputKeys) noexcept {
        if (inputKeys->_refCount.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete inputKeys;
        }
    }

private:
    TfSmallVector<Exec_InputKey, 1> _inputKeys;

    mutable std::atomic_int _refCount;
};

/// A reference-counted pointer to a shared mutable vector of input keys.
using Exec_InputKeyVectorRefPtr =
    TfDelegatedCountPtr<Exec_InputKeyVector>;

/// A reference-counted pointer to a shared immutable vector of input keys.
using Exec_InputKeyVectorConstRefPtr =
    TfDelegatedCountPtr<const Exec_InputKeyVector>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
