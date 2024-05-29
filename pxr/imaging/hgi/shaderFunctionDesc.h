//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// <li>textureType:
///   Type of the texture (e.g. array texture, shadow, etc.).</li>
/// <li>bindIndex:
///   The index of the resource.</li>
/// <li>arraySize:
///   If arraySize > 0, indicates the size of the array. Note that textureType 
///   = HgiTextureTypeTexture and arraySize = 2 would create an array with two 
///   textures whereas textureType  = HgiTextureTypeArrayTexture and arraySize 
///   = 0 would create a single array texture (arrays of array textures are 
///   supported as well).</li>
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
    HgiShaderTextureType textureType;
    uint32_t bindIndex;
    size_t arraySize;
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
    uint32_t bindIndex;
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
/// Describes a param passed into a shader or between shader stages.
///
/// <ul>
/// <li>nameInShader:
///   The name written from the codegen into the shader file for the param.</li>
/// <li>type:
///   Type of the param within the shader file.</li>
/// <li>location:
///   For OpenGL, optionally specify the layout location.
///   For Metal, if no role is specified, this generates the role</li>
/// <li>interstageSlot:
///   Optionally specify an index for interstage parameters.</li>
/// <li>interpolation:
///   Optionally specify the interpolation: Default, Flat or NoPerspective.</li>
/// <li>sampling:
///   Optionally specify the sampling: Default, Centroid or Sample.</li>
/// <li>storage:
///   Optionally specify the storage type: Default, Patch.</li>
/// <li>role:
///   Optionally a role can be specified, like position, uv, color.</li>
/// <li>arraySize:
///   If specified, generates an array type parameter with given size.</li>
/// </ul>
///
struct HgiShaderFunctionParamDesc
{
    HGI_API
    HgiShaderFunctionParamDesc();

    std::string nameInShader;
    std::string type;
    int32_t location;
    int32_t interstageSlot;
    HgiInterpolationType interpolation;
    HgiSamplingType sampling;
    HgiStorageType storage;
    std::string role;
    std::string arraySize;
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

/// \struct HgiShaderFunctionParamBlockDesc
///
/// Describes an interstage param block between shader stages
///
/// <ul>
/// <li>blockName:
///   The name used to match blocks between shader stages.</li>
/// <li>instanceName:
///   The name used to scope access to block members.</li>
/// <li>members:
///   The members of the block.</li>
/// <li>arraySize:
///   If specified, generates a block with given size.</li>
/// <li>interstageSlot:
///   The interstage slot index of the first member of the block,
///   subsequent members are assigned sequential interstage slot indices.</li>
/// </ul>
///
struct HgiShaderFunctionParamBlockDesc
{
    HGI_API
    HgiShaderFunctionParamBlockDesc();

    struct Member {
        std::string name;
        std::string type;
        HgiInterpolationType interpolation;
        HgiSamplingType sampling;
    };
    using MemberVector = std::vector<Member>;

    std::string blockName;
    std::string instanceName;
    MemberVector members;
    std::string arraySize;
    int32_t interstageSlot;
};

using HgiShaderFunctionParamBlockDescVector =
    std::vector<HgiShaderFunctionParamBlockDesc>;

HGI_API
bool operator==(
    const HgiShaderFunctionParamBlockDesc& lhs,
    const HgiShaderFunctionParamBlockDesc& rhs);

HGI_API
bool operator!=(
    const HgiShaderFunctionParamBlockDesc& lhs,
    const HgiShaderFunctionParamBlockDesc& rhs);

HGI_API
bool operator==(
    const HgiShaderFunctionParamBlockDesc::Member& lhs,
    const HgiShaderFunctionParamBlockDesc::Member& rhs);

HGI_API
bool operator!=(
    const HgiShaderFunctionParamBlockDesc::Member& lhs,
    const HgiShaderFunctionParamBlockDesc::Member& rhs);

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
/// <li>spacing
///   The spacing used by the tessellation primitive generator</li>
/// <li>ordering
///   The ordering used by the tessellation primitive generator</li>
/// <li>numVertsInPerPatch:
///   The number of vertices in per patch</li>
/// <li>numVertsOutPerPatch:
///   The number of vertices out per patch</li>
/// </ul>
///
struct HgiShaderFunctionTessellationDesc
{
    enum class PatchType { Triangles, Quads, Isolines };
    enum class Spacing { Equal, FractionalEven, FractionalOdd };
    enum class Ordering { CW, CCW };
    HGI_API
    HgiShaderFunctionTessellationDesc();

