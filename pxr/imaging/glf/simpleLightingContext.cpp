//
// Copyright 2016 Pixar
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
/// \file simpleLightingContext.cpp

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/debugCodes.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"
#include "pxr/imaging/glf/uniformBlock.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/arch/hash.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/base/trace/trace.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((lightingUB, "Lighting"))
    ((shadowUB, "Shadow"))
    ((bindlessShadowUB, "BindlessShadowSamplers"))
    ((materialUB, "Material"))
    ((postSurfaceShaderUB, "PostSurfaceShaderParams"))
    ((shadowSampler, "shadowTexture"))
    ((shadowCompareSampler, "shadowCompareTexture"))
);

// XXX:
// currently max number of lights are limited to 16 by
// GL_MAX_VARYING_VECTORS for having the varying attribute
//    out vec2 FshadowFilterWidth[NUM_LIGHTS];
// which is defined in simpleLighting.glslfx.
static const int _maxLightsUsed = 16;

/* static */
GlfSimpleLightingContextRefPtr
GlfSimpleLightingContext::New()
{
    return TfCreateRefPtr(new This());
}

GlfSimpleLightingContext::GlfSimpleLightingContext() :
    _shadows(TfCreateRefPtr(new GlfSimpleShadowArray())),
    _worldToViewMatrix(1.0),
    _projectionMatrix(1.0),
    _sceneAmbient(0.01, 0.01, 0.01, 1.0),
    _useLighting(false),
    _useShadows(false),
    _useColorMaterialDiffuse(false),
    _lightingUniformBlockValid(false),
    _shadowUniformBlockValid(false),
    _materialUniformBlockValid(false),
    _postSurfaceShaderStateValid(false)
{
}

GlfSimpleLightingContext::~GlfSimpleLightingContext()
{
}

void
GlfSimpleLightingContext::SetLights(GlfSimpleLightVector const & lights)
{
    _lights = lights;
    _lightingUniformBlockValid = false;
    _shadowUniformBlockValid = false;
    _postSurfaceShaderStateValid = false;

    int numLights = GetNumLightsUsed();

    _useShadows = false;
    for (int i = 0;i < numLights; ++i) {
        if (_lights[i].HasShadow()) {
            _useShadows = true;
            break;
        }
    }
}

const GlfSimpleLightVector &
GlfSimpleLightingContext::GetLights() const
{
    return _lights;
}

int
GlfSimpleLightingContext::GetNumLightsUsed() const
{
    return std::min((int)_lights.size(), _maxLightsUsed);
}

int
GlfSimpleLightingContext::ComputeNumShadowsUsed() const
{
    int numShadows = 0;
    for (auto const& light : _lights) {
        if (light.HasShadow() && numShadows <= light.GetShadowIndexEnd()) {
            numShadows = light.GetShadowIndexEnd() + 1;
        }
    }
    return numShadows;
}

void
GlfSimpleLightingContext::SetShadows(GlfSimpleShadowArrayRefPtr const & shadows)
{
    _shadows = shadows;
    _shadowUniformBlockValid = false;
}

GlfSimpleShadowArrayRefPtr const &
GlfSimpleLightingContext::GetShadows() const
{
    return _shadows;
}

void
GlfSimpleLightingContext::SetMaterial(GlfSimpleMaterial const & material)
{
    if (_material != material) {
        _material = material;
        _materialUniformBlockValid = false;
    }
}

GlfSimpleMaterial const &
GlfSimpleLightingContext::GetMaterial() const
{
    return _material;
}

void
GlfSimpleLightingContext::SetSceneAmbient(GfVec4f const & sceneAmbient)
{
    if (_sceneAmbient != sceneAmbient) {
        _sceneAmbient = sceneAmbient;
        _materialUniformBlockValid = false;
    }
}

GfVec4f const &
GlfSimpleLightingContext::GetSceneAmbient() const
{
    return _sceneAmbient;
}

