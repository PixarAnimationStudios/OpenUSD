//
// Copyright 2020 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_IMAGING_HGI_PIPELINE_H
#define PXR_IMAGING_HGI_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/types.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiMultiSampleState
///
/// Properties to configure multi sampling.
///
/// <ul>
/// <li>alphaToCoverageEnable:
///   Fragment’s color.a determines coverage (screen door transparency).</li>
///
struct HgiMultiSampleState
{
    HGI_API
    HgiMultiSampleState();

    bool alphaToCoverageEnable;
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
/// Properties to configure multi sampling.
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
};

HGI_API
bool operator==(
    const HgiRasterizationState& lhs,
    const HgiRasterizationState& rhs);

HGI_API
bool operator!=(
    const HgiRasterizationState& lhs,
    const HgiRasterizationState& rhs);

/// \struct HgiDepthStencilState
///
/// Properties to configure depth and stencil test.
///
/// <ul>
/// <li>depthTestEnabled:
///   When enabled uses `depthCompareOp` to test if a fragment passes the 
///   depth test. Note that depth writes are automatically disabled when
///   depthTestEnabled is false.</li>
/// <li>depthWriteEnabled:
///   When enabled uses `depthCompareOp` to test if a fragment passes the 
///   depth test. Note that depth writes are automatically disabled when
///   depthTestEnabled is false.</li>
/// <li>stencilTestEnabled:
///   Enables the stencil test.</li>
/// </ul>
///
struct HgiDepthStencilState
{
    HGI_API
    HgiDepthStencilState();

    bool depthTestEnabled;
    bool depthWriteEnabled;
    bool stencilTestEnabled;
};

HGI_API
bool operator==(
    const HgiDepthStencilState& lhs,
    const HgiDepthStencilState& rhs);

HGI_API
bool operator!=(
    const HgiDepthStencilState& lhs,
    const HgiDepthStencilState& rhs);

/// \struct HgiPipelineDesc
///
/// Describes the properties needed to create a GPU pipeline.
///
/// <ul>
/// <li>pipelineType:
///   Bind point for pipeline (Graphics or Compute).</li>
/// <li>resourceBindings:
///   The resource bindings that will be bound when the pipeline is used.
///   Primarily used to query the vertex attributes.</li>
/// <li>shaderProgram:
///   Shader functions/stages used in this pipeline.</li>
/// <li>depthState:
///   (Graphics pipeline only)
///   Describes depth state for a pipeline.</li>
/// <li>multiSampleState:
///   (Graphics pipeline only)
///   Various settings to control multi-sampling.</li>
/// <li>rasterizationState:
///   (Graphics pipeline only)
///   Various settings to control rasterization.</li>
/// </ul>
///
struct HgiPipelineDesc
{
    HGI_API
    HgiPipelineDesc();

    std::string debugName;
    HgiPipelineType pipelineType;
    HgiResourceBindingsHandle resourceBindings;
    HgiShaderProgramHandle shaderProgram;
    HgiDepthStencilState depthState;
    HgiMultiSampleState multiSampleState;
    HgiRasterizationState rasterizationState;
};

HGI_API
bool operator==(
    const HgiPipelineDesc& lhs,
    const HgiPipelineDesc& rhs);

HGI_API
bool operator!=(
    const HgiPipelineDesc& lhs,
    const HgiPipelineDesc& rhs);


///
/// \class HgiPipeline
///
/// Represents a graphics platform independent GPU pipeline resource.
///
/// Base class for Hgi pipelines.
/// To the client (HdSt) pipeline resources are referred to via
/// opaque, stateless handles (HgiPipelineHandle).
///
class HgiPipeline
{
public:
    HGI_API
    virtual ~HgiPipeline();

    /// The descriptor describes the object.
    HGI_API
    HgiPipelineDesc const& GetDescriptor() const;

protected:
    HGI_API
    HgiPipeline(HgiPipelineDesc const& desc);

    HgiPipelineDesc _descriptor;

private:
    HgiPipeline() = delete;
    HgiPipeline & operator=(const HgiPipeline&) = delete;
    HgiPipeline(const HgiPipeline&) = delete;
};

/// Explicitly instantiate and define pipeline handle
template class HgiHandle<class HgiPipeline>;
typedef HgiHandle<class HgiPipeline> HgiPipelineHandle;
typedef std::vector<HgiPipelineHandle> HgiPipelineHandleVector;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
