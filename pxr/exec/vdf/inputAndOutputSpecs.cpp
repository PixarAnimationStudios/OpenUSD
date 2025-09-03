//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/inputAndOutputSpecs.h"

#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfInputAndOutputSpecs::VdfInputAndOutputSpecs(
    const VdfInputSpecs &inputSpecs,
    const VdfOutputSpecs &outputSpecs) :
    _inputSpecs(inputSpecs),
    _outputSpecs(outputSpecs)
{}

VdfInputAndOutputSpecs::VdfInputAndOutputSpecs(VdfInputAndOutputSpecs && rhs) :
    _inputSpecs(std::move(rhs._inputSpecs)),
    _outputSpecs(std::move(rhs._outputSpecs))
{}

VdfInputAndOutputSpecs &
VdfInputAndOutputSpecs::operator=(VdfInputAndOutputSpecs && rhs)
{
    _inputSpecs = std::move(rhs._inputSpecs);
    _outputSpecs = std::move(rhs._outputSpecs);
    return *this;
}

bool
VdfInputAndOutputSpecs::operator==(const VdfInputAndOutputSpecs &rhs) const
{
    // Early bail out.
    if (this == &rhs) {
        return true;
    }

    return _inputSpecs == rhs._inputSpecs && _outputSpecs == rhs._outputSpecs;
}

size_t
VdfInputAndOutputSpecs::GetHash() const
{
    // We only hash up to 'num' elements in the front and back of the output
    // and input specs.  This is because they may become very large and all we
    // need is just a hash.
    
    constexpr size_t num = 3;

    // Look at the inputSpecs.
    const VdfInputSpecs &inputSpecs = GetInputSpecs();
    const size_t sizeInputs = inputSpecs.GetSize();
    size_t hash = TfHash()(sizeInputs);

    const size_t numInputsFront = std::min(sizeInputs, num);
    for (size_t i = 0; i < numInputsFront; ++i) {
        hash = TfHash::Combine(hash, inputSpecs.GetInputSpec(i)->GetHash());
    }
        
    if (sizeInputs > num) {
        const size_t numInputsBack = std::min(sizeInputs - num, num);
        for (size_t i = sizeInputs - numInputsBack; i < sizeInputs; ++i) {
            hash = TfHash::Combine(
                hash, inputSpecs.GetInputSpec(i)->GetHash());
        }
    }

    // Also let the outputSpecs contribute.
    const VdfOutputSpecs &outputSpecs = GetOutputSpecs();
    const size_t sizeOutputs = outputSpecs.GetSize();
    hash = TfHash::Combine(hash, sizeOutputs);

    const size_t numOutputsFront = std::min(sizeOutputs, num);
    for (size_t i = 0; i < numOutputsFront; ++i) {
        hash = TfHash::Combine(hash, outputSpecs.GetOutputSpec(i)->GetHash());
    }
        
    if (sizeOutputs > num) {
        const size_t numOutputsBack = std::min(sizeOutputs - num, num);
        for (size_t i = sizeOutputs - numOutputsBack; i < sizeOutputs; ++i) {
            hash = TfHash::Combine(
                hash, outputSpecs.GetOutputSpec(i)->GetHash());
        }
    }

    return hash;
}

PXR_NAMESPACE_CLOSE_SCOPE
