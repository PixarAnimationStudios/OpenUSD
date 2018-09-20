//
// Copyright 2018 Pixar
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

#include "pxr/imaging/hdx/colorizeTask.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderPassState.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxColorizeTask::HdxColorizeTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _renderBuffer(nullptr)
    , _outputBuffer(nullptr)
    , _outputBufferSize(0)
    , _converged(false)
{
    _CreateBlitResources();
}

HdxColorizeTask::~HdxColorizeTask()
{
    _DestroyBlitResources();
}

bool
HdxColorizeTask::IsConverged() const
{
    return _converged;
}

void
HdxColorizeTask::_CreateBlitResources()
{
    // Store the context we created the FBO on, for sanity checking.
    _owningContext = GlfGLContext::GetCurrentGLContext();

    glGenFramebuffers(1, &_outputFramebuffer);
    GLint restoreReadFB, restoreDrawFB;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoreReadFB);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &restoreDrawFB);
    glBindFramebuffer(GL_FRAMEBUFFER, _outputFramebuffer);

    glGenTextures(1, &_outputTexture);
    GLint restoreTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &restoreTexture);
    glBindTexture(GL_TEXTURE_2D, _outputTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           _outputTexture, 0);

    glBindTexture(GL_TEXTURE_2D, restoreTexture);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreReadFB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, restoreDrawFB);

    GLF_POST_PENDING_GL_ERRORS();
}

void
HdxColorizeTask::_DestroyBlitResources()
{
    // If the owning context is gone, we don't need to do anything. Otherwise,
    // make sure the owning context is active before deleting the FBO.
    if (!_owningContext->IsValid()) {
        return;
    }
    GlfGLContextScopeHolder contextHolder(_owningContext);

    glDeleteTextures(1, &_outputTexture);
    glDeleteFramebuffers(1, &_outputFramebuffer);

    GLF_POST_PENDING_GL_ERRORS();
}

void
HdxColorizeTask::_Blit()
{
    if (!TF_VERIFY(_owningContext->IsCurrent())) {
        // If we're rendering with a different context than the render pass
        // was created with, recreate the FBO.

        if (_owningContext->IsValid()) {
            GlfGLContextScopeHolder contextHolder(_owningContext);
            glDeleteFramebuffers(1, &_outputFramebuffer);
        }
        _owningContext = GlfGLContext::GetCurrentGLContext();
        glGenFramebuffers(1, &_outputFramebuffer);
        GLint restoreReadFB, restoreDrawFB;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoreReadFB);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &restoreDrawFB);
        glBindFramebuffer(GL_FRAMEBUFFER, _outputFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                _outputTexture, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreReadFB);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, restoreDrawFB);
    }

    GLuint width = _renderBuffer->GetWidth();
    GLuint height = _renderBuffer->GetHeight();

    GLint restoreTexture, restoreReadFB;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &restoreTexture);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoreReadFB);
    glBindTexture(GL_TEXTURE_2D, _outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, _outputBuffer);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, _outputFramebuffer);
    glBlitFramebuffer(0, 0, width, height,
                      0, 0, width, height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreReadFB);
    glBindTexture(GL_TEXTURE_2D, restoreTexture);

    GLF_POST_PENDING_GL_ERRORS();
}

struct _Colorizer {
    typedef void (*ColorizerCallback)
        (uint8_t* dest, uint8_t* src, size_t nPixels);
    TfToken aovName;
    HdFormat aovFormat;
    ColorizerCallback callback;
};

static void _colorizeColor(uint8_t* dest, uint8_t* src, size_t nPixels)
{
    memcpy(dest, src, nPixels*4);
}
static void _colorizeNdcDepth(uint8_t* dest, uint8_t* src, size_t nPixels)
{
    // depth is in clip space, so remap (-1, 1) to (0,1) and clamp.
    float *depthBuffer = reinterpret_cast<float*>(src);
    for (size_t i = 0; i < nPixels; ++i) {
        float valuef =
            std::min(std::max((depthBuffer[i] * 0.5f) + 0.5f, 0.0f), 1.0f);
        uint8_t value = (uint8_t)(255.0f * valuef);
        dest[i*4+0] = value;
        dest[i*4+1] = value;
        dest[i*4+2] = value;
        dest[i*4+3] = 255;
    }
}
static void _colorizeLinearDepth(uint8_t* dest, uint8_t* src, size_t nPixels)
{
    // linearDepth is depth from the camera, in world units. Its range is
    // [0, N] for some maximum N; to display it, rescale to [0, 1] and
    // splat that across RGB.
    float maxDepth = 0.0f;
    float *depthBuffer = reinterpret_cast<float*>(src);
    for (size_t i = 0; i < nPixels; ++i) {
        maxDepth = std::max(depthBuffer[i], maxDepth);
    }
    if (maxDepth != 0.0f) {
        for (size_t i = 0; i < nPixels; ++i) {
            float valuef =
                std::min(std::max((depthBuffer[i] / maxDepth), 0.0f), 1.0f);
            uint8_t value =
                (uint8_t)(255.0f * valuef);
            dest[i*4+0] = value;
            dest[i*4+1] = value;
            dest[i*4+2] = value;
            dest[i*4+3] = 255;
        }
    }
}
static void _colorizeNormal(uint8_t* dest, uint8_t* src, size_t nPixels)
{
    float *normalBuffer = reinterpret_cast<float*>(src);
    for (size_t i = 0; i < nPixels; ++i) {
        GfVec3f n(normalBuffer[i*3+0], normalBuffer[i*3+1],
                  normalBuffer[i*3+2]);
        n = (n * 0.5f) + GfVec3f(0.5f);
        dest[i*4+0] = (uint8_t)(n[0] * 255.0f);
        dest[i*4+1] = (uint8_t)(n[1] * 255.0f);
        dest[i*4+2] = (uint8_t)(n[2] * 255.0f);
        dest[i*4+3] = 255;
    }
}
static void _colorizeId(uint8_t* dest, uint8_t* src, size_t nPixels)
{
    // XXX: this is legacy ID-display behavior, but an alternative is to
    // hash the ID to 3 bytes and use those as color. Even fancier,
    // hash to hue and stratified (saturation, value) levels, etc.
    memcpy(dest, src, nPixels*4);
}
static void _colorizePrimvar(uint8_t* dest, uint8_t* src, size_t nPixels)
{
    float *primvarBuffer = reinterpret_cast<float*>(src);
    for (size_t i = 0; i < nPixels; ++i) {
        GfVec3f p(std::fmod(primvarBuffer[i*3+0], 1.0f),
                  std::fmod(primvarBuffer[i*3+1], 1.0f),
                  std::fmod(primvarBuffer[i*3+2], 1.0f));
        if (p[0] < 0.0f) { p[0] += 1.0f; }
        if (p[1] < 0.0f) { p[1] += 1.0f; }
        if (p[2] < 0.0f) { p[2] += 1.0f; }
        dest[i*4+0] = (uint8_t)(p[0] * 255.0f);
        dest[i*4+1] = (uint8_t)(p[1] * 255.0f);
        dest[i*4+2] = (uint8_t)(p[2] * 255.0f);
        dest[i*4+3] = 255;
    }
}

