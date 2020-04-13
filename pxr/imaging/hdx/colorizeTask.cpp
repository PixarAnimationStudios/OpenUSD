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
#include "pxr/imaging/hdx/colorizeTask.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxColorizeTask::HdxColorizeTask(HdSceneDelegate* delegate, SdfPath const& id)
 : HdxProgressiveTask(id)
 , _aovName()
 , _aovBufferPath()
 , _depthBufferPath()
 , _applyColorQuantization(false)
 , _aovBuffer(nullptr)
 , _depthBuffer(nullptr)
 , _outputBuffer(nullptr)
 , _outputBufferSize(0)
 , _converged(false)
 , _compositor()
 , _needsValidation(false)
{
}

HdxColorizeTask::~HdxColorizeTask()
{
    delete[] _outputBuffer;
}

bool
HdxColorizeTask::IsConverged() const
{
    return _converged;
}

struct _Colorizer {
    typedef void (*ColorizerCallback)
        (uint8_t* dest, uint8_t* src, size_t nPixels, uint32_t imageWidth);
    TfToken aovName;
    HdFormat aovFormat;
    ColorizerCallback callback;
};

static void _colorizeNdcDepth(
    uint8_t* dest, uint8_t* src, size_t nPixels, uint32_t imageWidth)
{
    // depth is in clip space, so remap (-1, 1) to (0,1) and clamp.
    float *depthBuffer = reinterpret_cast<float*>(src);

    WorkParallelForN(nPixels,
        [&dest, &src, &nPixels, &imageWidth, &depthBuffer]
        (size_t begin, size_t end) 
    {
        for (size_t i=begin; i<end; ++i) {
            float valuef =
                std::min(std::max((depthBuffer[i] * 0.5f) + 0.5f, 0.0f), 1.0f);
            uint8_t value = (uint8_t)(255.0f * valuef);
            // special case 1.0 (far plane) as all black.
            if (depthBuffer[i] >= 1.0f) {
                value = 0;
            }
            dest[i*4+0] = value;
            dest[i*4+1] = value;
            dest[i*4+2] = value;
            dest[i*4+3] = 255;
        }
    });
}

static void _colorizeCameraDepth(
    uint8_t* dest, uint8_t* src, size_t nPixels, uint32_t imageWidth)
{
    // cameraDepth is depth from the camera, in world units. Its range is
    // [0, N] for some maximum N; to display it, rescale to [0, 1] and
    // splat that across RGB.
    float maxDepth = 0.0f;
    float *depthBuffer = reinterpret_cast<float*>(src);
    for (size_t i = 0; i < nPixels; ++i) {
        maxDepth = std::max(depthBuffer[i], maxDepth);
    }

    if (maxDepth != 0.0f) {
        WorkParallelForN(nPixels,
            [&dest, &src, &nPixels, &imageWidth, &depthBuffer, &maxDepth]
            (size_t begin, size_t end)
        {
            for (size_t i=begin; i<end; ++i) {
                float valuef =
                    std::min(std::max((depthBuffer[i] / maxDepth), 0.0f), 1.0f);
                uint8_t value =
                    (uint8_t)(255.0f * valuef);
                dest[i*4+0] = value;
                dest[i*4+1] = value;
                dest[i*4+2] = value;
                dest[i*4+3] = 255;
            }
        });
    }
}

static void _colorizeNormal(
    uint8_t* dest, uint8_t* src, size_t nPixels, uint32_t imageWidth)
{
    float *normalBuffer = reinterpret_cast<float*>(src);

    WorkParallelForN(nPixels,
        [&dest, &src, &nPixels, &imageWidth, &normalBuffer]
        (size_t begin, size_t end)
    {
        for (size_t i=begin; i<end; ++i) {
            GfVec3f n(normalBuffer[i*3+0], normalBuffer[i*3+1],
                      normalBuffer[i*3+2]);
            n = (n * 0.5f) + GfVec3f(0.5f);
            dest[i*4+0] = (uint8_t)(n[0] * 255.0f);
            dest[i*4+1] = (uint8_t)(n[1] * 255.0f);
            dest[i*4+2] = (uint8_t)(n[2] * 255.0f);
            dest[i*4+3] = 255;
        }
    });
}

