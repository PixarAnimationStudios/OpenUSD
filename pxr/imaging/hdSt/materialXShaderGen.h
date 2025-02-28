//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MATERIALX_SHADER_GEN_H
#define PXR_IMAGING_HD_ST_MATERIALX_SHADER_GEN_H

#include "pxr/pxr.h"

#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenMsl/MslShaderGenerator.h>
#include <MaterialXGenGlsl/VkShaderGenerator.h>

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
/// network, targeting OpenGL GLSL.

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

/// \class HdStMaterialXShaderGenVkGlsl
///
/// Generates a glslfx shader with a surfaceShader function for a MaterialX 
/// network, targeting Vulkan GLSL.

class HdStMaterialXShaderGenVkGlsl
    : public HdStMaterialXShaderGen<MaterialX::VkShaderGenerator>
{
public:
    HdStMaterialXShaderGenVkGlsl(HdSt_MxShaderGenInfo const& mxHdInfo);
    
    static MaterialX::ShaderGeneratorPtr create(
            HdSt_MxShaderGenInfo const& mxHdInfo) {
        return std::make_shared<HdStMaterialXShaderGenVkGlsl>(mxHdInfo);
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
/// Generates a glslfx shader with a surfaceShader function for a MaterialX 
/// network, targeting Metal Shading Language.

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

// Helper functions to aid building both MaterialX 1.38.X and 1.39.X
// once MaterialX 1.38.X is no longer required these should likely be removed.
namespace HdStMaterialXHelpers {

bool MxTypeIsNone(MaterialX::TypeDesc typeDesc);
bool MxTypeIsSurfaceShader(MaterialX::TypeDesc typeDesc);
bool MxTypeDescIsFilename(const MaterialX::TypeDesc typeDesc);
const MaterialX::TypeDesc GetMxTypeDesc(const MaterialX::ShaderPort* port);
const std::string MxGetTypeString(
    MaterialX::SyntaxPtr syntax, const std::string& typeName);
const std::string& GetVector2Name();

} // namespace HdStMaterialXHelpers

PXR_NAMESPACE_CLOSE_SCOPE

#endif
