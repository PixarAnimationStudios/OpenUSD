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
#include "pxr/imaging/hdEmbree/renderer.h"

#include "pxr/imaging/hdEmbree/renderBuffer.h"
#include "pxr/imaging/hdEmbree/config.h"
#include "pxr/imaging/hdEmbree/context.h"
#include "pxr/imaging/hdEmbree/mesh.h"

#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

HdEmbreeRenderer::HdEmbreeRenderer()
    : _aovBindings()
    , _aovNames()
    , _aovBindingsNeedValidation(false)
    , _aovBindingsValid(false)
    , _width(0)
    , _height(0)
    , _viewMatrix(1.0f) // == identity
    , _projMatrix(1.0f) // == identity
    , _inverseViewMatrix(1.0f) // == identity
    , _inverseProjMatrix(1.0f) // == identity
    , _scene(nullptr)
    , _samplesToConvergence(0)
    , _ambientOcclusionSamples(0)
    , _enableSceneColors(false)
{
}

HdEmbreeRenderer::~HdEmbreeRenderer()
{
}

void
HdEmbreeRenderer::SetScene(RTCScene scene)
{
    _scene = scene;
}

void
HdEmbreeRenderer::SetSamplesToConvergence(int samplesToConvergence)
{
    _samplesToConvergence = samplesToConvergence;
}

void
HdEmbreeRenderer::SetAmbientOcclusionSamples(int ambientOcclusionSamples)
{
    _ambientOcclusionSamples = ambientOcclusionSamples;
}

void
HdEmbreeRenderer::SetEnableSceneColors(bool enableSceneColors)
{
    _enableSceneColors = enableSceneColors;
}

void
HdEmbreeRenderer::SetViewport(unsigned int width, unsigned int height)
{
    _width = width;
    _height = height;

    // Re-validate the attachments, since attachment viewport and
    // render viewport need to match.
    _aovBindingsNeedValidation = true;
}

void
HdEmbreeRenderer::SetCamera(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projMatrix)
{
    _viewMatrix = viewMatrix;
    _projMatrix = projMatrix;
    _inverseViewMatrix = viewMatrix.GetInverse();
    _inverseProjMatrix = projMatrix.GetInverse();
}

void
HdEmbreeRenderer::SetAovBindings(
    HdRenderPassAovBindingVector const &aovBindings)
{
    _aovBindings = aovBindings;
    _aovNames.resize(_aovBindings.size());
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        _aovNames[i] = HdParsedAovToken(_aovBindings[i].aovName);
    }

    // Re-validate the attachments.
    _aovBindingsNeedValidation = true;
}

bool
HdEmbreeRenderer::_ValidateAovBindings()
{
    if (!_aovBindingsNeedValidation) {
        return _aovBindingsValid;
    }

    _aovBindingsNeedValidation = false;
    _aovBindingsValid = true;

    for (size_t i = 0; i < _aovBindings.size(); ++i) {

        // By the time the attachment gets here, there should be a bound
        // output buffer.
        if (_aovBindings[i].renderBuffer == nullptr) {
            TF_WARN("Aov '%s' doesn't have any renderbuffer bound",
                    _aovNames[i].name.GetText());
            _aovBindingsValid = false;
            continue;
        }

        // Currently, HdEmbree only supports color, linearDepth, and primId
        if (_aovNames[i].name != HdAovTokens->color &&
            _aovNames[i].name != HdAovTokens->linearDepth &&
            _aovNames[i].name != HdAovTokens->depth &&
            _aovNames[i].name != HdAovTokens->primId &&
            _aovNames[i].name != HdAovTokens->Neye &&
            _aovNames[i].name != HdAovTokens->normal &&
            !_aovNames[i].isPrimvar) {
            TF_WARN("Unsupported attachment with Aov '%s' won't be rendered to",
                    _aovNames[i].name.GetText());
        }

        HdFormat format = _aovBindings[i].renderBuffer->GetFormat();

        // depth is only supported for float32 attachments
        if ((_aovNames[i].name == HdAovTokens->linearDepth ||
             _aovNames[i].name == HdAovTokens->depth) &&
            format != HdFormatFloat32) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // primId is only supported for int32 attachments
        if (_aovNames[i].name == HdAovTokens->primId &&
            format != HdFormatInt32) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // Normal is only supported for vec3 attachments of float.
        if ((_aovNames[i].name == HdAovTokens->Neye ||
             _aovNames[i].name == HdAovTokens->normal) &&
            format != HdFormatFloat32Vec3) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // Primvars support vec3 output (though some channels may not be used).
        if (_aovNames[i].isPrimvar &&
            format != HdFormatFloat32Vec3) {
            TF_WARN("Aov 'primvars:%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // color is only supported for vec3/vec4 attachments of float,
        // unorm, or snorm.
        if (_aovNames[i].name == HdAovTokens->color) {
            switch(format) {
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

            // only clear float formats with float, int with int, float3 with
            // float3.
            if ((format == HdFormatFloat32 && clearType.type != HdTypeFloat) ||
                (format == HdFormatInt32 && clearType.type != HdTypeInt32) ||
                (format == HdFormatFloat32Vec3 &&
                 clearType.type != HdTypeFloatVec3)) {
                TF_WARN("Aov '%s' clear value type '%s' isn't compatible with"
                        " format %s",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str(),
                        TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
            }
        }

        // make sure the attachment and render viewports match.
        // XXX: we could possibly relax this in the future.
        if (_aovBindings[i].renderBuffer->GetWidth() != _width ||
            _aovBindings[i].renderBuffer->GetHeight() != _height) {
            TF_WARN("Aov '%s' viewport (%u, %u) doesn't match render viewport"
                    " (%u, %u)",
                    _aovNames[i].name.GetText(),
                    _aovBindings[i].renderBuffer->GetWidth(),
                    _aovBindings[i].renderBuffer->GetHeight(),
                    _width, _height);

            // if the viewports don't match, we block rendering.
            _aovBindingsValid = false;
        }
    }

    return _aovBindingsValid;
}

/* static */
GfVec4f
HdEmbreeRenderer::_GetClearColor(VtValue const& clearValue)
{
    HdTupleType type = HdGetValueTupleType(clearValue);
    if (type.count != 1) {
        return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }

    switch(type.type) {
        case HdTypeFloatVec3:
        {
            GfVec3f f =
                *(static_cast<const GfVec3f*>(HdGetValueData(clearValue)));
            return GfVec4f(f[0], f[1], f[2], 1.0f);
        }
        case HdTypeFloatVec4:
        {
            GfVec4f f =
                *(static_cast<const GfVec4f*>(HdGetValueData(clearValue)));
            return GfVec4f(f[0], f[1], f[2], 1.0f);
        }
        case HdTypeDoubleVec3:
        {
            GfVec3d f =
                *(static_cast<const GfVec3d*>(HdGetValueData(clearValue)));
            return GfVec4f(f[0], f[1], f[2], 1.0f);
        }
        case HdTypeDoubleVec4:
        {
            GfVec4d f =
                *(static_cast<const GfVec4d*>(HdGetValueData(clearValue)));
            return GfVec4f(f[0], f[1], f[2], 1.0f);
        }
        default:
            return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

void
HdEmbreeRenderer::Clear()
{
    if (!_ValidateAovBindings()) {
        return;
    }

    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        if (_aovBindings[i].clearValue.IsEmpty()) {
            continue;
        }

        HdEmbreeRenderBuffer *rb = 
            static_cast<HdEmbreeRenderBuffer*>(_aovBindings[i].renderBuffer);

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

void
HdEmbreeRenderer::MarkAovBuffersUnconverged()
{
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdEmbreeRenderBuffer *rb =
            static_cast<HdEmbreeRenderBuffer*>(_aovBindings[i].renderBuffer);
        rb->SetConverged(false);
    }
}

void
HdEmbreeRenderer::Render(HdRenderThread *renderThread)
{
    // Commit any pending changes to the scene.
    rtcCommit(_scene);

    if (!_ValidateAovBindings()) {
        return;
    }

    // Map all of the attachments.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        static_cast<HdEmbreeRenderBuffer*>(
            _aovBindings[i].renderBuffer)->Map();
    }

    // Render the image. Each pass through the loop adds a sample per pixel
    // (with jittered ray direction); the longer the loop runs, the less noisy
    // the image becomes. We add a cancellation point once per loop.
    //
    // We consider the image converged after N samples, which is a convenient
    // and simple heuristic.
    for (int i = 0; i < _samplesToConvergence; ++i) {
        unsigned int tileSize = HdEmbreeConfig::GetInstance().tileSize;
        const unsigned int numTilesX = (_width + tileSize-1) / tileSize;
        const unsigned int numTilesY = (_height + tileSize-1) / tileSize;

        // Render by scheduling square tiles of the sample buffer in a parallel
        // for loop.
        WorkParallelForN(numTilesX*numTilesY,
            std::bind(&HdEmbreeRenderer::_RenderTiles, this,
                (i == 0) ? nullptr : renderThread,
                std::placeholders::_1, std::placeholders::_2));

        // After the first pass, mark the single-sampled attachments as
        // converged and unmap them. If there are no multisampled attachments,
        // we are done.
        if (i == 0) {
            bool moreWork = false;
            for (size_t i = 0; i < _aovBindings.size(); ++i) {
                HdEmbreeRenderBuffer *rb = static_cast<HdEmbreeRenderBuffer*>(
                    _aovBindings[i].renderBuffer);
                if (!rb->IsMultiSampled()) {
                    rb->Unmap();
                    rb->SetConverged(true);
                } else {
                    moreWork = true;
                }
            }
            if (!moreWork) {
                break;
            }
        }

        // Cancellation point.
        if (renderThread->IsStopRequested()) {
            break;
        }
    }

    // Mark the multisampled attachments as converged and unmap them.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdEmbreeRenderBuffer *rb = static_cast<HdEmbreeRenderBuffer*>(
            _aovBindings[i].renderBuffer);
        if (rb->IsMultiSampled()) {
            rb->Unmap();
            rb->SetConverged(true);
        }
    }
}

void
HdEmbreeRenderer::_RenderTiles(HdRenderThread *renderThread,
                               size_t tileStart, size_t tileEnd)
{
    unsigned int tileSize =
        HdEmbreeConfig::GetInstance().tileSize;
    const unsigned int numTilesX = (_width + tileSize-1) / tileSize;

    // Initialize the RNG for this tile (each tile creates one as
    // a lazy way to do thread-local RNGs).
    size_t seed = std::chrono::system_clock::now().time_since_epoch().count();
    boost::hash_combine(seed, tileStart);
    std::default_random_engine random(seed);

    // Create a uniform distribution for jitter calculations.
    std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    std::function<float()> uniform_float = std::bind(uniform_dist, random);

    // _RenderTiles gets a range of tiles; iterate through them.
    for (unsigned int tile = tileStart; tile < tileEnd; ++tile) {

        // Cancellation point.
        if (renderThread && renderThread->IsStopRequested()) {
            break;
        }

        // Compute the pixel location of tile boundaries.
        const unsigned int tileY = tile / numTilesX;
        const unsigned int tileX = tile - tileY * numTilesX; 
        // (Above is equivalent to: tileX = tile % numTilesX)
        const unsigned int x0 = tileX * tileSize;
        const unsigned int y0 = tileY * tileSize;
        // Clamp far boundaries to the viewport, in case tileSize doesn't
        // neatly divide _width or _height.
        const unsigned int x1 = std::min(x0+tileSize, _width);
        const unsigned int y1 = std::min(y0+tileSize, _height);

        // Loop over pixels casting rays.
        for (unsigned int y = y0; y < y1; ++y) {
            for (unsigned int x = x0; x < x1; ++x) {

                // Jitter the camera ray direction.
                GfVec2f jitter(0.0f, 0.0f);
                if (HdEmbreeConfig::GetInstance().jitterCamera) {
                    jitter = GfVec2f(uniform_float(), uniform_float());
                }

                // Un-transform the pixel's NDC coordinates through the
                // projection matrix to get the trace of the camera ray in the
                // near plane.
                GfVec3f ndc = GfVec3f(2 * ((x + jitter[0]) / _width) - 1,
                                      2 * ((y + jitter[1]) / _height) - 1,
                                      -1);
                GfVec3f nearPlaneTrace = _inverseProjMatrix.Transform(ndc);

                GfVec3f origin;
                GfVec3f dir;

                bool isOrthographic = round(_projMatrix[3][3]) == 1;
                if (isOrthographic) {
                    // During orthographic projection: trace parallel rays
                    // from the near plane trace.
                    origin = nearPlaneTrace;
                    dir = GfVec3f(0,0,-1);
                } else {
                    // Otherwise, assume this is a perspective projection;
                    // project from the camera origin through the
                    // near plane trace.
                    origin = GfVec3f(0,0,0);
                    dir = nearPlaneTrace;
                }
                // Transform camera rays to world space.
                origin = _inverseViewMatrix.Transform(origin);
                dir = _inverseViewMatrix.TransformDir(dir).GetNormalized();

                // Trace the ray.
                _TraceRay(x, y, origin, dir, random);
            }
        }
    }
}

/// Fill in an RTCRay structure from the given parameters.
static void
_PopulateRay(RTCRay *ray, GfVec3f const& origin, 
             GfVec3f const& dir, float nearest)
{
    ray->org[0] = origin[0];
    ray->org[1] = origin[1];
    ray->org[2] = origin[2];
    ray->dir[0] = dir[0];
    ray->dir[1] = dir[1];
    ray->dir[2] = dir[2];
    ray->tnear = nearest; 
    ray->tfar = std::numeric_limits<float>::infinity();
    ray->geomID = RTC_INVALID_GEOMETRY_ID;
    ray->primID = RTC_INVALID_GEOMETRY_ID;
    ray->mask = -1;
    ray->time = 0.0f;
}

/// Generate a random cosine-weighted direction ray (in the hemisphere
/// around <0,0,1>).  The input is a pair of uniformly distributed random
/// numbers in the range [0,1].
///
/// The algorithm here is to generate a random point on the disk, and project
/// that point to the unit hemisphere.
static GfVec3f
_CosineWeightedDirection(GfVec2f const& uniform_float)
{
    GfVec3f dir;
    float theta = 2.0f * M_PI * uniform_float[0];
    float eta = uniform_float[1];
    float sqrteta = sqrtf(eta);
    dir[0] = cosf(theta) * sqrteta;
    dir[1] = sinf(theta) * sqrteta;
    dir[2] = sqrtf(1.0f-eta);
    return dir;
}

void
HdEmbreeRenderer::_TraceRay(unsigned int x, unsigned int y,
                            GfVec3f const &origin, GfVec3f const &dir,
                            std::default_random_engine &random)
{
    // Intersect the camera ray.
    RTCRay ray;
    _PopulateRay(&ray, origin, dir, 0.0f);
    rtcIntersect(_scene, ray);

    // Write AOVs to attachments that aren't converged.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdEmbreeRenderBuffer *renderBuffer =
            static_cast<HdEmbreeRenderBuffer*>(_aovBindings[i].renderBuffer);

        if (renderBuffer->IsConverged()) {
            continue;
        }

        if (_aovNames[i].name == HdAovTokens->color) {
            GfVec4f clearColor = _GetClearColor(_aovBindings[i].clearValue);
            GfVec4f sample = _ComputeColor(ray, random, clearColor);
            renderBuffer->Write(GfVec3i(x,y,1), 4, sample.data());
        } else if ((_aovNames[i].name == HdAovTokens->linearDepth ||
                    _aovNames[i].name == HdAovTokens->depth) &&
                   renderBuffer->GetFormat() == HdFormatFloat32) {
            float depth;
            bool ndc = (_aovNames[i].name == HdAovTokens->depth);
            if(_ComputeDepth(ray, &depth, ndc)) {
                renderBuffer->Write(GfVec3i(x,y,1), 1, &depth);
            }
        } else if (_aovNames[i].name == HdAovTokens->primId &&
                   renderBuffer->GetFormat() == HdFormatInt32) {
            int32_t primId;
            if (_ComputePrimId(ray, &primId)) {
                renderBuffer->Write(GfVec3i(x,y,1), 1, &primId);
            }
        } else if ((_aovNames[i].name == HdAovTokens->Neye ||
                    _aovNames[i].name == HdAovTokens->normal) &&
                   renderBuffer->GetFormat() == HdFormatFloat32Vec3) {
            GfVec3f normal;
            bool eye = (_aovNames[i].name == HdAovTokens->Neye);
            if (_ComputeNormal(ray, &normal, eye)) {
                renderBuffer->Write(GfVec3i(x,y,1), 3, normal.data());
            }
        } else if (_aovNames[i].isPrimvar &&
                   renderBuffer->GetFormat() == HdFormatFloat32Vec3) {
            GfVec3f value;
            if (_ComputePrimvar(ray, _aovNames[i].name, &value)) {
                renderBuffer->Write(GfVec3i(x,y,1), 3, value.data());
            }
        }
    }
}

bool
HdEmbreeRenderer::_ComputePrimId(RTCRay const& rayHit,
                                 int32_t *primId)
{
    if (rayHit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    // Get the instance and prototype context structures for the hit prim.
    HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
            rtcGetUserData(_scene, rayHit.instID));

    HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
            rtcGetUserData(instanceContext->rootScene, rayHit.geomID));

    *primId = prototypeContext->rprim->GetPrimId();
    return true;
}

