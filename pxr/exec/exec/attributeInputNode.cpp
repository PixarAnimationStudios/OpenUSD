//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/attributeInputNode.h"

#include "pxr/exec/exec/typeRegistry.h"

#include "pxr/exec/ef/time.h"
#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/rawValueAccessor.h"
#include "pxr/exec/vdf/tokens.h"

#include "pxr/base/vt/value.h"
#include "pxr/usd/usd/timeCode.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
    Exec_AttributeInputNodeTokens, EXEC_ATTRIBUTE_INPUT_NODE_TOKENS);

Exec_AttributeInputNode::Exec_AttributeInputNode(
    VdfNetwork *const network,
    EsfAttributeQuery &&attributeQuery,
    TfType valueType)
    : VdfNode(
        network,
        VdfInputSpecs().ReadConnector(
            TfType::Find<EfTime>(),
            Exec_AttributeInputNodeTokens->time),
        VdfOutputSpecs().Connector(
            valueType,
            TfToken(VdfTokens->out)))
    , _attributeQuery(std::move(attributeQuery))
    , _isTimeDependent(false)
{
    UpdateTimeDependence();
}

Exec_AttributeInputNode::~Exec_AttributeInputNode() = default;

void
Exec_AttributeInputNode::UpdateValueResolutionState()
{
    _attributeQuery->Initialize();
    TF_VERIFY(_attributeQuery->IsValid());
}

bool
Exec_AttributeInputNode::UpdateTimeDependence()
{
    const bool wasTimeDependent = _isTimeDependent;
    _isTimeDependent = _attributeQuery->ValueMightBeTimeVarying();
    return wasTimeDependent != _isTimeDependent;
}

bool
Exec_AttributeInputNode::IsTimeVarying(
    const EfTime &from, const EfTime &to) const
{
    return _attributeQuery->IsTimeVarying(from.GetTimeCode(), to.GetTimeCode());
}

void
Exec_AttributeInputNode::Compute(VdfContext const &context) const
{
    const UsdTimeCode time = context.GetInputValue<EfTime>(
        Exec_AttributeInputNodeTokens->time).GetTimeCode();

    if (VtValue resolvedValue; _attributeQuery->Get(&resolvedValue, time)) {
        VdfRawValueAccessor(context).SetOutputVector(
            *GetOutput(VdfTokens->out),
            VdfMask::AllOnes(1),
            ExecTypeRegistry::GetInstance().CreateVector(resolvedValue));
    }
}

VdfMask::Bits
Exec_AttributeInputNode::_ComputeInputDependencyMask(
    const VdfMaskedOutput &maskedOutput,
    const VdfConnection &inputConnection) const
{
    // This node has one output, and it depends on the time input, which is the
    // only input on the node, so the logic here is a straight forward
    // one-to-one dependency.
    // 
    // Note, that we do not check whether the node is time-dependent when
    // traversing in the input direction, and always report the time dependency
    // as encoded in the network. This is to ensure that cached traversals in
    // the input direction - primarily the schedules - do not go invalid when
    // time dependencies on input nodes change. The trade-off then is that the
    // time node is typically included in schedules, but this comes at very
    // little cost.
    
    return inputConnection.GetMask().GetBits();
}

VdfMask 
Exec_AttributeInputNode::_ComputeOutputDependencyMask(
    const VdfConnection &inputConnection,
    const VdfMask &inputDependencyMask,
    const VdfOutput &output) const
{
    // There is only one input, and one output on this node, so we do not need
    // to look at output names for dependency computation: We can assume that
    // the dependency being computed is always from the 'value' output to the
    // 'time' input.
    
    // If the node is potentially time-varying, there is a dependency.
    // Otherwise, there is not.
    return IsTimeDependent() ? VdfMask::AllOnes(1) : VdfMask();
}

PXR_NAMESPACE_CLOSE_SCOPE
