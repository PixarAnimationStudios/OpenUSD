//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GRAPHICS_PIPELINE_H
#define PXR_IMAGING_HGI_GRAPHICS_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/attachmentDesc.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/types.h"

#include "pxr/base/gf/vec2f.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiVertexAttributeDesc
///
/// Describes one attribute of a vertex.
///
/// <ul>
/// <li>format:
///   Format of the vertex attribute.</li>
/// <li>offset:
///    The byte offset of the attribute in vertex buffer</li>
/// <li>shaderBindLocation:
///    The location of the attribute in the shader. layout(location = X)</li>
/// </ul>
///
struct HgiVertexAttributeDesc
{
    HGI_API
    HgiVertexAttributeDesc();

    HgiFormat format;
    uint32_t offset;
    uint32_t shaderBindLocation;
};
using HgiVertexAttributeDescVector = std::vector<HgiVertexAttributeDesc>;

HGI_API
bool operator==(
    const HgiVertexAttributeDesc& lhs,
    const HgiVertexAttributeDesc& rhs);

HGI_API
inline bool operator!=(
    const HgiVertexAttributeDesc& lhs,
    const HgiVertexAttributeDesc& rhs);


/// \struct HgiVertexBufferDesc
///
/// Describes the attributes of a vertex buffer.
///
/// <ul>
/// <li>bindingIndex:
///    Binding location for this vertex buffer.</li>
/// <li>vertexAttributes:
///   List of vertex attributes (in vertex buffer).</li>
/// <li>vertexStepFunction:
///   The rate at which data is pulled for this vertex buffer.</li>
/// <li>vertexStride:
///   The byte size of a vertex (distance between two vertices).</li>
/// </ul>
///
struct HgiVertexBufferDesc
{
    HGI_API
    HgiVertexBufferDesc();

    uint32_t bindingIndex;
    HgiVertexAttributeDescVector vertexAttributes;
    HgiVertexBufferStepFunction vertexStepFunction;
    uint32_t vertexStride;
};
using HgiVertexBufferDescVector = std::vector<HgiVertexBufferDesc>;

HGI_API
bool operator==(
    const HgiVertexBufferDesc& lhs,
    const HgiVertexBufferDesc& rhs);

HGI_API
inline bool operator!=(
    const HgiVertexBufferDesc& lhs,
    const HgiVertexBufferDesc& rhs);


/// \struct HgiMultiSampleState
///
/// Properties to configure multi sampling.
///
/// <ul>
/// <li>multiSampleEnable:
///   When enabled and sampleCount and attachments match and allow for it, use 
///   multi-sampling.</li>
/// <li>alphaToCoverageEnable:
///   Fragment's color.a determines coverage (screen door transparency).</li>
/// <li>alphaToOneEnable:
///   Fragment's color.a is replaced by the maximum representable alpha
///   value for fixed-point color attachments, or by 1.0 for floating-point
///   attachments.</li>
/// <li>sampleCount:
///   The number of samples for each fragment. Must match attachments</li>
/// </ul>
///
struct HgiMultiSampleState
{
    HGI_API
    HgiMultiSampleState();

    bool multiSampleEnable;
    bool alphaToCoverageEnable;
    bool alphaToOneEnable;
    HgiSampleCount sampleCount;
};

HGI_API
bool operator==(
    const HgiMultiSampleState& lhs,
    const HgiMultiSampleState& rhs);

HGI_API
bool operator!=(
    const HgiMultiSampleState& lhs,
    const HgiMultiSampleState& rhs);


/// \struct HgiRasterizationState
///
/// Properties to configure the rasterization state.
///
/// <ul>
/// <li>polygonMode:
///   Determines the rasterization draw mode of primitve (triangles).</li>
/// <li>lineWidth:
///   The width of lines when polygonMode is set to line drawing.</li>
/// <li>cullMode:
///   Determines the culling rules for primitives (triangles).</li>
/// <li>winding:
///   The rule that determines what makes a front-facing primitive.</li>
/// <li>rasterizationEnabled:
///   When false all primitives are discarded before rasterization stage.</li>
/// <li>depthClampEnabled:
///   When enabled clamps the clip space depth to the view volume, rather than
///   clipping the depth to the near and far planes.</li>
/// <li>depthRange:
///   The mapping of NDC depth values to window depth values.</li>
/// <li>conservativeRaster:
///   When enabled, any pixel at least partially covered by a rendered primitive
///   will be rasterized.</li>
/// <li>numClipDistances:
///   The number of user-defined clip distances.</li>
/// </ul>
///
struct HgiRasterizationState
{
    HGI_API
    HgiRasterizationState();

