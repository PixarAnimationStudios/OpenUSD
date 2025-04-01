//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent/glInterop.h"

#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiPresentGLInterop::HgiPresentGLInterop(Hgi* hgi,
    HgiPresentGLInteropHandle const &handle,
    HgiPresentCompositionParams const &composition)
    : HgiPresentImpl(hgi)
    , _composition(composition)
{
    _fboName = handle.fboName;
}

bool
HgiPresentGLInterop::IsFormatSupported(HgiFormat colorFormat) const
{
    // Integer formats are not supported (this requires the GL interop to
    // support additional sampler types), nor are compressed formats.
    return HgiIsFloatFormat(colorFormat) && !HgiIsCompressed(colorFormat);
}

bool
HgiPresentGLInterop::IsValid() const
{
    // HgiInterop doesn't have a way to check, so assume always valid.
    return true;
}

void
HgiPresentGLInterop::Present(
        HgiTextureHandle const &srcColor,
        HgiTextureHandle const &srcDepth)
{
    _interop.TransferToApp(_hgi,
        srcColor,
        srcDepth,
        _composition,
        _fboName);
}


PXR_NAMESPACE_CLOSE_SCOPE