void
GlfSimpleLightingContext::SetCamera(GfMatrix4d const &worldToViewMatrix,
                                     GfMatrix4d const &projectionMatrix)
{
    if (_worldToViewMatrix != worldToViewMatrix) {
        _worldToViewMatrix = worldToViewMatrix;
        _lightingUniformBlockValid = false;
        _shadowUniformBlockValid = false;
    }
    _projectionMatrix = projectionMatrix;
}

void
GlfSimpleLightingContext::SetUseLighting(bool val)
{
    if (_useLighting != val) {
        _useLighting = val;
        _lightingUniformBlockValid = false;
    }
}

bool
GlfSimpleLightingContext::GetUseLighting() const
{
    return _useLighting;
}

bool
GlfSimpleLightingContext::GetUseShadows() const
{
    return _useShadows;
}

void
GlfSimpleLightingContext::SetUseColorMaterialDiffuse(bool val)
{
    if (_useColorMaterialDiffuse != val) {
        _lightingUniformBlockValid = false;
        _useColorMaterialDiffuse = val;
    }
}

bool
GlfSimpleLightingContext::GetUseColorMaterialDiffuse() const
{
    return _useColorMaterialDiffuse;
}

void
GlfSimpleLightingContext::InitUniformBlockBindings(
        GlfBindingMapPtr const &bindingMap) const
{
    // populate uniform bindings (XXX: need better API)
    bindingMap->GetUniformBinding(_tokens->lightingUB);
    bindingMap->GetUniformBinding(_tokens->shadowUB);
    bindingMap->GetUniformBinding(_tokens->materialUB);
    bindingMap->GetUniformBinding(_tokens->postSurfaceShaderUB);

    if (GlfSimpleShadowArray::GetBindlessShadowMapsEnabled()) {
        bindingMap->GetUniformBinding(_tokens->bindlessShadowUB);
    }
}

void
GlfSimpleLightingContext::InitSamplerUnitBindings(
        GlfBindingMapPtr const &bindingMap) const
{
    if (!GlfSimpleShadowArray::GetBindlessShadowMapsEnabled()) {
        bindingMap->GetSamplerUnit(_tokens->shadowSampler);
        bindingMap->GetSamplerUnit(_tokens->shadowCompareSampler);
    }
}

inline void
setVec3(float *dst, GfVec3f const & vec)
{
    dst[0] = vec[0];
    dst[1] = vec[1];
    dst[2] = vec[2];
}

inline static void
setVec4(float *dst, GfVec4f const &vec)
{
    dst[0] = vec[0];
    dst[1] = vec[1];
    dst[2] = vec[2];
    dst[3] = vec[3];
}

inline static void
setMatrix(float *dst, GfMatrix4d const & mat)
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            dst[i*4+j] = (float)mat[i][j];
}

