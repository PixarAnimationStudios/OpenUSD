//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_METAL_H
#define PXR_IMAGING_HGIPRESENT_METAL_H

#include "pxr/pxr.h"

#include "pxr/base/tf/smallVector.h"

#include "pxr/imaging/hgiPresent/presentImpl.h"


PXR_NAMESPACE_OPEN_SCOPE


class HgiMetal;

/// \class HgiPresentWindowMetal
///
/// Present to a Metal Window using CAMetalLayer.
///
class HgiPresentWindowMetal final: public HgiPresentImpl
{
public:
    explicit HgiPresentWindowMetal(HgiMetal* hgi,
        HgiPresentWindowParams const &params);

    ~HgiPresentWindowMetal() override;

    bool IsFormatSupported(HgiFormat colorFormat) const override;

    bool IsValid() const override;

    void Present(
        HgiTextureHandle const &hgiSrcTexture,
        HgiTextureHandle const &srcDepth) override;

private:
    HgiMetal* _hgiMetal{};
    HgiPresentWindowParams _params{};

    const CAMetalLayer* _metalLayer = nullptr;
    std::unique_ptr<struct MetalResources> _resources;
};

/// The HgiMetal definition requires Objective-C++,
/// this function can be used to dynamic_cast<HgiMetal*>
/// from regular C++.
HgiMetal*
DynamicCastHgiMetal(Hgi* hgi);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
