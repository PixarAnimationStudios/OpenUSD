//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent/metal.h"

#include "pxr/base/gf/range1d.h"

#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/texture.h"

#include "pxr/imaging/hgiPresent/debugCodes.h"

#include <unordered_map>
#include <iostream>

#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>


// This code uses the Metal APIs directly since it only works with HgiMetal.
// It doesn't really makes much sense to use the Hgi abstraction if it's always
// HgiMetal: it just adds overhead. Unless we go the full mile abstracting
// over surfaces, swapchains and other presentation concerns, using the Hgi
// layer doesn't have any real benefits. Implementing it would be a lot of
// additional work to replace not that many lines of API specific code.

namespace {
PXR_NAMESPACE_USING_DIRECTIVE

std::optional<MTLPixelFormat>
_FindPreferredSurfaceFormatMatch(HgiFormat format, bool needs_sRGB_Transform)
{
    if (needs_sRGB_Transform) {
        return MTLPixelFormatBGRA8Unorm_sRGB;
    }

    switch (format) {
    case HgiFormatUNorm8Vec4:
        return MTLPixelFormatBGRA8Unorm;
    case HgiFormatUNorm8Vec4srgb:
        return MTLPixelFormatBGRA8Unorm_sRGB;
    case HgiFormatFloat16Vec4:
        return MTLPixelFormatRGBA16Float;
    case HgiFormatFloat32Vec3:
    case HgiFormatFloat16Vec3:
    case HgiFormatFloat32Vec4:
        return MTLPixelFormatRGBA16Float;
    case HgiFormatInvalid:
    case HgiFormatCount:
        TF_CODING_ERROR("Invalid Format");
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

std::optional<CGColorSpaceRef>
_MetalColorSpaceFromName(TfToken const &name)
{
    // Color spaces not in this map are unsupported.
    // Nil means no color matching.
    static const std::unordered_map<TfToken, CFStringRef, TfToken::HashFunctor>
        namedColorSpaces = {
            {GfColorSpaceNames->CIEXYZ, kCGColorSpaceGenericXYZ},
            {GfColorSpaceNames->Data, nil},
            {GfColorSpaceNames->Raw, nil},
            {GfColorSpaceNames->Unknown, nil},
            {GfColorSpaceNames->LinearAP1, kCGColorSpaceACESCGLinear},
            {GfColorSpaceNames->LinearDisplayP3,
                kCGColorSpaceExtendedLinearDisplayP3},
            {GfColorSpaceNames->LinearRec2020,
                kCGColorSpaceExtendedLinearITUR_2020},
            {GfColorSpaceNames->LinearRec709, kCGColorSpaceLinearSRGB},
            // See note about VK_COLOR_SPACE_BT709_NONLINEAR_EXT in vulkan.cpp
            {GfColorSpaceNames->G18Rec709, kCGColorSpaceITUR_709},
            {GfColorSpaceNames->G22AdobeRGB, kCGColorSpaceAdobeRGB1998},
            // See note about VK_COLOR_SPACE_BT709_NONLINEAR_EXT in vulkan.cpp
            {GfColorSpaceNames->G22Rec709, kCGColorSpaceITUR_709},
            {GfColorSpaceNames->SRGBP3D65, kCGColorSpaceDisplayP3},
            {GfColorSpaceNames->SRGBRec709, kCGColorSpaceSRGB}, // Extended?
        };

    if (const auto iter = namedColorSpaces.find(name);
        iter != namedColorSpaces.end()) {
        return iter-> second ? CGColorSpaceCreateWithName(iter->second) : nil;
    }

    return std::nullopt;
}

id<MTLComputePipelineState>
_MakeComputePipeline(id<MTLDevice> device)
{
    NSString *shaderSource = @R"(
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

kernel void flipAndCopyTexture(
    texture2d<float, access::read> inTexture,
    texture2d<float, access::write> outTexture,
    uint2 inCoord [[thread_position_in_grid]])
{
    if(inCoord.x >= outTexture.get_width() ||
        inCoord.y >= outTexture.get_height()) {
        return;
    }

    uint2 outCoord{inCoord.x, outTexture.get_height() - inCoord.y};
    outTexture.write(inTexture.read(inCoord), outCoord);
}
)";

    MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
    options.fastMathEnabled = YES;
    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource
                                                  options:options
                                                    error:&error];
    [options release];
    options = nil;

    if (!library) {
        NSString *errStr = [error localizedDescription];
        TF_FATAL_CODING_ERROR("Failed to compile present shader: %s",
            [errStr UTF8String]);
    }

    id<MTLFunction> computeColorCopyProgram =
        [library newFunctionWithName:@"flipAndCopyTexture"];
    id<MTLComputePipelineState> computePipelineStateColor =
        [device newComputePipelineStateWithFunction:computeColorCopyProgram
                                              error:&error];

    if (!computePipelineStateColor) {
        NSString *errStr = [error localizedDescription];
        TF_FATAL_CODING_ERROR( "Failed to create present pipeline: %s",
            [errStr UTF8String]);
        return nil;
    }

    return computePipelineStateColor;
}
}

