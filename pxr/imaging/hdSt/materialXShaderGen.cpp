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
#include "pxr/imaging/hdSt/materialXShaderGen.h"
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/base/tf/stringUtils.h"

#include <MaterialXCore/Value.h>
#include <MaterialXGenGlsl/Nodes/SurfaceNodeGlsl.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/ShaderGenerator.h>
#include <MaterialXGenShader/Syntax.h>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    // Create a customized version of the class mx::SurfaceNodeGlsl
    // to be able to notify the shader generator when we start/end
    // emitting the code for the SurfaceNode
    class HdStMaterialXSurfaceNodeGen : public mx::SurfaceNodeGlsl
    {
    public:
        static mx::ShaderNodeImplPtr create() {
            return std::make_shared<HdStMaterialXSurfaceNodeGen>();
        }

        void emitFunctionCall(
            const mx::ShaderNode& node, 
            mx::GenContext& context,
            mx::ShaderStage& stage) const override
        {
            HdStMaterialXShaderGen& shadergen = 
                static_cast<HdStMaterialXShaderGen&>(context.getShaderGenerator());
            
            shadergen.SetEmittingSurfaceNode(true);
            mx::SurfaceNodeGlsl::emitFunctionCall(node, context, stage);
            shadergen.SetEmittingSurfaceNode(false);
        }
    };
}

static const std::string MxHdTangentString = 
R"(
    // Calculate a worldspace tangent vector
    vec3 normalWorld = vec3(HdGet_worldToViewInverseMatrix() * vec4(Neye, 0.0));
    vec3 tangentWorld = cross(normalWorld, vec3(0, 1, 0));
    if (length(tangentWorld) < M_FLOAT_EPS) {
        tangentWorld = cross(normalWorld, vec3(1, 0, 0));
    }
)";

static const std::string MxHdLightString = 
R"(#if NUM_LIGHTS > 0
    for (int i = 0; i < NUM_LIGHTS; ++i) {
        LightSource light = GetLightSource(i);

        // Save the indirect light transformation
        if (light.isIndirectLight) {
            hdTransformationMatrix = light.worldToLightTransform;
        }
        // Save the direct light data
        else {
            // Type Only supporting Point Lights
            $lightData[u_numActiveLightSources].type = 1; // point

            // Position (Hydra position in ViewSpace)
            $lightData[u_numActiveLightSources].position = 
                (HdGet_worldToViewInverseMatrix() * light.position).xyz;

            // Color and Intensity 
            // Note: in Storm, diffuse = lightColor * intensity;
            float intensity = max( max(light.diffuse.r, light.diffuse.g), 
                                   light.diffuse.b);
            $lightData[u_numActiveLightSources].color = light.diffuse.rgb/intensity;
            $lightData[u_numActiveLightSources].intensity = intensity;
            
            // Attenuation 
            // Hydra: vec3(const, linear, quadratic)
            // MaterialX: const = 0.0, linear = 1.0, quadratic = 2.0
            if (light.attenuation.z > 0) {
                $lightData[u_numActiveLightSources].decay_rate = 2.0;
            }
            else if (light.attenuation.y > 0) {
                $lightData[u_numActiveLightSources].decay_rate = 1.0;
            }
            else {
                $lightData[u_numActiveLightSources].decay_rate = 0.0;
            }

            // ShadowOcclusion value
            #if USE_SHADOWS
                u_lightData[u_numActiveLightSources].shadowOcclusion = 
                    light.hasShadow ? shadowing(i, Peye) : 1.0;
            #else 
                u_lightData[u_numActiveLightSources].shadowOcclusion = 1.0;
            #endif

            u_numActiveLightSources++;
        }
    }
#endif
)";

