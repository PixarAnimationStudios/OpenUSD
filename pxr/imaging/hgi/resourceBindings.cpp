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
#include "pxr/imaging/hgi/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiResourceBindings::HgiResourceBindings(HgiResourceBindingsDesc const& desc)
    : _descriptor(desc)
{
}

HgiResourceBindings::~HgiResourceBindings()
{
}

HgiResourceBindingsDesc const&
HgiResourceBindings::GetDescriptor() const
{
    return _descriptor;
}

HgiVertexAttributeDesc::HgiVertexAttributeDesc()
    : format(HgiFormatFloat32Vec4)
    , offset(0)
    , shaderBindLocation(0)
{
}

bool operator==(
    const HgiVertexAttributeDesc& lhs,
    const HgiVertexAttributeDesc& rhs)
{
    return lhs.format == rhs.format &&
           lhs.offset == rhs.offset &&
           lhs.shaderBindLocation == rhs.shaderBindLocation;
}

bool operator!=(
    const HgiVertexAttributeDesc& lhs,
    const HgiVertexAttributeDesc& rhs)
{
    return !(lhs == rhs);
}

HgiVertexBufferDesc::HgiVertexBufferDesc()
    : bindingIndex(0)
    , vertexStride(0)
{
}

bool operator==(
    const HgiVertexBufferDesc& lhs,
    const HgiVertexBufferDesc& rhs)
{
    return lhs.bindingIndex == rhs.bindingIndex &&
           lhs.vertexAttributes == rhs.vertexAttributes &&
           lhs.vertexStride == rhs.vertexStride;
}

bool operator!=(
    const HgiVertexBufferDesc& lhs,
    const HgiVertexBufferDesc& rhs)
{
    return !(lhs == rhs);
}

HgiBufferBindDesc::HgiBufferBindDesc()
    : bindingIndex(0)
    , stageUsage(HgiShaderStageVertex)
{
}

bool operator==(
    const HgiBufferBindDesc& lhs,
    const HgiBufferBindDesc& rhs)
{
    return lhs.buffers == rhs.buffers &&
           lhs.resourceType == rhs.resourceType &&
           lhs.offsets == rhs.offsets &&
           lhs.bindingIndex == rhs.bindingIndex &&
           lhs.stageUsage == rhs.stageUsage;
}

bool operator!=(
    const HgiBufferBindDesc& lhs,
    const HgiBufferBindDesc& rhs)
{
    return !(lhs == rhs);
}

HgiTextureBindDesc::HgiTextureBindDesc()
    : bindingIndex(0)
    , stageUsage(HgiShaderStageFragment)
{
}

bool operator==(
    const HgiTextureBindDesc& lhs,
    const HgiTextureBindDesc& rhs)
{
    return lhs.textures == rhs.textures &&
           lhs.resourceType == rhs.resourceType &&
           lhs.bindingIndex == rhs.bindingIndex &&
           lhs.stageUsage == rhs.stageUsage;
}

bool operator!=(
    const HgiTextureBindDesc& lhs,
    const HgiTextureBindDesc& rhs)
{
    return !(lhs == rhs);
}

HgiResourceBindingsDesc::HgiResourceBindingsDesc()
    : pipelineType(HgiPipelineTypeGraphics)
{
}

bool operator==(
    const HgiResourceBindingsDesc& lhs,
    const HgiResourceBindingsDesc& rhs)
{
    return lhs.debugName == rhs.debugName &&
           lhs.pipelineType == rhs.pipelineType &&
           lhs.buffers == rhs.buffers &&
           lhs.textures == rhs.textures &&
           lhs.vertexBuffers == rhs.vertexBuffers;
}

bool operator!=(
    const HgiResourceBindingsDesc& lhs,
    const HgiResourceBindingsDesc& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
