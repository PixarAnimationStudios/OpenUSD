//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_NOOP_H
#define PXR_IMAGING_HGIPRESENT_NOOP_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiPresent/presentImpl.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiPresentNoOp
///
/// Does nothing. Provides a fallback when no presentation
/// is configured.
///
class HgiPresentNoOp final: public HgiPresentImpl
{
public:
    explicit HgiPresentNoOp(Hgi* hgi,
        HgiPresentNoOpParams const &params);

    ~HgiPresentNoOp() override = default;

    bool IsFormatSupported(HgiFormat colorFormat) const override;

    bool IsValid() const override;

    void Present(
        HgiTextureHandle const &srcColor,
        HgiTextureHandle const &srcDepth) override;

private:
    bool _valid;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