static void _colorizeId(
    uint8_t* dest, uint8_t* src, size_t nPixels, uint32_t imageWidth)
{
    // XXX: this is legacy ID-display behavior, but an alternative is to
    // hash the ID to 3 bytes and use those as color. Even fancier,
    // hash to hue and stratified (saturation, value) levels, etc.
    int32_t *idBuffer = reinterpret_cast<int32_t*>(src);

    WorkParallelForN(nPixels,
        [&dest, &src, &nPixels, &imageWidth, &idBuffer]
        (size_t begin, size_t end)
    {
            for (size_t i=begin; i<end; ++i) {
            int32_t id = idBuffer[i];
            dest[i*4+0] = (uint8_t)(id & 0xff);
            dest[i*4+1] = (uint8_t)((id >> 8) & 0xff);
            dest[i*4+2] = (uint8_t)((id >> 16) & 0xff);
            dest[i*4+3] = 255;
        }
    });
}

static void _colorizePrimvar(
    uint8_t* dest, uint8_t* src, size_t nPixels, uint32_t imageWidth)
{
    float *primvarBuffer = reinterpret_cast<float*>(src);

    WorkParallelForN(nPixels,
        [&dest, &src, &nPixels, &imageWidth, &primvarBuffer]
        (size_t begin, size_t end) 
    {
        for (size_t i=begin; i<end; ++i) {
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
    });
}

// Prman linear to display
static float DspyLinearTosRGB(float u)
{
    return u < 0.0031308f ? 12.92f * u : 1.055f * powf(u, 0.4167f) - 0.055f;
}

// Prman DspyQuantize
static int DspyQuantize(
    float value, int x, int y, int k, int min, int max, int dither)
{
    float const s_order[8][8] = {
        {
            -0.49219f,
            0.00781f,
            -0.36719f,
            0.13281f,
            -0.46094f,
            0.03906f,
            -0.33594f,
            0.16406f,
        },
        {
            0.25781f,
            -0.24219f,
            0.38281f,
            -0.11719f,
            0.28906f,
            -0.21094f,
            0.41406f,
            -0.08594f,
        },
        {
            -0.30469f,
            0.19531f,
            -0.42969f,
            0.07031f,
            -0.27344f,
            0.22656f,
            -0.39844f,
            0.10156f,
        },
        {
            0.44531f,
            -0.05469f,
            0.32031f,
            -0.17969f,
            0.47656f,
            -0.02344f,
            0.35156f,
            -0.14844f,
        },
        {
            -0.44531f,
            0.05469f,
            -0.32031f,
            0.17969f,
            -0.47656f,
            0.02344f,
            -0.35156f,
            0.14844f,
        },
        {
            0.30469f,
            -0.19531f,
            0.42969f,
            -0.07031f,
            0.27344f,
            -0.22656f,
            0.39844f,
            -0.10156f,
        },
        {
            -0.25781f,
            0.24219f,
            -0.38281f,
            0.11719f,
            -0.28906f,
            0.21094f,
            -0.41406f,
            0.08594f,
        },
        {
            0.49219f,
            -0.00781f,
            0.36719f,
            -0.13281f,
            0.46094f,
            -0.03906f,
            0.33594f,
            -0.16406f,
        },
    };
    int dx, dy;
    switch (k & 3)
    {
    case 0:
        dx = x & 7;
        dy = y & 7;
        break;
    case 1:
        dx = 7 - (y & 7);
        dy = x & 7;
        break;
    case 2:
        dx = 7 - (x & 7);
        dy = 7 - (y & 7);
        break;
    case 3:
        dx = y & 7;
        dy = 7 - (x & 7);
        break;
    }  // (k & 3)

    value *= max - min;
    if (dither) value += s_order[dy][dx] + 0.49999f;
    int result = min + (int)floorf(value);
    result = min > result ? min : result;
    result = max < result ? max : result;
    return result;
}

static void _float32ToDisplay(
    uint8_t* dest, 
    uint8_t* src, 
    size_t nPixels,
    uint32_t imageWidth)
{
    float *colorBuffer = reinterpret_cast<float*>(src);

    WorkParallelForN(nPixels,
        [&dest, &src, &nPixels, &imageWidth, &colorBuffer]
        (size_t begin, size_t end)
    {
        for (size_t i=begin; i<end; ++i) {
            GfVec4f n(colorBuffer[i*4+0], colorBuffer[i*4+1],
                      colorBuffer[i*4+2], colorBuffer[i*4+3]);

            int x = i % imageWidth;
            int y = i / imageWidth;

            dest[i*4+0] = DspyQuantize(
                DspyLinearTosRGB(n[0]), x, y, 0, 0, UINT8_MAX, true);
            dest[i*4+1] = DspyQuantize(
                DspyLinearTosRGB(n[1]), x, y, 1, 0, UINT8_MAX, true);
            dest[i*4+2] = DspyQuantize(
                DspyLinearTosRGB(n[2]), x, y, 2, 0, UINT8_MAX, true);

            dest[i*4+3] = (uint8_t)(n[3] * 255.0f);
        }
    });
}

static void _float16ToDisplay(
    uint8_t* dest, 
    uint8_t* src, 
    size_t nPixels,
    uint32_t imageWidth)
{
    GfHalf *colorBuffer = reinterpret_cast<GfHalf*>(src);

    WorkParallelForN(nPixels,
        [&dest, &src, &nPixels, &imageWidth, &colorBuffer]
        (size_t begin, size_t end)
    {
        for (size_t i=begin; i<end; ++i) {
            GfVec4f n(colorBuffer[i*4+0], colorBuffer[i*4+1],
                      colorBuffer[i*4+2], colorBuffer[i*4+3]);

            int x = i % imageWidth;
            int y = i / imageWidth;

            dest[i*4+0] = DspyQuantize(
                DspyLinearTosRGB(n[0]), x, y, 0, 0, UINT8_MAX, true);
            dest[i*4+1] = DspyQuantize(
                DspyLinearTosRGB(n[1]), x, y, 1, 0, UINT8_MAX, true);
            dest[i*4+2] = DspyQuantize(
                DspyLinearTosRGB(n[2]), x, y, 2, 0, UINT8_MAX, true);

            dest[i*4+3] = (uint8_t)(n[3] * 255.0f);
        }
    });
}

static void _uint8ToDisplay(
    uint8_t* dest, 
    uint8_t* src, 
    size_t nPixels,
    uint32_t imageWidth)
{
    uint8_t *colorBuffer = reinterpret_cast<uint8_t*>(src);

    WorkParallelForN(nPixels,
        [&dest, &src, &nPixels, &imageWidth, &colorBuffer]
        (size_t begin, size_t end)
    {
        for (size_t i=begin; i<end; ++i) {
            GfVec4f n(colorBuffer[i*4+0] / 255.0f, 
                      colorBuffer[i*4+1] / 255.0f,
                      colorBuffer[i*4+2] / 255.0f, 
                      colorBuffer[i*4+3] / 255.0f);

            int x = i % imageWidth;
            int y = i / imageWidth;

            dest[i*4+0] = DspyQuantize(
                DspyLinearTosRGB(n[0]), x, y, 0, 0, UINT8_MAX, true);
            dest[i*4+1] = DspyQuantize(
                DspyLinearTosRGB(n[1]), x, y, 1, 0, UINT8_MAX, true);
            dest[i*4+2] = DspyQuantize(
                DspyLinearTosRGB(n[2]), x, y, 2, 0, UINT8_MAX, true);

            dest[i*4+3] = (uint8_t)(n[3] * 255.0f);
        }
    });
}

// XXX: It would be nice to make the colorizers more flexible on input format,
// but this gets the job done.
static _Colorizer _colorizerTable[] = {
    { HdAovTokens->color, HdFormatUNorm8Vec4, _uint8ToDisplay },
    { HdAovTokens->color, HdFormatFloat16Vec4, _float16ToDisplay },
    { HdAovTokens->color, HdFormatFloat32Vec4, _float32ToDisplay },
    { HdAovTokens->depth, HdFormatFloat32, _colorizeNdcDepth },
    { HdAovTokens->cameraDepth, HdFormatFloat32, _colorizeCameraDepth },
    { HdAovTokens->Neye, HdFormatFloat32Vec3, _colorizeNormal },
    { HdAovTokens->normal, HdFormatFloat32Vec3, _colorizeNormal },
    { HdAovTokens->primId, HdFormatInt32, _colorizeId },
    { HdAovTokens->elementId, HdFormatInt32, _colorizeId },
    { HdAovTokens->instanceId, HdFormatInt32, _colorizeId },
};

void
HdxColorizeTask::Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxColorizeTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            _aovName = params.aovName;
            _aovBufferPath = params.aovBufferPath;
            _depthBufferPath = params.depthBufferPath;
            _applyColorQuantization = params.applyColorQuantization;
            _needsValidation = true;
        }
    }
    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxColorizeTask::Prepare(HdTaskContext* ctx, HdRenderIndex *renderIndex)
{
    _aovBuffer = nullptr;
    _depthBuffer = nullptr;

    // An empty _aovBufferPath disables the task
    if (_aovBufferPath.IsEmpty()) {
        return;
    }

    _aovBuffer = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer, _aovBufferPath));

    if (!_aovBuffer) {
        if (_needsValidation) {
            TF_WARN("Bad AOV input buffer path %s", _aovBufferPath.GetText());
            _needsValidation = false;
        }
        return;
    }

    if (!_depthBufferPath.IsEmpty()) {
        _depthBuffer = static_cast<HdRenderBuffer*>(
            renderIndex->GetBprim(
                HdPrimTypeTokens->renderBuffer, _depthBufferPath));
        if (!_depthBuffer && _needsValidation) {
            TF_WARN("Bad depth input buffer path %s",
                    _depthBufferPath.GetText());
        }
    }

    if (_needsValidation) {
        _needsValidation = false;

        if (_aovName == HdAovTokens->color &&
            _aovBuffer->GetFormat() == HdFormatUNorm8Vec4) {
            return;
        }
        for (auto& colorizer : _colorizerTable) {
            if (_aovName == colorizer.aovName &&
                _aovBuffer->GetFormat() == colorizer.aovFormat) {
                return;
            }
        }
        if (HdParsedAovToken(_aovName).isPrimvar &&
            _aovBuffer->GetFormat() == HdFormatFloat32Vec3) {
            return;
        }
        TF_WARN("Unsupported AOV input %s with format %s",
                _aovName.GetText(),
                TfEnum::GetName(_aovBuffer->GetFormat()).c_str());
    }
}

