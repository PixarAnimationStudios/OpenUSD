//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_INPUT_AND_OUTPUT_SPECS_H
#define PXR_EXEC_VDF_INPUT_AND_OUTPUT_SPECS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/connectorSpecs.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class VdfInputAndOutputSpecs
///
/// \brief Hashable holder of a VdfInputSpec and VdfOutputSpec
///
class VdfInputAndOutputSpecs
{
public:
    /// Constructs a VdfInputAndOutputSpec
    ///
    VDF_API
    VdfInputAndOutputSpecs(
        const VdfInputSpecs &inputSpecs,
        const VdfOutputSpecs &outputSpecs);

    VdfInputAndOutputSpecs(const VdfInputAndOutputSpecs &) = delete;
    VdfInputAndOutputSpecs &operator=(const VdfInputAndOutputSpecs &) = delete;

    VDF_API
    VdfInputAndOutputSpecs(VdfInputAndOutputSpecs &&);

    VDF_API
    VdfInputAndOutputSpecs &operator=(VdfInputAndOutputSpecs &&);

    /// Returns the specs of the input connectors
    ///
    const VdfInputSpecs& GetInputSpecs() const { return _inputSpecs; }

    /// Returns the specs of the output connectors
    ///
    const VdfOutputSpecs& GetOutputSpecs() const { return _outputSpecs; }

    /// Appends \p inputSpecs.
    ///
    void AppendInputSpecs(const VdfInputSpecs &inputSpecs) {
        _inputSpecs.Append(inputSpecs);
    }
    
    /// Appends \p outputSpecs.
    ///
    void AppendOutputSpecs(const VdfOutputSpecs &outputSpecs) {
        _outputSpecs.Append(outputSpecs);
    }

    /// Returns true if this == \p rhs.
    ///
    VDF_API
    bool operator==(const VdfInputAndOutputSpecs &rhs) const;

    /// Returns true if this != \p rhs.
    ///
    bool operator!=(const VdfInputAndOutputSpecs &rhs) const {
        return !(*this == rhs); 
    }

    /// Computes the hash value for this instance.
    ///
    VDF_API
    size_t GetHash() const;

private:

    // This holds on to the specs of the input connectors.
    VdfInputSpecs _inputSpecs;

    // This holds on to the specs of the output connectors.
    VdfOutputSpecs _outputSpecs;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