// XXX: It would be nice to make the colorizers more flexible on input format,
// but this gets the job done.
static _Colorizer _colorizerTable[] = {
    { HdAovTokens->color, HdFormatUNorm8Vec4, _colorizeColor },
    { HdAovTokens->depth, HdFormatFloat32, _colorizeNdcDepth },
    { HdAovTokens->linearDepth, HdFormatFloat32, _colorizeLinearDepth },
    { HdAovTokens->Neye, HdFormatFloat32Vec3, _colorizeNormal },
    { HdAovTokens->normal, HdFormatFloat32Vec3, _colorizeNormal },
    { HdAovTokens->primId, HdFormatInt32, _colorizeId },
};

void
HdxColorizeTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_renderBuffer)) {
        return;
    }

    // Resolve the renderbuffer before we read it.
    _renderBuffer->Resolve();

    // Allocate the scratch space, if needed.
    size_t sz = _renderBuffer->GetWidth() * _renderBuffer->GetHeight();
    if (!_outputBuffer || _outputBufferSize != sz) {
        delete[] _outputBuffer;
        _outputBuffer = new uint8_t[sz*4];
        _outputBufferSize = sz;
    }

    _converged = _renderBuffer->IsConverged();

    // Colorize!
    for (auto& colorizer : _colorizerTable) {
        if (_aovName == colorizer.aovName &&
            _renderBuffer->GetFormat() == colorizer.aovFormat) {
            colorizer.callback(_outputBuffer, _renderBuffer->Map(),
                               _outputBufferSize);
            _renderBuffer->Unmap();
            break;
        }
    }
    // (special handling for primvar tokens).
    if (HdAovIdentifier(_aovName).isPrimvar &&
        _renderBuffer->GetFormat() == HdFormatFloat32Vec3) {
        _colorizePrimvar(_outputBuffer, _renderBuffer->Map(),
                         _outputBufferSize);
        _renderBuffer->Unmap();
    }

    // Blit!
    _Blit();
}

void
HdxColorizeTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdDirtyBits bits = _GetTaskDirtyBits();

    bool validate = false;
    if (bits & HdChangeTracker::DirtyParams) {
        HdxColorizeTaskParams params;

        if (_GetSceneDelegateValue(HdTokens->params, &params)) {
            _aovName = params.aovName;
            _renderBufferId = params.renderBuffer;
            validate = true;
        }
    }

    HdRenderIndex &renderIndex = GetDelegate()->GetRenderIndex();
    _renderBuffer = static_cast<HdRenderBuffer*>(
        renderIndex.GetBprim(HdPrimTypeTokens->renderBuffer, _renderBufferId));

    if (validate) {
        bool match = false;
        for (auto& colorizer : _colorizerTable) {
            if (_aovName == colorizer.aovName &&
                _renderBuffer->GetFormat() == colorizer.aovFormat) {
                match = true;
                break;
            }
        }
        if (HdAovIdentifier(_aovName).isPrimvar &&
            _renderBuffer->GetFormat() == HdFormatFloat32Vec3) {
            match = true;
        }
        if (!match) {
            TF_WARN("Unsupported AOV input %s with format %s",
                _aovName.GetText(),
                TfEnum::GetName(_renderBuffer->GetFormat()).c_str());
        }
    }
}

// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxColorizeTaskParams& pv)
{
    out << "ColorizeTask Params: (...) "
        << pv.aovName << " "
        << pv.renderBuffer;
    return out;
}

bool operator==(const HdxColorizeTaskParams& lhs,
                const HdxColorizeTaskParams& rhs)
{
    return lhs.aovName      == rhs.aovName      &&
           lhs.renderBuffer == rhs.renderBuffer;
}

bool operator!=(const HdxColorizeTaskParams& lhs,
                const HdxColorizeTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
