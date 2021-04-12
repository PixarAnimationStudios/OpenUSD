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

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStMaterialXShaderGen
///
/// Generates a glslfx shader with a surfaceShader function for a MaterialX 
/// network
class HdStMaterialXShaderGen : public MaterialX::GlslShaderGenerator
{
public:
    HdStMaterialXShaderGen(MaterialX::StringMap const& mxHdTextureMap);

    static MaterialX::ShaderGeneratorPtr create(
            MaterialX::StringMap const& mxHdTextureMap=MaterialX::StringMap()) {
        return std::make_shared<HdStMaterialXShaderGen>(mxHdTextureMap); 
    }

    MaterialX::ShaderPtr generate(const std::string& shaderName,
                           MaterialX::ElementPtr mxElement,
                           MaterialX::GenContext& mxContext) const override;

protected:
    void _EmitGlslfxShader(const MaterialX::ShaderGraph& mxGraph,
                           MaterialX::GenContext& mxContext,
                           MaterialX::ShaderStage& mxStage) const;

private:

    /// These three helper functions generate the Glslfx Shader
    void _EmitGlslfxHeader(MaterialX::ShaderStage& mxStage) const;

    void _EmitMxFunctions(const MaterialX::ShaderGraph& mxGraph,
                          MaterialX::GenContext& mxContext,
                          MaterialX::ShaderStage& mxStage) const;

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

    // Store MaterialX texture node names and their Hydra counterparts to 
    // initialize the texture sampler values 
    MaterialX::StringMap _mxHdTextureMap;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif