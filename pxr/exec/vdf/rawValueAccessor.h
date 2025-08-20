//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_RAW_VALUE_ACCESSOR_H
#define PXR_EXEC_VDF_RAW_VALUE_ACCESSOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfContext;
class VdfInput;
class VdfMask;
class VdfOutput;
class VdfVector;

////////////////////////////////////////////////////////////////////////////////
///
/// This class grants special access to the raw VdfVector data associated with
/// inputs and outputs on a VdfNode.
///
/// Note, that input and output data is typically accessed using the VdfContext.
/// In some special cases, nodes may require type-agnostic access to the entire
/// VdfVector stored for a specific input or output.
///
/// We do not want this class to be used in typical plugin code (mover, gprims,
/// etc.), thus inputs and outputs are referred to using VdfInput and VdfOutput
/// instances, rather than name tokens. Only classes derived from VdfNode will
/// typically have access to these objects.
///
class VdfRawValueAccessor : public VdfIterator
{
public:

    /// Constructs a VdfRawValueAccessor from a VdfContext.
    ///
    VdfRawValueAccessor(const VdfContext &context) : _context(context) {}

    /// Returns the first VdfVector at the input for \p input. Returns nullptr
    /// if there is no connection on the supplied input or if the requested
    /// input does not exist.
    ///
    /// Note, this method purposefully accepts a VdfInput instead of a TfToken,
    /// such that it can only be used where VdfInputs are available.
    ///
    VDF_API
    const VdfVector *GetInputVector(
        const VdfInput &input,
        VdfMask *mask = nullptr) const;

    /// Sets the \p output value to the given \p value using the passed
    /// in \p mask.
    ///
    /// Note, this method purposefully accepts a VdfOutput instead of a TfToken,
    /// such that it can only be used where VdfOutputs are available.
    ///
    VDF_API
    void SetOutputVector(
        const VdfOutput &output,
        const VdfMask &mask,
        const VdfVector &value);

    /// \overload
    ///
    VDF_API
    void SetOutputVector(
        const VdfOutput &output,
        const VdfMask &mask,
        VdfVector &&value);

private:

    // Common implementation of SetOutputVector overloads.
    template <typename Vector>
    void _SetOutputVector(
        const VdfOutput &output,
        const VdfMask &mask,
        Vector &&value);

private:

    // The context used to access the input/output data.
    const VdfContext &_context;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