void
GlfSimpleLightingContext::BindUniformBlocks(GlfBindingMapPtr const &bindingMap)
{
    GLF_GROUP_FUNCTION();
    
    if (!_lightingUniformBlock)
        _lightingUniformBlock = GlfUniformBlock::New("_lightingUniformBlock");
    if (!_shadowUniformBlock)
        _shadowUniformBlock = GlfUniformBlock::New("_shadowUniformBlock");
    if (!_materialUniformBlock)
        _materialUniformBlock = GlfUniformBlock::New("_materialUniformBlock");
    
    const bool usingBindlessShadowMaps = 
        GlfSimpleShadowArray::GetBindlessShadowMapsEnabled();
    
    if (usingBindlessShadowMaps && !_bindlessShadowlUniformBlock) {
        _bindlessShadowlUniformBlock =
            GlfUniformBlock::New("_bindlessShadowUniformBlock");
    }

    bool shadowExists = false;
    if ((!_lightingUniformBlockValid ||
         !_shadowUniformBlockValid) && _lights.size() > 0) {
        int numLights = GetNumLightsUsed();
        int numShadows = ComputeNumShadowsUsed();

        // 16byte aligned
        struct LightSource {
            float position[4];
            float ambient[4];
            float diffuse[4];
            float specular[4];
            float spotDirection[4];
            float spotCutoff;
            float spotFalloff;
            float padding[2];
            float attenuation[4];
            float worldToLightTransform[16];
            int32_t shadowIndexStart;
            int32_t shadowIndexEnd;
            int32_t hasShadow;
            int32_t isIndirectLight;
        };

        struct Lighting {
            int32_t useLighting;
            int32_t useColorMaterialDiffuse;
            int32_t padding[2];
            ARCH_PRAGMA_PUSH
            ARCH_PRAGMA_ZERO_SIZED_STRUCT
            LightSource lightSource[0];
            ARCH_PRAGMA_POP
        };

        // 16byte aligned
        struct ShadowMatrix {
            float viewToShadowMatrix[16];
            float shadowToViewMatrix[16];
            float blur;
            float bias;
            float padding[2];
        };

        struct Shadow {
            ARCH_PRAGMA_PUSH
            ARCH_PRAGMA_ZERO_SIZED_STRUCT
            ShadowMatrix shadow[0];
            ARCH_PRAGMA_POP
        };

        // Use a uniform buffer block for the array of 64bit bindless handles.
        //
        // glf/shaders/simpleLighting.glslfx uses a uvec2 array instead of
        // uint64_t.
        // Note that uint64_t has different padding rules depending on the
        // layout: std140 results in 128bit alignment, while shared (default)
        // results in 64bit alignment.
        struct PaddedHandle {
            uint64_t handle;
            //uint64_t padding; // Skip padding since we don't need it.
        };

        struct BindlessShadowSamplers {
            ARCH_PRAGMA_PUSH
            ARCH_PRAGMA_ZERO_SIZED_STRUCT
            PaddedHandle shadowCompareTextures[0];
            ARCH_PRAGMA_POP
        };

        size_t lightingSize = sizeof(Lighting) + sizeof(LightSource) * numLights;
        size_t shadowSize = sizeof(ShadowMatrix) * numShadows;
        Lighting *lightingData = (Lighting *)alloca(lightingSize);
        Shadow *shadowData = (Shadow *)alloca(shadowSize);
        memset(shadowData, 0, shadowSize);
        memset(lightingData, 0, lightingSize);
        
        BindlessShadowSamplers *bindlessHandlesData = nullptr;
        size_t bindlessHandlesSize = 0;
        if (usingBindlessShadowMaps) {
            bindlessHandlesSize = sizeof(PaddedHandle) * numShadows;
            bindlessHandlesData = 
                (BindlessShadowSamplers*)alloca(bindlessHandlesSize);
            memset(bindlessHandlesData, 0, bindlessHandlesSize);
        }

        GfMatrix4d viewToWorldMatrix = _worldToViewMatrix.GetInverse();

        lightingData->useLighting = _useLighting;
        lightingData->useColorMaterialDiffuse = _useColorMaterialDiffuse;

        for (int i = 0; _useLighting && i < numLights; ++i) {
            GlfSimpleLight const &light = _lights[i];

            setVec4(lightingData->lightSource[i].position,
                    light.GetPosition() * _worldToViewMatrix);
            setVec4(lightingData->lightSource[i].diffuse, light.GetDiffuse());
            setVec4(lightingData->lightSource[i].ambient, light.GetAmbient());
            setVec4(lightingData->lightSource[i].specular, light.GetSpecular());
            setVec3(lightingData->lightSource[i].spotDirection,
                    _worldToViewMatrix.TransformDir(light.GetSpotDirection()));
            setVec3(lightingData->lightSource[i].attenuation,
                    light.GetAttenuation());
            lightingData->lightSource[i].spotCutoff = light.GetSpotCutoff();
            lightingData->lightSource[i].spotFalloff = light.GetSpotFalloff();
            setMatrix(lightingData->lightSource[i].worldToLightTransform,
                      light.GetTransform().GetInverse());
            lightingData->lightSource[i].hasShadow = light.HasShadow();
            lightingData->lightSource[i].isIndirectLight = light.IsDomeLight();

            if (lightingData->lightSource[i].hasShadow) {
                int shadowIndexStart = light.GetShadowIndexStart();
                lightingData->lightSource[i].shadowIndexStart =
                    shadowIndexStart;
                int shadowIndexEnd = light.GetShadowIndexEnd();
                lightingData->lightSource[i].shadowIndexEnd = shadowIndexEnd;

                for (int shadowIndex = shadowIndexStart;
                     shadowIndex <= shadowIndexEnd; ++shadowIndex) {
                    GfMatrix4d viewToShadowMatrix = viewToWorldMatrix *
                        _shadows->GetWorldToShadowMatrix(shadowIndex);
                    GfMatrix4d shadowToViewMatrix =
                        viewToShadowMatrix.GetInverse();

                    shadowData->shadow[shadowIndex].bias = light.GetShadowBias();
                    shadowData->shadow[shadowIndex].blur = light.GetShadowBlur();
                    
                    setMatrix(
                        shadowData->shadow[shadowIndex].viewToShadowMatrix,
                        viewToShadowMatrix);
                    setMatrix(
                        shadowData->shadow[shadowIndex].shadowToViewMatrix,
                        shadowToViewMatrix);
                }

                shadowExists = true;
            }
        }

        _lightingUniformBlock->Update(lightingData, lightingSize);
        _lightingUniformBlockValid = true;

        if (shadowExists) {
            _shadowUniformBlock->Update(shadowData, shadowSize);
            _shadowUniformBlockValid = true;

            if (usingBindlessShadowMaps) {
                std::vector<uint64_t> const& shadowMapHandles =
                    _shadows->GetBindlessShadowMapHandles();

                for (size_t i = 0; i < shadowMapHandles.size(); i++) {
                    bindlessHandlesData->shadowCompareTextures[i].handle
                        = shadowMapHandles[i];
                }

                _bindlessShadowlUniformBlock->Update(
                    bindlessHandlesData, bindlessHandlesSize);
            }
        }
    }

    _lightingUniformBlock->Bind(bindingMap, _tokens->lightingUB);

    if (shadowExists) {
        _shadowUniformBlock->Bind(bindingMap, _tokens->shadowUB);

        if (usingBindlessShadowMaps) {
            _bindlessShadowlUniformBlock->Bind(
                bindingMap, _tokens->bindlessShadowUB);
        }
    }

    if (!_materialUniformBlockValid) {
        // has to be matched with the definition of simpleLightingShader.glslfx
        struct Material {
            float ambient[4];
            float diffuse[4];
            float specular[4];
            float emission[4];
            float sceneColor[4];  // XXX: should be separated?
            float shininess;
            float padding[3];
        } materialData;

        memset(&materialData, 0, sizeof(materialData));

        setVec4(materialData.ambient, _material.GetAmbient());
        setVec4(materialData.diffuse, _material.GetDiffuse());
        setVec4(materialData.specular, _material.GetSpecular());
        setVec4(materialData.emission, _material.GetEmission());
        materialData.shininess = _material.GetShininess();
        setVec4(materialData.sceneColor, _sceneAmbient);

        _materialUniformBlock->Update(&materialData, sizeof(materialData));
        _materialUniformBlockValid = true;
    }

    _materialUniformBlock->Bind(bindingMap, _tokens->materialUB);

    _BindPostSurfaceShaderParams(bindingMap);
}

