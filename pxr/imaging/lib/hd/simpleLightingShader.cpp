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
#include "pxr/imaging/hd/simpleLightingShader.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include <boost/functional/hash.hpp>

#include <string>
#include <sstream>

HdSimpleLightingShader::HdSimpleLightingShader() 
    : _lightingContext(GlfSimpleLightingContext::New())
    , _bindingMap(TfCreateRefPtr(new GlfBindingMap()))
    , _useLighting(true)
{
    // TODO: robust binding from codegen
    _bindingMap->GetUniformBinding(TfToken("GlobalUniform"));
    _bindingMap->GetUniformBinding(TfToken("DrawDataBuffer"));
    _lightingContext->InitUniformBlockBindings(_bindingMap);
    _lightingContext->InitSamplerUnitBindings(_bindingMap);

    _glslfx.reset(new GlfGLSLFX(HdPackageSimpleLightingShader()));
}

HdSimpleLightingShader::~HdSimpleLightingShader()
{
}

/* virtual */
HdSimpleLightingShader::ID
HdSimpleLightingShader::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    TfToken glslfxFile = HdPackageSimpleLightingShader();
    size_t numLights = _useLighting ? _lightingContext->GetNumLightsUsed() : 0;
    bool useShadows = _useLighting ? _lightingContext->GetUseShadows() : false;

    size_t hash = glslfxFile.Hash();
    boost::hash_combine(hash, numLights);
    boost::hash_combine(hash, useShadows);

    return (ID)hash;
}

/* virtual */
std::string
HdSimpleLightingShader::GetSource(TfToken const &shaderStageKey) const
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    std::string source = _glslfx->GetSource(shaderStageKey);

    if (source.empty()) return source;

    std::stringstream defineStream;
    size_t numLights = _useLighting ? _lightingContext->GetNumLightsUsed() : 0;
    bool useShadows = _useLighting ? _lightingContext->GetUseShadows() : false;
    defineStream << "#define NUM_LIGHTS " << numLights<< "\n";
    defineStream << "#define USE_SHADOWS " << (int)(useShadows) << "\n";

    return defineStream.str() + source;
}

/* virtual */
void
HdSimpleLightingShader::SetCamera(GfMatrix4d const &worldToViewMatrix,
                                          GfMatrix4d const &projectionMatrix)
{
    _lightingContext->SetCamera(worldToViewMatrix, projectionMatrix);
}

/* virtual */
void
HdSimpleLightingShader::BindResources(Hd_ResourceBinder const &binder,
                                      int program)
{
    // XXX: we'd like to use Hd_ResourceBinder instead of GlfBindingMap.
    //
    _bindingMap->AssignUniformBindingsToProgram(program);
    _lightingContext->BindUniformBlocks(_bindingMap);

    _bindingMap->AssignSamplerUnitsToProgram(program);
    _lightingContext->BindSamplers(_bindingMap);
}

/* virtual */
void
HdSimpleLightingShader::UnbindResources(Hd_ResourceBinder const &binder,
                                        int program)
{
    // XXX: we'd like to use Hd_ResourceBinder instead of GlfBindingMap.
    //
    _lightingContext->UnbindSamplers(_bindingMap);
}

/*virtual*/
void
HdSimpleLightingShader::AddBindings(HdBindingRequestVector *customBindings)
{
}

void
HdSimpleLightingShader::SetLightingStateFromOpenGL()
{
    _lightingContext->SetStateFromOpenGL();
}

void
HdSimpleLightingShader::SetLightingState(
    GlfSimpleLightingContextPtr const &src)
{
    if (src) {
        _useLighting = true;
        _lightingContext->SetUseLighting(not src->GetLights().empty());
        _lightingContext->SetLights(src->GetLights());
        _lightingContext->SetMaterial(src->GetMaterial());
        _lightingContext->SetSceneAmbient(src->GetSceneAmbient());
        _lightingContext->SetShadows(src->GetShadows());
    } else {
        // XXX:
        // if src is null, turn off lights (this is temporary used for shadowmap drawing).
        // see GprimUsdBaseIcBatch::Draw()
        _useLighting = false;
    }
}
