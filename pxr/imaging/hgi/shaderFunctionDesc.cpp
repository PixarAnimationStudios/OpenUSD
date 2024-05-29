//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/shaderFunctionDesc.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiShaderFunctionTextureDesc::HgiShaderFunctionTextureDesc()
  : dimensions(2)
  , format(HgiFormatInvalid)
  , textureType(HgiShaderTextureTypeTexture)
  , bindIndex(0)
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

HgiShaderFunctionComputeDesc::HgiShaderFunctionComputeDesc()
    : localSize(GfVec3i(0, 0, 0))
{ 
}

HgiShaderFunctionParamDesc::HgiShaderFunctionParamDesc()
  : location(-1)
  , interstageSlot(-1)
  , interpolation(HgiInterpolationDefault)
  , sampling(HgiSamplingDefault)
  , storage(HgiStorageDefault)
{
}

HgiShaderFunctionParamBlockDesc::HgiShaderFunctionParamBlockDesc()
  : interstageSlot(-1)
{
}

HgiShaderFunctionTessellationDesc::HgiShaderFunctionTessellationDesc()
  : patchType(PatchType::Triangles)
  , spacing(Spacing::Equal)
  , ordering(Ordering::CCW)
  , numVertsPerPatchIn("3")
  , numVertsPerPatchOut("3")
{
}

HgiShaderFunctionDesc::HgiShaderFunctionDesc() 
  : shaderStage(0)
  , shaderCodeDeclarations(nullptr)
  , shaderCode(nullptr)
  , generatedShaderCodeOut(nullptr)
  , textures()
  , constantParams()
  , stageInputs()
  , stageOutputs()
  , computeDescriptor()
  , tessellationDescriptor()
  , geometryDescriptor()
  , fragmentDescriptor()
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

HgiShaderFunctionGeometryDesc::HgiShaderFunctionGeometryDesc() 
  : inPrimitiveType(InPrimitiveType::Triangles)
  , outPrimitiveType(OutPrimitiveType::TriangleStrip)
  , outMaxVertices("3")
{
}

bool operator==(
    const HgiShaderFunctionGeometryDesc& lhs,
    const HgiShaderFunctionGeometryDesc& rhs)
{
    return lhs.inPrimitiveType == rhs.inPrimitiveType &&
           lhs.outPrimitiveType == rhs.outPrimitiveType &&
           lhs.outMaxVertices == rhs.outMaxVertices;
}

bool operator!=(
    const HgiShaderFunctionGeometryDesc& lhs,
    const HgiShaderFunctionGeometryDesc& rhs)
{
    return !(lhs == rhs);
}

HgiShaderFunctionFragmentDesc::HgiShaderFunctionFragmentDesc()
    : earlyFragmentTests(false)
{
}

bool operator==(
    const HgiShaderFunctionFragmentDesc& lhs,
    const HgiShaderFunctionFragmentDesc& rhs)
{
    return lhs.earlyFragmentTests == rhs.earlyFragmentTests;
}

bool operator!=(
    const HgiShaderFunctionFragmentDesc& lhs,
    const HgiShaderFunctionFragmentDesc& rhs)
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
           lhs.location == rhs.location &&
           lhs.interstageSlot == rhs.interstageSlot &&
           lhs.interpolation == rhs.interpolation &&
           lhs.sampling == rhs.sampling &&
           lhs.storage == rhs.storage &&
           lhs.role == rhs.role &&
           lhs.arraySize == rhs.arraySize;
}

bool operator!=(
    const HgiShaderFunctionParamDesc& lhs,
    const HgiShaderFunctionParamDesc& rhs)
{
    return !(lhs == rhs);
}

bool operator==(
    const HgiShaderFunctionParamBlockDesc& lhs,
    const HgiShaderFunctionParamBlockDesc& rhs)
{
    return lhs.blockName == rhs.blockName &&
           lhs.instanceName == rhs.instanceName &&
           lhs.members == rhs.members &&
           lhs.arraySize == rhs.arraySize &&
           lhs.interstageSlot == rhs.interstageSlot;
}

bool operator!=(
    const HgiShaderFunctionParamBlockDesc& lhs,
    const HgiShaderFunctionParamBlockDesc& rhs)
{
    return !(lhs == rhs);
}

bool operator==(
    const HgiShaderFunctionParamBlockDesc::Member& lhs,
    const HgiShaderFunctionParamBlockDesc::Member& rhs)
{
    return lhs.name == rhs.name &&
           lhs.type == rhs.type;
}

bool operator!=(
    const HgiShaderFunctionParamBlockDesc::Member& lhs,
    const HgiShaderFunctionParamBlockDesc::Member& rhs)
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
           lhs.spacing == rhs.spacing &&
           lhs.ordering == rhs.ordering &&
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
           // lhs.shaderCodeDeclarations == rhs.shaderCodeDeclarations
           // lhs.shaderCode == rhs.shaderCode
           // lhs.generatedShaderCodeOut == rhs.generatedShaderCodeOut
           lhs.textures == rhs.textures &&
           lhs.constantParams == rhs.constantParams &&
           lhs.stageInputs == rhs.stageInputs &&
           lhs.stageOutputs == rhs.stageOutputs &&
           lhs.computeDescriptor == rhs.computeDescriptor &&
           lhs.tessellationDescriptor == rhs.tessellationDescriptor &&
           lhs.geometryDescriptor == rhs.geometryDescriptor &&
           lhs.fragmentDescriptor == rhs.fragmentDescriptor;
}

