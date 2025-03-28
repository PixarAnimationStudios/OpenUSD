//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <Metal/Metal.h>

#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/capabilities.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/texture.h"


PXR_NAMESPACE_OPEN_SCOPE

HgiMetalTexture::HgiMetalTexture(HgiMetal *hgi, HgiTextureDesc const & desc)
    : HgiTexture(desc)
    , _textureId(nil)
{
    MTLResourceOptions resourceOptions = MTLResourceStorageModePrivate;
    MTLTextureUsage usage = MTLTextureUsageShaderRead;

    MTLPixelFormat mtlFormat = HgiMetalConversions::GetPixelFormat(
        desc.format, desc.usage);

    if (desc.usage &
        (HgiTextureUsageBitsColorTarget | HgiTextureUsageBitsDepthTarget)) {
        usage |= MTLTextureUsageRenderTarget;
    }

    if (desc.usage & HgiTextureUsageBitsShaderWrite) {
        usage |= MTLTextureUsageShaderWrite;
    }

    const size_t width = desc.dimensions[0];
    const size_t height = desc.dimensions[1];
    const size_t depth = desc.dimensions[2];

    MTLTextureDescriptor* texDesc;

    texDesc =
        [MTLTextureDescriptor
         texture2DDescriptorWithPixelFormat:mtlFormat
                                      width:width
                                     height:height
                                  mipmapped:NO];
    
    texDesc.mipmapLevelCount = desc.mipLevels;

    texDesc.arrayLength = desc.layerCount;
    texDesc.resourceOptions = resourceOptions;
    texDesc.usage = usage;

#if (defined(__MAC_10_15) && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_15) \
    || __IPHONE_OS_VERSION_MAX_ALLOWED >= 130000
        if (@available(macOS 10.15, ios 13.0, *)) {
            texDesc.swizzle = MTLTextureSwizzleChannelsMake(
                HgiMetalConversions::GetComponentSwizzle(
                    desc.componentMapping.r),
                HgiMetalConversions::GetComponentSwizzle(
                    desc.componentMapping.g),
                HgiMetalConversions::GetComponentSwizzle(
                    desc.componentMapping.b),
                HgiMetalConversions::GetComponentSwizzle(
                    desc.componentMapping.a));
        }
#endif

    if (desc.type == HgiTextureType3D) {
        texDesc.depth = depth;
        texDesc.textureType = MTLTextureType3D;
    } else if (desc.type == HgiTextureType2DArray) {
        texDesc.textureType = MTLTextureType2DArray;
    } else if (desc.type == HgiTextureType1D) {
        texDesc.textureType = MTLTextureType1D;
    } else if (desc.type == HgiTextureType1DArray) {
        texDesc.textureType = MTLTextureType1DArray;
    }

    if (desc.sampleCount > 1) {
        texDesc.sampleCount = desc.sampleCount;
        texDesc.textureType = MTLTextureType2DMultisample;
    }

    _textureId = [hgi->GetPrimaryDevice() newTextureWithDescriptor:texDesc];

    if (desc.initialData && desc.pixelsByteSize > 0) {
        // Depth, stencil, depth-stencil, and multisample textures must be
        // allocated with MTLStorageModePrivate, which should not be used with
        // replaceRegion. We create a temporary, non-private texture to fill
        // with the initial data, then  blit the data from that texture
        // to our original, private texture.

        // Modify texture descriptor to describe the temp texture.
#if defined(ARCH_OS_OSX)
        texDesc.resourceOptions = MTLResourceStorageModeManaged;
#else
        texDesc.resourceOptions = MTLResourceStorageModeShared;
#endif
        texDesc.sampleCount = 1;
        if (desc.type == HgiTextureType3D) {
            texDesc.textureType = MTLTextureType3D;
        } else if (desc.type == HgiTextureType2DArray) {
            texDesc.textureType = MTLTextureType2DArray;
        } else if (desc.type == HgiTextureType1D) {
            texDesc.textureType = MTLTextureType1D;
        } else if (desc.type == HgiTextureType1DArray) {
            texDesc.textureType = MTLTextureType1DArray;        
        }
        texDesc.usage = MTLTextureUsageShaderRead;
        texDesc.pixelFormat = HgiMetalConversions::GetPixelFormat(
            desc.format, HgiTextureUsageBitsShaderRead);    

        // Create temp texture and fill with initial data.
        id<MTLTexture> tempTextureId =
            [hgi->GetPrimaryDevice() newTextureWithDescriptor:texDesc];

        size_t perPixelSize = HgiGetDataSizeOfFormat(desc.format);

        // Upload each (available) mip
        const std::vector<HgiMipInfo> mipInfos =
            HgiGetMipInfos(
                desc.format,
                desc.dimensions,
                desc.layerCount,
                desc.pixelsByteSize);
        const size_t mipLevels = std::min(
            mipInfos.size(), size_t(desc.mipLevels));
        const char * const initialData = reinterpret_cast<const char *>(
            desc.initialData);

        for (size_t mip = 0; mip < mipLevels; mip++) {
            const HgiMipInfo &mipInfo = mipInfos[mip];

            const size_t width = mipInfo.dimensions[0];
            const size_t height = mipInfo.dimensions[1];
            const size_t bytesPerRow = perPixelSize * width;

            if (desc.type == HgiTextureType1D) {
                [tempTextureId replaceRegion:MTLRegionMake1D(0, width)
                              mipmapLevel:mip
                                withBytes:initialData + mipInfo.byteOffset
                              bytesPerRow:bytesPerRow];
            } else if (desc.type == HgiTextureType2D) {
                [tempTextureId replaceRegion:MTLRegionMake2D(0, 0, width, height)
                              mipmapLevel:mip
                                withBytes:initialData + mipInfo.byteOffset
                              bytesPerRow:bytesPerRow];
            } else if (desc.type == HgiTextureType3D) {
                const size_t depth = mipInfo.dimensions[2];
                const size_t imageBytes = bytesPerRow * height;
                for (size_t d = 0; d < depth; d++) {
                    const size_t offset = d * imageBytes;
                    [tempTextureId
                        replaceRegion:MTLRegionMake3D(0, 0, d, width, height, 1)
                          mipmapLevel:mip
                                slice:0
                            withBytes:initialData + mipInfo.byteOffset + offset
                          bytesPerRow:bytesPerRow
                        bytesPerImage:0];
                }
            } else if (desc.type == HgiTextureType2DArray) {
                const size_t imageBytes = bytesPerRow * height;
                for (int slice = 0; slice < desc.layerCount; slice++) {
                    char const *sliceBase =
                        static_cast<char const*>(initialData) +
                            mipInfo.byteOffset + imageBytes * slice;

                        [tempTextureId replaceRegion:MTLRegionMake2D(0, 0,
                                                    width, height)
                                      mipmapLevel:mip
                                            slice:slice
                                        withBytes:sliceBase
                                      bytesPerRow:bytesPerRow
                                    bytesPerImage:0];
                }
            } else if (desc.type == HgiTextureType1DArray) {
                const size_t imageBytes = bytesPerRow;
                for (int slice = 0; slice < desc.layerCount; slice++) {
                    char const *sliceBase =
                        static_cast<char const*>(initialData) +
                            mipInfo.byteOffset + imageBytes * slice;

                        [tempTextureId replaceRegion:MTLRegionMake1D(0, width)
                                      mipmapLevel:mip
                                            slice:slice
                                        withBytes:sliceBase
                                      bytesPerRow:bytesPerRow
                                    bytesPerImage:0];
                }
            } else {
                TF_CODING_ERROR("Missing Texture upload implementation");
            }
        }

        // Blit data from temp texture to original texture.
        id<MTLCommandBuffer> commandBuffer = [hgi->GetQueue() commandBuffer];
        id<MTLBlitCommandEncoder> blitCommandEncoder =
            [commandBuffer blitCommandEncoder];
        int sliceCount = 1;
        if (desc.type == HgiTextureType1DArray ||
            desc.type == HgiTextureType2DArray) {
            sliceCount = desc.layerCount;
        }
        [blitCommandEncoder copyFromTexture:tempTextureId
                                sourceSlice:0
                                sourceLevel:0
                                  toTexture:_textureId
                           destinationSlice:0
                           destinationLevel:0
                                 sliceCount:sliceCount
                                 levelCount:mipLevels];
        [blitCommandEncoder endEncoding];
        [commandBuffer commit];
        [tempTextureId release];
    }
    
    if (!(usage & MTLTextureUsageRenderTarget)) {
        id <MTLCommandBuffer> commandBuffer = [hgi->GetQueue() commandBuffer];
        id <MTLBlitCommandEncoder> blitCommandEncoder = [commandBuffer blitCommandEncoder];
        [blitCommandEncoder optimizeContentsForGPUAccess:_textureId];
        [blitCommandEncoder endEncoding];
        [commandBuffer commit];
    }

    HGIMETAL_DEBUG_LABEL(_textureId, _descriptor.debugName.c_str());
}

