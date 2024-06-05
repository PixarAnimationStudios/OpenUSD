//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_CAPABILITIES_H
#define PXR_IMAGING_HGI_METAL_CAPABILITIES_H

#include <Metal/Metal.h>
#include "pxr/pxr.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgi/capabilities.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiMetalCapabilities
///
/// Reports the capabilities of the Metal device.
///
class HgiMetalCapabilities final : public HgiCapabilities
{
public:
    HGIMETAL_API
    ~HgiMetalCapabilities() override;
    
    HGIMETAL_API
    int GetAPIVersion() const override;
    
    HGIMETAL_API
    int GetShaderVersion() const override;

    MTLResourceOptions defaultStorageMode;
    bool hasVertexMemoryBarrier;
    bool useParallelEncoder;
    bool requiresIndirectDrawFix;
    bool requiresReturnAfterDiscard;

protected:
    friend class HgiMetal;

    HGIMETAL_API
    HgiMetalCapabilities(id<MTLDevice> device);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

