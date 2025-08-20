//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/timeInputNode.h"

#include "pxr/exec/ef/time.h"

PXR_NAMESPACE_OPEN_SCOPE

EfTimeInputNode::EfTimeInputNode(VdfNetwork *network)
:   VdfRootNode(
        network,
        VdfOutputSpecs().Connector<EfTime>(VdfTokens->out))
{
}

EfTimeInputNode::~EfTimeInputNode() = default;

PXR_NAMESPACE_CLOSE_SCOPE