    PatchType patchType;
    Spacing spacing;
    Ordering ordering;
    std::string numVertsPerPatchIn;
    std::string numVertsPerPatchOut;
};

HGI_API
bool operator==(
        const HgiShaderFunctionTessellationDesc& lhs,
        const HgiShaderFunctionTessellationDesc& rhs);

HGI_API
bool operator!=(
        const HgiShaderFunctionTessellationDesc& lhs,
        const HgiShaderFunctionTessellationDesc& rhs);

/// \struct HgiShaderFunctionGeometryDesc
///
/// Describes a geometry function's description
///
/// <ul>
/// <li>inPrimitiveType:
///   The input primitive type.</li>
/// <li>outPrimitiveType:
///   The output primitive type.</li>
/// <li>outMaxVertices:
///   The maximum number of vertices written by a single invovation of the 
///   geometry shader.</li>
/// </ul>
///
struct HgiShaderFunctionGeometryDesc
{
    enum class InPrimitiveType { 
        Points, Lines, LinesAdjacency, Triangles, TrianglesAdjacency };
    enum class OutPrimitiveType { 
        Points, LineStrip, TriangleStrip };

    HGI_API
    HgiShaderFunctionGeometryDesc();

    InPrimitiveType inPrimitiveType;
    OutPrimitiveType outPrimitiveType;
    std::string outMaxVertices;
};

HGI_API
bool operator==(
        const HgiShaderFunctionGeometryDesc& lhs,
        const HgiShaderFunctionGeometryDesc& rhs);

HGI_API
bool operator!=(
        const HgiShaderFunctionGeometryDesc& lhs,
        const HgiShaderFunctionGeometryDesc& rhs);

///
/// Describes a fragment function's description
///
/// <ul>
/// <li>earlyFragmentTests:
///   Fragment shader tests will be performed before fragment
///   shader execution when enabled.</li>
/// </ul>
///
struct HgiShaderFunctionFragmentDesc
{
    HGI_API
    HgiShaderFunctionFragmentDesc();

