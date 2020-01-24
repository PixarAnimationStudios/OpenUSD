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

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/renderPassShader.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hio/glslfx.h"

#include <boost/functional/hash.hpp>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// Name shader uses to read AOV, i.e., shader calls
// HdGet_AOVNAMEReadback().
static
TfToken
_GetReadbackName(const TfToken &aovName)
{
    return TfToken(aovName.GetString() + "Readback");
}

HdStRenderPassShader::HdStRenderPassShader()
    : HdStShaderCode()
    , _glslfxFile(HdStPackageRenderPassShader())
    , _hash(0)
    , _hashValid(false)
    , _cullStyle(HdCullStyleNothing)
{
    _glslfx.reset(new HioGlslfx(_glslfxFile));
}

HdStRenderPassShader::HdStRenderPassShader(TfToken const &glslfxFile)
    : HdStShaderCode()
    , _glslfxFile(glslfxFile)   // user-defined
    , _hash(0)
    , _hashValid(false)
    , _cullStyle(HdCullStyleNothing)
{
    _glslfx.reset(new HioGlslfx(_glslfxFile));
}

/*virtual*/
HdStRenderPassShader::~HdStRenderPassShader()
{
    // nothing
}

/*virtual*/
HdStRenderPassShader::ID
HdStRenderPassShader::ComputeHash() const
{
    // if nothing changed, returns the cached hash value
    if (_hashValid) return _hash;

    _hash = _glslfx->GetHash();

    // cullFaces are dynamic, no need to put in the hash.

    // Custom buffer bindings may vary over time, requiring invalidation
    // of down stream clients.
    TF_FOR_ALL(it, _customBuffers) {
        boost::hash_combine(_hash, it->second.ComputeHash());
    }
    _hashValid = true;

    return (ID)_hash;
}

/*virtual*/
std::string
HdStRenderPassShader::GetSource(TfToken const &shaderStageKey) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    return _glslfx->GetSource(shaderStageKey);
}

// Helper to bind texture from given AOV to GLSL program identified
// by \p program.
static
void
_BindTexture(const int program,
             const HdRenderPassAovBinding &aov,
             const HdBinding &binding)
{
    if (binding.GetType() != HdBinding::TEXTURE_2D) {
        TF_CODING_ERROR("When binding readback for aov '%s', binding is "
                        "not of type TEXTURE_2D.",
                        aov.aovName.GetString().c_str());
        return;
    }
    
    HdRenderBuffer * const buffer = aov.renderBuffer;
    if (!buffer) {
        TF_CODING_ERROR("When binding readback for aov '%s', AOV has invalid "
                        "render buffer.", aov.aovName.GetString().c_str());
        return;
    }

    // Get texture from AOV's render buffer.
    const bool multiSampled = false;
    HgiGLTexture * const texture = dynamic_cast<HgiGLTexture*>(
        buffer->GetHgiTextureHandle(multiSampled));
    if (!texture) {
        TF_CODING_ERROR("When binding readback for aov '%s', AOV is not backed "
                        "by HgiGLTexture.", aov.aovName.GetString().c_str());
        return;
    }

    // Get OpenGL texture Id.
    const int textureId = texture->GetTextureId();

    // XXX:-matthias
    // Some of this code is duplicated, see HYD-1788.

    // Sampler unit was determined during binding resolution.
    // Use it to bind texture.
    const int samplerUnit = binding.GetTextureUnit();
    glActiveTexture(GL_TEXTURE0 + samplerUnit);
    glBindTexture(GL_TEXTURE_2D, (GLuint) textureId);
    glBindSampler(samplerUnit, 0);

    // Set uniform sampler2D to sampler unit.
    glProgramUniform1i(program, binding.GetLocation(),
                       samplerUnit);
}