PXR_NAMESPACE_OPEN_SCOPE


struct MetalResources
{
    id<MTLComputePipelineState> computePipeline = nil;
};

HgiPresentWindowMetal::HgiPresentWindowMetal(HgiMetal *hgi,
    HgiPresentWindowParams const &params)
    : HgiPresentImpl(hgi)
    , _hgiMetal{hgi}
    , _params(params)
    , _resources(std::make_unique<MetalResources>())
{
    @autoreleasepool {
        if (!_hgiMetal) {
            TF_CODING_ERROR("hgi must be a valid HgiMetal pointer");
            return;
        }

        if (!std::holds_alternative<HgiPresentMetalWindowHandle>(_params.window)) {
            TF_WARN(
                "Window handle is unsupported by Metal: presentation is disabled");
            return;
        }

        const bool needs_sRGB_Transform =
            _params.srcColorSpace == GfColorSpaceNames->LinearRec709 &&
            _params.surfaceColorSpace == GfColorSpaceNames->SRGBRec709;
        const auto maybePixelFormat = _FindPreferredSurfaceFormatMatch(
            _params.preferredSurfaceFormat, needs_sRGB_Transform);
        if (!maybePixelFormat) {
            TF_WARN(
                "No compatible Metal layer format found: presentation is disabled");
            return;
        }

        const auto maybeColorSpace =
            _MetalColorSpaceFromName(_params.surfaceColorSpace);
        if (!maybeColorSpace) {
            TF_WARN(
                "Requested Metal layer color space not found: presentation is disabled");
            return;
        }

        if (TfDebug::IsEnabled(HGIPRESENT_DUMP_CANDIDATE_SURFACE_FORMATS)) {
            std::string colorSpaceNameCStr;
            colorSpaceNameCStr.resize(128);
            if (CFStringRef colorSpaceName =
                    CGColorSpaceCopyName(*maybeColorSpace)) {
                CFStringGetCString(colorSpaceName, colorSpaceNameCStr.data(),
                    colorSpaceNameCStr.length(), kCFStringEncodingUTF8);
                CFRelease(colorSpaceName);
            }
            std::cout << "Metal surface format: " << *maybePixelFormat <<
                ", " << colorSpaceNameCStr << std::endl;
        }

        _metalLayer = std::get<HgiPresentMetalWindowHandle>(_params.window).layer;

        if (_metalLayer.device == nil) {
            _metalLayer.device = _hgiMetal->GetPrimaryDevice();
        }

        _metalLayer.allowsNextDrawableTimeout = NO;
        // So we can write to the drawable
        _metalLayer.framebufferOnly = NO;
        _metalLayer.maximumDrawableCount = 2;
        _metalLayer.pixelFormat = *maybePixelFormat;
        _metalLayer.colorspace = *maybeColorSpace;
        // Needs release: https://github.com/KhronosGroup/MoltenVK/issues/940
        CGColorSpaceRelease(*maybeColorSpace);
#if defined(ARCH_OS_OSX)
        _metalLayer.displaySyncEnabled = _params.wantVsync;
#endif

        if (const auto pipeline =
            _MakeComputePipeline(_hgiMetal->GetPrimaryDevice())) {
            [pipeline retain];
            _resources->computePipeline = pipeline;
        }
    }
}

