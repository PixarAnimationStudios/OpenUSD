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
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"

PXR_NAMESPACE_OPEN_SCOPE

enum LoFiGeometricProgramType {
    LOFI_PROGRAM_MESH,
    LOFI_PROGRAM_CURVE,
    LOFI_PROGRAM_POINT,
    LOFI_PROGRAM_INSTANCE
};

typedef std::vector<LoFiVertexBufferChannel> LoFiVertexBufferChannelList;

class LoFiCodeGen
{
public:
    typedef size_t ID;

    /// Constructor.
    LoFiCodeGen(LoFiGeometricProgramType type, 
        const LoFiVertexBufferChannelList& channels);
    
    /// Return the hash value of glsl shader to be generated.
    ID ComputeHash() const;
    
    /// Return the generated vertex shader source
    const std::string &GetVertexShaderCode() const { return _vertexCode; }

    /// Return the generated geometry shader source
    const std::string &GetGeometryShaderCode() const { return _geometryCode; }

    /// Return the generated fragment shader source
    const std::string &GetFragmentShadeCode() const { return _fragmentCode; }

    void GenerateMeshCode();

private:
    
    void _GenerateVersion(std::stringstream& ss);
    void _AddInputChannel(std::stringstream& ss, LoFiVertexBufferChannel channel,
        size_t index, TfToken& type);
    void _AddInputUniform(std::stringstream& ss, const std::string& name, 
        TfToken& type);
    void _AddInputAttribute(std::stringstream& ss,  LoFiVertexBufferChannel channel, 
        TfToken& type);
    void _AddOutputAttribute(std::stringstream& ss, LoFiVertexBufferChannel channel,
        TfToken& type);

    // channels
    LoFiVertexBufferChannelList _channels;

    // source buckets
    std::stringstream _vertex, _geometry, _fragment;

    // generated codes (for diagnostics)
    std::string                 _vertexCode;
    std::string                 _geometryCode;
    std::string                 _fragmentCode;

    size_t                      _glslVersion;
    LoFiGeometricProgramType    _type;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_SHADER_CODE_H
