//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/shaderFunction.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiShaderFunctionDesc const&
HgiShaderFunction::GetDescriptor() const
{
    return _descriptor;
}

HgiShaderFunction::HgiShaderFunction(HgiShaderFunctionDesc const& desc)
    : _descriptor(desc)
{
}

HgiShaderFunction::~HgiShaderFunction() = default;

PXR_NAMESPACE_CLOSE_SCOPE
