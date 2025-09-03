//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_OUTPUT_KEY_H
#define PXR_EXEC_EXEC_OUTPUT_KEY_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/exec/computationDefinition.h"

#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/schemaConfigKey.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Specifies an output to compile.
///
/// Compilation uses output keys to identify outputs to compile along with
/// parameters needed for their compilation.
/// 
class Exec_OutputKey
{
public:
    Exec_OutputKey(
        const EsfObject &providerObject,
        const EsfSchemaConfigKey dispatchingSchemaKey,
        const Exec_ComputationDefinition *const computationDefinition,
        const TfToken &disambiguatingId = {})
        : _providerObject(providerObject)
        , _dispatchingSchemaKey(dispatchingSchemaKey)
        , _computationDefinition(computationDefinition)
        , _disambiguatingId(disambiguatingId)
    {}

    /// Returns the object that provides the computation.
    const EsfObject &GetProviderObject() const {
        return _providerObject;
    }

    /// Returns the schema config key that should be used for computation lookup
    /// for any input keys that request dispatched inputs, when compiling the
    /// node that provides the output described by this key.
    ///
    /// The returned config key is either the key for the output key's provider
    /// or the key for the provider at the start of a recursive dispatched
    /// computation chain.
    ///
    EsfSchemaConfigKey GetDispatchingSchemaKey() const {
        return _dispatchingSchemaKey;
    }

    /// Returns the definition of the computation to compile.
    const Exec_ComputationDefinition *GetComputationDefinition() const {
        return _computationDefinition;
    }

    /// Returns a token that can be used to distinguish different computations
    /// that share the same computationName.
    ///
    const TfToken &GetDisambiguatingId() const {
        return _disambiguatingId;
    }

    /// Identity class. See Exec_OutputKey::Identity below.
    class Identity;

    /// Constructs and returns an identity for this output key.
    inline Identity MakeIdentity() const;

private:
    EsfObject _providerObject;
    EsfSchemaConfigKey _dispatchingSchemaKey;
    const Exec_ComputationDefinition *_computationDefinition;
    TfToken _disambiguatingId;
};

/// Lightweight identity that represents an Exec_OutputKey.
/// 
/// Instances of this class contain all the information necessary to represent
/// an Exec_OutputKey, while being lightweight, comparable, and hashable. They
/// can be used, for example, as key types in hash maps.
/// 
/// \note
/// Identities are not automatically maintained across scene edits.
///
class Exec_OutputKey::Identity
{
public:
    explicit Identity(const Exec_OutputKey &key)
        : _providerPath(key._providerObject->GetPath(nullptr))
        , _computationDefinition(key._computationDefinition)
        , _disambiguatingId(key._disambiguatingId)
    {}

    bool operator==(const Exec_OutputKey::Identity &rhs) const {
        return
            _providerPath == rhs._providerPath &&
            _computationDefinition == rhs._computationDefinition &&
            _disambiguatingId == rhs._disambiguatingId;
    }

    bool operator!=(const Exec_OutputKey::Identity &rhs) const {
        return !(*this == rhs);
    }

    template <typename HashState>
    friend void TfHashAppend(
        HashState& h, const Exec_OutputKey::Identity& identity) {
        h.Append(identity._providerPath);
        h.Append(identity._computationDefinition);
        h.Append(identity._disambiguatingId);
    }

    /// Return a human-readable description of this value key for diagnostic
    /// purposes.
    /// 
    EXEC_API std::string GetDebugName() const;

private:
    SdfPath _providerPath;
    const Exec_ComputationDefinition *_computationDefinition;
    TfToken _disambiguatingId;
};

Exec_OutputKey::Identity 
Exec_OutputKey::MakeIdentity() const
{
    return Identity(*this);
}

/// A vector of output keys.
///
/// This is chosen for efficient storage of output keys generated from
/// Exec_CompilationTask%s, where often just a single output key is generated
/// per input.
///
using Exec_OutputKeyVector = TfSmallVector<Exec_OutputKey, 1>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
