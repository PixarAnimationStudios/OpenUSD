//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "renderer.h"
#include "debugCodes.h"
#include "renderBuffer.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"

#include <random>

PXR_NAMESPACE_OPEN_SCOPE

HdParticleFieldRenderer::HdParticleFieldRenderer()
    : _aovBindings(), _aovNames(), _aovBindingsNeedValidation(false)
    , _aovBindingsValid(false), _width(0), _height(0)
    , _viewMatrix(1.0f), _projMatrix(1.0f), _samplesToConvergence(0)
    , _completedSamples(0) {}

HdParticleFieldRenderer::~HdParticleFieldRenderer() = default;

void HdParticleFieldRenderer::addGaussianSplats(
    const Hd3DGaussianSplat& splatPrim, const std::string& splatName) {

    size_t numSplats = splatPrim.GetPositions().size();
    if (numSplats == 0)
        return;

    GaussianSplats::Ptr splats = GaussianSplats::create();

    splats->positions = splatPrim.GetPositions();

    if (!splatPrim.GetOrientations().empty()) {
        splats->rotations = splatPrim.GetOrientations();
    }

    if (!splatPrim.GetScales().empty()) {
        splats->scales = splatPrim.GetScales();
    }

    if (!splatPrim.GetOpacities().empty()) {
        splats->opacities = splatPrim.GetOpacities();
    }

    if (!splatPrim.GetSphericalHarmonics().empty()) {
        splats->sphericalHarmonicsDegree =
            splatPrim.GetSphericalHarmonicsDegree();
        splats->sphericalHarmonics = splatPrim.GetSphericalHarmonics();
    }

    splats->xform  = splatPrim.GetTransform();
    splats->primID = splatPrim.GetPrimId();

    _gsRenderer.addGaussianSplats(splatName, splats);
}

void HdParticleFieldRenderer::removeGaussianSplats(
    const std::string& splatName) {
    _gsRenderer.removeGaussianSplats(splatName);
}

void HdParticleFieldRenderer::SetSamplesToConvergence(
    int samplesToConvergence) {
    _samplesToConvergence = samplesToConvergence;
}

void HdParticleFieldRenderer::SetDataWindow(const GfRect2i& dataWindow) {
    _dataWindow = dataWindow;

    // Re-validate the attachments, since attachment viewport and
    // render viewport need to match.
    _aovBindingsNeedValidation = true;
}

void HdParticleFieldRenderer::SetCamera(const GfMatrix4d& viewMatrix,
        const GfMatrix4d& projMatrix) {
    _gsRenderer.setWorldToViewMatrix(GfMatrix4f(viewMatrix));
    _gsRenderer.setProjMatrix(GfMatrix4f(projMatrix));
}

void HdParticleFieldRenderer::SetAovBindings(
        HdRenderPassAovBindingVector const& aovBindings) {
    _aovBindings = aovBindings;
    _aovNames.resize(_aovBindings.size());
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        _aovNames[i] = HdParsedAovToken(_aovBindings[i].aovName);
    }

    // Re-validate the attachments.
    _aovBindingsNeedValidation = true;
}

bool HdParticleFieldRenderer::_ValidateAovBindings() {
    if (!_aovBindingsNeedValidation) {
        return _aovBindingsValid;
    }

    _aovBindingsNeedValidation = false;
    _aovBindingsValid          = true;

    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        // By the time the attachment gets here, there should be a bound
        // output buffer.
        if (_aovBindings[i].renderBuffer == nullptr) {
            TF_WARN("Aov '%s' doesn't have any renderbuffer bound",
                    _aovNames[i].name.GetText());
            _aovBindingsValid = false;
            continue;
        }

        if (_aovNames[i].name != HdAovTokens->color &&
            _aovNames[i].name != HdAovTokens->depth &&
            _aovNames[i].name != HdAovTokens->primId) {
            TF_WARN("Unsupported attachment with Aov '%s' won't be rendered to",
                _aovNames[i].name.GetText());
        }

        HdFormat format = _aovBindings[i].renderBuffer->GetFormat();

        // depth is only supported for float32 attachments
        if (_aovNames[i].name == HdAovTokens->depth &&
            format != HdFormatFloat32) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                _aovNames[i].name.GetText(),
                TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // ids are only supported for int32 attachments
        if (_aovNames[i].name == HdAovTokens->primId &&
            format != HdFormatInt32) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                _aovNames[i].name.GetText(),
                TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // color is only supported for vec3/vec4 attachments of float,
        // unorm, or snorm.
        if (_aovNames[i].name == HdAovTokens->color) {
            switch (format) {
            case HdFormatUNorm8Vec4:
            case HdFormatUNorm8Vec3:
            case HdFormatSNorm8Vec4:
            case HdFormatSNorm8Vec3:
            case HdFormatFloat32Vec4:
            case HdFormatFloat32Vec3:
                break;
            default:
                TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
                break;
            }
        }

        // make sure the clear value is reasonable for the format of the
        // attached buffer.
        if (!_aovBindings[i].clearValue.IsEmpty()) {
            HdTupleType clearType =
                HdGetValueTupleType(_aovBindings[i].clearValue);

            // array-valued clear types aren't supported.
            if (clearType.count != 1) {
                TF_WARN("Aov '%s' clear value type '%s' is an array",
                    _aovNames[i].name.GetText(),
                    _aovBindings[i].clearValue.GetTypeName().c_str());
                _aovBindingsValid = false;
            }

            // color only supports float/double vec3/4
            if (_aovNames[i].name == HdAovTokens->color &&
                clearType.type != HdTypeFloatVec3 &&
                clearType.type != HdTypeFloatVec4 &&
                clearType.type != HdTypeDoubleVec3 &&
                clearType.type != HdTypeDoubleVec4) {
                TF_WARN("Aov '%s' clear value type '%s' isn't compatible",
                    _aovNames[i].name.GetText(),
                    _aovBindings[i].clearValue.GetTypeName().c_str());
                _aovBindingsValid = false;
            }

            // only clear float formats with float, int with int.
            if ((format == HdFormatFloat32 && clearType.type != HdTypeFloat) ||
                (format == HdFormatInt32 && clearType.type != HdTypeInt32)) {
                TF_WARN("Aov '%s' clear value type '%s' isn't compatible with"
                        " format %s",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str(),
                        TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
            }
        }
    }

    return _aovBindingsValid;
}

