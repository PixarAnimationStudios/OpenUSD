//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HGIGL_SHADERGENERATOR_H
#define PXR_IMAGING_HGIGL_SHADERGENERATOR_H

#include "pxr/imaging/hgi/shaderGenerator.h"
#include "pxr/imaging/hgiGL/shaderSection.h"
#include "pxr/imaging/hgiGL/api.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

using HgiGLShaderSectionUniquePtrVector =
    std::vector<std::unique_ptr<HgiGLShaderSection>>;

/// \class HgiGLShaderGenerator
///
/// Takes in a descriptor and spits out GLSL code through it's execute function.
///
class HgiGLShaderGenerator final: public HgiShaderGenerator
{
public:
    HGIGL_API
    explicit HgiGLShaderGenerator(
        Hgi const *hgi,
        const HgiShaderFunctionDesc &descriptor);

    //This is not commonly consumed by the end user, but is available.
    HGIGL_API
    HgiGLShaderSectionUniquePtrVector* GetShaderSections();

    template<typename SectionType, typename ...T>
    SectionType *CreateShaderSection(T && ...t);

protected:
    HGIGL_API
    void _Execute(std::ostream &ss) override;

private:
    HgiGLShaderGenerator() = delete;
    HgiGLShaderGenerator & operator=(const HgiGLShaderGenerator&) = delete;
    HgiGLShaderGenerator(const HgiGLShaderGenerator&) = delete;

    void _WriteVersion(std::ostream &ss);
    void _WriteExtensions(std::ostream &ss);
    void _WriteMacros(std::ostream &ss);

    void _WriteTextures(const HgiShaderFunctionTextureDescVector &textures);

    void _WriteBuffers(const HgiShaderFunctionBufferDescVector &buffers);

    void _WriteConstantParams(
        const HgiShaderFunctionParamDescVector &parameters);

    //For writing shader inputs and outputs who are very similarly written
    void _WriteInOuts(
        const HgiShaderFunctionParamDescVector &parameters,
        const std::string &qualifier);
    void _WriteInOutBlocks(
        const HgiShaderFunctionParamBlockDescVector &parameterBlocks,
        const std::string &qualifier);
    
    Hgi const *_hgi;
    HgiGLShaderSectionUniquePtrVector _shaderSections;
    std::vector<std::string> _shaderLayoutAttributes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
