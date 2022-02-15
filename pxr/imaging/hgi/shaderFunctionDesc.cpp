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
#include "pxr/imaging/hgi/shaderFunctionDesc.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiShaderFunctionTextureDesc::HgiShaderFunctionTextureDesc()
  : dimensions(2)
  , format(HgiFormatInvalid)
  , textureType(HgiShaderTextureTypeTexture)
  , arraySize(0)
  , writable(false)
{
}

HgiShaderFunctionBufferDesc::HgiShaderFunctionBufferDesc()
  : bindIndex(0)
  , arraySize(0)
  , binding(HgiBindingTypeValue)
  , writable(false)
{
}

HgiShaderFunctionParamDesc::HgiShaderFunctionParamDesc() = default;

HgiShaderFunctionComputeDesc::HgiShaderFunctionComputeDesc()
    : localSize(GfVec3i(0, 0, 0))
{ 
}

HgiShaderFunctionTessellationDesc::HgiShaderFunctionTessellationDesc()
= default;

HgiShaderFunctionDesc::HgiShaderFunctionDesc() 
  : shaderStage(0)
  , shaderCode(nullptr)
  , textures()
  , constantParams()
  , stageInputs()
  , stageOutputs()
  , tessellationDescriptor()
  , computeDescriptor()
{
}

bool operator==(
    const HgiShaderFunctionTextureDesc& lhs,
    const HgiShaderFunctionTextureDesc& rhs)
{
    return lhs.nameInShader == rhs.nameInShader &&
           lhs.dimensions == rhs.dimensions &&
           lhs.format == rhs.format &&
           lhs.textureType == rhs.textureType &&
           lhs.arraySize == rhs.arraySize &&
           lhs.writable == rhs.writable;
}

bool operator!=(
    const HgiShaderFunctionTextureDesc& lhs,
    const HgiShaderFunctionTextureDesc& rhs)
{
    return !(lhs == rhs);
}

bool operator==(
    const HgiShaderFunctionBufferDesc& lhs,
    const HgiShaderFunctionBufferDesc& rhs)
{
    return lhs.nameInShader == rhs.nameInShader &&
           lhs.type == rhs.type &&
           lhs.bindIndex == rhs.bindIndex &&
           lhs.arraySize == rhs.arraySize &&
           lhs.binding == rhs.binding &&
           lhs.writable == rhs.writable;
}

bool operator!=(
    const HgiShaderFunctionBufferDesc& lhs,
    const HgiShaderFunctionBufferDesc& rhs)
{
    return !(lhs == rhs);
}

bool operator==(
    const HgiShaderFunctionParamDesc& lhs,
    const HgiShaderFunctionParamDesc& rhs)
{
    return lhs.nameInShader == rhs.nameInShader &&
           lhs.type == rhs.type && 
           lhs.role == rhs.role &&
           lhs.attribute == rhs.attribute &&
           lhs.attributeIndex == rhs.attributeIndex;
}

bool operator!=(
    const HgiShaderFunctionParamDesc& lhs,
    const HgiShaderFunctionParamDesc& rhs)
{
    return !(lhs == rhs);
}

bool operator==(
        const HgiShaderFunctionComputeDesc& lhs,
        const HgiShaderFunctionComputeDesc& rhs)
{
    return lhs.localSize == rhs.localSize;
}

bool operator!=(
    const HgiShaderFunctionComputeDesc& lhs,
    const HgiShaderFunctionComputeDesc& rhs)
{
    return !(lhs == rhs);
}

bool operator==(
        const HgiShaderFunctionTessellationDesc& lhs,
        const HgiShaderFunctionTessellationDesc& rhs)
{
    return lhs.patchType == rhs.patchType &&
    lhs.numVertsPerPatchIn == rhs.numVertsPerPatchIn &&
    lhs.numVertsPerPatchOut == rhs.numVertsPerPatchOut;
}

bool operator!=(
    const HgiShaderFunctionTessellationDesc& lhs,
    const HgiShaderFunctionTessellationDesc& rhs)
{
    return !(lhs == rhs);
}

bool operator==(
    const HgiShaderFunctionDesc& lhs,
    const HgiShaderFunctionDesc& rhs)
{
    return lhs.debugName == rhs.debugName &&
           lhs.shaderStage == rhs.shaderStage &&
           // Omitted. Only used tmp during shader compile
           // lhs.shaderCode == rhs.shaderCode
           lhs.textures == rhs.textures &&
           lhs.constantParams == rhs.constantParams &&
           lhs.stageInputs == rhs.stageInputs &&
           lhs.stageOutputs == rhs.stageOutputs &&
           lhs.computeDescriptor == rhs.computeDescriptor &&
           lhs.tessellationDescriptor == rhs.tessellationDescriptor;
}

bool operator!=(
    const HgiShaderFunctionDesc& lhs,
    const HgiShaderFunctionDesc& rhs)
{
    return !(lhs == rhs);
}