void HdParticleFieldRenderer::MarkAovBuffersUnconverged() {
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdParticleFieldRenderBuffer* rb =
            static_cast<HdParticleFieldRenderBuffer*>(
                _aovBindings[i].renderBuffer);
        rb->SetConverged(false);
    }
}

static bool _IsContained(const GfRect2i& rect, int width, int height) {
    return rect.GetMinX() >= 0 && rect.GetMaxX() < width &&
           rect.GetMinY() >= 0 && rect.GetMaxY() < height;
}

/// Rendering entrypoint: add one sample per pixel to the whole sample
/// buffer, and then loop until the image is converged.  After each pass,
/// the image will be resolved into a color buffer.
///   \param renderThread A handle to the render thread, used for checking
///                       for cancellation and locking the color buffer.
void HdParticleFieldRenderer::Render(HdRenderThread* renderThread) {
    TF_DEBUG(HDPARTICLEFIELD_GENERAL).Msg(
        "[%s] Starting Render\n", TF_FUNC_NAME().c_str());

    _completedSamples.store(0);

    if (!_ValidateAovBindings()) {
        // We aren't going to render anything. Just mark all AOVs as converged
        // so that we will stop rendering.
        for (size_t i = 0; i < _aovBindings.size(); ++i) {
            HdParticleFieldRenderBuffer* rb =
                static_cast<HdParticleFieldRenderBuffer*>(
                    _aovBindings[i].renderBuffer);
            rb->SetConverged(true);
        }
        // XXX:validation
        TF_WARN("Could not validate Aovs. Render will not complete");
        return;
    }

    _width  = 0;
    _height = 0;

    // Map all of the attachments.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        static_cast<HdParticleFieldRenderBuffer*>(
            _aovBindings[i].renderBuffer)->Map();

        if (i == 0) {
            _width  = _aovBindings[i].renderBuffer->GetWidth();
            _height = _aovBindings[i].renderBuffer->GetHeight();
        } else {
            if (_width != _aovBindings[i].renderBuffer->GetWidth() ||
                _height != _aovBindings[i].renderBuffer->GetHeight()) {
                TF_CODING_ERROR(
                    "HDGaussianSplats render buffers have inconsistent sizes");
            }
        }
    }

    if (_width > 0 || _height > 0) {
        if (!_IsContained(_dataWindow, _width, _height)) {
            TF_CODING_ERROR("dataWindow is larger than render buffer");
        }
    }

    // Render the image. Each pass through the loop adds a sample per pixel
    // (with jittered ray direction); the longer the loop runs, the less noisy
    // the image becomes. We add a cancellation point once per loop.
    //
    // We consider the image converged after N samples, which is a convenient
    // and simple heuristic.
    for (int i = 0; i < _samplesToConvergence; ++i) {
        // Pause point.
        while (renderThread->IsPauseRequested()) {
            if (renderThread->IsStopRequested()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        // Cancellation point.
        if (renderThread->IsStopRequested()) {
            break;
        }

        // Cancellation point.
        if (renderThread && renderThread->IsStopRequested()) {
            break;
        }

        {
            HdParticleFieldRenderBuffer* colorRenderBuffer;
            HdParticleFieldRenderBuffer* depthRenderBuffer;
            HdParticleFieldRenderBuffer* primIDRenderBuffer;

            // Write AOVs to attachments that aren't converged.
            for (size_t i = 0; i < _aovBindings.size(); ++i) {
                HdParticleFieldRenderBuffer* renderBuffer =
                    static_cast<HdParticleFieldRenderBuffer*>(
                        _aovBindings[i].renderBuffer);

                if (renderBuffer->IsConverged()) {
                    continue;
                }

                if (_aovNames[i].name == HdAovTokens->color) {
                    if (renderBuffer) {
                        colorRenderBuffer = renderBuffer;
                    }
                } else if (_aovNames[i].name == HdAovTokens->depth &&
                           renderBuffer->GetFormat() == HdFormatFloat32) {
                    if (renderBuffer) {
                        depthRenderBuffer = renderBuffer;
                    }
                } else if (_aovNames[i].name == HdAovTokens->primId &&
                           renderBuffer->GetFormat() == HdFormatInt32) {
                    if (renderBuffer) {
                        primIDRenderBuffer = renderBuffer;
                    }
                }
            }

            if (!_gsRenderer.renderGaussianSplatScene(
                    colorRenderBuffer, depthRenderBuffer, primIDRenderBuffer)) {
                printf("error occurred while rendering\n");
            }
        }

        // After the first pass, mark the single-sampled attachments as
        // converged and unmap them. If there are no multisampled attachments,
        // we are done.
        if (i == 0) {
            bool moreWork = false;
            for (size_t i = 0; i < _aovBindings.size(); ++i) {
                HdParticleFieldRenderBuffer* rb =
                    static_cast<HdParticleFieldRenderBuffer*>(
                        _aovBindings[i].renderBuffer);
                if (rb->IsMultiSampled()) {
                    moreWork = true;
                }
            }
            if (!moreWork) {
                _completedSamples.store(i + 1);
                break;
            }
        }

        // Track the number of completed samples for external consumption.
        _completedSamples.store(i + 1);

        // Cancellation point.
        if (renderThread->IsStopRequested()) {
            break;
        }
    }

    // Mark the multisampled attachments as converged and unmap all buffers.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdParticleFieldRenderBuffer* rb =
            static_cast<HdParticleFieldRenderBuffer*>(
                _aovBindings[i].renderBuffer);
        rb->Unmap();
        rb->SetConverged(true);
    }
}