bool
HdEmbreeRenderer::_ComputeDepth(RTCRay const& rayHit,
                                float *depth,
                                bool ndc)
{
    if (rayHit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    if (ndc) {
        GfVec3f hitPos = GfVec3f(rayHit.org[0] + rayHit.tfar * rayHit.dir[0],
            rayHit.org[1] + rayHit.tfar * rayHit.dir[1],
            rayHit.org[2] + rayHit.tfar * rayHit.dir[2]);

        hitPos = _viewMatrix.Transform(hitPos);
        hitPos = _projMatrix.Transform(hitPos);

        *depth = hitPos[2];
    } else {
        *depth = rayHit.tfar;
    }
    return true;
}

bool
HdEmbreeRenderer::_ComputeNormal(RTCRay const& rayHit,
                                 GfVec3f *normal,
                                 bool eye)
{
    if (rayHit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
                rtcGetUserData(_scene, rayHit.instID));

    HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
                rtcGetUserData(instanceContext->rootScene, rayHit.geomID));

    GfVec3f n = -GfVec3f(rayHit.Ng[0], rayHit.Ng[1], rayHit.Ng[2]);
    if (prototypeContext->primvarMap.count(HdTokens->normals) > 0) {
        prototypeContext->primvarMap[HdTokens->normals]->Sample(
                rayHit.primID, rayHit.u, rayHit.v, &n);
    }

    n = instanceContext->objectToWorldMatrix.TransformDir(n);
    if (eye) {
        n = _viewMatrix.TransformDir(n);
    }
    n.Normalize();

    *normal = n;
    return true;
}

