//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/inputVector.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/rawValueAccessor.h"

PXR_NAMESPACE_OPEN_SCOPE

Vdf_InputVectorBase::Vdf_InputVectorBase(
    VdfNetwork *network,
    const VdfOutputSpecs &outputSpecs,
    VdfVector &&values)
    : VdfNode(network, VdfInputSpecs(), outputSpecs)
    , _values(std::move(values))
{}

Vdf_InputVectorBase::~Vdf_InputVectorBase() = default;

void
Vdf_InputVectorBase::Compute(const VdfContext &context) const
{
    VdfRawValueAccessor(context).SetOutputVector(
        *GetOutput(), VdfMask::AllOnes(_values.GetSize()), _values);
}

size_t
Vdf_InputVectorBase::GetMemoryUsage() const
{
    return _GetMemoryUsage<VdfNode>(
        *this, _values.EstimateElementMemory() * _values.GetSize());
}


VdfEmptyInputVector::VdfEmptyInputVector(
    VdfNetwork *network,
    const TfType &type) :
    Vdf_InputVectorBase(
        network,
        VdfOutputSpecs()
            .Connector(type, VdfTokens->out),
        VdfExecutionTypeRegistry::CreateEmptyVector(type))
{
}

VdfEmptyInputVector::~VdfEmptyInputVector() = default;

bool
VdfEmptyInputVector::_IsDerivedEqual(const VdfNode &rhs) const
{
    // If we got here, VdfNode::IsEqual already determined that the input and
    // output specs are equal, and therefore their types are equal.  So if rhs
    // is an empty input vector, we know it's an empty input vector of the
    // same type as this one and therfore they are equal.
    return rhs.IsA<VdfEmptyInputVector>();
}

PXR_NAMESPACE_CLOSE_SCOPE
