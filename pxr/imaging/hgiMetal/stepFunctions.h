//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_STEP_FUNCTIONS_H
#define PXR_IMAGING_HGI_METAL_STEP_FUNCTIONS_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiMetal/api.h"

#include <cstdint>
#include <vector>

#include <Metal/Metal.h>

PXR_NAMESPACE_OPEN_SCOPE

// We implement multi-draw indirect commands on Metal by encoding
// separate draw commands for each draw.
//
// Some aspects of drawing command primitive input assembly work
// differently on Metal than other graphics APIs. There are two
// concerns that we need to account for while processing a buffer
// with multiple indirect draw commands.
//
// 1) Metal does not support a vertex attrib divisor, so in order to
// have vertex attributes which advance once per draw command we use
// a constant vertex buffer step function and advance the vertex buffer
// binding offset explicitly by executing setVertexBufferOffset for
// the vertex buffers associated with "perDrawCommand" vertex attributes.
//
// 2) Metal does not support a base vertex offset for control point
// vertex attributes when drawing patches. It is inconvenient and
// expensive to encode a distinct controlPointIndex buffer for each
// draw that shares a patch topology. Instead, we use a per patch
// control point vertex buffer step function, and explicitly advance
// the vertex buffer binding offset by executing setVertexBufferOffset
// for the vertex buffers associated with "perPatchControlPoint"
// vertex attributes.

struct HgiGraphicsPipelineDesc;

/// \struct HgiMetalStepFunctionDesc
///
/// For passing in vertex buffer step function parameters.
///
struct HgiMetalStepFunctionDesc
{
    HgiMetalStepFunctionDesc(
            uint32_t bindingIndex,
            uint32_t byteOffset,
            uint32_t vertexStride)
        : bindingIndex(bindingIndex)
        , byteOffset(byteOffset)
        , vertexStride(vertexStride)
        { }
    uint32_t bindingIndex;
    uint32_t byteOffset;
    uint32_t vertexStride;
};

using HgiMetalStepFunctionDescVector = std::vector<HgiMetalStepFunctionDesc>;

class HgiMetalStepFunctions
{
public:
    HGIMETAL_API
    HgiMetalStepFunctions();
    
    HGIMETAL_API
    HgiMetalStepFunctions(
        HgiGraphicsPipelineDesc const &graphicsDesc,
        HgiVertexBufferBindingVector const &bindings);

    HGIMETAL_API
    void Init(HgiGraphicsPipelineDesc const &graphicsDesc);
    
    HGIMETAL_API
    void Bind(HgiVertexBufferBindingVector const &bindings);
    
    HGIMETAL_API
    void SetVertexBufferOffsets(
        id<MTLRenderCommandEncoder> encoder,
        uint32_t baseInstance);
    
    HGIMETAL_API
    void SetPatchBaseOffsets(
        id<MTLRenderCommandEncoder> encoder,
        uint32_t baseInstance);
    
    HGIMETAL_API
    HgiMetalStepFunctionDescVector const &GetPatchBaseDescs() const
    {
        return _patchBaseDescs;
    }
    
    HGIMETAL_API
    uint32_t GetDrawBufferIndex() const
    {
        return _drawBufferIndex;
    }

private:
    HgiMetalStepFunctionDescVector _vertexBufferDescs;
    HgiMetalStepFunctionDescVector _patchBaseDescs;
    uint32_t _drawBufferIndex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
