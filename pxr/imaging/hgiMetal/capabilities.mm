//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiMetal/capabilities.h"
#include "pxr/imaging/hgiMetal/hgi.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/envSetting.h"

#include <Metal/Metal.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIMETAL_ENABLE_INDIRECT_COMMAND_BUFFER, true,
                      "Enable indirect command buffers");

HgiMetalCapabilities::HgiMetalCapabilities(id<MTLDevice> device)
{
    if (@available(macOS 10.14.5, ios 12.0, *)) {
        _SetFlag(HgiDeviceCapabilitiesBitsConcurrentDispatch, true);
    }

#if defined(ARCH_OS_IPHONE)
    bool const hasIPhone = true;
    bool const hasIntelGPU = false;
#elif defined(ARCH_OS_OSX)
    bool const hasIPhone = false;
    bool const hasIntelGPU = [device isLowPower];
#endif

    bool unifiedMemory = false;
    bool barycentrics = false;
    bool hasAppleSilicon = false;
    if (@available(macOS 100.100, ios 12.0, *)) {
        unifiedMemory = true;
    } else if (@available(macOS 10.15, ios 13.0, *)) {
#if defined(ARCH_OS_IPHONE) || (defined(__MAC_10_15) && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_15)
        unifiedMemory = [device hasUnifiedMemory];
#else
        unifiedMemory = [device isLowPower];
#endif
        // On macOS 10.15 and 11.0 the AMD drivers reported the wrong value for
        // supportsShaderBarycentricCoordinates so check both flags.
        // Also, Intel GPU drivers do not correctly support barycentrics.
        barycentrics = ([device supportsShaderBarycentricCoordinates]
                    || [device areBarycentricCoordsSupported])
                    && !hasIntelGPU;
        
#if defined(ARCH_OS_OSX)
        hasAppleSilicon = [device hasUnifiedMemory] && ![device isLowPower];
#endif
    }

    bool icbSupported = false;
    if (hasAppleSilicon && !hasIntelGPU && !hasIPhone) {
        // Indirect command buffers supported only on
        // Apple Silicon GPUs with macOS 12.3 or later.
        if (@available(macOS 12.3, *)) {
            icbSupported = true;
        }
    }
    if (!TfGetEnvSetting(HGIMETAL_ENABLE_INDIRECT_COMMAND_BUFFER)) {
        icbSupported = false;
    }

    _SetFlag(HgiDeviceCapabilitiesBitsUnifiedMemory, unifiedMemory);

    _SetFlag(HgiDeviceCapabilitiesBitsBuiltinBarycentrics, barycentrics);

    _SetFlag(HgiDeviceCapabilitiesBitsShaderDoublePrecision, false);
    
    _SetFlag(HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne, false);

    _SetFlag(HgiDeviceCapabilitiesBitsCppShaderPadding, true);
    
    _SetFlag(HgiDeviceCapabilitiesBitsMetalTessellation, true);

    _SetFlag(HgiDeviceCapabilitiesBitsMultiDrawIndirect, true);
    
    _SetFlag(HgiDeviceCapabilitiesBitsIndirectCommandBuffers, icbSupported);

    // This is done to decide whether to use a workaround for post tess
    // patch primitive ID lookup. The bug causes the firstPatch offset
    // to be included incorrectly in the primitive ID. Our workaround
    // is to subtract it based on the base primitive offset
    // Found in MacOS 13. If confirmed fixed for MacOS 14, add a check
    // if we are on MacOS 14 or less
    //bool isMacOs13OrLess = NSProcessInfo.processInfo.operatingSystemVersion.majorVersion <= 13
    //bool requireBasePrimitiveOffset = hasAppleSilicon && isMacOs13OrLess;
    bool const requiresBasePrimitiveOffset =
                        hasAppleSilicon || hasIntelGPU || hasIPhone;
    _SetFlag(HgiDeviceCapabilitiesBitsBasePrimitiveOffset,
             requiresBasePrimitiveOffset);

    // Intel GPU drivers and iOS w/o barycentrics do not correctly
    // support primitive_id.
    bool const requiresPrimitiveIdEmulation =
                        hasIntelGPU || (hasIPhone && !barycentrics);
    _SetFlag(HgiDeviceCapabilitiesBitsPrimitiveIdEmulation,
             requiresPrimitiveIdEmulation);

    defaultStorageMode = MTLResourceStorageModeShared;
#if defined(ARCH_OS_OSX)
    if (!unifiedMemory) {
        defaultStorageMode = MTLResourceStorageModeManaged;
    }
#endif

    _maxUniformBlockSize          = 64 * 1024;
    _maxShaderStorageBlockSize    = 1 * 1024 * 1024 * 1024;
    _uniformBufferOffsetAlignment = 16;
    _maxClipDistances             = 8;
    _pageSizeAlignment            = 4096;

    // Apple Silicon only support memory barriers between vertex stages after
    // macOS 12.3.
    hasVertexMemoryBarrier = !hasAppleSilicon;
    if (@available(macOS 12.3, *)) {
        hasVertexMemoryBarrier = true;
    }

    // Vega GPUs require a fix to the indirect draw before macOS 12.2
    requiresIndirectDrawFix = false;
    if ([[device name] rangeOfString: @"Vega"].location != NSNotFound) {
        if (@available(macOS 12.2, *)) {}
        else
        {
            requiresIndirectDrawFix = true;
        }
    }

    // Return immediately from the fragment shader main function after
    // executing discard_fragment() in order to avoid side effects
    // from buffer writes. We disable this behavior for MTLGPUFamilyApple9
    // (Apple M3) devices until macOS 14.4.
    requiresReturnAfterDiscard = true;
    if ([[device name] rangeOfString: @"Apple M3"].location != NSNotFound) {
        if (@available(macOS 14.4, *)) {}
        else
        {
            requiresReturnAfterDiscard = false;
        }
    }

    useParallelEncoder = true;
}

HgiMetalCapabilities::~HgiMetalCapabilities() = default;

int
HgiMetalCapabilities::GetAPIVersion() const
{
    if (@available(macOS 10.15, ios 13.0, *)) {
        return APIVersion_Metal3_0;
    }
    if (@available(macOS 10.13, ios 11.0, *)) {
        return APIVersion_Metal2_0;
    }
    
    return APIVersion_Metal1_0;
}

int
HgiMetalCapabilities::GetShaderVersion() const
{
    // Note: This is not the Metal Shader Language version. It is provided for
    // compatibility with code that is asking for the GLSL version.
    return 450;
}

PXR_NAMESPACE_CLOSE_SCOPE
