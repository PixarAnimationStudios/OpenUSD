//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HGIWEBGPU_SHADERGENERATOR_H
#define PXR_IMAGING_HGIWEBGPU_SHADERGENERATOR_H

#include "pxr/imaging/hgi/shaderGenerator.h"
#include "pxr/imaging/hgiWebGPU/shaderSection.h"
#include "pxr/imaging/hgiWebGPU/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiWebGPU;

/// \class HgiWebGPUShaderGenerator
///
/// Takes in a descriptor and spits out GLSL code through it's execute function.
///
class HgiWebGPUShaderGenerator final: public HgiShaderGenerator
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUShaderGenerator(
        HgiWebGPU const *hgi,
        const HgiShaderFunctionDesc &descriptor);

    //This is not commonly consumed by the end user, but is available.
    HGIWEBGPU_API
    HgiWebGPUShaderSectionUniquePtrVector* GetShaderSections();

    template<typename SectionType, typename ...T>
    SectionType *CreateShaderSection(T && ...t);

protected:
    HGIWEBGPU_API
    void _Execute(std::ostream &ss) override;

private:
    HgiWebGPUShaderGenerator() = delete;
    HgiWebGPUShaderGenerator & operator=(const HgiWebGPUShaderGenerator&) = delete;
    HgiWebGPUShaderGenerator(const HgiWebGPUShaderGenerator&) = delete;

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

    HgiWebGPUShaderSectionUniquePtrVector _shaderSections;
    HgiWebGPU const *_hgi;
    std::vector<std::string> _shaderLayoutAttributes;
    uint32_t _inLocationIndex;
    uint32_t _outLocationIndex;
    std::string _version;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