HdStMaterialXShaderGen::HdStMaterialXShaderGen(
    MxHdInfo const& mxHdInfo)
    : GlslShaderGenerator(), 
      _mxHdTextureMap(mxHdInfo.textureMap),
      _mxHdPrimvarMap(mxHdInfo.primvarMap),
      _materialTag(mxHdInfo.materialTag),
      _bindlessTexturesEnabled(mxHdInfo.bindlessTexturesEnabled),
      _emittingSurfaceNode(false)
{
    _defaultTexcoordName = (mxHdInfo.defaultTexcoordName == mx::EMPTY_STRING) ?
                            "st" : mxHdInfo.defaultTexcoordName;

    // Register the customized version of the Surface node generator
    registerImplementation("IM_surface_" + GlslShaderGenerator::TARGET, 
        HdStMaterialXSurfaceNodeGen::create);
}

// Based on GlslShaderGenerator::generate()
// Generates a glslfx shader and stores that in the pixel shader stage where it
// can be retrieved with getSourceCode()
mx::ShaderPtr 
HdStMaterialXShaderGen::generate(
    const std::string& shaderName,
    mx::ElementPtr mxElement,
    mx::GenContext & mxContext) const
{
    mx::ShaderPtr shader = createShader(shaderName, mxElement, mxContext);

    // Turn on fixed float formatting to make sure float values are
    // emitted with a decimal point and not as integers, and to avoid
    // any scientific notation which isn't supported by all OpenGL targets.
    mx::ScopedFloatFormatting fmt(mx::Value::FloatFormatFixed);

    // Create the glslfx (Pixel) Shader
    mx::ShaderStage& shaderStage = shader->getStage(mx::Stage::PIXEL);
    _EmitGlslfxShader(shader->getGraph(), mxContext, shaderStage);
    replaceTokens(_tokenSubstitutions, shaderStage);
    return shader;
}

void 
HdStMaterialXShaderGen::_EmitGlslfxShader(
    const mx::ShaderGraph& mxGraph,
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage) const
{
    // Add a per-light shadowOcclusion value to the lightData uniform block
    addStageUniform(mx::HW::LIGHT_DATA, mx::Type::FLOAT, 
        "shadowOcclusion", mxStage);

    _EmitGlslfxHeader(mxStage);
    _EmitMxFunctions(mxGraph, mxContext, mxStage);
    _EmitMxSurfaceShader(mxGraph, mxContext, mxStage);
}

void 
HdStMaterialXShaderGen::_EmitGlslfxHeader(mx::ShaderStage& mxStage) const
{
    // Glslfx version and configuration
    emitLine("-- glslfx version 0.1", mxStage, false);
    emitLineBreak(mxStage);
    emitComment("File Generated with HdStMaterialXShaderGen.", mxStage);
    emitLineBreak(mxStage);
    emitString(
        R"(-- configuration)" "\n"
        R"({)" "\n", mxStage);

    // insert materialTag metadata
    {
        emitString(R"(    "metadata": {)" "\n", mxStage);
        std::string line;
        line += "        \"materialTag\": \"" + _materialTag + "\"\n";
        emitString(line, mxStage);
        emitString(R"(    }, )""\n", mxStage);
    }

    // insert primvar information if needed
    if (!_mxHdPrimvarMap.empty()) {
        emitString(R"(    "attributes": {)" "\n", mxStage);
        std::string line; unsigned int i = 0;
        for (auto primvarPair : _mxHdPrimvarMap) {
            const mx::TypeDesc *mxType = mx::TypeDesc::get(primvarPair.second);
            if (mxType == nullptr) {
                TF_WARN("MaterialX geomprop '%s' has unknown type '%s'",
                        primvarPair.first.c_str(), primvarPair.second.c_str());
            }
            std::string type = mxType ? _syntax->getTypeName(mxType) : "vec2";

            line += "        \"" + primvarPair.first + "\": {\n";
            line += "            \"type\": \"" + type + "\"\n";
            line += "        }";
            line += (i < _mxHdPrimvarMap.size() - 1) ? ",\n" : "\n";
            i++;
        }
        emitString(line, mxStage);
        emitString(R"(    }, )""\n", mxStage);
    }
    // insert texture information if needed
    if (!_mxHdTextureMap.empty()) {
        emitString(R"(    "textures": {)" "\n", mxStage);
        std::string line; unsigned int i = 0;
        for (auto texturePair : _mxHdTextureMap) {
            line += "        \"" + texturePair.second + "\": {\n        }";
            line += (i < _mxHdTextureMap.size() - 1) ? ",\n" : "\n";
            i++;
        }
        emitString(line, mxStage);
        emitString(R"(    }, )""\n", mxStage);
    }
    emitString(
        R"(    "techniques": {)" "\n"
        R"(        "default": {)" "\n"
        R"(            "surfaceShader": { )""\n"
        R"(                "source": [ "MaterialX.Surface" ])""\n"
        R"(            })""\n"
        R"(        })""\n"
        R"(    })""\n"
        R"(})" "\n\n", mxStage);
    emitLine("-- glsl MaterialX.Surface", mxStage, false);
    emitLineBreak(mxStage);
    emitLineBreak(mxStage);
}

static bool 
_IsHardcodedPublicUniform(const mx::TypeDesc& varType)
{
    // Most major types of public uniforms are set through 
    // HdSt_MaterialParamVector in HdStMaterialXFilter's
    // _AddMaterialXParams function, the rest are hardcoded 
    // in the shader
    if (varType.getBaseType() != mx::TypeDesc::BASETYPE_FLOAT &&
        varType.getBaseType() != mx::TypeDesc::BASETYPE_INTEGER) {
        return true;
    }
    if (varType.getSize() < 1 || varType.getSize() > 4) {
        return true;
    }

    return false;
}

// Similar to GlslShaderGenerator::emitPixelStage() with alterations and 
// additions to match Pxr's codeGen
void
HdStMaterialXShaderGen::_EmitMxFunctions(
    const mx::ShaderGraph& mxGraph,
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage) const
{
#if MATERIALX_MAJOR_VERSION == 1 && MATERIALX_MINOR_VERSION == 38 && MATERIALX_BUILD_VERSION == 3
    std::string libPath;
#else
    // Starting from MaterialX 1.38.4 at PR 877, we must add the "libraries" part:
    std::string libPath = "libraries/";
#endif
    // Add global constants and type definitions
    emitInclude(libPath + "stdlib/" + mx::GlslShaderGenerator::TARGET
                + "/lib/mx_math.glsl", mxContext, mxStage);
    emitLine("#if NUM_LIGHTS > 0", mxStage, false);
    emitLine("#define MAX_LIGHT_SOURCES NUM_LIGHTS", mxStage, false);
    emitLine("#else", mxStage, false);
    emitLine("#define MAX_LIGHT_SOURCES 1", mxStage, false);
    emitLine("#endif", mxStage, false);
    emitLine("#define DIRECTIONAL_ALBEDO_METHOD " +
            std::to_string(int(mxContext.getOptions().hwDirectionalAlbedoMethod)), 
            mxStage, false);
    emitLineBreak(mxStage);
    emitTypeDefinitions(mxContext, mxStage);

    // Add all constants
    const mx::VariableBlock& constants = mxStage.getConstantBlock();
    if (!constants.empty()) {
        emitVariableDeclarations(constants, _syntax->getConstantQualifier(),
                                 mx::Syntax::SEMICOLON, 
                                 mxContext, mxStage, false);
        emitLineBreak(mxStage);
    }

    // Add all uniforms
    for (const auto& it : mxStage.getUniformBlocks()) {
        const mx::VariableBlock& uniforms = *it.second;

        // Skip light uniforms as they are handled separately
        if (!uniforms.empty() && uniforms.getName() != mx::HW::LIGHT_DATA) {
            emitComment("Uniform block: " + uniforms.getName(), mxStage);
            emitVariableDeclarations(uniforms, mx::EMPTY_STRING, 
                                     mx::Syntax::SEMICOLON, mxContext, mxStage);
            emitLineBreak(mxStage);
        }
    }

    // If bindlessTextures are not enabled, the above for loop skips 
    // initializing textures. Initialize them here by defining mappings 
    // to the appropriate HdGetSampler function. 
    if (!_bindlessTexturesEnabled) {

        // Define mappings for the DomeLight Textures
        emitLine("#ifdef HD_HAS_domeLightIrradiance", mxStage, false);
        emitLine("#define u_envRadiance HdGetSampler_domeLightPrefilter() ", mxStage, false);
        emitLine("#define u_envIrradiance HdGetSampler_domeLightIrradiance() ", mxStage, false);
        emitLine("#else", mxStage, false);
        emitLine("uniform sampler2D u_envRadiance;", mxStage, false);
        emitLine("uniform sampler2D u_envIrradiance;", mxStage, false);
        emitLine("#endif", mxStage, false);
        emitLineBreak(mxStage);

        // Define mappings for the MaterialX Textures
        if (!_mxHdTextureMap.empty()) {
            emitComment("Define MaterialX to Hydra Sampler mappings", mxStage);
            for (auto texturePair : _mxHdTextureMap) {
                emitLine(TfStringPrintf("#define %s_file HdGetSampler_%s()",
                                        texturePair.first.c_str(),
                                        texturePair.second.c_str()),
                        mxStage, false);
            }
            emitLineBreak(mxStage);
        }
    }

    const bool lighting = mxGraph.hasClassification( 
                              mx::ShaderNode::Classification::SHADER | 
                              mx::ShaderNode::Classification::SURFACE)
                          || mxGraph.hasClassification(
                              mx::ShaderNode::Classification::BSDF);
    const bool shadowing = (lighting && mxContext.getOptions().hwShadowMap) ||
                           mxContext.getOptions().hwWriteDepthMoments;

    // Add light data block if needed
    if (lighting) {
        const mx::VariableBlock& lightData = mxStage.getUniformBlock(mx::HW::LIGHT_DATA);
        emitLine("struct " + lightData.getName(), mxStage, false);
        emitScopeBegin(mxStage);
        emitVariableDeclarations(lightData, mx::EMPTY_STRING, 
                                 mx::Syntax::SEMICOLON, 
                                 mxContext, mxStage, false);
        emitScopeEnd(mxStage, true);
        emitLineBreak(mxStage);
        emitLine(lightData.getName() + " " 
                + lightData.getInstance() + "[MAX_LIGHT_SOURCES]", mxStage);
        emitLineBreak(mxStage);
        emitLineBreak(mxStage);
    }

    // Add vertex data struct and the mxInit function which initializes mx
    // values with the Hd equivalents
    const mx::VariableBlock& vertexData = mxStage.getInputBlock(mx::HW::VERTEX_DATA);
    if (!vertexData.empty()) {

        // add Mx VertexData
        emitComment("MaterialX's VertexData", mxStage);
        std::string mxVertexDataName = "mx" + vertexData.getName(); 
        emitLine("struct " + mxVertexDataName, mxStage, false);
        emitScopeBegin(mxStage);
        emitVariableDeclarations(vertexData, mx::EMPTY_STRING,
                                 mx::Syntax::SEMICOLON, 
                                 mxContext, mxStage, false);
        emitScopeEnd(mxStage, false, false);
        emitString(mx::Syntax::SEMICOLON, mxStage);
        emitLineBreak(mxStage);

        // Add the vd declaration
        emitLine(mxVertexDataName + " " + vertexData.getInstance(), mxStage);
        emitLineBreak(mxStage);
        emitLineBreak(mxStage);

        // add the mxInit function to convert Hd -> Mx data
        _EmitMxInitFunction(vertexData, mxStage);
    }

    // Emit lighting and shadowing code
    if (lighting) {
        emitSpecularEnvironment(mxContext, mxStage);
    }
    if (shadowing) {
        emitInclude(libPath + "pbrlib/" + mx::GlslShaderGenerator::TARGET
                    + "/lib/mx_shadow.glsl", mxContext, mxStage);
    }

    // Emit directional albedo table code.
    if (mxContext.getOptions().hwDirectionalAlbedoMethod == 
            mx::HwDirectionalAlbedoMethod::DIRECTIONAL_ALBEDO_TABLE ||
        mxContext.getOptions().hwWriteAlbedoTable) {
        emitInclude(libPath + "pbrlib/" + mx::GlslShaderGenerator::TARGET
                    + "/lib/mx_table.glsl", mxContext, mxStage);
        emitLineBreak(mxStage);
    }

    // Set the include file to use for uv transformations,
    // depending on the vertical flip flag.
    if (mxContext.getOptions().fileTextureVerticalFlip) {
        _tokenSubstitutions[mx::ShaderGenerator::T_FILE_TRANSFORM_UV] = 
            libPath + "stdlib/" + mx::GlslShaderGenerator::TARGET +
            "/lib/mx_transform_uv_vflip.glsl";
    }
    else {
        _tokenSubstitutions[mx::ShaderGenerator::T_FILE_TRANSFORM_UV] = 
            libPath + "stdlib/" + mx::GlslShaderGenerator::TARGET + 
            "/lib/mx_transform_uv.glsl";
    }

    // Emit uv transform code globally if needed.
    if (mxContext.getOptions().hwAmbientOcclusion) {
        emitInclude(ShaderGenerator::T_FILE_TRANSFORM_UV, mxContext, mxStage);
    }

    // Add light sampling functions
    emitLightFunctionDefinitions(mxGraph, mxContext, mxStage);

    // Add all functions for node implementations
    emitFunctionDefinitions(mxGraph, mxContext, mxStage);
}

// Similar to GlslShaderGenerator::emitPixelStage() with alterations and 
// additions to match Pxr's codeGen
void 
HdStMaterialXShaderGen::_EmitMxSurfaceShader(
    const mx::ShaderGraph& mxGraph,
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage) const
{
    // Add surfaceShader function
    setFunctionName("surfaceShader", mxStage);
    emitLine("vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord)", mxStage, false);
    emitScopeBegin(mxStage);

    emitComment("Initialize MaterialX Variables", mxStage);
    emitLine("mxInit(Peye, Neye)", mxStage);

    const mx::ShaderGraphOutputSocket* outputSocket = mxGraph.getOutputSocket();
    if (mxGraph.hasClassification(mx::ShaderNode::Classification::CLOSURE) &&
        !mxGraph.hasClassification(mx::ShaderNode::Classification::SHADER)) {
        // Handle the case where the mxGraph is a direct closure.
        // We don't support rendering closures without attaching 
        // to a surface shader, so just output black.
        emitLine(outputSocket->getVariable() + " = vec4(0.0, 0.0, 0.0, 1.0)",
                 mxStage);
    }
    else if (mxContext.getOptions().hwWriteDepthMoments) {
        emitLine(outputSocket->getVariable() + 
                " = vec4(mx_compute_depth_moments(), 0.0, 1.0)", mxStage);
    }
    else if (mxContext.getOptions().hwWriteAlbedoTable) {
        emitLine(outputSocket->getVariable() + 
                " = vec4(mx_ggx_directional_albedo_generate_table(), 0.0, 1.0)",
                mxStage);
    }
    else {
        // Surface shaders need special handling.
        if (mxGraph.hasClassification(mx::ShaderNode::Classification::SHADER | 
                                      mx::ShaderNode::Classification::SURFACE))
        {
            // Emit all texturing nodes. These are inputs to any
            // closure/shader nodes and need to be emitted first.
            emitFunctionCalls(mxGraph, mxContext, mxStage, mx::ShaderNode::Classification::TEXTURE);

            // Emit function calls for all surface shader nodes.
            // These will internally emit their closure function calls.
            emitFunctionCalls(mxGraph, mxContext, mxStage, mx::ShaderNode::Classification::SHADER | 
                                                           mx::ShaderNode::Classification::SURFACE);
        }
        else
        {
            // No surface shader graph so just generate all
            // function calls in order.
            emitFunctionCalls(mxGraph, mxContext, mxStage);
        }

        // Emit final output
        std::string finalOutputReturn = "vec4 mxOut = " ;
        const mx::ShaderOutput* outputConnection = outputSocket->getConnection();
        if (outputConnection) {

            std::string finalOutput = outputConnection->getVariable();
            const std::string& channels = outputSocket->getChannels();
            if (!channels.empty()) {
                finalOutput = _syntax->getSwizzledVariable(finalOutput, 
                                            outputConnection->getType(),
                                            channels, outputSocket->getType());
            }

            if (mxGraph.hasClassification(mx::ShaderNode::Classification::SURFACE)) {
                if (mxContext.getOptions().hwTransparency) {
                    emitLine("float outAlpha = clamp(1.0 - dot(" + finalOutput 
                            + ".transparency, vec3(0.3333)), 0.0, 1.0)", mxStage);
                    emitLine(finalOutputReturn + "vec4(" 
                            + finalOutput + ".color, outAlpha)", mxStage);
                }
                else {
                    emitLine(finalOutputReturn + 
                             "vec4(" + finalOutput + ".color, 1.0)", mxStage);
                }
            }
            else {
                if (!outputSocket->getType()->isFloat4()) {
                    toVec4(outputSocket->getType(), finalOutput);
                }
                emitLine(finalOutputReturn + 
                         "vec4(" + finalOutput + ".color, 1.0)", mxStage);
            }
        }
        else {
            const std::string outputValue = outputSocket->getValue() 
                            ? _syntax->getValue(outputSocket->getType(), 
                                                *outputSocket->getValue()) 
                            : _syntax->getDefaultValue(outputSocket->getType());
            if (!outputSocket->getType()->isFloat4()) {
                std::string finalOutput = outputSocket->getVariable() + "_tmp";
                emitLine(_syntax->getTypeName(outputSocket->getType()) + " " 
                        + finalOutput + " = " + outputValue, mxStage);
                toVec4(outputSocket->getType(), finalOutput);
                emitLine(finalOutputReturn + finalOutput, mxStage);
            }
            else {
                emitLine(finalOutputReturn + outputValue, mxStage);
            }
        }

        // Emit color overrides (mainly for selection highlighting)
        emitLine("mxOut = ApplyColorOverrides(mxOut)", mxStage);
    }
    emitLine("return mxOut", mxStage);

    // End surfaceShader function
    emitScopeEnd(mxStage);
    emitLineBreak(mxStage);
}


void 
HdStMaterialXShaderGen::_EmitMxInitFunction(
    mx::VariableBlock const& vertexData,
    mx::ShaderStage& mxStage) const
{
    setFunctionName("mxInit", mxStage);
    emitLine("void mxInit(vec4 Peye, vec3 Neye)", mxStage, false);
    emitScopeBegin(mxStage);

    emitComment("Convert HdData to MxData", mxStage);

    // Initialize the position of the view in worldspace
    emitLine("u_viewPosition = vec3(HdGet_worldToViewInverseMatrix()"
             " * vec4(0.0, 0.0, 0.0, 1.0))", mxStage);
    
    // Calculate the worldspace tangent vector
    emitString(MxHdTangentString, mxStage);

    // Add the vd declaration that translates HdVertexData -> MxVertexData
    std::string mxVertexDataName = "mx" + vertexData.getName();
    _EmitMxVertexDataDeclarations(vertexData, mxVertexDataName, 
                                  vertexData.getInstance(), 
                                  mx::Syntax::COMMA, mxStage);
    emitLineBreak(mxStage);

    // Initialize MaterialX parameters with HdGet_ equivalents
    emitComment("Initialize Material Parameters", mxStage);
    const mx::VariableBlock& paramsBlock =
        mxStage.getUniformBlock(mx::HW::PUBLIC_UNIFORMS);
    for (size_t i = 0; i < paramsBlock.size(); ++i) {
        const auto variable = paramsBlock[i];
        const auto variableType = variable->getType();
        if (!_IsHardcodedPublicUniform(*variableType)) {
            emitLine(variable->getVariable() + " = HdGet_" +
                variable->getVariable() + "()", mxStage);
        }
    }
    emitLineBreak(mxStage);

    // Initialize the Indirect Light Textures
    // Note: only need to initialize textures when bindlessTextures are enabled,
    // when bindlessTextures are not enabled, mappings are defined in 
    // HdStMaterialXShaderGen::_EmitMxFunctions
    emitLine("#ifdef HD_HAS_domeLightIrradiance", mxStage, false);
    if (_bindlessTexturesEnabled) {
        emitLine("u_envIrradiance = HdGetSampler_domeLightIrradiance()", mxStage);
        emitLine("u_envRadiance = HdGetSampler_domeLightPrefilter()", mxStage);
    }
    emitLine("u_envRadianceMips = textureQueryLevels(u_envRadiance)", mxStage);
    emitLine("#endif", mxStage, false);
    emitLineBreak(mxStage);

    // Initialize MaterialX Texture samplers with HdGetSampler equivalents
    if (_bindlessTexturesEnabled && !_mxHdTextureMap.empty()) {
        emitComment("Initialize Material Textures", mxStage);
        for (auto texturePair : _mxHdTextureMap) {
            emitLine(texturePair.first + "_file = "
                    "HdGetSampler_" + texturePair.second + "()", mxStage);
        }
        emitLineBreak(mxStage);
    }

    // Gather Direct light data from Hydra and apply the Hydra transformation 
    // matrix to the environment map matrix (u_envMatrix) to account for the
    // domeLight's transform. 
    // Note: MaterialX initializes u_envMatrix as a 180 rotation about the 
    // Y-axis (Y-up)
    emitLine("mat4 hdTransformationMatrix = mat4(1.0)", mxStage);
    emitString(MxHdLightString, mxStage);
    emitLine("u_envMatrix = u_envMatrix * hdTransformationMatrix", mxStage);

    emitScopeEnd(mxStage);
    emitLineBreak(mxStage);
}

// Generates the Mx VertexData that is needed for the Mx Shader
void 
HdStMaterialXShaderGen::_EmitMxVertexDataDeclarations(
    mx::VariableBlock const& block, 
    std::string const& mxVertexDataName, 
    std::string const& mxVertexDataVariable,
    std::string const& separator,
    mx::ShaderStage& mxStage) const
{
    // vd = mxVertexData(
    std::string line = mxVertexDataVariable + " = " + mxVertexDataName + "(";

    for (size_t i = 0; i < block.size(); ++i) {
        line += _EmitMxVertexDataLine(block[i], separator);
        // remove the separator from the last data line
        if (i == block.size() - 1) {
            line = line.substr(0, line.size() - separator.size());
        }
    }
    // add ending )
    line += ")";

    emitLine(line, mxStage);
}

std::string
HdStMaterialXShaderGen::_EmitMxVertexDataLine(
    const mx::ShaderPort* variable,
    std::string const& separator) const
{
    // Connect the mxVertexData variable with the appropriate pxr variable
    // making sure to convert the Hd data (viewSpace) to Mx data (worldSpace)
    std::string hdVariableDef;
    const std::string mxVariableName = variable->getVariable();
    if (mxVariableName.compare(mx::HW::T_POSITION_WORLD) == 0) {

        // Convert to WorldSpace position
        hdVariableDef = "vec3(HdGet_worldToViewInverseMatrix() * Peye)"
                        + separator;
    }
    else if (mxVariableName.compare(mx::HW::T_NORMAL_WORLD) == 0) {

        // Convert to WorldSpace normal (calculated in MxHdTangentString)
        hdVariableDef = "normalWorld" + separator;
    }
    else if (mxVariableName.compare(mx::HW::T_TANGENT_WORLD) == 0) {

        // Calculated in MxHdTangentString
        hdVariableDef = "tangentWorld" + separator;
    }
    else if (mxVariableName.compare(mx::HW::T_POSITION_OBJECT) == 0) {

        hdVariableDef  = "HdGet_points()" + separator;
    }
    else if (mxVariableName.compare(0, mx::HW::T_TEXCOORD.size(), 
                                    mx::HW::T_TEXCOORD) == 0) {
        
        // Wrap initialization inside #ifdef in case the object does not have 
        // the st primvar
        hdVariableDef = TfStringPrintf("\n"
                "    #ifdef HD_HAS_%s\n"
                "        HdGet_%s(),\n"
                "    #else\n"
                "        %s(0.0),\n"
                "    #endif\n        ", 
                _defaultTexcoordName.c_str(), _defaultTexcoordName.c_str(),
                _syntax->getTypeName(variable->getType()).c_str());
    }
    else if (mxVariableName.compare(0, mx::HW::T_IN_GEOMPROP.size(), 
                                    mx::HW::T_IN_GEOMPROP) == 0) {
        // Wrap initialization inside #ifdef in case the object does not have 
        // the geomprop primvar
        // Note: variable name format: 'T_IN_GEOMPROP_geomPropName';
        const std::string geompropName = mxVariableName.substr(
                                            mx::HW::T_IN_GEOMPROP.size());
        hdVariableDef = TfStringPrintf("\n"
                "    #ifdef HD_HAS%s\n"
                "        HdGet%s(),\n"
                "    #else\n"
                "        %s(0.0),\n"
                "    #endif\n        ", 
                geompropName.c_str(), geompropName.c_str(),
                _syntax->getTypeName(variable->getType()).c_str());
    }
    else {
        const std::string valueStr = variable->getValue() 
            ? _syntax->getValue(variable->getType(), *variable->getValue(), true)
            : _syntax->getDefaultValue(variable->getType(), true);
        hdVariableDef = valueStr.empty() ? mx::EMPTY_STRING : valueStr + separator;
    }

    return hdVariableDef.empty() ? mx::EMPTY_STRING : hdVariableDef;
}


void
HdStMaterialXShaderGen::emitVariableDeclarations(
    mx::VariableBlock const& block,
    std::string const& qualifier,
    std::string const& separator,
    mx::GenContext& context,
    mx::ShaderStage& stage,
    bool assignValue) const
{
    // Mx variables that need to be initialized with Hd Values
    static const mx::StringSet MxHdVariables = {
        mx::HW::T_VIEW_POSITION, 
        mx::HW::T_ENV_IRRADIANCE,   // Irradiance texture
        mx::HW::T_ENV_RADIANCE,     // Environment map OR prefilter texture
        mx::HW::T_ENV_RADIANCE_MIPS,
        mx::HW::T_ENV_RADIANCE_SAMPLES,
        mx::HW::T_ALBEDO_TABLE      // BRDF texture
    };

    // Most public uniforms are set from outside the shader
    const bool isPublicUniform = block.getName() == mx::HW::PUBLIC_UNIFORMS;

    for (size_t i = 0; i < block.size(); ++i)
    {
        emitLineBegin(stage);
        const auto variable = block[i];
        const auto varType = variable->getType();

        // If bindlessTextures are not enabled the Mx Smpler names are mapped 
        // to the Hydra equivalents in HdStMaterialXShaderGen::_EmitMxFunctions
        if (!_bindlessTexturesEnabled && varType == mx::Type::FILENAME) {
            continue;
        }

        // Only declare the variables that we need to initialize with Hd Data
        if ( (isPublicUniform && !_IsHardcodedPublicUniform(*varType))
            || MxHdVariables.count(variable->getName()) ) {
            emitVariableDeclaration(variable, mx::EMPTY_STRING,
                                    context, stage, false);
        }
        // Otherwise assign the value from MaterialX
        else {
            emitVariableDeclaration(variable, qualifier,
                                    context, stage, assignValue);
        }
        emitString(separator, stage);
        emitLineEnd(stage, false);
    }
}

void 
HdStMaterialXShaderGen::emitLine(
    const std::string& str, 
    MaterialX::ShaderStage& stage, 
    bool semicolon) const
{
    mx::GlslShaderGenerator::emitLine(str, stage, semicolon);

    // When emitting the Light loop code for the Surface node, the variable
    // 'occlusion' represents shadow occlusion. We don't use MaterialX's
    // shadow implementation (hwShadowMap is false). Instead, use our own 
    // per-light occlusion value calculated in mxInit() and stored in lightData
    if (_emittingSurfaceNode && str == "vec3 L = lightShader.direction") {
        emitLine(
            "occlusion = u_lightData[activeLightIndex].shadowOcclusion", stage);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
