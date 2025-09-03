//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/leafNode.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/vdf/connectorSpecs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<EfLeafNode>();
}

TF_DEFINE_PUBLIC_TOKENS(EfLeafTokens, EF_LEAF_TOKENS);

EfLeafNode::EfLeafNode(
    VdfNetwork *network,
    TfType inputType)
    : VdfNode(
        network,
        VdfInputSpecs()
            .ReadConnector(inputType, EfLeafTokens->in),
        VdfOutputSpecs())
{
}

EfLeafNode::~EfLeafNode() = default;

// Because of the way leaf nodes are used, this method should never be called.
void
EfLeafNode::Compute(const VdfContext &context) const
{
    TF_FATAL_ERROR("This method shouldn't be called.");
}

PXR_NAMESPACE_CLOSE_SCOPE