bool operator!=(
    const HgiShaderFunctionDesc& lhs,
    const HgiShaderFunctionDesc& rhs)
{
    return !(lhs == rhs);
}

void
HgiShaderFunctionAddTexture(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const uint32_t bindIndex /* = 0 */,
    const uint32_t dimensions /* = 2 */,
    const HgiFormat &format /* = HgiFormatFloat32Vec4*/,
    const HgiShaderTextureType textureType /* = HgiShaderTextureTypeTexture */)
{
    HgiShaderFunctionTextureDesc texDesc;
    texDesc.nameInShader = nameInShader;
    texDesc.bindIndex = bindIndex;
    texDesc.dimensions = dimensions;
    texDesc.format = format;
    texDesc.textureType = textureType;
    texDesc.arraySize = 0;
    texDesc.writable = false;

    desc->textures.push_back(std::move(texDesc));
}

void
HgiShaderFunctionAddArrayOfTextures(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const uint32_t arraySize,
    const uint32_t bindIndex /* = 0 */,
    const uint32_t dimensions /* = 2 */,
    const HgiFormat &format /* = HgiFormatFloat32Vec4*/,
    const HgiShaderTextureType textureType /* = HgiShaderTextureTypeTexture */)
{
    HgiShaderFunctionTextureDesc texDesc;
    texDesc.nameInShader = nameInShader;
    texDesc.bindIndex = bindIndex;
    texDesc.dimensions = dimensions;
    texDesc.format = format;
    texDesc.textureType = textureType;
    texDesc.arraySize = arraySize;
    texDesc.writable = false;

    desc->textures.push_back(std::move(texDesc));
}

void
HgiShaderFunctionAddWritableTexture(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const uint32_t bindIndex /* = 0 */,
    const uint32_t dimensions /* = 2 */,
    const HgiFormat &format /* = HgiFormatFloat32Vec4*/,
    const HgiShaderTextureType textureType /* = HgiShaderTextureTypeTexture */)
{
    HgiShaderFunctionTextureDesc texDesc;
    texDesc.nameInShader = nameInShader;
    texDesc.bindIndex = bindIndex;
    texDesc.dimensions = dimensions;
    texDesc.format = format;
    texDesc.textureType = textureType;
    texDesc.arraySize = 0;
    texDesc.writable = true;

    desc->textures.push_back(std::move(texDesc));
}

void
HgiShaderFunctionAddBuffer(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const uint32_t bindIndex,
    const HgiBindingType binding,
    const uint32_t arraySize)
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
    HgiShaderFunctionDesc *desc,
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
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role)
{
    HgiShaderFunctionParamDesc paramDesc;
    paramDesc.nameInShader = nameInShader;
    paramDesc.type = type;
    paramDesc.role = role;
    
    desc->constantParams.push_back(std::move(paramDesc));
}

void
HgiShaderFunctionAddStageInput(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role)
{
    HgiShaderFunctionParamDesc paramDesc;
    paramDesc.nameInShader = nameInShader;
    paramDesc.type = type;
    paramDesc.role = role;

    desc->stageInputs.push_back(std::move(paramDesc));
}

void
HgiShaderFunctionAddStageInput(
        HgiShaderFunctionDesc *functionDesc,
        HgiShaderFunctionParamDesc const &paramDesc)
{
    functionDesc->stageInputs.push_back(paramDesc);
}

void
HgiShaderFunctionAddGlobalVariable(
   HgiShaderFunctionDesc *desc,
   const std::string &nameInShader,
   const std::string &type,
   const std::string &arraySize)
{
    HgiShaderFunctionParamDesc paramDesc;
    paramDesc.nameInShader = nameInShader;
    paramDesc.type = type;
    paramDesc.arraySize = arraySize;
    desc->stageGlobalMembers.push_back(std::move(paramDesc));
}

void
HgiShaderFunctionAddStageOutput(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role,
    const std::string &arraySize)
{
    HgiShaderFunctionParamDesc paramDesc;
    paramDesc.nameInShader = nameInShader;
    paramDesc.type = type;
    paramDesc.role = role;
    paramDesc.arraySize = arraySize;

    desc->stageOutputs.push_back(std::move(paramDesc));
}

void
HgiShaderFunctionAddStageOutput(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const uint32_t location)
{
    HgiShaderFunctionParamDesc paramDesc;
    paramDesc.nameInShader = nameInShader;
    paramDesc.type = type;
    paramDesc.location = location;

    desc->stageOutputs.push_back(std::move(paramDesc));
}

void
HgiShaderFunctionAddStageOutput(
        HgiShaderFunctionDesc *functionDesc,
        HgiShaderFunctionParamDesc const &paramDesc)
{
    functionDesc->stageOutputs.push_back(paramDesc);
}


PXR_NAMESPACE_CLOSE_SCOPE
