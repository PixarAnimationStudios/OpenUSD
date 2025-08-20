//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPUTATION_DEFINITION_H
#define PXR_EXEC_EXEC_COMPUTATION_DEFINITION_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/inputKey.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/tf/token.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class EsfJournal;
class EsfObjectInterface;
class Exec_Program;
class VdfNode;

/// A base class that defines an exec computation.
///
/// For a given computation provider object, a computation definition can
/// provide the result type, generate input keys, and compile a VdfNode.
///
class Exec_ComputationDefinition
{
public:
    Exec_ComputationDefinition(
        const Exec_ComputationDefinition &) = delete;
    Exec_ComputationDefinition &operator=(
        const Exec_ComputationDefinition &) = delete;

    virtual ~Exec_ComputationDefinition();

    /// Returns the name of the computation.
    const TfToken &GetComputationName() const {
        return _computationName;
    }

    /// Returns the type of values that are produced by this computation.
    ///
    /// \note
    /// \p metadataKey is only used for the computeMetadata builtin computation.
    ///
    virtual TfType GetResultType(
        const EsfObjectInterface &providerObject,
        const TfToken &metadataKey,
        EsfJournal *journal) const;

    /// Returns the type of values returned to external clients of execution
    /// that request this computation.
    ///
    virtual TfType GetExtractionType(
        const EsfObjectInterface &providerObject) const;

    /// Returns `true` if this computation is a dispatched computation.
    virtual bool IsDispatched() const;

    /// Returns the keys that indicate how to source the input values required
    /// to evaluate the computation when the provider is \p providerObject.
    ///
    /// Keys are returned by a reference-counted pointer. The keys may be shared
    /// by the definition, or they may be created specifically for the
    /// given \p providerObject. If the definition has no inputs, this returns
    /// a valid pointer to an empty vector. It never returns a null pointer.
    ///
    /// Any scene access needed to determine the input keys is recorded in
    /// \p journal.
    ///
    virtual Exec_InputKeyVectorConstRefPtr GetInputKeys(
        const EsfObjectInterface &providerObject,
        EsfJournal *journal) const = 0;

    /// Compiles the node that implements the computation when the provider is
    /// \p providerObject, adding it to the network owned by \p program.
    ///
    /// The information in \p nodeJournal will be used to determine when the
    /// node must be uncompiled.
    ///
    /// \note
    /// \p metadataKey is only used for the computeMetadata builtin computation.
    ///
    virtual VdfNode *CompileNode(
        const EsfObjectInterface &providerObject,
        const TfToken &metadataKey,
        EsfJournal *nodeJournal,
        Exec_Program *program) const = 0;

protected:
    /// Creates a definition for a computation.
    Exec_ComputationDefinition(
        TfType resultType,
        const TfToken &computationName);

private:
    const TfType _resultType;
    const TfToken _computationName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
