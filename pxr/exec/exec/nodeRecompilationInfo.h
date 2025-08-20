//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_NODE_RECOMPILATION_INFO_H
#define PXR_EXEC_EXEC_NODE_RECOMPILATION_INFO_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/inputKey.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/schemaConfigKey.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfInput;

/// Stores required information to recompile inputs of an arbitrary node.
class Exec_NodeRecompilationInfo
{
public:
    Exec_NodeRecompilationInfo(
        const EsfObject &provider,
        const EsfSchemaConfigKey dispatchingSchemaId,
        Exec_InputKeyVectorConstRefPtr &&inputKeys)
        : _provider(provider)
        , _dispatchingSchemaId(dispatchingSchemaId)
        , _inputKeys(std::move(inputKeys))
    {}

    /// Gets the provider of the node, which serves as the input resolution
    /// origin.
    ///
    const EsfObject &GetProvider() const {
        return _provider;
    }

    /// Gets the schema config key from the output key that was used to
    /// initially compile the node.
    ///
    /// The returned schema config key is the config key of the provider (if
    /// this node is for a non-dispatched computation), or the config key of the
    /// dispatcher (if this node is for a dispatched computation).
    ///
    EsfSchemaConfigKey GetDispatchingSchemaKey() const {
        return _dispatchingSchemaId;
    }

    /// Gets the input keys to re-resolve \p input on the node.
    ///
    /// Returns a vector of pointers to each input key with the same name and
    /// type as \p input. If there are no matching input keys, then the returned
    /// vector is empty and an error is raised.
    ///
    TfSmallVector<const Exec_InputKey *, 1> GetInputKeys(
        const VdfInput &input) const;

private:
    // The node's provider.
    // TODO: This needs to be updated in response to namespace edits.
    const EsfObject _provider;
    
    const EsfSchemaConfigKey _dispatchingSchemaId;

    Exec_InputKeyVectorConstRefPtr _inputKeys;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