void
GlfSimpleLightingContext::BindSamplers(GlfBindingMapPtr const &bindingMap)
{
    if (GlfSimpleShadowArray::GetBindlessShadowMapsEnabled()) {
        // Bindless shadow maps are made resident on creation.
        return;
    }

    int shadowSampler = bindingMap->GetSamplerUnit(_tokens->shadowSampler);
    int shadowCompareSampler = bindingMap->GetSamplerUnit(_tokens->shadowCompareSampler);

    glActiveTexture(GL_TEXTURE0 + shadowSampler);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _shadows->GetShadowMapTexture());
    glBindSampler(shadowSampler, _shadows->GetShadowMapDepthSampler());

    glActiveTexture(GL_TEXTURE0 + shadowCompareSampler);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _shadows->GetShadowMapTexture());
    glBindSampler(shadowCompareSampler, _shadows->GetShadowMapCompareSampler());

    glActiveTexture(GL_TEXTURE0);
}

void
GlfSimpleLightingContext::UnbindSamplers(GlfBindingMapPtr const &bindingMap)
{
    if (GlfSimpleShadowArray::GetBindlessShadowMapsEnabled()) {
        // We leave the bindless shadow maps as always resident.
        return;
    }

    int shadowSampler = bindingMap->GetSamplerUnit(_tokens->shadowSampler);
    int shadowCompareSampler = bindingMap->GetSamplerUnit(_tokens->shadowCompareSampler);

    glActiveTexture(GL_TEXTURE0 + shadowSampler);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindSampler(shadowSampler, 0);

    glActiveTexture(GL_TEXTURE0 + shadowCompareSampler);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindSampler(shadowCompareSampler, 0);

    glActiveTexture(GL_TEXTURE0);
}

