//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_COMPUTE_CMDS_DESC_H
#define PXR_IMAGING_HGI_COMPUTE_CMDS_DESC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiComputeCmdsDesc
///
/// Describes the properties to construct a HgiComputeCmds.
///
/// <ul>
/// <li>dispatchMethod:
///   The dispatch method for compute encoders.</li>
/// </ul>
///
struct HgiComputeCmdsDesc
{
    HGI_API
    HgiComputeCmdsDesc();

    HgiComputeDispatch dispatchMethod;
};

HGI_API
bool operator==(
    const HgiComputeCmdsDesc& lhs,
    const HgiComputeCmdsDesc& rhs);

HGI_API
bool operator!=(
    const HgiComputeCmdsDesc& lhs,
    const HgiComputeCmdsDesc& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
