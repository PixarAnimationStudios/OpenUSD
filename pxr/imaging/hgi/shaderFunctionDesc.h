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

#ifndef PXR_IMAGING_HGI_SHADERFUNCTIONDESC_H
#define PXR_IMAGING_HGI_SHADERFUNCTIONDESC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiShaderFunctionTextureDesc
///
/// Describes a texture to be passed into a shader
///
/// <ul>
/// <li>nameInShader:
///   The name written from the codegen into shader file for the texture.</li>
/// <li>dimensions:
///   1d, 2d or 3d texture declaration.</li>
/// <li>format
///   Format of the texture. This is required in APIs where sampler types depend
///   on the texture (e.g., GL) </li>
/// <li>writable
///   Whether the texture is writable.</li>
/// </ul>
///
struct HgiShaderFunctionTextureDesc
{
    HGI_API
    HgiShaderFunctionTextureDesc();

    std::string nameInShader;
    uint32_t dimensions;
    HgiFormat format;
    bool writable;
};

using HgiShaderFunctionTextureDescVector =
    std::vector<HgiShaderFunctionTextureDesc>;

HGI_API
bool operator==(
    const HgiShaderFunctionTextureDesc& lhs,
    const HgiShaderFunctionTextureDesc& rhs);

HGI_API
bool operator!=(
    const HgiShaderFunctionTextureDesc& lhs,
    const HgiShaderFunctionTextureDesc& rhs);

/// \struct HgiShaderFunctionBufferDesc
///
/// Describes a buffer to be passed into a shader
///
/// <ul>
/// <li>nameInShader:
///   The name written from the codegen into shader file for the texture.</li>
/// <li>type:
///   Type of the param within the shader file.</li>
/// <li>bindIndex:
///   The index of the resource.</li>
/// <li>arraySize:
///   The size of the array when binding is a HgiBindingTypeArray.</li>
/// <li>bindingType:
///   The binding model to use to expose the buffer to the shader.</li>
/// <li>writeable:
///   Whether the resource is writable.</li>
/// </ul>
///
struct HgiShaderFunctionBufferDesc
{
    HGI_API
    HgiShaderFunctionBufferDesc();

    std::string nameInShader;
    std::string type;
    int32_t bindIndex;
    uint32_t arraySize;
    HgiBindingType binding;
    bool writable;
};

using HgiShaderFunctionBufferDescVector =
    std::vector<HgiShaderFunctionBufferDesc>;

HGI_API
bool operator==(
    const HgiShaderFunctionBufferDesc& lhs,
    const HgiShaderFunctionBufferDesc& rhs);

HGI_API
bool operator!=(
    const HgiShaderFunctionBufferDesc& lhs,
    const HgiShaderFunctionBufferDesc& rhs);

/// \struct HgiShaderFunctionParamDesc
///
/// Describes a constant param passed into a shader
///
/// <ul>
/// <li>nameInShader:
///   The name written from the codegen into the shader file for the param.</li>
/// <li>type:
///   Type of the param within the shader file.</li>
/// <li>role:
///   Optionally a role can be specified, like position, uv, color.</li>
/// <li>attribute:
///   Optionally an attribute can be specified, like versions or addresses.</li>
/// <li>attributeIndex:
///   Used in metal, to specify indicies of attributes.</li>
/// </ul>
///
struct HgiShaderFunctionParamDesc
{
    HGI_API
    HgiShaderFunctionParamDesc();

    std::string nameInShader;
    std::string type;
    std::string role;
    std::string attribute;
    std::string attributeIndex;
};

using HgiShaderFunctionParamDescVector =
    std::vector<HgiShaderFunctionParamDesc>;

HGI_API
bool operator==(
    const HgiShaderFunctionParamDesc& lhs,
    const HgiShaderFunctionParamDesc& rhs);

HGI_API
bool operator!=(
    const HgiShaderFunctionParamDesc& lhs,
    const HgiShaderFunctionParamDesc& rhs);

/// \struct HgiShaderFunctionComputeDesc
///
/// Describes a compute function's description
///
/// <ul>
/// <li>localSize:
///   Optional. Specifices the 3D size of the local thread grouping. Defaults to
///   0, meaning it is not set. When x > 0, y and z must also be set > 0. 
///   When localSize is set to > 0, the following source is generated:
///   GLSL: layout(local_size_x = localSize[0],
///      local_size_y = localSize[1], local_size_z = localSize[2]) in;
///   MSL: [[max_total_threads_per_threadgroup(localSize[0] * 
///      localSize[1] * localSize[w])]]
/// </li>
/// </ul>
///
struct HgiShaderFunctionComputeDesc
{
    HGI_API
    HgiShaderFunctionComputeDesc();

    GfVec3i localSize;
};

HGI_API
bool operator==(
        const HgiShaderFunctionComputeDesc& lhs,
        const HgiShaderFunctionComputeDesc& rhs);

