//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/extensibleNode.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfExtensibleNode::VdfExtensibleNode(
    VdfNetwork *network,
    const VdfInputSpecs &inputSpecs,
    const VdfOutputSpecs &outputSpecs) 
    : VdfNode(network)
    , _inputAndOutputSpecs(inputSpecs, outputSpecs)
{
    _InitializeInputAndOutputSpecs(&_inputAndOutputSpecs);
}

/* virtual */
VdfExtensibleNode::~VdfExtensibleNode()
{
    // We have to clear out the pointer here so that the destructor of our base
    // class doesn't try to clean it up itself.  This is an okay compromise for
    // the functionality we're trying to achieve without having to overhaul the
    // class hierarchy around VdfNode.
    _ClearInputAndOutputSpecsPointer();
}

void
VdfExtensibleNode::AddOutputSpecs(
    const VdfOutputSpecs &specs,
    std::vector<VdfOutput*> *resultOutputs)
{
    TRACE_FUNCTION();

    _inputAndOutputSpecs.AppendOutputSpecs(specs);

    // Build and store ouputs from the specs.
    _AppendOutputs(specs, resultOutputs);
}

void
VdfExtensibleNode::AddInputSpecs(
    const VdfInputSpecs &newSpecs,
    std::vector<VdfInput*> *resultInputs)
{
    TRACE_FUNCTION();

    _inputAndOutputSpecs.AppendInputSpecs(newSpecs);

    // Build and store inputs from the specs.
    _AppendInputs(newSpecs, resultInputs);
}

const VdfInputAndOutputSpecs *
VdfExtensibleNode::_AcquireInputAndOutputSpecsPointer(
    const VdfInputSpecs &inputSpecs,
    const VdfOutputSpecs &outputSpecs)
{
    _inputAndOutputSpecs = VdfInputAndOutputSpecs(inputSpecs, outputSpecs);
    return &_inputAndOutputSpecs;
}

void 
VdfExtensibleNode::_ReleaseInputAndOutputSpecsPointer(
    const VdfInputAndOutputSpecs *specs)
{
    // No-op here, just make sure this is our pointer
    TF_VERIFY(specs == &_inputAndOutputSpecs);
}

PXR_NAMESPACE_CLOSE_SCOPE