void
GlfSimpleLightingContext::SetStateFromOpenGL()
{
    // import classic GL light's parameters into shaded lights
    SetUseLighting(glIsEnabled(GL_LIGHTING) == GL_TRUE);

    GfMatrix4d worldToViewMatrix;
    glGetDoublev(GL_MODELVIEW_MATRIX, worldToViewMatrix.GetArray());
    GfMatrix4d viewToWorldMatrix = worldToViewMatrix.GetInverse();

    GLint nLights = 0;
    glGetIntegerv(GL_MAX_LIGHTS, &nLights);

    GlfSimpleLightVector lights;
    lights.reserve(nLights);

    GlfSimpleLight light;
    for(int i = 0; i < nLights; ++i)
    {
        int lightName = GL_LIGHT0 + i;
        if (glIsEnabled(lightName)) {
            GLfloat position[4], color[4];

            glGetLightfv(lightName, GL_POSITION, position);
            light.SetPosition(GfVec4f(position)*viewToWorldMatrix);
            
            glGetLightfv(lightName, GL_AMBIENT, color);
            light.SetAmbient(GfVec4f(color));
            
            glGetLightfv(lightName, GL_DIFFUSE, color);
            light.SetDiffuse(GfVec4f(color));
            
            glGetLightfv(lightName, GL_SPECULAR, color);
            light.SetSpecular(GfVec4f(color));

            GLfloat spotDirection[3];
            glGetLightfv(lightName, GL_SPOT_DIRECTION, spotDirection);
            light.SetSpotDirection(
                viewToWorldMatrix.TransformDir(GfVec3f(spotDirection)));

            GLfloat floatValue;

            glGetLightfv(lightName, GL_SPOT_CUTOFF, &floatValue);
            light.SetSpotCutoff(floatValue);

            glGetLightfv(lightName, GL_SPOT_EXPONENT, &floatValue);
            light.SetSpotFalloff(floatValue);

            GfVec3f attenuation;
            glGetLightfv(lightName, GL_CONSTANT_ATTENUATION, &floatValue);
            attenuation[0] = floatValue;

            glGetLightfv(lightName, GL_LINEAR_ATTENUATION, &floatValue);
            attenuation[1] = floatValue;

            glGetLightfv(lightName, GL_QUADRATIC_ATTENUATION, &floatValue);
            attenuation[2] = floatValue;

            light.SetAttenuation(attenuation);

            lights.push_back(light);
        }
    }

    SetLights(lights);

    GlfSimpleMaterial material;

    GLfloat color[4], shininess;
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, color);
    material.SetAmbient(GfVec4f(color));
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, color);
    material.SetDiffuse(GfVec4f(color));
    glGetMaterialfv(GL_FRONT, GL_SPECULAR, color);
    material.SetSpecular(GfVec4f(color));
    glGetMaterialfv(GL_FRONT, GL_EMISSION, color);
    material.SetEmission(GfVec4f(color));
    glGetMaterialfv(GL_FRONT, GL_SHININESS, &shininess);
    // clamp to 0.0001, since pow(0,0) is undefined in GLSL.
    shininess = std::max(0.0001f, shininess);
    material.SetShininess(shininess);

    SetMaterial(material);

    GfVec4f sceneAmbient;
    glGetFloatv(GL_LIGHT_MODEL_AMBIENT, &sceneAmbient[0]);
    SetSceneAmbient(sceneAmbient);
}

