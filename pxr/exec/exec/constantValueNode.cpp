//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/constantValueNode.h"

#include "pxr/exec/exec/typeRegistry.h"

#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/rawValueAccessor.h"
#include "pxr/exec/vdf/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_ConstantValueNode::Exec_ConstantValueNode(
    VdfNetwork *const network,
    const VtValue &value)
    : VdfNode(
        network,
        VdfInputSpecs(),
        VdfOutputSpecs().Connector(
            value.GetType(),
            TfToken(VdfTokens->out)))
    , _value(value)
{
}

Exec_ConstantValueNode::~Exec_ConstantValueNode() = default;

void
Exec_ConstantValueNode::Compute(const VdfContext &context) const
{
    VdfRawValueAccessor(context).SetOutputVector(
        *GetOutput(VdfTokens->out),
        VdfMask::AllOnes(1),
        ExecTypeRegistry::GetInstance().CreateVector(_value));
}

PXR_NAMESPACE_CLOSE_SCOPE