void
HdxColorizeTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // _aovBuffer is null if the task is disabled
    // because _aovBufferPath is empty or
    // we failed to look up the renderBuffer in the render index,
    // in which case the error was previously reported
    if (!_aovBuffer) {
        // If there is no aov buffer to colorize, then this task is never
        // going to do anything, and so should immediately be marked as
        // converged.
        _converged = true;
        return;
    }

    // Allocate the scratch space, if needed.
    size_t size = _aovBuffer->GetWidth() * _aovBuffer->GetHeight();
    if (!_applyColorQuantization && _aovName == HdAovTokens->color) {
        size = 0;
    }

    if (_outputBufferSize != size) {
        delete[] _outputBuffer;
        _outputBuffer = (size != 0) ? (new uint8_t[size*4]) : nullptr;
        _outputBufferSize = size;
    }

    _converged = _aovBuffer->IsConverged();
    if (_depthBuffer) {
        _converged = _converged && _depthBuffer->IsConverged();
    }

    // Resolve the buffers before we read them.
    _aovBuffer->Resolve();
    if (_depthBuffer) {
        _depthBuffer->Resolve();
    }

    // XXX: Right now, we colorize on the CPU, before uploading data to the
    // fullscreen pass.  It would be much better if the colorizer callbacks
    // were done in fragment shaders. This is particularly important for
    // backends that keep renderbuffers on the GPU.

    // Colorize!
    bool depthAware = false;
    if (_depthBuffer && _depthBuffer->GetFormat() == HdFormatFloat32) {
        uint8_t* db = reinterpret_cast<uint8_t*>(_depthBuffer->Map());
        _compositor.SetTexture(TfToken("depth"),
                               _depthBuffer->GetWidth(),
                               _depthBuffer->GetHeight(),
                               HdFormatFloat32, db);
        _depthBuffer->Unmap();
        depthAware = true;
    } else {
        // If no depth buffer is bound, don't draw with depth.
        _compositor.SetTexture(TfToken("depth"),
                               0, 0, HdFormatInvalid, nullptr);
    }

    if (!_applyColorQuantization && _aovName == HdAovTokens->color) {
        // Special handling for color: to avoid a copy, just read the data
        // from the render buffer if no quantization is requested.
        _compositor.SetTexture(TfToken("color"),
                               _aovBuffer->GetWidth(),
                               _aovBuffer->GetHeight(),
                               _aovBuffer->GetFormat(),
                               _aovBuffer->Map());
        _aovBuffer->Unmap();
    } else {

        // Otherwise, colorize into the scratch buffer.
        bool colorized = false;

        // Check the colorizer callbacks.
        for (auto& colorizer : _colorizerTable) {
            if (_aovName == colorizer.aovName &&
                _aovBuffer->GetFormat() == colorizer.aovFormat) {
                uint32_t width = _aovBuffer->GetWidth();
                uint8_t* ab = reinterpret_cast<uint8_t*>(_aovBuffer->Map());
                colorizer.callback(_outputBuffer, ab, _outputBufferSize, width);
                _aovBuffer->Unmap();
                colorized = true;
                break;
            }
        }

        // Special handling for primvar tokens: they all go through the same
        // function...
        if (!colorized && HdParsedAovToken(_aovName).isPrimvar &&
            _aovBuffer->GetFormat() == HdFormatFloat32Vec3) {
            uint32_t width = _aovBuffer->GetWidth();
            uint8_t* ab = reinterpret_cast<uint8_t*>(_aovBuffer->Map());
            _colorizePrimvar(_outputBuffer, ab, _outputBufferSize, width);
            _aovBuffer->Unmap();
            colorized = true;
        }

        // Upload the scratch buffer.
        if (colorized) {
            _compositor.SetTexture(TfToken("color"),
                                   _aovBuffer->GetWidth(),
                                   _aovBuffer->GetHeight(),
                                   HdFormatUNorm8Vec4,
                                   _outputBuffer);
        } else {
            // Skip the compositor if we have no color data.
            return;
        }
    }

    // Blit!
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    _compositor.SetProgramToCompositor(depthAware);
    _compositor.Draw();

    if (!blendEnabled) {
        glDisable(GL_BLEND);
    }
}


// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxColorizeTaskParams& pv)
{
    out << "ColorizeTask Params: (...) "
        << pv.aovName << " "
        << pv.aovBufferPath << " "
        << pv.depthBufferPath << " "
        << pv.applyColorQuantization;
    return out;
}

bool operator==(const HdxColorizeTaskParams& lhs,
                const HdxColorizeTaskParams& rhs)
{
    return lhs.aovName         == rhs.aovName          &&
           lhs.aovBufferPath   == rhs.aovBufferPath    &&
           lhs.depthBufferPath == rhs.depthBufferPath  &&
           lhs.applyColorQuantization == rhs.applyColorQuantization;
}

bool operator!=(const HdxColorizeTaskParams& lhs,
                const HdxColorizeTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
