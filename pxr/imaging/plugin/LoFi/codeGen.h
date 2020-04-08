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

enum LoFiGeometricProgramType {
    LOFI_PROGRAM_MESH,
    LOFI_PROGRAM_CURVE,
    LOFI_PROGRAM_POINT,
    LOFI_PROGRAM_INSTANCE
};

typedef std::vector<LoFiAttributeChannel> LoFiAttributeChannelList;

class LoFiCodeGen
{
public:
    typedef size_t ID;

    /// Constructor.
    LoFiCodeGen(LoFiGeometricProgramType type, 
        const LoFiShaderCodeSharedPtrList& shaders);

    LoFiCodeGen(LoFiGeometricProgramType type, 
        const LoFiBindingList& uniformBindings,
        const LoFiBindingList& vertexBufferBindings,
        const LoFiShaderCodeSharedPtrList& shaders);
    
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
                            bool indexed=false);
    
    void _EmitStageEmittor(std::stringstream &ss,
                          TfToken const &stage,
                          TfToken const &name,
                          TfToken const &type,
                          int arraySize,
                          int index = -1);

    
    void _GenerateVersion();
    void _GenerateCommon();
    void _GenerateConstants();
    void _GeneratePrimvars(bool hasGeometryShader);
    void _GenerateUniforms();
    void _GenerateResults();

    // shader code
    LoFiShaderCodeSharedPtrList _shaders;

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
    LoFiGeometricProgramType    _type;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_SHADER_CODE_H