class GlfSimpleLightingContext::_PostSurfaceShaderState {
public:
    _PostSurfaceShaderState(size_t hash, GlfSimpleLightVector const & lights)
        : _hash(hash)
    {
        _Init(lights);
    }

    std::string const & GetShaderSource() const {
        return _shaderSource;
    }

    GlfUniformBlockRefPtr const & GetUniformBlock() const {
        return _uniformBlock;
    }

    size_t GetHash() const {
        return _hash;
    }

private:
    void _Init(GlfSimpleLightVector const & lights);

    std::string _shaderSource;;
    GlfUniformBlockRefPtr _uniformBlock;
    size_t _hash;
};

void
GlfSimpleLightingContext::_PostSurfaceShaderState::_Init(
        GlfSimpleLightVector const & lights)
{
    TRACE_FUNCTION();

    // Generate shader code and aggregate uniform block data

    // 
    // layout(std140) uniform PostSurfaceShaderParams {
    //     MurkPostParams light1;
    //     CausticsParams light2;
    //     ...
    // } postSurface;
    // 
    // MAT4 GetWorldToViewInverseMatrix();
    // vec4 postSurfaceShader(vec4 Peye, vec3 Neye, vec4 color)
    // {
    //   vec4 Pworld = vec4(GetWorldToViewInverseMatrix() * Peye);
    //   color = ApplyMurkPostWorldSpace(postSurface.light1,color,Pworld.xyz);
    //   color = ApplyCausticsWorldSpace(postSurface.light2,color,Pworld.xyz);
    //   ...
    //   return color
    // }
    //
    std::stringstream lightsSourceStr;
    std::stringstream paramsSourceStr;
    std::stringstream applySourceStr;

    std::vector<uint8_t> uniformData;

    std::set<TfToken> activeShaderIdentifiers;
    size_t activeShaders = 0;
    for (GlfSimpleLight const & light: lights) {

        TfToken const & shaderIdentifier = light.GetPostSurfaceIdentifier();
        std::string const & shaderSource = light.GetPostSurfaceShaderSource();
        VtUCharArray const & shaderParams = light.GetPostSurfaceShaderParams();

        if (shaderIdentifier.IsEmpty() ||
            shaderSource.empty() ||
            shaderParams.empty()) {
            continue;
        }

        // omit lights with misaligned parameter data
        // GLSL std140 packing has a base alignment of "vec4"
        size_t const std140Alignment = 4*sizeof(float);
        if ((shaderParams.size() % std140Alignment) != 0) {
            TF_CODING_ERROR("Invalid shader params size (%zd bytes) "
                            "for %s (must be a multiple of %zd)\n",
                            shaderParams.size(),
                            light.GetID().GetText(),
                            std140Alignment);
            continue;
        }

        TF_DEBUG(GLF_DEBUG_POST_SURFACE_LIGHTING).Msg( 
                "PostSurfaceLight: %s: %s\n",
                shaderIdentifier.GetText(),
                light.GetID().GetText());

        ++activeShaders;

        // emit per-light type shader source only one time
        if (!activeShaderIdentifiers.count(shaderIdentifier)) {
            activeShaderIdentifiers.insert(shaderIdentifier);
            lightsSourceStr << shaderSource;
        }

        // add a per-light parameter declaration to the uniform block
        paramsSourceStr << "    "
                  << shaderIdentifier << "Params "
                  << "light"<<activeShaders << ";\n";

        // append a call to apply the shader with per-light parameters
        applySourceStr << "    "
                << "color = Apply"<<shaderIdentifier<<"WorldSpace("
                << "postSurface.light"<<activeShaders << ", color, Pworld.xyz"
                << ");\n";

        uniformData.insert(uniformData.end(),
                           shaderParams.begin(), shaderParams.end());
    }

    if (activeShaders < 1) {
        return;
    }

    _shaderSource = lightsSourceStr.str();

    _shaderSource +=
        "layout(std140) uniform PostSurfaceShaderParams {\n";
    _shaderSource += paramsSourceStr.str();
    _shaderSource +=
        "} postSurface;\n\n";

    _shaderSource +=
        "MAT4 GetWorldToViewInverseMatrix();\n"
        "vec4 postSurfaceShader(vec4 Peye, vec3 Neye, vec4 color)\n"
        "{\n"
        "    vec4 Pworld = vec4(GetWorldToViewInverseMatrix() * Peye);\n"
        "    color.rgb /= color.a;\n";
    _shaderSource += applySourceStr.str();
    _shaderSource +=
        "    color.rgb *= color.a;\n"
        "    return color;\n"
        "}\n\n";

    _uniformBlock = GlfUniformBlock::New("_postSurfaceShaderUniformBlock");
    _uniformBlock->Update(uniformData.data(), uniformData.size());
}

