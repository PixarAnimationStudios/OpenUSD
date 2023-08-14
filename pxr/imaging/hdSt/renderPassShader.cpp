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
#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hdSt/binding.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"

#include "pxr/imaging/hio/glslfx.h"

#include <boost/functional/hash.hpp>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _NamedTextureIdentifier {
    TfToken name;
    HdStTextureIdentifier id;
};

using _NamedTextureIdentifiers = std::vector<_NamedTextureIdentifier>;

TfToken
_GetInputName(const TfToken &aovName)
{
    return TfToken(aovName.GetString() + "Readback");
}

// An AOV is backed by a render buffer. And Storm backs a render buffer
// by a texture. The identifier for this texture can be obtained from
// the HdStRenderBuffer.
_NamedTextureIdentifiers
_GetNamedTextureIdentifiers(
    HdRenderPassAovBindingVector const &aovInputBindings,
    HdRenderIndex * const renderIndex)
{
    _NamedTextureIdentifiers result;
    result.reserve(aovInputBindings.size());

    for (const HdRenderPassAovBinding &aovBinding : aovInputBindings) {
        if (HdStRenderBuffer * const renderBuffer =
                dynamic_cast<HdStRenderBuffer*>(
                    renderIndex->GetBprim(
                        HdPrimTypeTokens->renderBuffer,
                        aovBinding.renderBufferId))) {
            result.push_back(
                _NamedTextureIdentifier{
                    _GetInputName(aovBinding.aovName),
                    renderBuffer->GetTextureIdentifier(
                        /* multiSampled = */ false)});
        }
    }

    return result;
}

// Check whether the given named texture handles match the given
// named texture identifiers.
bool
_AreHandlesValid(
    const HdStShaderCode::NamedTextureHandleVector &namedTextureHandles,
    const _NamedTextureIdentifiers &namedTextureIdentifiers)
{
    if (namedTextureHandles.size() != namedTextureIdentifiers.size()) {
        return false;
    }

    for (size_t i = 0; i < namedTextureHandles.size(); i++) {
        const HdStShaderCode::NamedTextureHandle namedTextureHandle =
            namedTextureHandles[i];
        const _NamedTextureIdentifier namedTextureIdentifier =
            namedTextureIdentifiers[i];

        if (namedTextureHandle.name != namedTextureIdentifier.name) {
            return false;
        }
        const HdStTextureObjectSharedPtr &textureObject =
            namedTextureHandle.handle->GetTextureObject();
        if (textureObject->GetTextureIdentifier() != namedTextureIdentifier.id) {
            return false;
        }
    }

    return true;
}

}

HdStRenderPassShader::HdStRenderPassShader()
    : HdStRenderPassShader(HdStPackageRenderPassShader())
{
}

HdStRenderPassShader::HdStRenderPassShader(TfToken const &glslfxFile)
    : HdStShaderCode()
    , _glslfxFile(glslfxFile)   // user-defined
    , _glslfx(std::make_unique<HioGlslfx>(glslfxFile))
    , _hash(0)
    , _hashValid(false)
{
}

/*virtual*/
HdStRenderPassShader::~HdStRenderPassShader() = default;

/* virtual */
HioGlslfx const *
HdStRenderPassShader::_GetGlslfx() const
{
    return _glslfx.get();
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

    for (const HdStShaderCode::NamedTextureHandle &namedHandle :
             _namedTextureHandles) {
        
        // Use name and hash only - not the texture itself as this
        // does not affect the generated shader source.
        boost::hash_combine(_hash, namedHandle.name);
        boost::hash_combine(_hash, namedHandle.hash);
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

/*virtual*/
void
HdStRenderPassShader::BindResources(const int program,
                                    HdSt_ResourceBinder const &binder)
{
    TF_FOR_ALL(it, _customBuffers) {
        binder.Bind(it->second);
    }

    HdSt_TextureBinder::BindResources(binder, _namedTextureHandles);
}

/*virtual*/
void
HdStRenderPassShader::UnbindResources(const int program,
                                      HdSt_ResourceBinder const &binder)
{
    TF_FOR_ALL(it, _customBuffers) {
        binder.Unbind(it->second);
    }

    HdSt_TextureBinder::UnbindResources(binder, _namedTextureHandles);
}

void
HdStRenderPassShader::AddBufferBinding(HdStBindingRequest const& req)
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
HdStRenderPassShader::AddBindings(HdStBindingRequestVector *customBindings)
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
}

HdSt_MaterialParamVector const &
HdStRenderPassShader::GetParams() const
{
    return _params;
}

HdStShaderCode::NamedTextureHandleVector const &
HdStRenderPassShader::GetNamedTextureHandles() const
{
    return _namedTextureHandles;
}

void
HdStRenderPassShader::UpdateAovInputTextures(
    HdRenderPassAovBindingVector const &aovInputBindings,
    HdRenderIndex * const renderIndex)
{
    TRACE_FUNCTION();

    // Compute the identifiers for the textures backing the requested
    // (resolved) AOVs.
    const _NamedTextureIdentifiers namedTextureIdentifiers =
        _GetNamedTextureIdentifiers(aovInputBindings, renderIndex);
    // If the (named) texture handles are up-to-date, there is nothing to do.
    if (_AreHandlesValid(_namedTextureHandles, namedTextureIdentifiers)) {
        return;
    }

    _hashValid = false;

    // Otherwise, we need to (re-)allocate texture handles for the
    // given texture identifiers.
    _namedTextureHandles.clear();
    _params.clear();

    HdStResourceRegistry * const resourceRegistry =
        dynamic_cast<HdStResourceRegistry*>(
            renderIndex->GetResourceRegistry().get());
    if (!TF_VERIFY(resourceRegistry)) {
        return;
    }

    for (const auto &namedTextureIdentifier : namedTextureIdentifiers) {
        static const HdSamplerParameters samplerParameters(
            HdWrapClamp, HdWrapClamp, HdWrapClamp,
            HdMinFilterNearest, HdMagFilterNearest);

        // Allocate texture handle for given identifier.
        HdStTextureHandleSharedPtr textureHandle =
            resourceRegistry->AllocateTextureHandle(
                namedTextureIdentifier.id,
                HdTextureType::Uv,
                samplerParameters,
                /* memoryRequest = */ 0,
                shared_from_this());
        // Add to _namedTextureHandles so that the texture will
        // be bound to the shader in BindResources.
        _namedTextureHandles.push_back(
            HdStShaderCode::NamedTextureHandle{
                namedTextureIdentifier.name,
                HdTextureType::Uv,
                std::move(textureHandle),
                /* hash = */ 0});
        
        // Add a corresponding param so that codegen is
        // generating the accessor HdGet_AOVNAMEReadback().
        _params.emplace_back(
            HdSt_MaterialParam::ParamTypeTexture,
            namedTextureIdentifier.name,
            VtValue(GfVec4f(0,0,0,0)));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