HgiPresentWindowMetal::~HgiPresentWindowMetal()
{
    @autoreleasepool {
        [_resources->computePipeline release];
    }
}

bool
HgiPresentWindowMetal::IsFormatSupported(HgiFormat colorFormat) const
{
    return HgiIsFloatFormat(colorFormat) && !HgiIsCompressed(colorFormat);
}

bool
HgiPresentWindowMetal::IsValid() const
{
    return _metalLayer && _resources->computePipeline;
}

void
HgiPresentWindowMetal::Present(HgiTextureHandle const &hgiSrcTexture,
    HgiTextureHandle const &)
{
    @autoreleasepool {
        if (!_metalLayer) {
            return;
        }

        const auto srcTexture = dynamic_cast<HgiMetalTexture *>(hgiSrcTexture.
            Get());
        if (!srcTexture) {
            TF_CODING_ERROR("srcColor must be a valid HgiVulkan pointer");
            return;
        }

        const auto srcTextureDesc = hgiSrcTexture->GetDescriptor();
        const auto srcWidth = srcTextureDesc.dimensions[0];
        const auto srcHeight = srcTextureDesc.dimensions[1];
        _metalLayer.drawableSize = CGSize{static_cast<float>(srcWidth),
            static_cast<float>(srcHeight)};

        id<CAMetalDrawable> drawable = [_metalLayer nextDrawable];

        id<MTLCommandBuffer> commandBuffer =
            [_hgiMetal->GetQueue() commandBuffer];

        NSUInteger preferredThreadsPerGroup =
            [_resources->computePipeline threadExecutionWidth];
        NSUInteger maxThreadsPerGroup =
            [_resources->computePipeline maxTotalThreadsPerThreadgroup];

        MTLSize groupSize = MTLSizeMake(preferredThreadsPerGroup,
            maxThreadsPerGroup / preferredThreadsPerGroup, 1);
        MTLSize dispatchGroups = MTLSizeMake(
            (srcWidth + groupSize.width - 1) / groupSize.width,
            (srcHeight + groupSize.height - 1) / groupSize.height,
            1);

        id<MTLComputeCommandEncoder> computeEncoder =
            [commandBuffer computeCommandEncoder];
        [computeEncoder setComputePipelineState:_resources->computePipeline];
        [computeEncoder setTexture:srcTexture->GetTextureId() atIndex:0];
        [computeEncoder setTexture:drawable.texture atIndex:1];

        [computeEncoder dispatchThreadgroups:dispatchGroups
                       threadsPerThreadgroup:groupSize];

        [computeEncoder endEncoding];

        if (_metalLayer.presentsWithTransaction) {
            [commandBuffer commit];
            [commandBuffer waitUntilScheduled];
            [drawable present];
        } else {
            [commandBuffer presentDrawable:drawable];
            [commandBuffer commit];
        }

        // We need to force a sync here because we don't have the
        // synchronization mechanism to prevent the AOV from being reused
        // before presentation is finished.
        [commandBuffer waitUntilCompleted];
   }
}

HgiMetal*
DynamicCastHgiMetal(Hgi* hgi)
{
    return dynamic_cast<HgiMetal*>(hgi);
}


PXR_NAMESPACE_CLOSE_SCOPE
