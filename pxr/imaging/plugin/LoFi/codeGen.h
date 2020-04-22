//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_SHADER_CODE_H
#define PXR_IMAGING_PLUGIN_LOFI_SHADER_CODE_H

#include "pxr/pxr.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>
#include "pxr/imaging/plugin/LoFi/binding.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef std::vector<LoFiAttributeChannel> LoFiAttributeChannelList;

class LoFiCodeGen
{
public:
    typedef size_t ID;

    /// Constructor.
    LoFiCodeGen(LoFiProgramType type, 
        const LoFiShaderCodeSharedPtr& shader);

    LoFiCodeGen(LoFiProgramType type, 
        const LoFiBindingList& uniformBindings,
        const LoFiBindingList& vertexBufferBindings,
        const LoFiShaderCodeSharedPtr& shader);
    
    /// Return the hash value of glsl shader to be generated.
    ID ComputeHash() const;
    
    /// Return the generated vertex shader source
    const std::string& GetVertexShaderCode() const { return _vertexCode; }

    /// Return the generated geometry shader source
    const std::string& GetGeometryShaderCode() const { return _geometryCode; }

    /// Return the generated fragment shader source
    const std::string& GetFragmentShaderCode() const { return _fragmentCode; }

    void GenerateProgramCode();

private:
    void _EmitDeclaration(  std::stringstream &ss,
                            TfToken const &name,
                            TfToken const &type,
                            LoFiBinding const &binding,
                            size_t arraySize = 0);

    void _EmitAccessor  (std::stringstream &ss,
                        TfToken const &name,
                        TfToken const &type,
                        LoFiBinding const &binding);

    void _EmitStageAccessor(std::stringstream &str,
                            TfToken const &stage,
                            TfToken const &name,
                            TfToken const &type,
                            int arraySize,
                            int* index=NULL);
    
    void _EmitStageEmittor(std::stringstream &ss,
                          TfToken const &stage,
                          TfToken const &name,
                          TfToken const &type,
                          int arraySize,
                          int* index=NULL);

    
    void _GenerateVersion();
    void _GeneratePrimvars(bool hasGeometryShader);
    void _GenerateUniforms();
    void _GenerateResults();

    // shader code
    LoFiShaderCodeSharedPtr     _shaderCode;

    // bindings
    LoFiBindingList             _uniformBindings;
    LoFiBindingList             _textureBindings;
    LoFiBindingList             _attributeBindings;

    // source buckets
    std::stringstream _genCommon, _genVS, _genGS, _genFS;

    // generated codes
    std::string                 _vertexCode;
    std::string                 _geometryCode;
    std::string                 _fragmentCode;

    size_t                      _glslVersion;
    LoFiProgramType             _type;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_SHADER_CODE_H