static size_t
_ComputeHash(GlfSimpleLightVector const & lights)
{
    TRACE_FUNCTION();

    // hash includes light type and shader source but not parameter values
    size_t hash = 0;
    for (GlfSimpleLight const & light: lights) {
        TfToken const & identifier = light.GetPostSurfaceIdentifier();
        std::string const & shaderSource = light.GetPostSurfaceShaderSource();

        hash = ArchHash64(identifier.GetText(), identifier.size(), hash);
        hash = ArchHash64(shaderSource.c_str(), shaderSource.size(), hash);
    }

    return hash;
}

void
GlfSimpleLightingContext::_ComputePostSurfaceShaderState()
{
    size_t hash = _ComputeHash(GetLights());
    if (!_postSurfaceShaderState ||
                (_postSurfaceShaderState->GetHash() != hash)) {
        _postSurfaceShaderState.reset(
                new _PostSurfaceShaderState(hash, GetLights()));
    }
    _postSurfaceShaderStateValid = true;
}

size_t
GlfSimpleLightingContext::ComputeShaderSourceHash()
{
    if (!_postSurfaceShaderStateValid) {
        _ComputePostSurfaceShaderState();
    }

    if (_postSurfaceShaderState) {
        return _postSurfaceShaderState->GetHash();
    }

    return 0;
}

std::string const &
GlfSimpleLightingContext::ComputeShaderSource(TfToken const &shaderStageKey)
{
    if (!_postSurfaceShaderStateValid) {
        _ComputePostSurfaceShaderState();
    }

    if (_postSurfaceShaderState &&
                shaderStageKey==HioGlslfxTokens->fragmentShader) {
        return _postSurfaceShaderState->GetShaderSource();
    }

    static const std::string empty;
    return empty;
}

void
GlfSimpleLightingContext::_BindPostSurfaceShaderParams(
        GlfBindingMapPtr const &bindingMap)
{
    if (!_postSurfaceShaderStateValid) {
        _ComputePostSurfaceShaderState();
    }

    if (_postSurfaceShaderState && _postSurfaceShaderState->GetUniformBlock()) {
        _postSurfaceShaderState->GetUniformBlock()->
                Bind(bindingMap, _tokens->postSurfaceShaderUB);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

