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
/// <li>type:
///   if it's a texture of floats, ints or any allowed type.</li>
/// </ul>
///
struct HgiShaderFunctionTextureDesc
{
    HGI_API
    HgiShaderFunctionTextureDesc();

    std::string nameInShader;
    uint32_t dimensions;
    std::string type;
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
/// <li>constantParams:
///   List of descriptions of constant params passed into a shader.</li>
/// <li>stageInputs:
///   List of descriptions of the inputs of the shader.</li>
/// <li>stageOutputs:
///   List of descriptions of the outputs of the shader.</li>
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
    std::vector<HgiShaderFunctionParamDesc> constantParams;
    std::vector<HgiShaderFunctionParamDesc> stageInputs;
    std::vector<HgiShaderFunctionParamDesc> stageOutputs;
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
    const std::string &type = "float");

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