/// Clear the bound aov buffers (typically before rendering).
void HdParticleFieldRenderer::Clear() {
    if (!_ValidateAovBindings()) {
        return;
    }

    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        if (_aovBindings[i].clearValue.IsEmpty()) {
            continue;
        }

        HdParticleFieldRenderBuffer* rb =
            static_cast<HdParticleFieldRenderBuffer*>(
                _aovBindings[i].renderBuffer);

        rb->Map();
        if (_aovNames[i].name == HdAovTokens->color) {
            GfVec4f clearColor = _GetClearColor(_aovBindings[i].clearValue);
            rb->Clear(4, clearColor.data());
        } else if (rb->GetFormat() == HdFormatInt32) {
            int32_t clearValue = _aovBindings[i].clearValue.Get<int32_t>();
            rb->Clear(1, &clearValue);
        } else if (rb->GetFormat() == HdFormatFloat32) {
            float clearValue = _aovBindings[i].clearValue.Get<float>();
            rb->Clear(1, &clearValue);
        } else if (rb->GetFormat() == HdFormatFloat32Vec3) {
            GfVec3f clearValue = _aovBindings[i].clearValue.Get<GfVec3f>();
            rb->Clear(3, clearValue.data());
        } // else, _ValidateAovBindings would have already warned.

        rb->Unmap();
        rb->SetConverged(false);
    }
}

/* static */
GfVec4f HdParticleFieldRenderer::_GetClearColor(VtValue const& clearValue) {
    HdTupleType type = HdGetValueTupleType(clearValue);
    if (type.count != 1) {
        return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }

    switch (type.type) {
    case HdTypeFloatVec3: {
        GfVec3f f = *(static_cast<const GfVec3f*>(HdGetValueData(clearValue)));
        return GfVec4f(f[0], f[1], f[2], 1.0f);
    }
    case HdTypeFloatVec4: {
        GfVec4f f = *(static_cast<const GfVec4f*>(HdGetValueData(clearValue)));
        return f;
    }
    case HdTypeDoubleVec3: {
        GfVec3d f = *(static_cast<const GfVec3d*>(HdGetValueData(clearValue)));
        return GfVec4f(f[0], f[1], f[2], 1.0f);
    }
    case HdTypeDoubleVec4: {
        GfVec4d f = *(static_cast<const GfVec4d*>(HdGetValueData(clearValue)));
        return GfVec4f(f);
    }
    default:
        return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
