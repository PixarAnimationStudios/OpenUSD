//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_ROOT_NODE_H
#define PXR_EXEC_VDF_ROOT_NODE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/node.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class VdfRootNode
///
/// Base class for root nodes.
///
/// A root node is a node that does not have any inputs and Compute() method.
/// Instead it has output(s) that are manually initialized.  As such, the
/// outputs can never be passed.  Downstream nodes might see no data (ie.
/// VdfContext::HasInputValue() will return false), if outputs have not been 
/// initialized manually. 
///
class VDF_API_TYPE VdfRootNode : public VdfNode
{
public:

    /// Returns \c true if the given node is a VdfRootNode. This method is
    /// an accelerated alternative to IsA<VdfRootNode>() or dynamic_cast.
    ///
    static bool IsARootNode(const VdfNode &node) {
        return node.GetNumInputs() == 0 && node.IsA<VdfRootNode>();
    }

protected:

    /// Note that VdfRootNodes don't have inputs.
    ///
    VdfRootNode(
        VdfNetwork *network,
        const VdfOutputSpecs &outputSpecs)
    :   VdfNode(network, VdfInputSpecs(), outputSpecs) {}

    VDF_API
    ~VdfRootNode() override;

private:

    // VdfRootNodes can't be computed.  Therefore this class overrides
    // the Compute() method finally which only produces an error message.
    VDF_API
    void Compute(const VdfContext &context) const override final;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
