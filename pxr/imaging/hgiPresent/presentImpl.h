//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_HGIPRESENTIMPL_H
#define PXR_IMAGING_HGIPRESENT_HGIPRESENTIMPL_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiPresent/present.h"

PXR_NAMESPACE_OPEN_SCOPE


class Hgi;
class HgiTexture;

class HgiPresentImpl
{
public:
    explicit HgiPresentImpl(Hgi* hgi)
        : _hgi(hgi)
    {}

    virtual ~HgiPresentImpl() = default;

    virtual bool IsFormatSupported(HgiFormat colorFormat) const = 0;

    virtual bool IsValid() const = 0;

    virtual void Present(
        HgiTextureHandle const &srcColor,
        HgiTextureHandle const &srcDepth) = 0;

protected:
    Hgi* _hgi;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
