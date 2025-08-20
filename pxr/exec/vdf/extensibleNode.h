//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXTENSIBLE_NODE_H
#define PXR_EXEC_VDF_EXTENSIBLE_NODE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/inputAndOutputSpecs.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfInput;
class VdfOutput;
class VdfNetwork;

/// \class VdfExtensibleNode
///
/// Base class for nodes that support dynamic creation of input and
/// output connectors.
///
class VDF_API_TYPE VdfExtensibleNode : public VdfNode
{
public:

    VDF_API
    VdfExtensibleNode(
        VdfNetwork *network,
        const VdfInputSpecs &inputSpecs,
        const VdfOutputSpecs &outputSpecs);

    /// Appends the supplied specs to this node, and creates and stores
    /// additional outputs from them.  A vector of VdfOutput* of the same
    /// size as \p specs is the result.
    VDF_API
    void AddOutputSpecs(
        const VdfOutputSpecs &specs,
        std::vector<VdfOutput*> *resultOutputs);

    /// Appends the supplied specs to this node, and creates and stores
    /// additional inputs from them. A vector of VdfInput* of the same
    /// size as \p specs is the result.
    VDF_API
    void AddInputSpecs(
        const VdfInputSpecs &specs,
        std::vector<VdfInput*> *resultInputs);

protected:

    /// Gets an input/output specs pointer that the node can use.  In the
    /// case of VdfExtensibleNodes, we manage the storage for the input/output
    /// specs as a member of node itself. 
    /// 
    VDF_API
    virtual const VdfInputAndOutputSpecs *_AcquireInputAndOutputSpecsPointer(
        const VdfInputSpecs &inputSpecs,
        const VdfOutputSpecs &outputSpecs);

    /// Releases an input/output specs pointer that was acquired with a
    /// previous call to _AcquireInputAndOutputSpecsPointer().  This is a 
    /// no-op for VdfExtensibleNode.
    ///
    VDF_API
    virtual void _ReleaseInputAndOutputSpecsPointer(
        const VdfInputAndOutputSpecs *specs);

    VDF_API
    virtual ~VdfExtensibleNode();

private:

    // Our own local input and output specs so that we can append to them
    // very quickly.
    VdfInputAndOutputSpecs _inputAndOutputSpecs;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