void
HgiShaderFunctionAddTexture(
    HgiShaderFunctionDesc * const desc,
    const std::string &nameInShader,
    const uint32_t dimensions /* = 2 */,
    const HgiFormat &format /* = HgiFormatFloat32Vec4*/,
    const HgiShaderTextureType textureType /* = HgiShaderTextureTypeTexture */)
{
    HgiShaderFunctionTextureDesc texDesc;
    texDesc.nameInShader = nameInShader;
    texDesc.dimensions = dimensions;
    texDesc.format = format;
    texDesc.textureType = textureType;
    texDesc.arraySize = 0;
    texDesc.writable = false;

    desc->textures.push_back(std::move(texDesc));
}

void
HgiShaderFunctionAddArrayOfTextures(
    HgiShaderFunctionDesc * const desc,
    const std::string &nameInShader,
    const uint32_t arraySize,
    const uint32_t dimensions /* = 2 */,
    const HgiFormat &format /* = HgiFormatFloat32Vec4*/,
    const HgiShaderTextureType textureType /* = HgiShaderTextureTypeTexture */)
{
    HgiShaderFunctionTextureDesc texDesc;
    texDesc.nameInShader = nameInShader;
    texDesc.dimensions = dimensions;
    texDesc.format = format;
    texDesc.textureType = textureType;
    texDesc.arraySize = arraySize;
    texDesc.writable = false;

    desc->textures.push_back(std::move(texDesc));
}

void
HgiShaderFunctionAddWritableTexture(
    HgiShaderFunctionDesc * const desc,
    const std::string &nameInShader,
    const uint32_t dimensions /* = 2 */,
    const HgiFormat &format /* = HgiFormatFloat32Vec4*/,
    const HgiShaderTextureType textureType /* = HgiShaderTextureTypeTexture */)
{
    HgiShaderFunctionTextureDesc texDesc;
    texDesc.nameInShader = nameInShader;
    texDesc.dimensions = dimensions;
    texDesc.format = format;
    texDesc.textureType = textureType;
    texDesc.arraySize = 0;
    texDesc.writable = true;

    desc->textures.push_back(std::move(texDesc));
}

void
HgiShaderFunctionAddBuffer(
    HgiShaderFunctionDesc * const desc,
    const std::string &nameInShader,
    const std::string &type,
    const uint32_t bindIndex,
    HgiBindingType binding,
    uint32_t arraySize)
{
    HgiShaderFunctionBufferDesc bufDesc;
    bufDesc.nameInShader = nameInShader;
    bufDesc.type = type;
    bufDesc.binding = binding;
    bufDesc.arraySize = arraySize;
    bufDesc.bindIndex = bindIndex;
    bufDesc.writable = false;

    desc->buffers.push_back(std::move(bufDesc));
}

void
HgiShaderFunctionAddWritableBuffer(
    HgiShaderFunctionDesc * const desc,
    const std::string &nameInShader,
    const std::string &type,
    const uint32_t bindIndex)
{
    HgiShaderFunctionBufferDesc bufDesc;
    bufDesc.nameInShader = nameInShader;
    bufDesc.type = type;
    bufDesc.bindIndex = bindIndex;
    bufDesc.binding = HgiBindingTypePointer;
    bufDesc.writable = true;

    desc->buffers.push_back(std::move(bufDesc));
}

void
HgiShaderFunctionAddConstantParam(
    HgiShaderFunctionDesc * const desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role,
    const std::string &attribute,
    const std::string &attributeIndex)
{
    HgiShaderFunctionParamDesc paramDesc;
    paramDesc.nameInShader = nameInShader;
    paramDesc.type = type;
    paramDesc.role = role;
    paramDesc.attribute = attribute;
    paramDesc.attributeIndex = attributeIndex;
    
    desc->constantParams.push_back(std::move(paramDesc));
}

void
HgiShaderFunctionAddStageInput(
    HgiShaderFunctionDesc * const desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role,
    const std::string &attribute,
    const std::string &attributeIndex)
{
    HgiShaderFunctionParamDesc paramDesc;
    paramDesc.nameInShader = nameInShader;
    paramDesc.type = type;
    paramDesc.role = role;
    paramDesc.attribute = attribute;
    paramDesc.attributeIndex = attributeIndex;
    
    desc->stageInputs.push_back(std::move(paramDesc));
}

void
HgiShaderFunctionAddStageOutput(
    HgiShaderFunctionDesc * const desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role,
    const std::string &attribute,
    const std::string &attributeIndex)
{
    HgiShaderFunctionParamDesc paramDesc;
    paramDesc.nameInShader = nameInShader;
    paramDesc.type = type;
    paramDesc.role = role;
    paramDesc.attribute = attribute;
    paramDesc.attributeIndex = attributeIndex;
    
    desc->stageOutputs.push_back(std::move(paramDesc));
}


PXR_NAMESPACE_CLOSE_SCOPE
