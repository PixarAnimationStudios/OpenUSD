//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_CONNECTION_H
#define PXR_EXEC_VDF_CONNECTION_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// A class that fully represents a connection between two VdfNodes.
///
class VdfConnection
{
public:

    VdfConnection(VdfOutput &output, const VdfMask &mask, VdfInput &input) :
        _output(&output),
        _mask(mask),
        _input(&input) {}

    /// Returns the source (ie. output) node for this connection.
    const VdfNode &GetSourceNode() const {
        TF_DEV_AXIOM(_output);
        return _output->GetNode();
    }

    /// Returns the source (ie. output) node for this connection.
    VdfNode &GetSourceNode() {
        TF_DEV_AXIOM(_output);
        return _output->GetNode();
    }

    /// Returns the target (ie. input) node for this connection.
    const VdfNode &GetTargetNode() const {
        TF_DEV_AXIOM(_input);
        return _input->GetNode();
    }

    /// Returns the target (ie. input) node for this connection.
    VdfNode &GetTargetNode() {
        TF_DEV_AXIOM(_input);
        return _input->GetNode();
    }

    /// Returns the output (ie. source) for this connection.
    const VdfOutput &GetSourceOutput() const {
        TF_DEV_AXIOM(_output);
        return *_output;
    }

    /// Returns the output (ie. source) for this connection.
    VdfOutput &GetSourceOutput() {
        TF_DEV_AXIOM(_output);
        return *_output;
    }

    /// Returns the output (ie. source) for this connection.
    //XXX: Remove this method and provide a VdfMaskedConstOutput
    VdfOutput &GetNonConstSourceOutput() const {
        TF_DEV_AXIOM(_output);
        return *_output;
    }

    /// Return the masked output (ie. source) for this connection.
    VdfMaskedOutput GetSourceMaskedOutput() const {
        return VdfMaskedOutput(&GetNonConstSourceOutput(), GetMask());
    }

    /// Returns the input connector (ie. target) for this connection.
    const VdfInput &GetTargetInput() const {
        TF_DEV_AXIOM(_input);
        return *_input;
    }

    /// Returns the input connector (ie. target) for this connection.
    VdfInput &GetTargetInput() {
        TF_DEV_AXIOM(_input);
        return *_input;
    }

    /// Returns the mask for this connection.
    const VdfMask &GetMask() const { return _mask; }

    /// Returns a debug string for this connection.
    VDF_API
    std::string GetDebugName() const;

private:

    // The output to which we are connected.
    VdfOutput *_output;

    // The mask for this connection.
    VdfMask _mask;

    // The target input-connector on the target node.
    VdfInput *_input;
};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