/*virtual*/
void
HdStRenderPassShader::BindResources(const int program,
                                    HdSt_ResourceBinder const &binder,
                                    HdRenderPassState const &state)
{
    TF_FOR_ALL(it, _customBuffers) {
        binder.Bind(it->second);
    }

    // set fallback states (should be moved to HdRenderPassState::Bind)
    unsigned int cullStyle = _cullStyle;
    binder.BindUniformui(HdShaderTokens->cullStyle, 1, &cullStyle);

    // Count how many textures we bind for check at the end.
    size_t numFulfilled = 0;

    // Loop over all AOVs for which a read back was requested.
    for (const HdRenderPassAovBinding &aovBinding : state.GetAovBindings()) {
        const TfToken &aovName = aovBinding.aovName;
        if (_aovReadbackRequests.count(aovName) > 0) {
            // Bind the texture.
            _BindTexture(program,
                         aovBinding,
                         binder.GetBinding(_GetReadbackName(aovName)));

            numFulfilled++;
        }
    }

    if (numFulfilled != _aovReadbackRequests.size()) {
        TF_CODING_ERROR("AOV bindings missing for requested readbacks.");
    }
}

// Helper to unbind what was bound with _BindTexture.
static
void
_UnbindTexture(const HdBinding &binding)
{
    if (binding.GetType() != HdBinding::TEXTURE_2D) {
        // Coding error already issued in _BindTexture.
        return;
    }

    const int samplerUnit = binding.GetTextureUnit();
    glActiveTexture(GL_TEXTURE0 + samplerUnit);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindSampler(samplerUnit, 0);
}    


/*virtual*/
void
HdStRenderPassShader::UnbindResources(const int program,
                                      HdSt_ResourceBinder const &binder,
                                      HdRenderPassState const &state)
{
    TF_FOR_ALL(it, _customBuffers) {
        binder.Unbind(it->second);
    }

    // Unbind all textures that were requested for AOV read back
    for (const HdRenderPassAovBinding &aovBinding : state.GetAovBindings()) {
        const TfToken &aovName = aovBinding.aovName;
        if (_aovReadbackRequests.count(aovName) > 0) {
            _UnbindTexture(binder.GetBinding(_GetReadbackName(aovName)));
        }
    }    

    glActiveTexture(GL_TEXTURE0);
}

void
HdStRenderPassShader::AddBufferBinding(HdBindingRequest const& req)
{
    _customBuffers[req.GetName()] = req;
    _hashValid = false;
}

void
HdStRenderPassShader::RemoveBufferBinding(TfToken const &name)
{
    _customBuffers.erase(name);
    _hashValid = false;
}

void
HdStRenderPassShader::ClearBufferBindings()
{
    _customBuffers.clear();
    _hashValid = false;
}

/*virtual*/
void
HdStRenderPassShader::AddBindings(HdBindingRequestVector *customBindings)
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
                         HdShaderTokens->cullStyle,
                         HdTypeUInt32));
}

void
HdStRenderPassShader::AddAovReadback(TfToken const &name)
{
    if (_aovReadbackRequests.count(name) > 0) {
        // Record readback request only once.
        return;
    }

    // Add request.
    _aovReadbackRequests.insert(name);

    // Add read back name to material params so that binding resolution
    // allocated a sampler unit and codegen generates an accessor
    // HdGet_NAMEReadback().
    _params.emplace_back(
        HdMaterialParam::ParamTypeTexture,
        _GetReadbackName(name),
        VtValue(GfVec4f(0.0)),
        SdfPath(),
        TfTokenVector(),
        HdTextureType::Uv);
}

void
HdStRenderPassShader::RemoveAovReadback(TfToken const &name)
{
    // Remove request.
    _aovReadbackRequests.erase(name);

    // And the corresponding material param.
    const TfToken accessorName = _GetReadbackName(name);
    std::remove_if(
        _params.begin(), _params.end(),
        [&accessorName](const HdMaterialParam &p) {
            return p.name == accessorName; });
}

HdMaterialParamVector const &
HdStRenderPassShader::GetParams() const
{
    return _params;
}

PXR_NAMESPACE_CLOSE_SCOPE
