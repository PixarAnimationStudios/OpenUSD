//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/texture.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

HgiTexture::HgiTexture(HgiTextureDesc const& desc)
    : _descriptor(desc)
{
}

HgiTexture::~HgiTexture() = default;

HgiTextureDesc const&
HgiTexture::GetDescriptor() const
{
    return _descriptor;
}

size_t
HgiTexture::_GetByteSizeOfResource(const HgiTextureDesc &descriptor)
{
    // Compute all mip levels down to 1x1(x1)
    const std::vector<HgiMipInfo> mipInfos = HgiGetMipInfos(
        descriptor.format, descriptor.dimensions, descriptor.layerCount);

    // Number of mip levels actually used.
    const size_t mipLevels = std::min(
        mipInfos.size(), size_t(descriptor.mipLevels));

    // Get the last mip level actually used.
    const HgiMipInfo &mipInfo = mipInfos[mipLevels - 1];

    // mipInfo.byteOffset is the sum of all mip levels prior
    // to the last mip level actually used.
    return
        mipInfo.byteOffset + descriptor.layerCount * mipInfo.byteSizePerLayer;

}

bool operator==(const HgiComponentMapping& lhs,
    const HgiComponentMapping& rhs)
{
    return lhs.r == rhs.r &&
           lhs.g == rhs.g &&
           lhs.b == rhs.b &&
           lhs.a == rhs.a;
}

bool operator!=(const HgiComponentMapping& lhs,
    const HgiComponentMapping& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const HgiTextureDesc& lhs,
    const HgiTextureDesc& rhs)
{
    return  lhs.debugName == rhs.debugName &&
            lhs.usage == rhs.usage &&
            lhs.format == rhs.format &&
            lhs.componentMapping == rhs.componentMapping &&
            lhs.type == rhs.type &&
            lhs.dimensions == rhs.dimensions &&
            lhs.sampleCount == rhs.sampleCount &&
            lhs.pixelsByteSize == rhs.pixelsByteSize
            // Omitted because data ptr is set to nullptr after CreateTexture
            // lhs.initialData == rhs.initialData
    ;
}

bool operator!=(const HgiTextureDesc& lhs,
    const HgiTextureDesc& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const HgiTextureViewDesc& lhs,
    const HgiTextureViewDesc& rhs)
{
    return  lhs.debugName == rhs.debugName &&
            lhs.format == rhs.format &&
            lhs.layerCount == rhs.layerCount &&
            lhs.mipLevels == rhs.mipLevels &&
            lhs.sourceTexture == rhs.sourceTexture &&
            lhs.sourceFirstLayer == rhs.sourceFirstLayer &&
            lhs.sourceFirstMip == rhs.sourceFirstMip
    ;
}

bool operator!=(const HgiTextureViewDesc& lhs,
    const HgiTextureViewDesc& rhs)
{
    return !(lhs == rhs);
}

HgiTextureView::HgiTextureView(HgiTextureViewDesc const& desc)
{
}

HgiTextureView::~HgiTextureView() = default;

void
HgiTextureView::SetViewTexture(HgiTextureHandle const& handle)
{
    _viewTexture = handle;
}

HgiTextureHandle const&
HgiTextureView::GetViewTexture() const
{
    return _viewTexture;
}

PXR_NAMESPACE_CLOSE_SCOPE
