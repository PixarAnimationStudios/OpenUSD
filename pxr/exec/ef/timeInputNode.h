//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_TIME_INPUT_NODE_H
#define PXR_EXEC_EF_TIME_INPUT_NODE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"

#include "pxr/exec/vdf/rootNode.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfTimeInputNode
///
/// \brief A root node which supplies a value for time.
///
class EF_API_TYPE EfTimeInputNode final : public VdfRootNode
{
public:

    /// Constructor
    ///
    EF_API
    EfTimeInputNode(VdfNetwork *network);

    /// Returns \c true if the given node is an EfTimeInputNode. This method is
    /// an accelerated alternative to IsA<EfTimeInputNode>() or dynamic_cast.
    ///
    static bool IsATimeInputNode(const VdfNode &node) {
        return node.GetNumInputs() == 0 && node.IsA<EfTimeInputNode>();
    }

private:

    // Only a network is allowed to delete nodes.
    virtual ~EfTimeInputNode();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
