//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent/noOp.h"

#include "pxr/imaging/hgi/hgi.h"


PXR_NAMESPACE_OPEN_SCOPE


HgiPresentNoOp::HgiPresentNoOp(Hgi* hgi,
    HgiPresentNoOpParams const &params)
    : HgiPresentImpl(hgi)
    , _valid(params.isValid)
{}

bool
HgiPresentNoOp::IsFormatSupported(HgiFormat) const
{
    return true;
}

bool
HgiPresentNoOp::IsValid() const
{
    return _valid;
}

void
HgiPresentNoOp::Present(
        HgiTextureHandle const &,
        HgiTextureHandle const &)
{
}


PXR_NAMESPACE_CLOSE_SCOPE
