//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_BUILTIN_COMPUTATION_REGISTRY_H
#define PXR_EXEC_EXEC_BUILTIN_COMPUTATION_REGISTRY_H

/// \file
/// This file defines the Exec_BuiltinComputationRegistry which is a singleton
/// class that associates each built-in computation with an
/// Exec_BuiltinComputationTraits that describes the behavior of the built-in
/// computation.

#include "pxr/pxr.h"

#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Describes how a built-in computation is allowed to be used.
class Exec_BuiltinComputationTraits
{
public:
    /// True if users can register a computation by this name.
    bool isUserDefinable = false;

    /// True if computations can explicitly request this built-in computation as
    /// a computation input.
    ///
    bool isInputConsumable = true;
    
    /// True if this built-in computation has a concrete
    /// Exec_ComputationDefinition instance in the Exec_DefinitionRegistry.
    ///
    bool hasDefinition = true;

    /// \name Builder Interface
    ///
    /// These methods are used to construct Exec_BuiltinComputationTraits
    /// objects without reliance on the member order.
    ///
    /// ## Example
    ///
    /// ```{.cpp}
    /// const auto traits = Exec_BuiltinComputaitonTraits()
    ///     .SetHasDefinition(false)
    ///     .SetIsInputConsumable(false)
    ///     .SetIsUserDefinable(true);
    /// ```
    ///
    /// @{

    Exec_BuiltinComputationTraits SetIsUserDefinable(
        const bool isUserDefinable_) && {
        isUserDefinable = isUserDefinable_;
        return std::move(*this);
    }

    Exec_BuiltinComputationTraits SetIsInputConsumable(
        const bool isInputConsumable_) && {
        isInputConsumable = isInputConsumable_;
        return std::move(*this);
    }

    Exec_BuiltinComputationTraits SetHasDefinition(
        const bool hasDefinition_) && {
        hasDefinition = hasDefinition_;
        return std::move(*this);
    }

    /// @}
};


/// A registry that contains the name of every built-in computation known to
/// exec, and associates each with an Exec_BuiltinComputationTraits.
///
class Exec_BuiltinComputationRegistry
{
public:
    /// Gets the singleton instance of the registry.
    static const Exec_BuiltinComputationRegistry &GetInstance();

    /// Returns true if \p computationName is reserved for built-in
    /// computations.
    ///
    /// Note this may return true even if the computation name has not been
    /// registered.
    ///
    static bool IsReservedName(const TfToken &computationName);

    /// Returns the string that prefixes all built-in computation names.
    static const char *GetReservedNamePrefix();

    /// Returns the number of registered built-in computations that have
    /// concrete Exec_ComputationDefinition objects in the definition registry.
    ///
    size_t GetNumComputationsWithDefinitions() const {
        return _numComputationsWithDefinitions;
    }

    /// Gets the traits associated with the built-in computation named
    /// \p computationName.
    ///
    /// If there is no such built-in computation, this returns a null pointer.
    ///
    const Exec_BuiltinComputationTraits *GetTraits(
        const TfToken &computationName) const;

private:
    // Only TfSingleton can create instances.
    friend class TfSingleton<Exec_BuiltinComputationRegistry>;

    Exec_BuiltinComputationRegistry();

    // Only these classes can register built-in computation names.
    friend class Exec_BuiltinComputationTokens;
    friend class Exec_PrivateBuiltinComputationTokens;

    // Obtains a mutable reference to the singleton in order to register
    // built-in computations.
    static Exec_BuiltinComputationRegistry &_GetInstanceForRegistration();

    // Registers a new built-in computation named \p computationName (without
    // the built-in computation prefix), and with the provided
    // \p computationTraits.
    TfToken _RegisterBuiltinComputation(
        const std::string &computationName,
        Exec_BuiltinComputationTraits computationTraits = {});

private:
    pxr_tsl::robin_map<TfToken, Exec_BuiltinComputationTraits, TfHash> _traits;
    size_t _numComputationsWithDefinitions = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif