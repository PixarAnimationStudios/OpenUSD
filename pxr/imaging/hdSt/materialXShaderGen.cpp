//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/materialXShaderGen.h"
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/base/tf/stringUtils.h"

#include <MaterialXCore/Value.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/ShaderGenerator.h>
#include <MaterialXGenShader/Syntax.h>
#include <MaterialXGenGlsl/Nodes/SurfaceNodeGlsl.h>
#include <MaterialXGenMsl/Nodes/SurfaceNodeMsl.h>
#include <MaterialXGenMsl/MslResourceBindingContext.h>
#include <MaterialXGenMsl/MslShaderGenerator.h>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE


static const std::string MxHdWorldSpaceVectors = 
R"(
    // Calculate the worldspace position and normal vectors
    vec3 positionWorld = vec3(HdGet_worldToViewInverseMatrix() * Peye);
    vec3 normalWorld = vec3(HdGet_worldToViewInverseMatrix() * vec4(Neye, 0.0));

    // Calculate the worldspace tangent vector 
#ifdef HD_HAS_%s
    vec3 tangentWorld = ComputeTangentVector(positionWorld, normalWorld, HdGet_%s());
#else 
    vec3 tangentWorld = cross(normalWorld, vec3(0, 1, 0));
    if (length(tangentWorld) < M_FLOAT_EPS) {
        tangentWorld = cross(normalWorld, vec3(1, 0, 0));
    }
#endif

)";

static const std::string MxHdLightString = 
R"(#if NUM_LIGHTS > 0
    for (int i = 0; i < NUM_LIGHTS; ++i) {
        LightSource light = GetLightSource(i);

        // Save the indirect light transformation
        if (light.isIndirectLight) {
            hdTransformationMatrix = light.worldToLightTransform;
            // Note: in Storm, diffuse = lightColor * intensity;
            u_envLightIntensity = max( max(light.diffuse.r, light.diffuse.g), 
                                   light.diffuse.b);
        }
        // Save the direct light data
        else {
            // Light Type and Position/Direction
            // Distant lights have Hydra attenuation = vec3(0.0, 0.0, 0.0)
            if (light.attenuation.x == 0.0 && light.attenuation.y == 0.0 && 
                light.attenuation.z == 0.0) {
                $lightData[u_numActiveLightSources].type = 2; // directional

                // Direction (Hydra position in ViewSpace)
                $lightData[u_numActiveLightSources].direction = 
                    (HdGet_worldToViewInverseMatrix() * -light.position).xyz;
            }
            // Treat all other lights as Point lights
            else {
                $lightData[u_numActiveLightSources].type = 1; // point

                // Position (Hydra position in ViewSpace)
                $lightData[u_numActiveLightSources].position = 
                    (HdGet_worldToViewInverseMatrix() * light.position).xyz;
            }

            // Color and Intensity 
            // Note: in Storm, diffuse = lightColor * intensity;
            float intensity = max( max(light.diffuse.r, light.diffuse.g), 
                                   light.diffuse.b);
            vec3 lightColor = (intensity == 0.0) 
                ? light.diffuse.rgb : light.diffuse.rgb/intensity;
            $lightData[u_numActiveLightSources].color = lightColor;
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

static bool 
_IsHardcodedPublicUniform(const mx::TypeDesc& varType)
{
    // Most major types of public uniforms are set through 
    // HdSt_MaterialParamVector in HdStMaterialXFilter's
    // _AddMaterialXParams function, the rest are hardcoded 
    // in the shader
    if (varType.getBaseType() != mx::TypeDesc::BASETYPE_FLOAT &&
        varType.getBaseType() != mx::TypeDesc::BASETYPE_INTEGER &&
        varType.getBaseType() != mx::TypeDesc::BASETYPE_BOOLEAN) {
        return true;
    }
    if (varType.getSize() < 1 || varType.getSize() > 4) {
        return true;
    }

    return false;
}


template<typename Base>
void
HdStMaterialXShaderGen<Base>::_EmitGlslfxHeader(mx::ShaderStage& mxStage) const
{
    // Glslfx version and configuration
    emitLine("-- glslfx version 0.1", mxStage, false);
    Base::emitLineBreak(mxStage);
    Base::emitComment("File Generated with HdStMaterialXShaderGen.", mxStage);
    Base::emitLineBreak(mxStage);
    Base::emitString("#import $TOOLS/hdSt/shaders/surfaceHelpers.glslfx\n", mxStage);
    Base::emitLineBreak(mxStage);
    Base::emitString(
        R"(-- configuration)" "\n"
        R"({)" "\n", mxStage);

    // insert materialTag metadata
    {
        Base::emitString(R"(    "metadata": {)" "\n", mxStage);
        std::string line = "";
        line += "        \"materialTag\": \"" + _materialTag + "\"\n";
        Base::emitString(line, mxStage);
        Base::emitString(R"(    }, )""\n", mxStage);
    }

    // insert primvar information if needed
    if (!_mxHdPrimvarMap.empty()) {
        Base::emitString(R"(    "attributes": {)" "\n", mxStage);
        std::string line = ""; unsigned int i = 0;
        for (mx::StringMap::const_reference primvarPair : _mxHdPrimvarMap) {
            const mx::TypeDesc *mxType = mx::TypeDesc::get(primvarPair.second);
            if (mxType == nullptr) {
                TF_WARN("MaterialX geomprop '%s' has unknown type '%s'",
                        primvarPair.first.c_str(), primvarPair.second.c_str());
            }
            const std::string type = mxType 
                ? Base::_syntax->getTypeName(mxType) : "vec2";

            line += "        \"" + primvarPair.first + "\": {\n";
            line += "            \"type\": \"" + type + "\"\n";
            line += "        }";
            line += (i < _mxHdPrimvarMap.size() - 1) ? ",\n" : "\n";
            i++;
        }
        Base::emitString(line, mxStage);
        Base::emitString(R"(    }, )""\n", mxStage);
    }
    // insert texture information if needed
    if (!_mxHdTextureMap.empty()) {
        Base::emitString(R"(    "textures": {)" "\n", mxStage);
        std::string line = ""; unsigned int i = 0;
        for (mx::StringMap::const_reference texturePair : _mxHdTextureMap) {
            line += "        \"" + texturePair.second + "\": {\n        }";
            line += (i < _mxHdTextureMap.size() - 1) ? ",\n" : "\n";
            i++;
        }
        Base::emitString(line, mxStage);
        Base::emitString(R"(    }, )""\n", mxStage);
    }
    Base::emitString(
        R"(    "techniques": {)" "\n"
        R"(        "default": {)" "\n"
        R"(            "surfaceShader": { )""\n"
        R"(                "source": [ "SurfaceHelpers.TangentSpace",)""\n"
        R"(                            "MaterialX.Surface" ])""\n"
        R"(            })""\n"
        R"(        })""\n"
        R"(    })""\n"
        R"(})" "\n\n", mxStage);
    emitLine("-- glsl MaterialX.Surface", mxStage, false);
    Base::emitLineBreak(mxStage);
    Base::emitLineBreak(mxStage);
}

// Similar to GlslShaderGenerator::emitPixelStage() with alterations and 
// additions to match Pxr's codeGen
template<typename Base>
void 
HdStMaterialXShaderGen<Base>::_EmitMxSurfaceShader(
    const mx::ShaderGraph& mxGraph,
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage) const
{
    // Add surfaceShader function
    Base::setFunctionName("surfaceShader", mxStage);
    emitLine("vec4 surfaceShader("
                "vec4 Peye, vec3 Neye, "
                "vec4 color, vec4 patchCoord)", mxStage, false);
    Base::emitScopeBegin(mxStage);

    Base::emitComment("Initialize MaterialX Variables", mxStage);
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
                                      mx::ShaderNode::Classification::SURFACE)){
            // Emit all texturing nodes. These are inputs to any
            // closure/shader nodes and need to be emitted first.
            Base::emitFunctionCalls(mxGraph, mxContext, mxStage,
                mx::ShaderNode::Classification::TEXTURE);

#if MATERIALX_MAJOR_VERSION == 1 && MATERIALX_MINOR_VERSION == 38 && \
    MATERIALX_BUILD_VERSION <= 4
            // Emit function calls for all surface shader nodes.
            // These will internally emit their closure function calls.
            Base::emitFunctionCalls(mxGraph, mxContext, mxStage, 
                mx::ShaderNode::Classification::SHADER | 
                mx::ShaderNode::Classification::SURFACE);
#else
            // Emit function calls for "root" closure/shader nodes.
            // These will internally emit function calls for any dependent 
            // closure nodes upstream.
            for (mx::ShaderGraphOutputSocket *socket : 
                    mxGraph.getOutputSockets()) {
                if (socket->getConnection()) {
                    const mx::ShaderNode* upstream =
                        socket->getConnection()->getNode();
                    if (upstream->getParent() == &mxGraph &&
                        (upstream->hasClassification(
                            mx::ShaderNode::Classification::CLOSURE) ||
                         upstream->hasClassification(
                            mx::ShaderNode::Classification::SHADER))) {
                        Base::emitFunctionCall(*upstream, mxContext, mxStage);
                    }
                }
            }
#endif
        }
        else {
            // No surface shader graph so just generate all
            // function calls in order.
            Base::emitFunctionCalls(mxGraph, mxContext, mxStage);
        }

        // Emit final output
        std::string finalOutputReturn = "vec4 mxOut = " ;
        const mx::ShaderOutput* outputConnection = outputSocket->getConnection();
        if (outputConnection) {

            std::string finalOutput = outputConnection->getVariable();
            const std::string& channels = outputSocket->getChannels();
            if (!channels.empty()) {
                finalOutput = Base::_syntax->getSwizzledVariable(
                    finalOutput, outputConnection->getType(),
                    channels, outputSocket->getType());
            }

            if (mxGraph.hasClassification(
                    mx::ShaderNode::Classification::SURFACE)) {
                if (mxContext.getOptions().hwTransparency) {
                    emitLine("float outAlpha = clamp(1.0 - dot(" + finalOutput 
                            + ".transparency, vec3(0.3333)), 0.0, 1.0)", mxStage);
                    emitLine(finalOutputReturn + "vec4(" 
                            + finalOutput + ".color, outAlpha)", mxStage);
                    emitLine("if (outAlpha < " + mx::HW::T_ALPHA_THRESHOLD + ")",
                             mxStage, false);
                    Base::emitScopeBegin(mxStage);
                    emitLine("discard", mxStage);
                    Base::emitScopeEnd(mxStage);
                }
                else {
                    emitLine(finalOutputReturn + 
                             "vec4(" + finalOutput + ".color, 1.0)", mxStage);
                }
            }
            else {
                if (!outputSocket->getType()->isFloat4()) {
                    Base::toVec4(outputSocket->getType(), finalOutput);
                }
                emitLine(finalOutputReturn + 
                         "vec4(" + finalOutput + ".color, 1.0)", mxStage);
            }
        }
        else {
            const std::string outputValue = outputSocket->getValue() 
                ? Base::_syntax->getValue(
                    outputSocket->getType(), *outputSocket->getValue()) 
                : Base::_syntax->getDefaultValue(outputSocket->getType());
            if (!outputSocket->getType()->isFloat4()) {
                std::string finalOutput = outputSocket->getVariable() + "_tmp";
                emitLine(Base::_syntax->getTypeName(outputSocket->getType()) 
                        + " " + finalOutput + " = " + outputValue, mxStage);
                Base::toVec4(outputSocket->getType(), finalOutput);
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
    Base::emitScopeEnd(mxStage);
    Base::emitLineBreak(mxStage);
}

static std::string
_GetTexcoordName(
    mx::VariableBlock const& vertexDataBlock,
    std::string const& defaultTexcoordName)
{
    // Texcoords could come from either a texcoord or a geomprop value node.
    // We prioritize using the texcoord name over the geomprop. 

    // Cycle through the vertexDataBlock to find the texcoord name.
    std::string texcoordName = defaultTexcoordName;
    for (size_t i = 0; i < vertexDataBlock.size(); ++i) {
        const mx::ShaderPort* variable = vertexDataBlock[i];
        const std::string mxVariableName = variable->getVariable();

        // If we have a texcoord node, use the default texcoord name.
        if (mxVariableName.compare(
                0, mx::HW::T_TEXCOORD.size(), mx::HW::T_TEXCOORD) == 0) {
            return defaultTexcoordName;
        }
        // Use the geomprop name if this is a vec2 geomprop value node 
        if (mxVariableName.compare(
                0, mx::HW::T_IN_GEOMPROP.size(), mx::HW::T_IN_GEOMPROP) == 0 &&
            variable->getType() == mx::Type::VECTOR2) {
            texcoordName = mxVariableName.substr(mx::HW::T_IN_GEOMPROP.size()+1);
        }
    }
    return texcoordName;
}

template<typename Base>
void 
HdStMaterialXShaderGen<Base>::_EmitMxInitFunction(
    mx::VariableBlock const& vertexData,
    mx::ShaderStage& mxStage) const
{
    Base::setFunctionName("mxInit", mxStage);
    emitLine("void mxInit(vec4 Peye, vec3 Neye)", mxStage, false);
    Base::emitScopeBegin(mxStage);

    Base::emitComment("Convert HdData to MxData", mxStage);

    // Initialize the position of the view in worldspace
    if (mxStage.getUniformBlock(mx::HW::PRIVATE_UNIFORMS).find(mx::HW::T_VIEW_POSITION)) {
        emitLine("u_viewPosition = vec3(HdGet_worldToViewInverseMatrix()"
            " * vec4(0.0, 0.0, 0.0, 1.0))", mxStage);
    }
    
    // Calculate the worldspace position, normal and tangent vectors
    const std::string texcoordName =
        _GetTexcoordName(vertexData, _defaultTexcoordName);
    Base::emitString(
        TfStringPrintf(MxHdWorldSpaceVectors.c_str(), 
            texcoordName.c_str(), texcoordName.c_str()), 
        mxStage);

    // Add the vd declaration that translates HdVertexData -> MxVertexData
    if (!vertexData.empty()) {
        std::string mxVertexDataName = "mx" + vertexData.getName();
        _EmitMxVertexDataDeclarations(vertexData, mxVertexDataName,
            vertexData.getInstance(),
            mx::Syntax::COMMA, mxStage);
        Base::emitLineBreak(mxStage);
    }

    // Initialize MaterialX parameters with HdGet_ equivalents
    Base::emitComment("Initialize Material Parameters", mxStage);
    const mx::VariableBlock& paramsBlock =
        mxStage.getUniformBlock(mx::HW::PUBLIC_UNIFORMS);
    for (size_t i = 0; i < paramsBlock.size(); ++i) {
        const mx::ShaderPort* variable = paramsBlock[i];
        const mx::TypeDesc* variableType = variable->getType();
        if (!_IsHardcodedPublicUniform(*variableType)) {
            emitLine(variable->getVariable() + " = HdGet_" +
                variable->getVariable() + "()", mxStage);
        }
    }
    Base::emitLineBreak(mxStage);

    // Initialize the Indirect Light Textures
    // Note: only need to initialize textures when bindlessTextures are enabled,
    // when bindlessTextures are not enabled, mappings are defined in 
    // HdStMaterialXShaderGen*::_EmitMxFunctions
    Base::emitComment("Initialize Indirect Light Textures and values", mxStage);
    if (_bindlessTexturesEnabled) {
        emitLine("#ifdef HD_HAS_domeLightIrradiance", mxStage, false);
        emitLine("u_envIrradiance = HdGetSampler_domeLightIrradiance()", mxStage);
        emitLine("u_envRadiance = HdGetSampler_domeLightPrefilter()", mxStage);
        emitLine("#else", mxStage, false);
        emitLine("u_envIrradiance = HdGetSampler_domeLightFallback()", mxStage);
        emitLine("u_envRadiance = HdGetSampler_domeLightFallback()", mxStage);
        emitLine("#endif", mxStage, false);
    }
    emitLine("u_envRadianceMips = textureQueryLevels(u_envRadiance)", mxStage);
    Base::emitLineBreak(mxStage);

    // Initialize MaterialX Texture samplers with HdGetSampler equivalents
    if (_bindlessTexturesEnabled && !_mxHdTextureMap.empty()) {
        Base::emitComment("Initialize Material Textures", mxStage);
        for (mx::StringMap::const_reference texturePair : _mxHdTextureMap) {
            if (texturePair.first == "domeLightFallback") {
                continue;
            }
            emitLine(texturePair.first + " = "
                    "HdGetSampler_" + texturePair.second + "()", mxStage);
        }
        Base::emitLineBreak(mxStage);
    }

    // Gather Direct light data from Hydra and apply the Hydra transformation 
    // matrix to the environment map matrix (u_envMatrix) to account for the
    // domeLight's transform. 
    // Note: MaterialX initializes u_envMatrix as a 180 rotation about the 
    // Y-axis (Y-up)
    emitLine("mat4 hdTransformationMatrix = mat4(1.0)", mxStage);
    Base::emitString(MxHdLightString, mxStage);
    emitLine("u_envMatrix = u_envMatrix * hdTransformationMatrix", mxStage);

    Base::emitScopeEnd(mxStage);
    Base::emitLineBreak(mxStage);
}

// Generates the Mx VertexData that is needed for the Mx Shader
template<typename Base>
void 
HdStMaterialXShaderGen<Base>::_EmitMxVertexDataDeclarations(
    mx::VariableBlock const& block, 
    std::string const& mxVertexDataName, 
    std::string const& mxVertexDataVariable,
    std::string const& separator,
    mx::ShaderStage& mxStage) const
{
    // vd = mxVertexData
    std::string line = mxVertexDataVariable + " = " + mxVertexDataName;

    const std::string &targetShadingLanguage = Base::getTarget();

    // add beginning ( or {
    if (targetShadingLanguage == mx::GlslShaderGenerator::TARGET) {
        line += "(";
    }
    else if (targetShadingLanguage == mx::MslShaderGenerator::TARGET) {
        line += "{";
    }
    else {
        TF_CODING_ERROR("MaterialX Shader Generator doesn't support %s",
                        targetShadingLanguage.c_str());
    }

    for (size_t i = 0; i < block.size(); ++i) {
        auto const& lineSeparator =
            (i == block.size() - 1) ? mx::EMPTY_STRING : separator;
        line += _EmitMxVertexDataLine(block[i], lineSeparator);
    }

    // add ending ) or }
    if (targetShadingLanguage == mx::GlslShaderGenerator::TARGET) {
        line += ")";
    }
    else if (targetShadingLanguage == mx::MslShaderGenerator::TARGET) {
        line += "}";
    }

    emitLine(line, mxStage);
}

template<typename Base>
std::string
HdStMaterialXShaderGen<Base>::_EmitMxVertexDataLine(
    const mx::ShaderPort* variable,
    std::string const& separator) const
{
    // Connect the mxVertexData variable with the appropriate pxr variable
    // making sure to convert the Hd data (viewSpace) to Mx data (worldSpace)
    std::string hdVariableDef;
    const std::string mxVariableName = variable->getVariable();

    if (mxVariableName.compare(mx::HW::T_POSITION_WORLD) == 0 ||
        mxVariableName.compare(mx::HW::T_NORMAL_WORLD) == 0 ||
        mxVariableName.compare(mx::HW::T_TANGENT_WORLD) == 0) {

        // Calculated in MxHdWorldSpaceVectors
        hdVariableDef = mxVariableName.substr(1) + separator;
    }
    else if (mxVariableName.compare(mx::HW::T_POSITION_OBJECT) == 0) {

        hdVariableDef  = "HdGet_points()" + separator;
    }
    else if (mxVariableName.compare(mx::HW::T_NORMAL_OBJECT) == 0) {

        hdVariableDef  = "HdGet_normals()" + separator;
    }
    else if (mxVariableName.compare(0, mx::HW::T_TEXCOORD.size(), 
                                    mx::HW::T_TEXCOORD) == 0) {
        
        // Wrap initialization inside #ifdef in case the object does not have 
        // the st primvar
        hdVariableDef = TfStringPrintf("\n"
                "    #ifdef HD_HAS_%s\n"
                "        HdGet_%s()%s\n"
                "    #else\n"
                "        %s(0.0)%s\n"
                "    #endif\n        ", 
                _defaultTexcoordName.c_str(), _defaultTexcoordName.c_str(),
                separator.c_str(),
                Base::_syntax->getTypeName(variable->getType()).c_str(),
                separator.c_str());
    }
    else if (mxVariableName.compare(0, mx::HW::T_IN_GEOMPROP.size(), 
                                    mx::HW::T_IN_GEOMPROP) == 0) {
        // Wrap initialization inside #ifdef in case the object does not have 
        // the geomprop primvar
        // Note: variable name format: 'T_IN_GEOMPROP_geomPropName';
        const std::string geompropName = mxVariableName.substr(
                                            mx::HW::T_IN_GEOMPROP.size()+1);
        
        // Get the Default Value for the gromprop
        std::string defaultValueString = 
            Base::_syntax->getDefaultValue(variable->getType());
        mx::StringMap::const_iterator defaultValueIt = 
            _mxHdPrimvarDefaultValueMap.find(geompropName);
        if (defaultValueIt != _mxHdPrimvarDefaultValueMap.end()) {
            if (!defaultValueIt->second.empty()) {
                defaultValueString =
                    Base::_syntax->getTypeName(variable->getType())
                    + "(" + defaultValueIt->second + ")";
            }
        }
        hdVariableDef = TfStringPrintf("\n"
                "    #ifdef HD_HAS_%s\n"
                "        HdGet_%s()%s\n"
                "    #else\n"
                "        %s%s\n"
                "    #endif\n        ", 
                geompropName.c_str(), geompropName.c_str(),
                separator.c_str(),
                defaultValueString.c_str(),
                separator.c_str());
    }
    else {
        const std::string valueStr = variable->getValue() 
            ? Base::_syntax->getValue(
                variable->getType(), *variable->getValue(), true)
            : Base::_syntax->getDefaultValue(variable->getType(), true);
        hdVariableDef = valueStr.empty() 
            ? mx::EMPTY_STRING : valueStr + separator;
    }

    return hdVariableDef.empty() ? mx::EMPTY_STRING : hdVariableDef;
}

template<typename Base>
void
HdStMaterialXShaderGen<Base>::emitVariableDeclarations(
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
        Base::emitLineBegin(stage);
        const mx::ShaderPort* variable = block[i];
        const mx::TypeDesc* varType = variable->getType();

        // If bindlessTextures are not enabled the Mx Smpler names are mapped 
        // to the Hydra equivalents in HdStMaterialXShaderGen*::_EmitMxFunctions
        if (!_bindlessTexturesEnabled && varType == mx::Type::FILENAME) {
            continue;
        }

        // Only declare the variables that we need to initialize with Hd Data
        if ( (isPublicUniform && !_IsHardcodedPublicUniform(*varType))
            || MxHdVariables.count(variable->getName()) ) {
            Base::emitVariableDeclaration(variable, mx::EMPTY_STRING,
                                    context, stage, false /* assignValue */);
        }
        // Otherwise assign the value from MaterialX
        else {
            Base::emitVariableDeclaration(variable, qualifier,
                                    context, stage, assignValue);
        }
        Base::emitString(separator, stage);
        Base::emitLineEnd(stage, false);
    }
}

template<typename Base>
void 
HdStMaterialXShaderGen<Base>::emitLine(
    const std::string& str, 
    mx::ShaderStage& stage, 
    bool semicolon) const
{
    Base::emitLine(str, stage, semicolon);

    // When emitting the Light loop code for the Surface node, the variable
    // 'occlusion' represents shadow occlusion. We don't use MaterialX's
    // shadow implementation (hwShadowMap is false). Instead, use our own 
    // per-light occlusion value calculated in mxInit() and stored in lightData.
    // Note: Metal uses float3, Glsl uses vec3, in the line we're looking for.
    if (_emittingSurfaceNode && (str == "vec3 L = lightShader.direction" ||
                                 str == "float3 L = lightShader.direction" )) {
        emitLine(
            "occlusion = u_lightData[activeLightIndex].shadowOcclusion", stage);
    }
}


template<typename Base>
void
HdStMaterialXShaderGen<Base>::_EmitConstantsUniformsAndTypeDefs(
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage,
    const std::string& constQualifier) const
{
    // Add global constants and type definitions
    emitLine("#if NUM_LIGHTS > 0", mxStage, false);
    emitLine("#define MAX_LIGHT_SOURCES NUM_LIGHTS", mxStage, false);
    emitLine("#else", mxStage, false);
    emitLine("#define MAX_LIGHT_SOURCES 1", mxStage, false);
    emitLine("#endif", mxStage, false);
    emitLine("#define DIRECTIONAL_ALBEDO_METHOD " +
            std::to_string(int(
                mxContext.getOptions().hwDirectionalAlbedoMethod)),
            mxStage, false);
    Base::emitLineBreak(mxStage);

    // Add all constants and ensure that values are initialized
    const mx::VariableBlock& constants = mxStage.getConstantBlock();
    if (!constants.empty()) {
        emitVariableDeclarations(constants, constQualifier,
                                 mx::Syntax::SEMICOLON,
                                 mxContext, mxStage, true /* assignValue */);
        Base::emitLineBreak(mxStage);
    }

    // Add all uniforms
    for (mx::VariableBlockMap::const_reference it : mxStage.getUniformBlocks()){
        const mx::VariableBlock& uniforms = *it.second;

        // Skip light uniforms as they are handled separately
        if (!uniforms.empty() && uniforms.getName() != mx::HW::LIGHT_DATA) {
            Base::emitComment("Uniform block: " + uniforms.getName(), mxStage);
            emitVariableDeclarations(uniforms, mx::EMPTY_STRING,
                                     mx::Syntax::SEMICOLON, mxContext, 
                                     mxStage, true /* assignValue */);
            Base::emitLineBreak(mxStage);
        }
    }
}

template<typename Base>
void
HdStMaterialXShaderGen<Base>::_EmitDataStructsAndFunctionDefinitions(
    const mx::ShaderGraph& mxGraph,
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage,
    MaterialX::StringMap* tokenSubstitutions) const
{
    const bool lighting =
        mxGraph.hasClassification(mx::ShaderNode::Classification::SHADER |
                                  mx::ShaderNode::Classification::SURFACE)
        || mxGraph.hasClassification(mx::ShaderNode::Classification::BSDF);
    const bool shadowing =
        (lighting && mxContext.getOptions().hwShadowMap)
        || mxContext.getOptions().hwWriteDepthMoments;

    // Add light data block if needed
    if (lighting) {
        const mx::VariableBlock& lightData =
            mxStage.getUniformBlock(mx::HW::LIGHT_DATA);
        emitLine("struct " + lightData.getName(), mxStage, false);
        Base::emitScopeBegin(mxStage);
        emitVariableDeclarations(lightData, mx::EMPTY_STRING,
                                 mx::Syntax::SEMICOLON,
                                 mxContext, mxStage, false /* assignValue */);
        Base::emitScopeEnd(mxStage, true);
        Base::emitLineBreak(mxStage);
        emitLine(lightData.getName() + " "
                + lightData.getInstance() + "[MAX_LIGHT_SOURCES]", mxStage);
        Base::emitLineBreak(mxStage);
        Base::emitLineBreak(mxStage);
    }

    // Add vertex data struct and the mxInit function which initializes mx
    // values with the Hd equivalents
    const mx::VariableBlock& vertexData =
        mxStage.getInputBlock(mx::HW::VERTEX_DATA);
    if (!vertexData.empty()) {

        // add Mx VertexData
        Base::emitComment("MaterialX's VertexData", mxStage);
        std::string mxVertexDataName = "mx" + vertexData.getName();
        emitLine("struct " + mxVertexDataName, mxStage, false);
        Base::emitScopeBegin(mxStage);
        emitVariableDeclarations(vertexData, mx::EMPTY_STRING,
                                 mx::Syntax::SEMICOLON,
                                 mxContext, mxStage, false /* assignValue */);
        Base::emitScopeEnd(mxStage, false, false);
        Base::emitString(mx::Syntax::SEMICOLON, mxStage);
        Base::emitLineBreak(mxStage);

        // Add the vd declaration
        emitLine(mxVertexDataName + " " + vertexData.getInstance(), mxStage);
        Base::emitLineBreak(mxStage);
        Base::emitLineBreak(mxStage);
    }

    // add the mxInit function to convert Hd -> Mx data
    _EmitMxInitFunction(vertexData, mxStage);

    // Emit lighting and shadowing code
    if (lighting) {
        Base::emitSpecularEnvironment(mxContext, mxStage);
        Base::emitTransmissionRender(mxContext, mxStage);
    }
    if (shadowing) {
        mx::ShaderGenerator::emitLibraryInclude(
            "pbrlib/" + mx::GlslShaderGenerator::TARGET
            + "/lib/mx_shadow.glsl", mxContext, mxStage);
    }

    // Emit directional albedo table code.
    if (mxContext.getOptions().hwDirectionalAlbedoMethod ==
            mx::HwDirectionalAlbedoMethod::DIRECTIONAL_ALBEDO_TABLE ||
        mxContext.getOptions().hwWriteAlbedoTable) {
        mx::ShaderGenerator::emitLibraryInclude(
            "pbrlib/" + mx::GlslShaderGenerator::TARGET
            + "/lib/mx_table.glsl", mxContext, mxStage);
        Base::emitLineBreak(mxStage);
    }

    // Set the include file to use for uv transformations,
    // depending on the vertical flip flag.
    if (mxContext.getOptions().fileTextureVerticalFlip) {
        (*tokenSubstitutions)[mx::ShaderGenerator::T_FILE_TRANSFORM_UV] =
            "mx_transform_uv_vflip.glsl";
    }
    else {
        (*tokenSubstitutions)[mx::ShaderGenerator::T_FILE_TRANSFORM_UV] =
            "mx_transform_uv.glsl";
    }

    // Emit uv transform code globally if needed.
    if (mxContext.getOptions().hwAmbientOcclusion) {
        mx::ShaderGenerator::emitLibraryInclude(
            "stdlib/" + Base::TARGET + "/lib/" +
                (*tokenSubstitutions)[mx::ShaderGenerator::T_FILE_TRANSFORM_UV],
            mxContext, mxStage);
    }

    // Prior to MaterialX 1.38.5 the token substitutions need to
    // include the full path to the .glsl files, so we prepend that
    // here.
#if MATERIALX_MAJOR_VERSION == 1 && MATERIALX_MINOR_VERSION == 38
    #if MATERIALX_BUILD_VERSION < 4
        (*tokenSubstitutions)[mx::ShaderGenerator::T_FILE_TRANSFORM_UV].insert(
            0, "stdlib/" + Base::TARGET + "/lib/");
    #elif MATERIALX_BUILD_VERSION == 4
        (*tokenSubstitutions)[mx::ShaderGenerator::T_FILE_TRANSFORM_UV].insert(
            0, "libraries/stdlib/" + Base::TARGET + "/lib/");
    #endif
#endif

    // Add light sampling functions
    Base::emitLightFunctionDefinitions(mxGraph, mxContext, mxStage);

    // Add all functions for node implementations
    Base::emitFunctionDefinitions(mxGraph, mxContext, mxStage);
}

// ----------------------------------------------------------------------------
//                          HdSt MaterialX ShaderGen OpenGL GLSL
// ----------------------------------------------------------------------------

namespace {
    // Create a customized version of the class mx::SurfaceNodeGlsl
    // to be able to notify the shader generator when we start/end
    // emitting the code for the SurfaceNode
    class HdStMaterialXSurfaceNodeGenGlsl : public mx::SurfaceNodeGlsl
    {
    public:
        static mx::ShaderNodeImplPtr create() {
            return std::make_shared<HdStMaterialXSurfaceNodeGenGlsl>();
        }

        void emitFunctionCall(
            const mx::ShaderNode& node, 
            mx::GenContext& context,
            mx::ShaderStage& stage) const override
        {
            HdStMaterialXShaderGenGlsl& shadergen =
                static_cast<HdStMaterialXShaderGenGlsl&>(
                    context.getShaderGenerator());
            
            shadergen.SetEmittingSurfaceNode(true);
            mx::SurfaceNodeGlsl::emitFunctionCall(node, context, stage);
            shadergen.SetEmittingSurfaceNode(false);
        }
    };
}


template<>
HdStMaterialXShaderGen<mx::GlslShaderGenerator>::HdStMaterialXShaderGen(
    HdSt_MxShaderGenInfo const& mxHdInfo)
    : mx::GlslShaderGenerator(),
      _mxHdTextureMap(mxHdInfo.textureMap),
      _mxHdPrimvarMap(mxHdInfo.primvarMap),
      _mxHdPrimvarDefaultValueMap(mxHdInfo.primvarDefaultValueMap),
      _materialTag(mxHdInfo.materialTag),
      _bindlessTexturesEnabled(mxHdInfo.bindlessTexturesEnabled),
      _emittingSurfaceNode(false)
{
    _defaultTexcoordName =
        (mxHdInfo.defaultTexcoordName == mx::EMPTY_STRING)
            ? "st" : mxHdInfo.defaultTexcoordName;

}

HdStMaterialXShaderGenGlsl::HdStMaterialXShaderGenGlsl(
    HdSt_MxShaderGenInfo const& mxHdInfo)
    : HdStMaterialXShaderGen<mx::GlslShaderGenerator>(mxHdInfo)
{
    // Register the customized version of the Surface node generator
    registerImplementation("IM_surface_" + mx::GlslShaderGenerator::TARGET,
        HdStMaterialXSurfaceNodeGenGlsl::create);
}

// Based on GlslShaderGenerator::generate()
// Generates a glslfx shader and stores that in the pixel shader stage where it
// can be retrieved with getSourceCode()
mx::ShaderPtr
HdStMaterialXShaderGenGlsl::generate(
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
HdStMaterialXShaderGenGlsl::_EmitGlslfxShader(
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

// Similar to GlslShaderGenerator::emitPixelStage() with alterations and
// additions to match Pxr's codeGen
void
HdStMaterialXShaderGenGlsl::_EmitMxFunctions(
    const mx::ShaderGraph& mxGraph,
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage) const
{
    mx::ShaderGenerator::emitLibraryInclude(
        "stdlib/" + mx::GlslShaderGenerator::TARGET
        + "/lib/mx_math.glsl", mxContext, mxStage);

    // Add type definitions
    emitTypeDefinitions(mxContext, mxStage);

    _EmitConstantsUniformsAndTypeDefs(
        mxContext, mxStage, _syntax->getConstantQualifier());

    // If bindlessTextures are not enabled, the above for loop skips
    // initializing textures. Initialize them here by defining mappings
    // to the appropriate HdGetSampler function.
    if (!_bindlessTexturesEnabled) {

        // Define mappings for the DomeLight Textures
        emitLine("#ifdef HD_HAS_domeLightIrradiance", mxStage, false);
        emitLine("#define u_envRadiance "
                 "HdGetSampler_domeLightPrefilter() ", mxStage, false);
        emitLine("#define u_envIrradiance "
                "HdGetSampler_domeLightIrradiance() ", mxStage, false);
        emitLine("#else", mxStage, false);
        emitLine("#define u_envRadiance "
                "HdGetSampler_domeLightFallback()", mxStage, false);
        emitLine("#define u_envIrradiance "
                "HdGetSampler_domeLightFallback()", mxStage, false);
        emitLine("#endif", mxStage, false);
        emitLineBreak(mxStage);

        // Define mappings for the MaterialX Textures
        if (!_mxHdTextureMap.empty()) {
            emitComment("Define MaterialX to Hydra Sampler mappings", mxStage);
            for (mx::StringMap::const_reference texturePair : _mxHdTextureMap) {
                if (texturePair.first == "domeLightFallback") {
                    continue;
                }
                emitLine(TfStringPrintf(
                    "#define %s HdGetSampler_%s()",
                        texturePair.first.c_str(),
                        texturePair.second.c_str()),
                    mxStage, false);
            }
            emitLineBreak(mxStage);
        }
    }

    _EmitDataStructsAndFunctionDefinitions(
        mxGraph, mxContext, mxStage, &_tokenSubstitutions);
}

// ----------------------------------------------------------------------------
//                    HdSt MaterialX ShaderGen Vulkan GLSL
// ----------------------------------------------------------------------------

namespace {
    // Create a customized version of the class mx::SurfaceNodeGlsl
    // to be able to notify the shader generator when we start/end
    // emitting the code for the SurfaceNode
    class HdStMaterialXSurfaceNodeGenVkGlsl : public mx::SurfaceNodeGlsl
    {
    public:
        static mx::ShaderNodeImplPtr create() {
            return std::make_shared<HdStMaterialXSurfaceNodeGenVkGlsl>();
        }

        void emitFunctionCall(
            const mx::ShaderNode& node, 
            mx::GenContext& context,
            mx::ShaderStage& stage) const override
        {
            HdStMaterialXShaderGenVkGlsl& shadergen =
                static_cast<HdStMaterialXShaderGenVkGlsl&>(
                    context.getShaderGenerator());
            
            shadergen.SetEmittingSurfaceNode(true);
            mx::SurfaceNodeGlsl::emitFunctionCall(node, context, stage);
            shadergen.SetEmittingSurfaceNode(false);
        }
    };
}

template<>
HdStMaterialXShaderGen<mx::VkShaderGenerator>::HdStMaterialXShaderGen(
    HdSt_MxShaderGenInfo const& mxHdInfo)
    : mx::VkShaderGenerator(),
      _mxHdTextureMap(mxHdInfo.textureMap),
      _mxHdPrimvarMap(mxHdInfo.primvarMap),
      _mxHdPrimvarDefaultValueMap(mxHdInfo.primvarDefaultValueMap),
      _materialTag(mxHdInfo.materialTag),
      _bindlessTexturesEnabled(mxHdInfo.bindlessTexturesEnabled),
      _emittingSurfaceNode(false)
{
    _defaultTexcoordName =
        (mxHdInfo.defaultTexcoordName == mx::EMPTY_STRING)
            ? "st" : mxHdInfo.defaultTexcoordName;

}

HdStMaterialXShaderGenVkGlsl::HdStMaterialXShaderGenVkGlsl(
    HdSt_MxShaderGenInfo const& mxHdInfo)
    : HdStMaterialXShaderGen<mx::VkShaderGenerator>(mxHdInfo)
{
    // Register the customized version of the Surface node generator
    registerImplementation("IM_surface_" + mx::VkShaderGenerator::TARGET,
        HdStMaterialXSurfaceNodeGenVkGlsl::create);
}

// Based on GlslShaderGenerator::generate()
// Generates a glslfx shader and stores that in the pixel shader stage where it
// can be retrieved with getSourceCode()
mx::ShaderPtr
HdStMaterialXShaderGenVkGlsl::generate(
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
HdStMaterialXShaderGenVkGlsl::_EmitGlslfxShader(
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

// Similar to GlslShaderGenerator::emitPixelStage() with alterations and
// additions to match Pxr's codeGen
void
HdStMaterialXShaderGenVkGlsl::_EmitMxFunctions(
    const mx::ShaderGraph& mxGraph,
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage) const
{
    emitLibraryInclude("stdlib/" + mx::VkShaderGenerator::TARGET
                       + "/lib/mx_math.glsl", mxContext, mxStage);

    // Add type definitions
    emitTypeDefinitions(mxContext, mxStage);

    _EmitConstantsUniformsAndTypeDefs(
        mxContext, mxStage, _syntax->getConstantQualifier());

    // If bindlessTextures are not enabled, the above for loop skips
    // initializing textures. Initialize them here by defining mappings
    // to the appropriate HdGetSampler function.
    if (!_bindlessTexturesEnabled) {

        // Define mappings for the DomeLight Textures
        emitLine("#ifdef HD_HAS_domeLightIrradiance", mxStage, false);
        emitLine("#define u_envRadiance "
                 "HdGetSampler_domeLightPrefilter() ", mxStage, false);
        emitLine("#define u_envIrradiance "
                "HdGetSampler_domeLightIrradiance() ", mxStage, false);
        emitLine("#else", mxStage, false);
        emitLine("#define u_envRadiance "
                "HdGetSampler_domeLightFallback()", mxStage, false);
        emitLine("#define u_envIrradiance "
                "HdGetSampler_domeLightFallback()", mxStage, false);
        emitLine("#endif", mxStage, false);
        emitLineBreak(mxStage);

        // Define mappings for the MaterialX Textures
        if (!_mxHdTextureMap.empty()) {
            emitComment("Define MaterialX to Hydra Sampler mappings", mxStage);
            for (mx::StringMap::const_reference texturePair : _mxHdTextureMap) {
                if (texturePair.first == "domeLightFallback") {
                    continue;
                }
                emitLine(TfStringPrintf(
                    "#define %s HdGetSampler_%s()",
                        texturePair.first.c_str(),
                        texturePair.second.c_str()),
                    mxStage, false);
            }
            emitLineBreak(mxStage);
        }
    }

    _EmitDataStructsAndFunctionDefinitions(
        mxGraph, mxContext, mxStage, &_tokenSubstitutions);
}

// ----------------------------------------------------------------------------
//                          HdSt MaterialX ShaderGen Metal
// ----------------------------------------------------------------------------

namespace {
    // Create a customized version of the class mx::SurfaceNodeMsl
    // to be able to notify the shader generator when we start/end
    // emitting the code for the SurfaceNode
    class HdStMaterialXSurfaceNodeGenMsl : public mx::SurfaceNodeMsl
    {
    public:
        static mx::ShaderNodeImplPtr create() {
            return std::make_shared<HdStMaterialXSurfaceNodeGenMsl>();
        }

        void emitFunctionCall(
            const mx::ShaderNode& node,
            mx::GenContext& context,
            mx::ShaderStage& stage) const override
        {
            HdStMaterialXShaderGenMsl& shadergen =
                static_cast<HdStMaterialXShaderGenMsl&>(
                    context.getShaderGenerator());

            shadergen.SetEmittingSurfaceNode(true);
            mx::SurfaceNodeMsl::emitFunctionCall(node, context, stage);
            shadergen.SetEmittingSurfaceNode(false);
        }
    };
}

template<>
HdStMaterialXShaderGen<mx::MslShaderGenerator>::HdStMaterialXShaderGen(
    HdSt_MxShaderGenInfo const& mxHdInfo)
    : mx::MslShaderGenerator(),
      _mxHdTextureMap(mxHdInfo.textureMap),
      _mxHdPrimvarMap(mxHdInfo.primvarMap),
      _mxHdPrimvarDefaultValueMap(mxHdInfo.primvarDefaultValueMap),
      _materialTag(mxHdInfo.materialTag),
      _bindlessTexturesEnabled(mxHdInfo.bindlessTexturesEnabled),
      _emittingSurfaceNode(false)
{
    _defaultTexcoordName =
        (mxHdInfo.defaultTexcoordName == mx::EMPTY_STRING)
            ? "st" : mxHdInfo.defaultTexcoordName;
}

HdStMaterialXShaderGenMsl::HdStMaterialXShaderGenMsl(
    HdSt_MxShaderGenInfo const& mxHdInfo)
    : HdStMaterialXShaderGen<mx::MslShaderGenerator>(mxHdInfo)
{
    // Register the customized version of the Surface node generator
    registerImplementation("IM_surface_" + mx::MslShaderGenerator::TARGET,
        HdStMaterialXSurfaceNodeGenMsl::create);
}

// Based on MslShaderGenerator::generate()
// Generates a glslfx shader and stores that in the pixel shader stage where it
// can be retrieved with getSourceCode()
mx::ShaderPtr
HdStMaterialXShaderGenMsl::generate(
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
    _EmitGlslfxMetalShader(shader->getGraph(), mxContext, shaderStage);
    replaceTokens(_tokenSubstitutions, shaderStage);

    // Metalize the glslfx shader
    MetalizeGeneratedShader(shaderStage);

    // USD has its own decleration of radians function.
    // We need to remove MaterialX declaration
    {
        std::string sourceCode = shaderStage.getSourceCode();
        size_t loc = sourceCode.find("float radians(float degree)");
        if(loc != std::string::npos) {
            sourceCode.insert(loc, "//");
        }
        shaderStage.setSourceCode(sourceCode);
    }

    return shader;
}

void
HdStMaterialXShaderGenMsl::_EmitGlslfxMetalShader(
    const mx::ShaderGraph& mxGraph,
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage) const
{
    _EmitGlslfxMetalHeader(mxContext, mxStage);

    mx::HwResourceBindingContextPtr resourceBindingCtx =
        getResourceBindingContext(mxContext);
    if (!resourceBindingCtx) {
        mxContext.pushUserData(
            mx::HW::USER_DATA_BINDING_CONTEXT,
            mx::MslResourceBindingContext::create());
        resourceBindingCtx = mxContext.getUserData<
            mx::HwResourceBindingContext>(mx::HW::USER_DATA_BINDING_CONTEXT);
    }
    resourceBindingCtx->emitDirectives(mxContext, mxStage);

    // Add a per-light shadowOcclusion value to the lightData uniform block
    addStageUniform(mx::HW::LIGHT_DATA, mx::Type::FLOAT,
        "shadowOcclusion", mxStage);

    // Add type definitions
    emitTypeDefinitions(mxContext, mxStage);

    emitConstantBufferDeclarations(mxContext, resourceBindingCtx, mxStage);

    // Add all constants
    emitConstants(mxContext, mxStage);

    // Add vertex data inputs block
    emitInputs(mxContext, mxStage);

    // Add the pixel shader output. 
    // This needs to be a float4 for rendering and upstream connection will be
    // converted to float4 if needed in emitFinalOutput()
    emitOutputs(mxContext, mxStage);

    _EmitMxFunctions(mxGraph, mxContext, mxStage);
    emitLine("#undef material", mxStage, false);
    _EmitMxSurfaceShader(mxGraph, mxContext, mxStage);
}

void
HdStMaterialXShaderGenMsl::_EmitGlslfxMetalHeader(
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage) const
{
    _EmitGlslfxHeader(mxStage);
    emitLineBreak(mxStage);
    emitLineBreak(mxStage);
    emitLine("//Metal Shading Language version " + getVersion(), mxStage, false);
    emitLine("#define __METAL__ 1", mxStage, false);
    emitMetalTextureClass(mxContext, mxStage);
}

// Similar to MslShaderGenerator::emitPixelStage() with alterations and
// additions to match Pxr's codeGen
void
HdStMaterialXShaderGenMsl::_EmitMxFunctions(
    const mx::ShaderGraph& mxGraph,
    mx::GenContext& mxContext,
    mx::ShaderStage& mxStage) const
{
    mx::ShaderGenerator::emitLibraryInclude(
        "pbrlib/" + mx::GlslShaderGenerator::TARGET
        + "/lib/mx_microfacet.glsl", mxContext, mxStage);
    mx::ShaderGenerator::emitLibraryInclude(
        "stdlib/" + mx::MslShaderGenerator::TARGET
        + "/lib/mx_math.metal", mxContext, mxStage);
    _EmitConstantsUniformsAndTypeDefs(
        mxContext, mxStage,_syntax->getConstantQualifier());

    // If bindlessTextures are not enabled, the above for loop skips
    // initializing textures. Initialize them here by defining mappings
    // to the appropriate HdGetSampler function.
    if (!_bindlessTexturesEnabled) {

        // Define mappings for the DomeLight Textures
        emitLine("#ifdef HD_HAS_domeLightIrradiance", mxStage, false);
        emitLine("#define u_envRadiance "
                "MetalTexture{HdGetSampler_domeLightPrefilter(), "
                "samplerBind_domeLightPrefilter} ", mxStage, false);
        emitLine("#define u_envIrradiance "
                "MetalTexture{HdGetSampler_domeLightIrradiance(), "
                "samplerBind_domeLightIrradiance} ", mxStage, false);
        emitLine("#else", mxStage, false);
        emitLine("#define u_envRadiance "
                "MetalTexture{HdGetSampler_domeLightFallback(), "
                "samplerBind_domeLightFallback}", mxStage, false);
        emitLine("#define u_envIrradiance "
                "MetalTexture{HdGetSampler_domeLightFallback(), "
                "samplerBind_domeLightFallback}", mxStage, false);
        emitLine("#endif", mxStage, false);
        emitLineBreak(mxStage);

        // Define mappings for the MaterialX Textures
        if (!_mxHdTextureMap.empty()) {
            emitComment("Define MaterialX to Hydra Sampler mappings", mxStage);
            for (mx::StringMap::const_reference texturePair : _mxHdTextureMap) {
                if (texturePair.first == "domeLightFallback") {
                    continue;
                }
                emitLine(TfStringPrintf(
                    "#define %s MetalTexture{HdGetSampler_%s(), samplerBind_%s}",
                        texturePair.first.c_str(),
                        texturePair.second.c_str(),
                        texturePair.second.c_str()),
                    mxStage, false);
            }
            emitLineBreak(mxStage);
        }
    }

    _EmitDataStructsAndFunctionDefinitions(
        mxGraph, mxContext, mxStage, &_tokenSubstitutions);
}


PXR_NAMESPACE_CLOSE_SCOPE