    HgiPolygonMode polygonMode;
    float lineWidth;
    HgiCullMode cullMode;
    HgiWinding winding;
    bool rasterizerEnabled;
    bool depthClampEnabled;
    GfVec2f depthRange;
    bool conservativeRaster;
    size_t numClipDistances;
};

HGI_API
bool operator==(
    const HgiRasterizationState& lhs,
    const HgiRasterizationState& rhs);

HGI_API
bool operator!=(
    const HgiRasterizationState& lhs,
    const HgiRasterizationState& rhs);

/// \struct HgiStencilState
///
/// Properties controlling the operation of the stencil test.
///
/// <ul>
/// <li>compareFn:
///   The function used to test the reference value with the masked
///   value read from the stencil buffer.</li>
/// <li>referenceValue:
//.   The reference value used by the stencil test function.</li>
/// <li>stencilFailOp:
///   The operation executed when the stencil test fails.</li>
/// <li>depthFailOp:
///   The operation executed when the stencil test passes but the
///   depth test fails.</li>
/// <li>depthStencilPassOp:
///   The operation executed when both stencil and depth tests pass.</li>
/// <li>readMask:
///   The mask applied to values before the stencil test function.</li>
/// <li>writeMask:
///   The mask applied when writing to the stencil buffer.</li>
/// </ul>
///
struct HgiStencilState
{
    HGI_API
    HgiStencilState();

    HgiCompareFunction compareFn;
    uint32_t referenceValue;
    HgiStencilOp stencilFailOp;
    HgiStencilOp depthFailOp;
    HgiStencilOp depthStencilPassOp;
    uint32_t readMask;
    uint32_t writeMask;
};

HGI_API
bool operator==(
    const HgiStencilState& lhs,
    const HgiStencilState& rhs);

HGI_API
bool operator!=(
    const HgiStencilState& lhs,
    const HgiStencilState& rhs);

/// \struct HgiDepthStencilState
///
/// Properties to configure depth and stencil test.
///
/// <ul>
/// <li>depthTestEnabled:
///   When enabled uses `depthCompareFn` to test if a fragment passes the
///   depth test. Note that depth writes are automatically disabled when
///   depthTestEnabled is false.</li>
/// <li>depthWriteEnabled:
///   When enabled uses `depthCompareFn` to test if a fragment passes the
///   depth test. Note that depth writes are automatically disabled when
///   depthTestEnabled is false.</li>
/// <li>depthCompareFn:
///   The function used to test depth values.</li>
/// <li>depthBiasEnabled:
///   When enabled applies a bias to depth values before the depth test.
/// <li>depthBiasConstantFactor:
///   The constant depth bias.</li>
/// <li>depthBiasSlopeFactor:
///   The depth bias that scales with the gradient of the primitive.</li>
/// <li>stencilTestEnabled:
///   Enables the stencil test.</li>
/// <li>stencilFront:
///   Stencil operation for front faces.</li>
/// <li>stencilBack:
///   Stencil operation for back faces.</li>
/// </ul>
///
struct HgiDepthStencilState
{
    HGI_API
    HgiDepthStencilState();

    bool depthTestEnabled;
    bool depthWriteEnabled;
    HgiCompareFunction depthCompareFn;

    bool depthBiasEnabled;
    float depthBiasConstantFactor;
    float depthBiasSlopeFactor;

    bool stencilTestEnabled;
    HgiStencilState stencilFront;
    HgiStencilState stencilBack;
};

HGI_API
bool operator==(
    const HgiDepthStencilState& lhs,
    const HgiDepthStencilState& rhs);

HGI_API
bool operator!=(
    const HgiDepthStencilState& lhs,
    const HgiDepthStencilState& rhs);

/// \struct HgiGraphicsShaderConstantsDesc
///
/// A small, but fast buffer of uniform data for shaders.
///
/// <ul>
/// <li>byteSize:
///    Size of the constants in bytes. (max 256 bytes)</li>
/// <li>stageUsage:
///    What shader stage(s) the constants will be used in.</li>
/// </ul>
///
struct HgiGraphicsShaderConstantsDesc {
    HGI_API
    HgiGraphicsShaderConstantsDesc();

    uint32_t byteSize;
    HgiShaderStage stageUsage;
};

HGI_API
bool operator==(
    const HgiGraphicsShaderConstantsDesc& lhs,
    const HgiGraphicsShaderConstantsDesc& rhs);