HgiMetalTexture::HgiMetalTexture(HgiMetal *hgi, HgiTextureViewDesc const & desc)
    : HgiTexture(desc.sourceTexture->GetDescriptor())
    , _textureId(nil)
{
    HgiMetalTexture* srcTexture =
        static_cast<HgiMetalTexture*>(desc.sourceTexture.Get());
    NSRange levels = NSMakeRange(
        desc.sourceFirstMip, desc.mipLevels);
    NSRange slices = NSMakeRange(
        desc.sourceFirstLayer, desc.layerCount);
    MTLPixelFormat mtlFormat = HgiMetalConversions::GetPixelFormat(
        desc.format, HgiTextureUsageBitsColorTarget);

    _textureId = [srcTexture->GetTextureId()
                  newTextureViewWithPixelFormat:mtlFormat
                  textureType:[srcTexture->GetTextureId() textureType]
                  levels:levels
                  slices:slices];
    
    // Update the texture descriptor to reflect the above
    _descriptor.debugName = desc.debugName;
    _descriptor.format = desc.format;
    _descriptor.layerCount = desc.layerCount;
    _descriptor.mipLevels = desc.mipLevels;
}

HgiMetalTexture::~HgiMetalTexture()
{
    if (_textureId != nil) {
        [_textureId release];
        _textureId = nil;
    }
}

size_t
HgiMetalTexture::GetByteSizeOfResource() const
{
    return _GetByteSizeOfResource(_descriptor);
}

uint64_t
HgiMetalTexture::GetRawResource() const
{
    return (uint64_t) _textureId;
}

id<MTLTexture>
HgiMetalTexture::GetTextureId() const
{
    return _textureId;
}

void 
HgiMetalTexture::SubmitLayoutChange(HgiTextureUsage newLayout)
{
    return;
}

PXR_NAMESPACE_CLOSE_SCOPE