bool
HdEmbreeRenderer::_ComputePrimvar(RTCRay const& rayHit,
                                  TfToken const& primvar,
                                  GfVec3f *value)
{
    if (rayHit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
                rtcGetUserData(_scene, rayHit.instID));

    HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
                rtcGetUserData(instanceContext->rootScene, rayHit.geomID));

    // XXX: This is a little clunky, although sample will early out if the
    // types don't match.
    if (prototypeContext->primvarMap.count(primvar) > 0) {
        HdEmbreePrimvarSampler *sampler = 
            prototypeContext->primvarMap[primvar];
        if (sampler->Sample(rayHit.primID, rayHit.u, rayHit.v, value)) {
            return true;
        }
        GfVec2f v2;
        if (sampler->Sample(rayHit.primID, rayHit.u, rayHit.v, &v2)) {
            value->Set(v2[0], v2[1], 0.0f);
            return true;
        }
        float v1;
        if (sampler->Sample(rayHit.primID, rayHit.u, rayHit.v, &v1)) {
            value->Set(v1, 0.0f, 0.0f);
            return true;
        }
    }
    return false;
}

GfVec4f
HdEmbreeRenderer::_ComputeColor(RTCRay const& rayHit,
                                std::default_random_engine &random,
                                GfVec4f const& clearColor)
{
    if (rayHit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return clearColor;
    }

    // Get the instance and prototype context structures for the hit prim.
    HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
                rtcGetUserData(_scene, rayHit.instID));

    HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
                rtcGetUserData(instanceContext->rootScene, rayHit.geomID));

    // Compute the worldspace location of the rayHit hit.
    GfVec3f hitPos = GfVec3f(rayHit.org[0] + rayHit.tfar * rayHit.dir[0],
            rayHit.org[1] + rayHit.tfar * rayHit.dir[1],
            rayHit.org[2] + rayHit.tfar * rayHit.dir[2]);

    // If a normal primvar is present (e.g. from smooth shading), use that
    // for shading; otherwise use the flat face normal.
    GfVec3f normal = -GfVec3f(rayHit.Ng[0], rayHit.Ng[1], rayHit.Ng[2]);
    if (prototypeContext->primvarMap.count(HdTokens->normals) > 0) {
        prototypeContext->primvarMap[HdTokens->normals]->Sample(
                rayHit.primID, rayHit.u, rayHit.v, &normal);
    }

    // If a color primvar is present, use that as diffuse color; otherwise,
    // use flat white.
    GfVec4f color = GfVec4f(1.0f, 1.0f, 1.0f, 1.0f);
    if (_enableSceneColors &&
            prototypeContext->primvarMap.count(HdTokens->color) > 0) {
        prototypeContext->primvarMap[HdTokens->color]->Sample(
                rayHit.primID, rayHit.u, rayHit.v, &color);
    }

    // Transform the normal from object space to world space.
    normal = instanceContext->objectToWorldMatrix.TransformDir(normal);

    // Make sure the normal is unit-length.
    normal.Normalize();

    // Lighting model: (camera dot normal), i.e. diffuse-only point light
    // centered on the camera.
    GfVec3f dir = GfVec3f(rayHit.dir[0], rayHit.dir[1], rayHit.dir[2]);
    float diffuseLight = fabs(GfDot(-dir, normal)) *
        HdEmbreeConfig::GetInstance().cameraLightIntensity;

    // Lighting gets modulated by an ambient occlusion term.
    float aoLightIntensity =
        _ComputeAmbientOcclusion(hitPos, normal, random);

    // Return color.xyz * diffuseLight * aoLightIntensity.
    // XXX: Transparency?
    GfVec3f finalColor = GfVec3f(color[0], color[1], color[2]) *
        diffuseLight * aoLightIntensity;

    // Clamp colors to [0,1].
    GfVec4f output;
    output[0] = std::max(0.0f, std::min(1.0f, finalColor[0]));
    output[1] = std::max(0.0f, std::min(1.0f, finalColor[1]));
    output[2] = std::max(0.0f, std::min(1.0f, finalColor[2]));
    output[3] = 1.0f;
    return output;
}