HGI_API
bool operator!=(
    const HgiGraphicsShaderConstantsDesc& lhs,
    const HgiGraphicsShaderConstantsDesc& rhs);

struct HgiTessellationLevel
{
    HGI_API
    HgiTessellationLevel();

    float innerTessLevel[2];
    float outerTessLevel[4];
};

/// \struct HgiTessellationState
///
/// Properties to configure tessellation.
///
/// <ul>
/// <li>patchType:
///   The type of tessellation patch.</li>
/// <li>primitiveIndexSize:
///   The number of control indices per patch.</li>
/// <li>tessellationLevel:
///   The fallback tessellation levels.</li>
/// </ul>
///
struct HgiTessellationState
{
    enum PatchType {
        Triangle,
        Quad,
        Isoline
    };

    enum TessFactorMode {
        Constant,
        TessControl,
        TessVertex
    };

    HGI_API
    HgiTessellationState();

    PatchType patchType;
    int primitiveIndexSize;
    TessFactorMode tessFactorMode = TessFactorMode::Constant;
    HgiTessellationLevel tessellationLevel;
};

/// \struct HgiGraphicsPipelineDesc
///
/// Describes the properties needed to create a GPU pipeline.
///
/// <ul>
/// <li>primitiveType:
///   Describes the stream of vertices (primitive topology).</li>
/// <li>shaderProgram:
///   Shader functions/stages used in this pipeline.</li>
/// <li>depthState:
///   Describes depth state for a pipeline.</li>
/// <li>multiSampleState:
///   Various settings to control multi-sampling.</li>
/// <li>rasterizationState:
///   Various settings to control rasterization.</li>
/// <li>vertexBuffers:
///   Description of the vertex buffers (per-vertex attributes).
///   The actual VBOs are bound via GraphicsCmds.</li>
/// <li>colorAttachmentDescs:
///   Describes each of the color attachments.</li>
/// <li>depthAttachmentDesc:
///   Describes the depth attachment (optional)
///   Use HgiFormatInvalid to indicate no depth attachment.</li>
/// <li>resolveAttachments:
///   Indicates whether or not to resolve the color and depth attachments.</li>
/// <li>shaderConstantsDesc:
///   Describes the shader uniforms.</li>
/// <li>tessellationState:
///   Describes the tessellation state.</li>
/// </ul>
///
struct HgiGraphicsPipelineDesc
{
    HGI_API
    HgiGraphicsPipelineDesc();

    std::string debugName;
    HgiPrimitiveType primitiveType;
    HgiShaderProgramHandle shaderProgram;
    HgiDepthStencilState depthState;
    HgiMultiSampleState multiSampleState;
    HgiRasterizationState rasterizationState;
    HgiVertexBufferDescVector vertexBuffers;
    HgiAttachmentDescVector colorAttachmentDescs;
    HgiAttachmentDesc depthAttachmentDesc;
    bool resolveAttachments;
    HgiGraphicsShaderConstantsDesc shaderConstantsDesc;
    HgiTessellationState tessellationState;
};

HGI_API
bool operator==(
    const HgiGraphicsPipelineDesc& lhs,
    const HgiGraphicsPipelineDesc& rhs);

HGI_API
bool operator!=(
    const HgiGraphicsPipelineDesc& lhs,
    const HgiGraphicsPipelineDesc& rhs);


///
/// \class HgiGraphicsPipeline
///
/// Represents a graphics platform independent GPU graphics pipeline
/// resource.
///
/// Base class for Hgi pipelines.
/// To the client (HdSt) pipeline resources are referred to via
/// opaque, stateless handles (HgiPipelineHandle).
///
class HgiGraphicsPipeline
{
public:
    HGI_API
    virtual ~HgiGraphicsPipeline();

    /// The descriptor describes the object.
    HGI_API
    HgiGraphicsPipelineDesc const& GetDescriptor() const;

protected:
    HGI_API
    HgiGraphicsPipeline(HgiGraphicsPipelineDesc const& desc);

    HgiGraphicsPipelineDesc _descriptor;

private:
    HgiGraphicsPipeline() = delete;
    HgiGraphicsPipeline & operator=(const HgiGraphicsPipeline&) = delete;
    HgiGraphicsPipeline(const HgiGraphicsPipeline&) = delete;
};

using HgiGraphicsPipelineHandle = HgiHandle<HgiGraphicsPipeline>;
using HgiGraphicsPipelineHandleVector = std::vector<HgiGraphicsPipelineHandle>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