HGI_API
bool operator!=(
        const HgiShaderFunctionComputeDesc& lhs,
        const HgiShaderFunctionComputeDesc& rhs);

/// \struct HgiShaderFunctionTessellationDesc
///
/// Describes a tessellation function's description
///
/// <ul>
/// <li>patchType:
///   The type of patch</li>
/// <li>numVertsInPerPatch:
///   The number of vertices in per patch</li>
/// <li>numVertsOutPerPatch:
///   The number of vertices out per patch</li>
/// </ul>
///
struct HgiShaderFunctionTessellationDesc
{
    enum class PatchType { Quad, Triangle };
    HGI_API
    HgiShaderFunctionTessellationDesc();

    PatchType patchType = PatchType::Triangle;
    uint32_t numVertsPerPatchIn = 3;
    uint32_t numVertsPerPatchOut = 3;
};

HGI_API
bool operator==(
        const HgiShaderFunctionTessellationDesc& lhs,
        const HgiShaderFunctionTessellationDesc& rhs);

HGI_API
bool operator!=(
        const HgiShaderFunctionTessellationDesc& lhs,
        const HgiShaderFunctionTessellationDesc& rhs);

/// \struct HgiShaderFunctionDesc
///
/// Describes the properties needed to create a GPU shader function.
///
/// <ul>
/// <li>debugName:
///   This label can be applied as debug label for gpu debugging.</li>
/// <li>shaderStage:
///   The shader stage this function represents.</li>
/// <li>shaderCode:
///   The ascii shader code used to compile the shader.</li>
/// <li>textures:
///   List of texture descriptions to be passed into a shader.</li>
/// <li>buffers:
///   List of buffer descriptions to be passed into a shader.</li>
/// <li>constantParams:
///   List of descriptions of constant params passed into a shader.</li>
/// <li>stageInputs:
///   List of descriptions of the inputs of the shader.</li>
/// <li>stageOutputs:
///   List of descriptions of the outputs of the shader.</li>
/// <li>tessellationDesc:
///   Description of tessellation shader function.</li>
/// <li>computeDescriptor:
///   Description of compute shader function.</li>
/// </ul>
///
struct HgiShaderFunctionDesc
{
    HGI_API
    HgiShaderFunctionDesc();
    std::string debugName;
    HgiShaderStage shaderStage;
    const char*  shaderCode;
    std::vector<HgiShaderFunctionTextureDesc> textures;
    std::vector<HgiShaderFunctionBufferDesc> buffers;
    std::vector<HgiShaderFunctionParamDesc> constantParams;
    std::vector<HgiShaderFunctionParamDesc> stageInputs;
    std::vector<HgiShaderFunctionParamDesc> stageOutputs;
    HgiShaderFunctionTessellationDesc tessellationDescriptor;
    HgiShaderFunctionComputeDesc computeDescriptor;
};

using HgiShaderFunctionDescVector =
    std::vector<HgiShaderFunctionDesc>;

HGI_API
bool operator==(
    const HgiShaderFunctionDesc& lhs,
    const HgiShaderFunctionDesc& rhs);

HGI_API
bool operator!=(
    const HgiShaderFunctionDesc& lhs,
    const HgiShaderFunctionDesc& rhs);

/// Adds texture descriptor to given shader function descriptor.
HGI_API
void
HgiShaderFunctionAddTexture(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    uint32_t dimensions = 2,
    const HgiFormat &format = HgiFormatFloat32Vec4);

/// Adds writable texture descriptor to given shader function descriptor.
HGI_API
void
HgiShaderFunctionAddWritableTexture(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    uint32_t dimensions = 2,
    const HgiFormat &format = HgiFormatFloat32Vec4);

/// Adds buffer descriptor to given shader function descriptor.
HGI_API
void
HgiShaderFunctionAddBuffer(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const uint32_t bindIndex,
    HgiBindingType binding,
    const uint32_t arraySize = 0
    );

/// Adds buffer descriptor to given shader function descriptor.
HGI_API
void
HgiShaderFunctionAddWritableBuffer(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const uint32_t bindIndex
    );

/// Adds constant function param descriptor to given shader function
/// descriptor.
HGI_API
void
HgiShaderFunctionAddConstantParam(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role = std::string(),
    const std::string &attribute = std::string(),
    const std::string &attributeIndex = std::string());

/// Adds stage input function param descriptor to given shader function
/// descriptor.
HGI_API
void
HgiShaderFunctionAddStageInput(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role = std::string(),
    const std::string &attribute = std::string(),
    const std::string &attributeIndex = std::string());

/// Adds stage output function param descriptor to given shader function
/// descriptor.
HGI_API
void
HgiShaderFunctionAddStageOutput(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role = std::string(),
    const std::string &attribute = std::string(),
    const std::string &attributeIndex = std::string());

PXR_NAMESPACE_CLOSE_SCOPE

#endif