float
HdEmbreeRenderer::_ComputeAmbientOcclusion(GfVec3f const& position,
                                           GfVec3f const& normal,
                                           std::default_random_engine &random)
{
    // Create a uniform random distribution for AO calculations.
    std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    std::function<float()> uniform_float = std::bind(uniform_dist, random);

    // 0 ambient occlusion samples means disable the ambient occlusion term.
    if (_ambientOcclusionSamples < 1) {
        return 1.0f;
    }

    float occlusionFactor = 0.0f;

    // For hemisphere sampling we need to choose a coordinate frame at this
    // point. For the purposes of _CosineWeightedDirection, the normal needs
    // to map to (0,0,1), but since the distribution is radially symmetric
    // we don't care about the other axes.
    GfMatrix3f basis(1);
    GfVec3f xAxis;
    if (fabsf(GfDot(normal, GfVec3f(0,0,1))) < 0.9f) {
        xAxis = GfCross(normal, GfVec3f(0,0,1));
    } else {
        xAxis = GfCross(normal, GfVec3f(0,1,0));
    }
    GfVec3f yAxis = GfCross(normal, xAxis);
    basis.SetColumn(0, xAxis.GetNormalized());
    basis.SetColumn(1, yAxis.GetNormalized());
    basis.SetColumn(2, normal);

    // Generate random samples, stratified with Latin Hypercube Sampling.
    // https://en.wikipedia.org/wiki/Latin_hypercube_sampling
    // Stratified sampling means we don't get all of our random samples
    // bunched in the far corner of the hemisphere, but instead have some
    // equal spacing guarantees.
    std::vector<GfVec2f> samples;
    samples.resize(_ambientOcclusionSamples);
    for (int i = 0; i < _ambientOcclusionSamples; ++i) {
        samples[i][0] = (float(i) + uniform_float()) / _ambientOcclusionSamples;
    }
    std::shuffle(samples.begin(), samples.end(), random);
    for (int i = 0; i < _ambientOcclusionSamples; ++i) {
        samples[i][1] = (float(i) + uniform_float()) / _ambientOcclusionSamples;
    }

    // Trace ambient occlusion rays. The occlusion factor is the fraction of
    // the hemisphere that's occluded when rays are traced to infinity,
    // computed by random sampling over the hemisphere.
    for (int i = 0; i < _ambientOcclusionSamples; i++)
    {
        // Sample in the hemisphere centered on the face normal. Use
        // cosine-weighted hemisphere sampling to bias towards samples which
        // will have a bigger effect on the occlusion term.
        GfVec3f shadowDir = basis * _CosineWeightedDirection(samples[i]);

        // Trace shadow ray, using the fast interface (rtcOccluded) since
        // we only care about intersection status, not intersection id.
        RTCRay shadow;
        _PopulateRay(&shadow, position, shadowDir, 0.001f);
        rtcOccluded(_scene, shadow);

        // Record this AO ray's contribution to the occlusion factor: a
        // boolean [In shadow/Not in shadow].
        if (shadow.geomID == RTC_INVALID_GEOMETRY_ID)
            occlusionFactor += GfDot(shadowDir, normal);
    }
    // Compute the average of the occlusion samples.
    occlusionFactor /= _ambientOcclusionSamples;

    return occlusionFactor;
}

PXR_NAMESPACE_CLOSE_SCOPE
