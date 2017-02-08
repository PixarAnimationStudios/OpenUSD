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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((_uint, "uint"))
);

HdRenderPassShader::HdRenderPassShader()
    : HdShaderCode()
    , _glslfxFile(HdPackageRenderPassShader())
    , _hash(0)
    , _hashValid(false)
    , _cullStyle(HdCullStyleNothing)
{
    _glslfx.reset(new GlfGLSLFX(_glslfxFile));
}

HdRenderPassShader::HdRenderPassShader(TfToken const &glslfxFile)
    : HdShaderCode()
    , _glslfxFile(glslfxFile)   // user-defined
    , _hash(0)
    , _hashValid(false)
    , _cullStyle(HdCullStyleNothing)
{
    _glslfx.reset(new GlfGLSLFX(_glslfxFile));
}

/*virtual*/
HdRenderPassShader::~HdRenderPassShader()
{
    // nothing
}

/*virtual*/
HdRenderPassShader::ID
HdRenderPassShader::ComputeHash() const
{
    // if nothing changed, returns the cached hash value
    if (_hashValid) return _hash;

    _hash = _glslfx->GetHash();

    // cullFaces are dynamic, no need to put in the hash.

    // Custom buffer bindings may vary over time, requiring invalidation
    // of down stream clients.
    TF_FOR_ALL(it, _customBuffers) {
        boost::hash_combine(_hash, it->second.ComputeHash());
        boost::hash_combine(_hash, 0);
    }
    _hashValid = true;

    return (ID)_hash;
}

/*virtual*/
std::string
HdRenderPassShader::GetSource(TfToken const &shaderStageKey) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    return _glslfx->GetSource(shaderStageKey);
}

/*virtual*/
void
HdRenderPassShader::BindResources(Hd_ResourceBinder const &binder, int program)
{
    TF_FOR_ALL(it, _customBuffers) {
        binder.Bind(it->second);
    }

    // set fallback states (should be moved to HdRenderPassState::Bind)
    unsigned int cullStyle = _cullStyle;
    binder.BindUniformui(HdShaderTokens->cullStyle, 1, &cullStyle);
}

/*virtual*/
void
HdRenderPassShader::UnbindResources(Hd_ResourceBinder const &binder,
                                     int program)
{
    TF_FOR_ALL(it, _customBuffers) {
        binder.Unbind(it->second);
    }
}

void
HdRenderPassShader::AddBufferBinding(HdBindingRequest const& req)
{
    _customBuffers[req.GetName()] = req;
    _hashValid = false;
}

void
HdRenderPassShader::RemoveBufferBinding(TfToken const &name)
{
    _customBuffers.erase(name);
    _hashValid = false;
}

void
HdRenderPassShader::ClearBufferBindings()
{
    _customBuffers.clear();
    _hashValid = false;
}

/*virtual*/
void
HdRenderPassShader::AddBindings(HdBindingRequestVector *customBindings)
{
    // note: be careful, the logic behind this function is tricky.
    //
    // customBindings will be used for two purpose.
    //   1. resouceBinder assigned the binding location and use it
    //      in Bind/UnbindResources. The resourceBinder is held by
    //      drawingProgram in each batch in the renderPass.
    //   2. codeGen generates macros to fill the placeholder of binding location
    //      in glslfx file.
    //
    // To make RenderPassShader work on DrawBatch::Execute(), _customBuffers and
    // other resources should be bound to the right binding locations which were
    // resolved at the compilation time of the drawingProgram.
    //
    // However, if we have 2 or more renderPassStates and if they all share
    // the same shader hash signature, drawingProgram will only be constructed
    // at the first renderPassState and then be reused for the subsequent
    // renderPassStates, because the shaderHash matches in
    // Hd_DrawBatch::_GetDrawingProgram().
    //
    // The shader hash computation must guarantee the consistency such that the
    // resourceBinder held in the drawingProgram is applicable to all other
    // renderPassStates as long as the hash matches.
    //

    customBindings->reserve(customBindings->size() + _customBuffers.size() + 1);
    TF_FOR_ALL(it, _customBuffers) {
        customBindings->push_back(it->second);
    }

    // typed binding to emit declaration and accessor.
    customBindings->push_back(
        HdBindingRequest(HdBinding::UNIFORM,
                         HdShaderTokens->cullStyle, _tokens->_uint));
}

PXR_NAMESPACE_CLOSE_SCOPE

