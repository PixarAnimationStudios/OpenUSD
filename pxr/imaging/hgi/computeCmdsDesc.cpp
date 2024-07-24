//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/computeCmdsDesc.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiComputeCmdsDesc::HgiComputeCmdsDesc()
    : dispatchMethod(HgiComputeDispatchSerial)
{
}

bool operator==(
    const HgiComputeCmdsDesc& lhs,
    const HgiComputeCmdsDesc& rhs)
{
    return  lhs.dispatchMethod == rhs.dispatchMethod;
}

bool operator!=(
    const HgiComputeCmdsDesc& lhs,
    const HgiComputeCmdsDesc& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
