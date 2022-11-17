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

class Hgi;

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
    explicit HgiVulkanShaderGenerator(
        Hgi const *hgi,
        const HgiShaderFunctionDesc &descriptor);

    //This is not commonly consumed by the end user, but is available.
    HGIVULKAN_API
    HgiVulkanShaderSectionUniquePtrVector* GetShaderSections();

    template<typename SectionType, typename ...T>
    SectionType *CreateShaderSection(T && ...t);

protected:
    HGIVULKAN_API
    void _Execute(std::ostream &ss) override;

private:
    HgiVulkanShaderGenerator() = delete;
    HgiVulkanShaderGenerator & operator=(const HgiVulkanShaderGenerator&) = delete;
    HgiVulkanShaderGenerator(const HgiVulkanShaderGenerator&) = delete;

    void _WriteVersion(std::ostream &ss);

    void _WriteExtensions(std::ostream &ss);
    
    void _WriteMacros(std::ostream &ss);

    void _WriteConstantParams(
        const HgiShaderFunctionParamDescVector &parameters);

    void _WriteTextures(const HgiShaderFunctionTextureDescVector& textures);
	
    void _WriteBuffers(const HgiShaderFunctionBufferDescVector &buffers);

    //For writing shader inputs and outputs who are very similarly written
    void _WriteInOuts(
        const HgiShaderFunctionParamDescVector &parameters,
        const std::string &qualifier);
    void _WriteInOutBlocks(
        const HgiShaderFunctionParamBlockDescVector &parameterBlocks,
        const std::string &qualifier);
    
    HgiVulkanShaderSectionUniquePtrVector _shaderSections;
    Hgi const *_hgi;
    uint32_t _textureBindIndexStart;
    uint32_t _inLocationIndex;
    uint32_t _outLocationIndex;
    std::vector<std::string> _shaderLayoutAttributes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