    bool earlyFragmentTests;

};

HGI_API
bool operator==(
        const HgiShaderFunctionFragmentDesc& lhs,
        const HgiShaderFunctionFragmentDesc& rhs);

HGI_API
bool operator!=(
        const HgiShaderFunctionFragmentDesc& lhs,
        const HgiShaderFunctionFragmentDesc& rhs);

/// \struct HgiShaderFunctionDesc
///
/// Describes the properties needed to create a GPU shader function.
///
/// <ul>
/// <li>debugName:
///   This label can be applied as debug label for gpu debugging.</li>
/// <li>shaderStage:
///   The shader stage this function represents.</li>
/// <li>shaderCodeDeclarations:
///   Optional ascii shader code containing defines and type declarations
///   which need to be emitted before generated resource bindings.</li>
/// <li>shaderCode:
///   The ascii shader code used to compile the shader.</li>
/// <li>generatedShaderCodeOut:
///   Optional pointer to a string that will be filled in with the
///   ascii shader code after shader generation is complete.</li>
/// <li>textures:
///   List of texture descriptions to be passed into a shader.</li>
/// <li>buffers:
///   List of buffer descriptions to be passed into a shader.</li>
/// <li>constantParams:
///   List of descriptions of constant params passed into a shader.</li>
/// <li>stageGlobalMembers:
///   List of descriptions of params declared at global scope.</li>
/// <li>stageInputs:
///   List of descriptions of the inputs of the shader.</li>
/// <li>stageOutputs:
///   List of descriptions of the outputs of the shader.</li>
/// <li>stageInputBlocks:
///   List of descriptions of the input blocks of the shader.</li>
/// <li>stageOutputsBlocks:
///   List of descriptions of the output blocks of the shader.</li>
/// <li>computeDescriptor:
///   Description of compute shader function.</li>
/// <li>tessellationDescriptor:
///   Description of tessellation shader function.</li>
/// <li>geometryDescriptor:
///   Description of geometry shader function.</li>
/// <li>fragmentDescriptor:
///   Description of fragment shader function.</li>
/// </ul>
///
struct HgiShaderFunctionDesc
{
    HGI_API
    HgiShaderFunctionDesc();
    std::string debugName;
    HgiShaderStage shaderStage;
    const char *shaderCodeDeclarations;
    const char *shaderCode;
    std::string *generatedShaderCodeOut;
    std::vector<HgiShaderFunctionTextureDesc> textures;
    std::vector<HgiShaderFunctionBufferDesc> buffers;
    std::vector<HgiShaderFunctionParamDesc> constantParams;
    std::vector<HgiShaderFunctionParamDesc> stageGlobalMembers;
    std::vector<HgiShaderFunctionParamDesc> stageInputs;
    std::vector<HgiShaderFunctionParamDesc> stageOutputs;
    std::vector<HgiShaderFunctionParamBlockDesc> stageInputBlocks;
    std::vector<HgiShaderFunctionParamBlockDesc> stageOutputBlocks;
    HgiShaderFunctionComputeDesc computeDescriptor;
    HgiShaderFunctionTessellationDesc tessellationDescriptor;
    HgiShaderFunctionGeometryDesc geometryDescriptor;
    HgiShaderFunctionFragmentDesc fragmentDescriptor;
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
    const uint32_t bindIndex = 0,
    uint32_t dimensions = 2,
    const HgiFormat &format = HgiFormatFloat32Vec4,
    const HgiShaderTextureType textureType = HgiShaderTextureTypeTexture);

/// Adds array of textures descriptor to given shader function descriptor.
HGI_API
void
HgiShaderFunctionAddArrayOfTextures(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const uint32_t arraySize,
    const uint32_t bindIndex = 0,
    const uint32_t dimensions = 2,
    const HgiFormat &format = HgiFormatFloat32Vec4,
    const HgiShaderTextureType textureType = HgiShaderTextureTypeTexture);

/// Adds writable texture descriptor to given shader function descriptor.
HGI_API
void
HgiShaderFunctionAddWritableTexture(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const uint32_t bindIndex = 0,
    const uint32_t dimensions = 2,
    const HgiFormat &format = HgiFormatFloat32Vec4,
    const HgiShaderTextureType textureType = HgiShaderTextureTypeTexture);

/// Adds buffer descriptor to given shader function descriptor.
HGI_API
void
HgiShaderFunctionAddBuffer(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const uint32_t bindIndex,
    HgiBindingType binding,
    const uint32_t arraySize = 0);

/// Adds buffer descriptor to given shader function descriptor.
HGI_API
void
HgiShaderFunctionAddWritableBuffer(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const uint32_t bindIndex);

/// Adds constant function param descriptor to given shader function
/// descriptor.
HGI_API
void
HgiShaderFunctionAddConstantParam(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role = std::string());

/// Adds stage input function param descriptor to given shader function
/// descriptor.
/// The location is will be set to the next available.
HGI_API
void
HgiShaderFunctionAddStageInput(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role = std::string());

/// Adds stage input function param descriptor to given shader function
/// descriptor given param descriptor.
HGI_API
void
HgiShaderFunctionAddStageInput(
        HgiShaderFunctionDesc *functionDesc,
        HgiShaderFunctionParamDesc const &paramDesc);

/// Interstage input.
HGI_API
void
HgiShaderFunctionAddGlobalVariable(
   HgiShaderFunctionDesc *desc,
   const std::string &nameInShader,
   const std::string &type,
   const std::string &arraySize);

/// Adds stage output function param descriptor to given shader function
/// descriptor.
HGI_API
void
HgiShaderFunctionAddStageOutput(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const std::string &role = std::string(),
    const std::string &arraySize = std::string());

/// Adds stage output function param descriptor to given shader function
/// descriptor.
HGI_API
void
HgiShaderFunctionAddStageOutput(
    HgiShaderFunctionDesc *desc,
    const std::string &nameInShader,
    const std::string &type,
    const uint32_t location);

/// Adds stage output function param descriptor to given shader function
/// descriptor given param descriptor.
HGI_API
void
HgiShaderFunctionAddStageOutput(
        HgiShaderFunctionDesc *functionDesc,
        HgiShaderFunctionParamDesc const &paramDesc);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
