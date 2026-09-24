//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file simpleLightingContext.cpp

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/debugCodes.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/diagnostic.h"

#include "pxr/base/trace/trace.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


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
    return (int)_lights.size();
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
            light.SetPosition(GfVec4f(GfVec4f(position)*viewToWorldMatrix));
            
            glGetLightfv(lightName, GL_AMBIENT, color);
            light.SetAmbient(GfVec4f(color));
            
            glGetLightfv(lightName, GL_DIFFUSE, color);
            light.SetDiffuse(GfVec4f(color));
            
            glGetLightfv(lightName, GL_SPECULAR, color);
            light.SetSpecular(GfVec4f(color));

            GLfloat spotDirection[3];
            glGetLightfv(lightName, GL_SPOT_DIRECTION, spotDirection);
            light.SetSpotDirection(
                GfVec3f(viewToWorldMatrix.TransformDir(GfVec3f(spotDirection))));

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

    _uniformData = VtUCharArray(uniformData.begin(), uniformData.end());
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
        // hash skips lights that won't generate shader code so light count
        // change doesn't trigger shader recompile
        if (!identifier.IsEmpty() || !shaderSource.empty()) {
            hash = ArchHash64(identifier.GetText(), identifier.size(), hash);
            hash = ArchHash64(shaderSource.c_str(), shaderSource.size(), hash);
        }
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


PXR_NAMESPACE_CLOSE_SCOPE

