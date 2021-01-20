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

#ifndef PXR_IMAGING_HGIVULKAN_SHADERGENERATOR_H
#define PXR_IMAGING_HGIVULKAN_SHADERGENERATOR_H

#include "pxr/imaging/hgi/shaderGenerator.h"
#include "pxr/imaging/hgiVulkan/shaderSection.h"
#include "pxr/imaging/hgiVulkan/api.h"

PXR_NAMESPACE_OPEN_SCOPE

using HgiVulkanShaderSectionUniquePtrVector =
    std::vector<std::unique_ptr<HgiVulkanShaderSection>>;

/// \class HgiVulkanShaderGenerator
///
/// Takes in a descriptor and spits out GLSL code through it's execute function.
///
class HgiVulkanShaderGenerator final: public HgiShaderGenerator
{
public:
    HGIVULKAN_API
    explicit HgiVulkanShaderGenerator(const HgiShaderFunctionDesc &descriptor);

    //This is not commonly consumed by the end user, but is available.
    HGIVULKAN_API
    HgiVulkanShaderSectionUniquePtrVector* GetShaderSections();

protected:
    HGIVULKAN_API
    void _Execute(
        std::ostream &ss,
        const std::string &originalShaderShader) override;

private:
    HgiVulkanShaderGenerator() = delete;
    HgiVulkanShaderGenerator & operator=(const HgiVulkanShaderGenerator&) = delete;
    HgiVulkanShaderGenerator(const HgiVulkanShaderGenerator&) = delete;

    void _WriteConstantParams(
        const HgiShaderFunctionParamDescVector &parameters);

    void _WriteTextures(const HgiShaderFunctionTextureDescVector& textures);

    //For writing shader inputs and outputs who are very similarly written
    void _WriteInOuts(
        const HgiShaderFunctionParamDescVector &parameters,
        const std::string &qualifier);
    
    HgiVulkanShaderSectionUniquePtrVector _shaderSections;
    uint32_t _bindIndex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
