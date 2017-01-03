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

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/package.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"
#include "pxr/imaging/glf/uniformBlock.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"

#include <algorithm>
#include <iostream>
#include <string>

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((lightingUB, "Lighting"))
    ((shadowUB, "Shadow"))
    ((materialUB, "Material"))
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
    _shadows(new GlfSimpleShadowArray(GfVec2i(1024, 1024), 0)),
    _worldToViewMatrix(1.0),
    _projectionMatrix(1.0),
    _sceneAmbient(0.01, 0.01, 0.01, 1.0),
    _useLighting(false),
    _useShadows(false),
    _useColorMaterialDiffuse(false),
    _lightingUniformBlockValid(false),
    _shadowUniformBlockValid(false),
    _materialUniformBlockValid(false)
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

    int numLights = GetNumLightsUsed();

    _useShadows = false;
    for (int i = 0;i < numLights; ++i) {
        if (_lights[i].HasShadow()) {
            _useShadows = true;
            break;
        }
    }
}

GlfSimpleLightVector &
GlfSimpleLightingContext::GetLights()
{
    return _lights;
}

int
GlfSimpleLightingContext::GetNumLightsUsed() const
{
    return std::min((int)_lights.size(), _maxLightsUsed);
}

void
GlfSimpleLightingContext::SetShadows(GlfSimpleShadowArrayRefPtr const & shadows)
{
    _shadows = shadows;
    _shadowUniformBlockValid = false;
}

GlfSimpleShadowArrayRefPtr const &
GlfSimpleLightingContext::GetShadows()
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
}

void
GlfSimpleLightingContext::InitSamplerUnitBindings(
        GlfBindingMapPtr const &bindingMap) const
{
    bindingMap->GetSamplerUnit(_tokens->shadowSampler);
    bindingMap->GetSamplerUnit(_tokens->shadowCompareSampler);
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
    if (!_lightingUniformBlock)
        _lightingUniformBlock = GlfUniformBlock::New();
    if (!_shadowUniformBlock)
        _shadowUniformBlock = GlfUniformBlock::New();
    if (!_materialUniformBlock)
        _materialUniformBlock = GlfUniformBlock::New();

    bool shadowExists = false;
    if ((!_lightingUniformBlockValid ||
         !_shadowUniformBlockValid) && _lights.size() > 0) {
        int numLights = GetNumLightsUsed();

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
            bool hasShadow;
            int32_t shadowIndex;
            int32_t padding2[2];
        };

        struct Lighting {
            int32_t useLighting;
            int32_t useColorMaterialDiffuse;
            int32_t padding[2];
            LightSource lightSource[0];
        };

        // 16byte aligned
        struct ShadowMatrix {
            float viewToShadowMatrix[16];
            float basis0[4];
            float basis1[4];
            float basis2[4];
            float bias;
            float padding[3];
        };

        struct Shadow {
            ShadowMatrix shadow[0];
        };

        size_t lightingSize = sizeof(Lighting) + sizeof(LightSource) * numLights;
        size_t shadowSize = sizeof(ShadowMatrix) * numLights;
        Lighting *lightingData = (Lighting *)alloca(lightingSize);
        Shadow *shadowData = (Shadow *)alloca(shadowSize);

        memset(shadowData, 0, shadowSize);
        memset(lightingData, 0, lightingSize);

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
            lightingData->lightSource[i].hasShadow = light.HasShadow();

            if (lightingData->lightSource[i].hasShadow) {
                int shadowIndex = light.GetShadowIndex();
                lightingData->lightSource[i].shadowIndex = shadowIndex;

                GfMatrix4d viewToShadowMatrix = viewToWorldMatrix *
                    _shadows->GetWorldToShadowMatrix(shadowIndex);

                double invBlur = 1.0/(std::max(0.0001F, light.GetShadowBlur()));
                GfMatrix4d mat = viewToShadowMatrix.GetInverse();
                GfVec4f xVec = GfVec4f(mat.GetRow(0) * invBlur);
                GfVec4f yVec = GfVec4f(mat.GetRow(1) * invBlur);
                GfVec4f zVec = GfVec4f(mat.GetRow(2));

                shadowData->shadow[shadowIndex].bias = light.GetShadowBias();
                setMatrix(shadowData->shadow[shadowIndex].viewToShadowMatrix,
                          viewToShadowMatrix);
                setVec4(shadowData->shadow[shadowIndex].basis0, xVec);
                setVec4(shadowData->shadow[shadowIndex].basis1, yVec);
                setVec4(shadowData->shadow[shadowIndex].basis2, zVec);

                shadowExists = true;
            }
        }

        _lightingUniformBlock->Update(lightingData, lightingSize);
        _lightingUniformBlockValid = true;

        if (shadowExists) {
            _shadowUniformBlock->Update(shadowData, shadowSize);
            _shadowUniformBlockValid = true;
        }
    }

    _lightingUniformBlock->Bind(bindingMap, _tokens->lightingUB);

    if (shadowExists) {
        _shadowUniformBlock->Bind(bindingMap, _tokens->shadowUB);
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
}

void
GlfSimpleLightingContext::BindSamplers(GlfBindingMapPtr const &bindingMap)
{
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
    SetUseLighting(glIsEnabled(GL_LIGHTING));

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
