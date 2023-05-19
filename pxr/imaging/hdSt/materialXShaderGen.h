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
#ifndef PXR_IMAGING_HD_ST_MATERIALX_SHADER_GEN_H
#define PXR_IMAGING_HD_ST_MATERIALX_SHADER_GEN_H

#include "pxr/pxr.h"

#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#if MATERIALX_MAJOR_VERSION >= 1 && MATERIALX_MINOR_VERSION >= 38 && \
    MATERIALX_BUILD_VERSION >= 7
#include <MaterialXGenMsl/MslShaderGenerator.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

struct HdSt_MxShaderGenInfo;

/// \class HdStMaterialXShaderGen
///
/// Generates a shader for Storm with a surfaceShader function for a MaterialX 
/// network
/// Specialized versions for Glsl and Metal are below.
///
template<typename Base>
class HdStMaterialXShaderGen : public Base
{
public:
    HdStMaterialXShaderGen(HdSt_MxShaderGenInfo const& mxHdInfo);

    virtual MaterialX::ShaderPtr generate(const std::string& shaderName,
                           MaterialX::ElementPtr mxElement,
                           MaterialX::GenContext& mxContext) const override = 0;

    // Overriding this function to catch and adjust SurfaceNode code
    void emitLine(const std::string& str, 
                  MaterialX::ShaderStage& stage, 
                  bool semicolon = true) const override;

    // Helper to catch when we start/end emitting code for the SurfaceNode
    void SetEmittingSurfaceNode(bool emittingSurfaceNode) {
        _emittingSurfaceNode = emittingSurfaceNode;
    }

protected:
    // Helper functions to generate the Glslfx Shader
    void _EmitGlslfxHeader(MaterialX::ShaderStage& mxStage) const;

    void _EmitMxSurfaceShader(const MaterialX::ShaderGraph& mxGraph,
                              MaterialX::GenContext& mxContext,
                              MaterialX::ShaderStage& mxStage) const;

    // Helper functions to generate the conversion between Hd and Mx VertexData 
    void _EmitMxInitFunction(MaterialX::VariableBlock const& vertexData,
                             MaterialX::ShaderStage& mxStage) const;
    
    void _EmitMxVertexDataDeclarations(MaterialX::VariableBlock const& block,
                                       std::string const& mxVertexDataName,
                                       std::string const& mxVertexDataVariable,
                                       std::string const& separator,
                                       MaterialX::ShaderStage& mxStage) const;

    std::string _EmitMxVertexDataLine(const MaterialX::ShaderPort* variable,
                                      std::string const& separator) const;

    // Overriding the MaterialX function to make sure we initialize some Mx
    // variables with the appropriate Hd Value. 
    void emitVariableDeclarations(MaterialX::VariableBlock const& block,
                                  std::string const& qualifier,
                                  std::string const& separator, 
                                  MaterialX::GenContext& context, 
                                  MaterialX::ShaderStage& stage,
                                  bool assignValue = true) const override;

    // This method was introduced in MaterialX 1.38.5 and replaced the
    // emitInclude method. We add this method for older versions of MaterialX
    // for backwards compatibility.
    void emitLibraryInclude(const MaterialX::FilePath& filename,
                            MaterialX::GenContext& context,
                            MaterialX::ShaderStage& stage) const;

    void _EmitConstantsUniformsAndTypeDefs(
        MaterialX::GenContext& mxContext,
        MaterialX::ShaderStage& mxStage,
        const std::string& constQualifier) const;

    void _EmitDataStructsAndFunctionDefinitions(
        const MaterialX::ShaderGraph& mxGraph,
        MaterialX::GenContext& mxContext,
        MaterialX::ShaderStage& mxStage,
        MaterialX::StringMap* tokenSubstitutions) const;

    // Store MaterialX and Hydra counterparts and other Hydra specific info
    // to generate an appropriate glslfx header and properly initialize 
    // MaterialX values.
    MaterialX::StringMap _mxHdTextureMap;
    MaterialX::StringMap _mxHdPrimvarMap;
    MaterialX::StringMap _mxHdPrimvarDefaultValueMap;
    std::string _defaultTexcoordName;
    std::string _materialTag;
    bool _bindlessTexturesEnabled;

    // Helper to catch code for the SurfaceNode
    bool _emittingSurfaceNode;
};


/// \class HdStMaterialXShaderGenGlsl
///
/// Generates a glslfx shader with a surfaceShader function for a MaterialX 
/// network

class HdStMaterialXShaderGenGlsl
    : public HdStMaterialXShaderGen<MaterialX::GlslShaderGenerator>
{
public:
    HdStMaterialXShaderGenGlsl(HdSt_MxShaderGenInfo const& mxHdInfo);
    
    static MaterialX::ShaderGeneratorPtr create(
            HdSt_MxShaderGenInfo const& mxHdInfo) {
        return std::make_shared<HdStMaterialXShaderGenGlsl>(mxHdInfo);
    }
    
    MaterialX::ShaderPtr generate(const std::string& shaderName,
                           MaterialX::ElementPtr mxElement,
                           MaterialX::GenContext& mxContext) const override;

private:
    void _EmitGlslfxShader(const MaterialX::ShaderGraph& mxGraph,
                           MaterialX::GenContext& mxContext,
                           MaterialX::ShaderStage& mxStage) const;

    void _EmitMxFunctions(const MaterialX::ShaderGraph& mxGraph,
                          MaterialX::GenContext& mxContext,
                          MaterialX::ShaderStage& mxStage) const;
};


/// \class HdStMaterialXShaderGenMsl
///
/// Generates a Glslfx shader with some additional Metal code, and a 
/// surfaceShader function for a MaterialX network

#if MATERIALX_MAJOR_VERSION >= 1 && MATERIALX_MINOR_VERSION >= 38 && \
    MATERIALX_BUILD_VERSION >= 7
class HdStMaterialXShaderGenMsl
    : public HdStMaterialXShaderGen<MaterialX::MslShaderGenerator>
{
public:
    HdStMaterialXShaderGenMsl(HdSt_MxShaderGenInfo const& mxHdInfo);
    
    static MaterialX::ShaderGeneratorPtr create(
            HdSt_MxShaderGenInfo const& mxHdInfo) {
        return std::make_shared<HdStMaterialXShaderGenMsl>(mxHdInfo);
    }
    
    MaterialX::ShaderPtr generate(const std::string& shaderName,
                           MaterialX::ElementPtr mxElement,
                           MaterialX::GenContext& mxContext) const override;
private:
    void _EmitGlslfxMetalShader(const MaterialX::ShaderGraph& mxGraph,
                                MaterialX::GenContext& mxContext,
                                MaterialX::ShaderStage& mxStage) const;
    
    /// These two helper functions generate the Glslfx-Metal Shader
    void _EmitGlslfxMetalHeader(MaterialX::GenContext& mxContext,
                                MaterialX::ShaderStage& mxStage) const;
    
    void _EmitMxFunctions(const MaterialX::ShaderGraph& mxGraph,
                          MaterialX::GenContext& mxContext,
                          MaterialX::ShaderStage& mxStage) const;
};
#endif


PXR_NAMESPACE_CLOSE_SCOPE

#endif
