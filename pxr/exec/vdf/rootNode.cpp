//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/rootNode.h"

PXR_NAMESPACE_OPEN_SCOPE

/* virtual */
VdfRootNode::~VdfRootNode()
{
}

/* virtual */
void VdfRootNode::Compute(const VdfContext &context) const
{
    TF_CODING_ERROR(
        "The Compute() method can't be called on VdfRootNode '%s'.",
        GetDebugName().c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE
