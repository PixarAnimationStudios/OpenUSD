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
#include "renderer.h"
#include "context.h"
#include "light.h"
#include "pcg_basic.h"

#include "pxr/imaging/plugin/hdEmbree/renderBuffer.h"
#include "pxr/imaging/plugin/hdEmbree/config.h"
#include "pxr/imaging/plugin/hdEmbree/context.h"
#include "pxr/imaging/plugin/hdEmbree/mesh.h"
#include <pxr/usd/usdLux/blackbody.h>

#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"

#include <boost/functional/hash.hpp>

#include <algorithm>
#include <chrono>
#include <embree3/rtcore_common.h>
#include <embree3/rtcore_geometry.h>
#include <embree3/rtcore_ray.h>
#include <embree3/rtcore_scene.h>
#include <limits>
#include <random>
#include <stdint.h>
#include <thread>

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
    , _completedSamples(0)
    , _domeIndex(-1)
{
}

HdEmbreeRenderer::~HdEmbreeRenderer()
{
    // delete any light textures that have been allocated
    for (auto const& light: _lights)
    {
        if (light.kind == LightKind::Dome)
        {
            delete[] light.texture.pixels;
        }
    }
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
HdEmbreeRenderer::SetDataWindow(const GfRect2i &dataWindow)
{
    _dataWindow = dataWindow;

    // Here for clients that do not use camera framing but the
    // viewport.
    //
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

unsigned HdEmbreeRenderer::SetLight(SdfPath const& lightPath, Light new_light, RTCDevice device)
{

    size_t light_index = -1;
    auto it = _lightMap.find(lightPath);
    if (it != _lightMap.end())
    {
        // if we're updating an existing light, first clean up the old old light's data
        Light& old_light = _lights[it->second];
        delete[] old_light.texture.pixels;
        old_light.texture.pixels = nullptr;
        if (old_light.rtcMeshId != RTC_INVALID_GEOMETRY_ID) {
            rtcDetachGeometry(_scene, old_light.rtcMeshId);
            rtcReleaseGeometry(old_light.rtcGeometry);
            old_light.rtcMeshId = RTC_INVALID_GEOMETRY_ID;
        }

        // then assign the new light to the existing slot
        _lights[it->second] = new_light;
        light_index = it->second;
    }
    else
    {
        // new light isn't in the scene already, add it
        _lights.push_back(new_light);
        _lightMap[lightPath] = _lights.size() - 1;
        light_index = _lights.size() - 1;
    }

    Light& light = _lights[light_index];

    // Now create the light geometry, if required
    // we do this last because we need the light index for the instance context
    if (light.kind == LightKind::Rect && light.visible)
    {
        // create light mesh
        GfVec3f v0(-light.rect.width/2, -light.rect.height/2, 0);
        GfVec3f v1( light.rect.width/2, -light.rect.height/2, 0);
        GfVec3f v2( light.rect.width/2,  light.rect.height/2, 0);
        GfVec3f v3(-light.rect.width/2,  light.rect.height/2, 0);

        v0 = light.xformLightToWorld.Transform(v0);
        v1 = light.xformLightToWorld.Transform(v1);
        v2 = light.xformLightToWorld.Transform(v2);
        v3 = light.xformLightToWorld.Transform(v3);

        light.rtcGeometry = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_QUAD);
        GfVec3f* vertices = (GfVec3f*)rtcSetNewGeometryBuffer(light.rtcGeometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(GfVec3f), 4);
        vertices[0] = v0;
        vertices[1] = v1;
        vertices[2] = v2;
        vertices[3] = v3;

        unsigned* index = (unsigned*)rtcSetNewGeometryBuffer(light.rtcGeometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT4, sizeof(unsigned)*4, 1);
        index[0] = 0; index[1] = 1; index[2] = 2; index[3] = 3;

        auto* ctx = new HdEmbreeInstanceContext;
        ctx->lightIndex = light_index;
        rtcSetGeometryUserData(light.rtcGeometry, ctx);
        rtcSetGeometryTimeStepCount(light.rtcGeometry, 1);

        rtcCommitGeometry(light.rtcGeometry);
        light.rtcMeshId = rtcAttachGeometry(_scene, light.rtcGeometry);
        if (light.rtcMeshId == RTC_INVALID_GEOMETRY_ID) {
            printf("could not create rect mesh\n");
        }
    } else if (light.kind == LightKind::Dome) {
        // if it's a dome, stash its index so we can evaluate it on misses
        _domeIndex = light_index;
    }

    return light_index;
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

        if (_aovNames[i].name != HdAovTokens->color &&
            _aovNames[i].name != HdAovTokens->cameraDepth &&
            _aovNames[i].name != HdAovTokens->depth &&
            _aovNames[i].name != HdAovTokens->primId &&
            _aovNames[i].name != HdAovTokens->instanceId &&
            _aovNames[i].name != HdAovTokens->elementId &&
            _aovNames[i].name != HdAovTokens->Neye &&
            _aovNames[i].name != HdAovTokens->normal &&
            !_aovNames[i].isPrimvar) {
            TF_WARN("Unsupported attachment with Aov '%s' won't be rendered to",
                    _aovNames[i].name.GetText());
        }

        HdFormat format = _aovBindings[i].renderBuffer->GetFormat();

        // depth is only supported for float32 attachments
        if ((_aovNames[i].name == HdAovTokens->cameraDepth ||
             _aovNames[i].name == HdAovTokens->depth) &&
            format != HdFormatFloat32) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // ids are only supported for int32 attachments
        if ((_aovNames[i].name == HdAovTokens->primId ||
             _aovNames[i].name == HdAovTokens->instanceId ||
             _aovNames[i].name == HdAovTokens->elementId) &&
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
            return f;
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
            return GfVec4f(f);
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

int
HdEmbreeRenderer::GetCompletedSamples() const
{
    return _completedSamples.load();
}

static
bool
_IsContained(const GfRect2i &rect, int width, int height)
{
    return
        rect.GetMinX() >= 0 && rect.GetMaxX() < width &&
        rect.GetMinY() >= 0 && rect.GetMaxY() < height;
}

void
HdEmbreeRenderer::Render(HdRenderThread *renderThread)
{
    _completedSamples.store(0);

    // Commit any pending changes to the scene.
    rtcCommitScene(_scene);

    if (!_ValidateAovBindings()) {
        // We aren't going to render anything. Just mark all AOVs as converged
        // so that we will stop rendering.
        for (size_t i = 0; i < _aovBindings.size(); ++i) {
            HdEmbreeRenderBuffer *rb = static_cast<HdEmbreeRenderBuffer*>(
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
        //
        // XXX
        //
        // A scene delegate might specify the path to a
        // render buffer instead of a pointer to the
        // render buffer.
        //
        static_cast<HdEmbreeRenderBuffer*>(
            _aovBindings[i].renderBuffer)->Map();

        if (i == 0) {
            _width  = _aovBindings[i].renderBuffer->GetWidth();
            _height = _aovBindings[i].renderBuffer->GetHeight();
        } else {
            if ( _width  != _aovBindings[i].renderBuffer->GetWidth() ||
                 _height != _aovBindings[i].renderBuffer->GetHeight()) {
                TF_CODING_ERROR(
                    "Embree render buffers have inconsistent sizes");
            }
        }
    }

    if (_width > 0 || _height > 0) {
        if (!_IsContained(_dataWindow, _width, _height)) {
            TF_CODING_ERROR(
                "dataWindow is larger than render buffer");
        
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

        const unsigned int tileSize = HdEmbreeConfig::GetInstance().tileSize;
        const unsigned int numTilesX =
            (_dataWindow.GetWidth() + tileSize-1) / tileSize;
        const unsigned int numTilesY =
            (_dataWindow.GetHeight() + tileSize-1) / tileSize;

        // Render by scheduling square tiles of the sample buffer in a parallel
        // for loop.
        WorkParallelForN(numTilesX*numTilesY,
            std::bind(&HdEmbreeRenderer::_RenderTiles, this, renderThread,
                std::placeholders::_1, std::placeholders::_2));

        // After the first pass, mark the single-sampled attachments as
        // converged and unmap them. If there are no multisampled attachments,
        // we are done.
        if (i == 0) {
            bool moreWork = false;
            for (size_t i = 0; i < _aovBindings.size(); ++i) {
                HdEmbreeRenderBuffer *rb = static_cast<HdEmbreeRenderBuffer*>(
                    _aovBindings[i].renderBuffer);
                if (rb->IsMultiSampled()) {
                    moreWork = true;
                }
            }
            if (!moreWork) {
                _completedSamples.store(i+1);
                break;
            }
        }

        // Track the number of completed samples for external consumption.
        _completedSamples.store(i+1);

        // Cancellation point.
        if (renderThread->IsStopRequested()) {
            break;
        }
    }

    // Mark the multisampled attachments as converged and unmap all buffers.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdEmbreeRenderBuffer *rb = static_cast<HdEmbreeRenderBuffer*>(
            _aovBindings[i].renderBuffer);
        rb->Unmap();
        rb->SetConverged(true);
    }
}

void
HdEmbreeRenderer::_RenderTiles(HdRenderThread *renderThread, 
                               size_t tileStart, size_t tileEnd)
{
    const unsigned int minX = _dataWindow.GetMinX();
    unsigned int minY = _dataWindow.GetMinY();
    const unsigned int maxX = _dataWindow.GetMaxX() + 1;
    unsigned int maxY = _dataWindow.GetMaxY() + 1;

    // If a client does not use AOVs and we have no render buffers,
    // _height is 0 and we shouldn't use it to flip the data window.
    if (_height > 0) {
        // The data window is y-Down but the image line order
        // is from bottom to top, so we need to flip it.
        std::swap(minY, maxY);
        minY = _height - minY;
        maxY = _height - maxY;
    }

    const unsigned int tileSize =
        HdEmbreeConfig::GetInstance().tileSize;
    const unsigned int numTilesX =
        (_dataWindow.GetWidth() + tileSize-1) / tileSize;

    // Random number state for this thread
    thread_local PCG pcg(std::hash<std::thread::id>()(std::this_thread::get_id()));

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
        const unsigned int x0 = tileX * tileSize + minX;
        const unsigned int y0 = tileY * tileSize + minY;
        // Clamp to data window, in case tileSize doesn't
        // neatly divide its with and height.
        const unsigned int x1 = std::min(x0+tileSize, maxX);
        const unsigned int y1 = std::min(y0+tileSize, maxY);

        // Loop over pixels casting rays.
        for (unsigned int y = y0; y < y1; ++y) {
            for (unsigned int x = x0; x < x1; ++x) {
                // Jitter the camera ray direction.
                GfVec2f jitter(0.0f, 0.0f);
                if (HdEmbreeConfig::GetInstance().jitterCamera) {
                    jitter = GfVec2f(pcg.uniform(), pcg.uniform());
                }

                // Un-transform the pixel's NDC coordinates through the
                // projection matrix to get the trace of the camera ray in the
                // near plane.
                const float w(_dataWindow.GetWidth());
                const float h(_dataWindow.GetHeight());

                const GfVec3f ndc(
                    2 * ((x + jitter[0] - minX) / w) - 1,
                    2 * ((y + jitter[1] - minY) / h) - 1,
                    -1);
                const GfVec3f nearPlaneTrace = _inverseProjMatrix.Transform(ndc);

                GfVec3f origin;
                GfVec3f dir;

                const bool isOrthographic = round(_projMatrix[3][3]) == 1;
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
                _TraceRay(x, y, origin, dir, pcg);
            }
        }
    }
}

/// Fill in an RTCRay structure from the given parameters.
static void
_PopulateRay(RTCRay *ray, GfVec3f const& origin, 
             GfVec3f const& dir, float nearest, 
             float furthest = std::numeric_limits<float>::infinity(),
             RayMask mask = RayMask::All)
{
    ray->org_x = origin[0];
    ray->org_y = origin[1];
    ray->org_z = origin[2];
    ray->tnear = nearest;

    ray->dir_x = dir[0];
    ray->dir_y = dir[1];
    ray->dir_z = dir[2];
    ray->time = 0.0f;

    ray->tfar = furthest;
    ray->mask = static_cast<uint32_t>(mask);
}

/// Fill in an RTCRayHit structure from the given parameters.
// note this containts a Ray and a RayHit
static void
_PopulateRayHit(RTCRayHit* rayHit, GfVec3f const& origin,
             GfVec3f const& dir, float nearest, 
             float furthest = std::numeric_limits<float>::infinity(), 
             RayMask mask = RayMask::All)
{
    // Fill in defaults for the ray
    _PopulateRay(&rayHit->ray, origin, dir, nearest, furthest, mask);

    // Fill in defaults for the hit
    rayHit->hit.primID = RTC_INVALID_GEOMETRY_ID;
    rayHit->hit.geomID = RTC_INVALID_GEOMETRY_ID;
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

bool HdEmbreeRenderer::_RayShouldContinue(RTCRayHit const& rayHit) const {
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        // missed, don't continue
        return false;
    }

    if (rayHit.hit.instID[0] == RTC_INVALID_GEOMETRY_ID) {
        // not hit an instance, but a "raw" geometry. This should be a light
        const HdEmbreeInstanceContext *instanceContext =
            static_cast<HdEmbreeInstanceContext*>(
                    rtcGetGeometryUserData(rtcGetGeometry(_scene, rayHit.hit.geomID)));

        if (instanceContext->lightIndex == -1) {
            // if this isn't a light, don't know what this is
            return false;
        }

        if ((rayHit.ray.mask & RayMask::Camera) && !_lights[instanceContext->lightIndex].visible_camera) {
            return true;
        } else if ((rayHit.ray.mask & RayMask::Shadow) && !_lights[instanceContext->lightIndex].visible_shadow) {
            return true;
        } else {
            return false;
        }
    }

    // XXX: otherwise this is a regular geo. we should handle visibility here too
    // eventually
    return false;
}

void
HdEmbreeRenderer::_TraceRay(unsigned int x, unsigned int y,
                            GfVec3f const &origin, GfVec3f const &dir,
                            PCG& pcg)
{
    // Intersect the camera ray.
    RTCRayHit rayHit; // EMBREE_FIXME: use RTCRay for occlusion rays
    rayHit.ray.flags = 0;
    _PopulateRayHit(&rayHit, origin, dir, 0.0f, std::numeric_limits<float>::max(), RayMask::Camera);
    {
      RTCIntersectContext context;
      rtcInitIntersectContext(&context);
      rtcIntersect1(_scene, &context, &rayHit);
      //
      // there is something odd about how this is used in Embree. Is it reversed
      // here and then when it it used in
      //      _ComputeNormal
      //      _ComputeColor
      // but not when it is used in
      //      _EmbreeCullFacess
      // this should probably all made to be consistent. What would make most
      // sense would be to remove this reversal, and then just change the test
      // in _EmbreeCullFaces. this would be the most performant solution.
      //
      rayHit.hit.Ng_x = -rayHit.hit.Ng_x;
      rayHit.hit.Ng_y = -rayHit.hit.Ng_y;
      rayHit.hit.Ng_z = -rayHit.hit.Ng_z;
    }

    if (_RayShouldContinue(rayHit)) {
        GfVec3f hitPos = GfVec3f(rayHit.ray.org_x + rayHit.ray.tfar * rayHit.ray.dir_x,
                rayHit.ray.org_y + rayHit.ray.tfar * rayHit.ray.dir_y,
                rayHit.ray.org_z + rayHit.ray.tfar * rayHit.ray.dir_z);

        _TraceRay(x, y, hitPos + dir * 0.001f, dir, pcg);
        return;
    }

    // Write AOVs to attachments that aren't converged.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdEmbreeRenderBuffer *renderBuffer =
            static_cast<HdEmbreeRenderBuffer*>(_aovBindings[i].renderBuffer);

        if (renderBuffer->IsConverged()) {
            continue;
        }

       if (_aovNames[i].name == HdAovTokens->color) {
            GfVec4f clearColor = _GetClearColor(_aovBindings[i].clearValue);
            GfVec4f sample = _ComputeColor(rayHit, pcg, clearColor);
            renderBuffer->Write(GfVec3i(x,y,1), 4, sample.data());
        } else if ((_aovNames[i].name == HdAovTokens->cameraDepth ||
                    _aovNames[i].name == HdAovTokens->depth) &&
                   renderBuffer->GetFormat() == HdFormatFloat32) {
            float depth;
            bool clip = (_aovNames[i].name == HdAovTokens->depth);
            if(_ComputeDepth(rayHit, &depth, clip)) {
                renderBuffer->Write(GfVec3i(x,y,1), 1, &depth);
            }
        } else if ((_aovNames[i].name == HdAovTokens->primId ||
                    _aovNames[i].name == HdAovTokens->elementId ||
                    _aovNames[i].name == HdAovTokens->instanceId) &&
                   renderBuffer->GetFormat() == HdFormatInt32) {
            int32_t id;
            if (_ComputeId(rayHit, _aovNames[i].name, &id)) {
                renderBuffer->Write(GfVec3i(x,y,1), 1, &id);
            }
        } else if ((_aovNames[i].name == HdAovTokens->Neye ||
                    _aovNames[i].name == HdAovTokens->normal) &&
                   renderBuffer->GetFormat() == HdFormatFloat32Vec3) {
            GfVec3f normal;
            bool eye = (_aovNames[i].name == HdAovTokens->Neye);
            if (_ComputeNormal(rayHit, &normal, eye)) {
                renderBuffer->Write(GfVec3i(x,y,1), 3, normal.data());
            }
        } else if (_aovNames[i].isPrimvar &&
                   renderBuffer->GetFormat() == HdFormatFloat32Vec3) {
            GfVec3f value;
            if (_ComputePrimvar(rayHit, _aovNames[i].name, &value)) {
                renderBuffer->Write(GfVec3i(x,y,1), 3, value.data());
            }
        }
    }
}

bool
HdEmbreeRenderer::_ComputeId(RTCRayHit const& rayHit, TfToken const& idType,
                             int32_t *id)
{
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    if (rayHit.hit.instID[0] == RTC_INVALID_GEOMETRY_ID) {
        // not hit an instance, but a "raw" geometry. This should be a light
        return false;
    }

    // Get the instance and prototype context structures for the hit prim.
    // We don't use embree's multi-level instancing; we
    // flatten everything in hydra. So instID[0] should always be correct.
    const HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
            rtcGetGeometryUserData(rtcGetGeometry(_scene, rayHit.hit.instID[0])));
    
    const HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
            rtcGetGeometryUserData(rtcGetGeometry(instanceContext->rootScene,rayHit.hit.geomID)));

    if (idType == HdAovTokens->primId) {
        *id = prototypeContext->rprim->GetPrimId();
    } else if (idType == HdAovTokens->elementId) {
        if (prototypeContext->primitiveParams.empty()) {
            *id = rayHit.hit.primID;
        } else {
            *id = HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam(
                prototypeContext->primitiveParams[rayHit.hit.primID]);
        }
    } else if (idType == HdAovTokens->instanceId) {
        *id = instanceContext->instanceId;
    } else {
        return false;
    }

    return true;
}

bool
HdEmbreeRenderer::_ComputeDepth(RTCRayHit const& rayHit,
                                float *depth,
                                bool clip)
{
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    if (rayHit.hit.instID[0] == RTC_INVALID_GEOMETRY_ID) {
        // not hit an instance, but a "raw" geometry. This should be a light
        return false;
    }

    if (clip) {
        GfVec3f hitPos = GfVec3f(rayHit.ray.org_x + rayHit.ray.tfar * rayHit.ray.dir_x,
            rayHit.ray.org_y + rayHit.ray.tfar * rayHit.ray.dir_y,
            rayHit.ray.org_z + rayHit.ray.tfar * rayHit.ray.dir_z);

        hitPos = _viewMatrix.Transform(hitPos);
        hitPos = _projMatrix.Transform(hitPos);

        // For the depth range transform, we assume [0,1].
        *depth = (hitPos[2] + 1.0f) / 2.0f;
    } else {
        *depth = rayHit.ray.tfar;
    }
    return true;
}

bool
HdEmbreeRenderer::_ComputeNormal(RTCRayHit const& rayHit,
                                 GfVec3f *normal,
                                 bool eye)
{
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    if (rayHit.hit.instID[0] == RTC_INVALID_GEOMETRY_ID) {
        // not hit an instance, but a "raw" geometry. This should be a light
        return false;
    }

    // We don't use embree's multi-level instancing; we
    // flatten everything in hydra. So instID[0] should always be correct.
    const HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
                rtcGetGeometryUserData(rtcGetGeometry(_scene,rayHit.hit.instID[0])));

    const HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
                rtcGetGeometryUserData(rtcGetGeometry(instanceContext->rootScene,rayHit.hit.geomID)));

    GfVec3f n = -GfVec3f(rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z);
    auto it = prototypeContext->primvarMap.find(HdTokens->normals);
    if (it != prototypeContext->primvarMap.end()) {
        it->second->Sample(rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v, &n);
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
HdEmbreeRenderer::_ComputePrimvar(RTCRayHit const& rayHit,
                                  TfToken const& primvar,
                                  GfVec3f *value)
{
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    if (rayHit.hit.instID[0] == RTC_INVALID_GEOMETRY_ID) {
        // not hit an instance, but a "raw" geometry. This should be a light
        return false;
    }

    // We don't use embree's multi-level instancing; we
    // flatten everything in hydra. So instID[0] should always be correct.
    const HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
                rtcGetGeometryUserData(rtcGetGeometry(_scene,rayHit.hit.instID[0])));

    const HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
                rtcGetGeometryUserData(rtcGetGeometry(instanceContext->rootScene,rayHit.hit.geomID)));

    // XXX: This is a little clunky, although sample will early out if the
    // types don't match.
    auto it = prototypeContext->primvarMap.find(primvar);
    if (it != prototypeContext->primvarMap.end()) {
        const HdEmbreePrimvarSampler *sampler = it->second;
        if (sampler->Sample(rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v, value)) {
            return true;
        }
        GfVec2f v2;
        if (sampler->Sample(rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v, &v2)) {
            value->Set(v2[0], v2[1], 0.0f);
            return true;
        }
        float v1;
        if (sampler->Sample(rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v, &v1)) {
            value->Set(v1, 0.0f, 0.0f);
            return true;
        }
    }
    return false;
}

struct ShapeSample {
    GfVec3f pWorld;
    GfVec3f nWorld;
    GfVec2f uv;
    float pdfA;
};

struct LightSample {
    GfVec3f Li;
    GfVec3f wI;
    float dist;
    float invPdfW;
};

float Lerp(float a, float b, float t) {
    return (1-t)*a + t*b;
}

GfVec3f Lerp(GfVec3f a, GfVec3f b, float t) {
    return (1.0f - t)*a + t*b;
}

float Sqr(float x) {
    return x*x;
}

float PosDot(GfVec3f a, GfVec3f b) {
    return std::max(0.0f, GfDot(a, b));
}

float Clamp(float x, float mn=0.0f, float mx=1.0f) {
    return std::max(mn, std::min(x, mx));
}

float Linstep(float a, float b, float t) {
    return Clamp((t - a)/(b - a));
}


float Smoothstep(float a, float b, float t) {
    t = Clamp((t - a)/(b - a));
    return t * t * (3.0f - 2.0f * t);
}

float Smootherstep(float a, float b, float t) {
    t = Clamp((t - a)/(b - a));
    return t * t * t * (t * (6.0f * t - 15.0f) + 10.0f);
}

// XXX: ported from PBRT
static GfVec3f SphericalDirection(float sinTheta, float cosTheta,
                                                float phi) {
    return GfVec3f(GfClamp(sinTheta, -1, 1) * GfCos(phi),
                    GfClamp(sinTheta, -1, 1) * GfSin(phi), GfClamp(cosTheta, -1, 1));
}

// XXX: ported from PBRT
static GfVec3f SampleUniformCone(GfVec2f u, float angle) 
{
    float cosAngle = GfCos(angle);
    float cosTheta = (1 - u[0]) + u[0] * cosAngle;
    float sinTheta = GfSqrt(GfMax(0.0f, 1.0f - cosTheta*cosTheta));
    float phi = u[1] * 2.0f * M_PI;
    return SphericalDirection(sinTheta, cosTheta, phi);
}

// XXX: ported from PBRT
static float UniformConePDF(float angle) {
    return 1 / (2 * M_PI * (1 - GfCos(angle)));
}

float HdEmbreeRenderer::_Visibility(GfVec3f const& position, GfVec3f const& direction, float dist)
{
    RTCRay shadow;
    shadow.flags = 0;
    _PopulateRay(&shadow, position, direction, 0.001f, dist, RayMask::Shadow);
    {
        RTCIntersectContext context;
        rtcInitIntersectContext(&context);
        rtcOccluded1(_scene,&context,&shadow);
    }
    // XXX: what do we do about shadow visibility (continuation) here?
    // probably need to use rtcIntersect instead of rtcOccluded

    // occluded sets tfar < 0 if the ray hit anything
    return shadow.tfar > 0.0f;
}

LightSample SampleDistantLight(Light const& light, GfVec3f const& position, float u1, float u2) 
{
    GfVec3f Le = light.color * light.intensity * powf(2.0f, light.exposure);
    if (light.enableColorTemperature) {
        Le = GfCompMult(Le, UsdLuxBlackbodyTemperatureAsRgb(light.colorTemperature));
    }
    
    if (light.distant.halfAngleRadians > 0.0f)
    {
        if (light.normalize)
        {
            float sinTheta = sinf(light.distant.halfAngleRadians);
            Le /= Sqr(sinTheta) * M_PI;
        }

        // There's an implicit double-negation of the wI direction here
        GfVec3f localDir = SampleUniformCone(GfVec2f(u1, u2), light.distant.halfAngleRadians);
        GfVec3f wI = light.xformLightToWorld.TransformDir(localDir);
        wI.Normalize();

        return LightSample {
            Le,
            wI,
            std::numeric_limits<float>::max(),
            1.0f / UniformConePDF(light.distant.halfAngleRadians)
        };
    }
    else
    {
        // delta case, infinite pdf
        GfVec3f wI = light.xformLightToWorld.TransformDir(GfVec3f(0.0f, 0.0f, 1.0f));
        wI.Normalize();

        return LightSample {
            Le,
            wI,
            std::numeric_limits<float>::max(),
            1.0f,
        };
    }
}

float AreaRect(GfMatrix4f const& xf, float width, float height) {
    const GfVec3f U = xf.TransformDir(GfVec3f{width, 0.0f, 0.0f});
    const GfVec3f V = xf.TransformDir(GfVec3f{0.0f, height, 0.0f});
    return GfCross(U, V).GetLength();
}

ShapeSample SampleRect(GfMatrix4f const& xf, float width, float height, float u1, float u2) {
    // Sample rectangle in object space
    const GfVec3f pLight(
      (u1 - 0.5f) * width,  
      (u2 - 0.5f) * height,  
      0.0f
    );
    const GfVec3f nLight(0.0f, 0.0f, -1.0f);
    const GfVec2f uv(u1, u2);

    // Transform to world space
    const GfVec3f pWorld = xf.Transform(pLight);
    const GfVec3f nWorld = xf.TransformDir(nLight).GetNormalized();

    const float area = AreaRect(xf, width, height);

    return ShapeSample {
        pWorld,
        nWorld,
        uv,
        1.0f / area
    };
}

float AreaSphere(GfMatrix4f const& xf, float radius) {
    // Area of the ellipsoid
    const float a = xf.TransformDir(GfVec3f{radius, 0.0f, 0.0f}).GetLength();
    const float b = xf.TransformDir(GfVec3f{0.0f, radius, 0.0f}).GetLength();
    const float c = xf.TransformDir(GfVec3f{0.0f, 0.0f, radius}).GetLength();
    const float ab = powf(a*b, 1.6f);
    const float ac = powf(a*c, 1.6f);
    const float bc = powf(b*c, 1.6f);
    return powf((ab + ac + bc)/3, 1/1.6f) * 4 * M_PI;
}

ShapeSample SampleSphere(GfMatrix4f const& xf, float radius, float u1, float u2) {
    // Sample sphere in light space
    const float z = 1 - 2 * u1;
    const float r = sqrtf(std::max(0.0f, 1 - z*z));
    const float phi = 2 * M_PI * u2;
    GfVec3f pLight{r * std::cos(phi), r * std::sin(phi), z};
    const GfVec3f nLight = pLight;
    pLight *= radius;
    const GfVec2f uv(u2, z);

    // Transform to world space
    const GfVec3f pWorld = xf.Transform(pLight);
    const GfVec3f nWorld = xf.TransformDir(nLight).GetNormalized();

    const float area = AreaSphere(xf, radius);

    return ShapeSample {
        pWorld,
        nWorld,
        uv,
        1.0f / area
    };
}

GfVec3f SampleDiskPolar(float u1, float u2)
{
    const float r = sqrtf(u1);
    const float theta = 2 * M_PI * u2;
    return GfVec3f(r * cosf(theta), r * sinf(theta), 0.0f);
}

float AreaDisk(GfMatrix4f const& xf, float radius) {
    // Calculate surface area of the ellipse
    const float a = xf.TransformDir(GfVec3f{radius, 0.0f, 0.0f}).GetLength();
    const float b = xf.TransformDir(GfVec3f{0.0f, radius, 0.0f}).GetLength();
    return M_PI * a * b;
}

ShapeSample SampleDisk(GfMatrix4f const& xf, float radius, float u1, float u2) {
    // Sample disk in light space
    GfVec3f pLight = SampleDiskPolar(u1, u2);
    const GfVec3f nLight(0.0f, 0.0f, -1.0f);
    const GfVec2f uv(pLight[0], pLight[1]);
    pLight *= radius;

    // Transform to world space
    const GfVec3f pWorld = xf.Transform(pLight);
    const GfVec3f nWorld = xf.TransformDir(nLight).GetNormalized();

    const float area = AreaDisk(xf, radius);

    return ShapeSample {
        pWorld,
        nWorld,
        uv,
        1.0f / area
    };
}

float AreaCylinder(GfMatrix4f const& xf, float radius, float length) {
    const float c = xf.TransformDir(GfVec3f{length, 0.0f, 0.0f}).GetLength();
    const float a = xf.TransformDir(GfVec3f{0.0f, radius, 0.0f}).GetLength();
    const float b = xf.TransformDir(GfVec3f{0.0f, 0.0f, radius}).GetLength();
    // Ramanujan's approximation to perimeter of ellipse
    const float e = M_PI * (3*(a+b) - sqrtf((3*a + b) * (a + 3*b)));
    return e * c;
}

ShapeSample SampleCylinder(GfMatrix4f const& xf, float radius, float length, float u1, float u2) {
    float z = Lerp(-length/2, length/2, u1);
    float phi = u2 * 2 * M_PI;
    // Compute cylinder sample position _pi_ and normal _n_ from $z$ and $\phi$
    GfVec3f pLight = GfVec3f(z, radius * cosf(phi), radius * sinf(phi));
    // Reproject _pObj_ to cylinder surface and compute _pObjError_
    float hitRad = sqrtf(Sqr(pLight[1]) + Sqr(pLight[2]));
    pLight[1] *= radius / hitRad;
    pLight[2] *= radius / hitRad;

    GfVec3f nLight(0.0f, pLight[1], pLight[2]);
    nLight.Normalize();

    // Transform to world space
    const GfVec3f pWorld = xf.Transform(pLight);
    const GfVec3f nWorld = xf.TransformDir(nLight).GetNormalized();

    const float area = AreaCylinder(xf, radius, length);

    return ShapeSample {
        pWorld,
        nWorld,
        GfVec2f(u2, u1),
        1.0f / area
    };
}

float Theta(GfVec3f const& v) {
    return acosf(Clamp(v[2], -1.f, 1.f));
}

float Phi(GfVec3f const& v) {
    float p = atan2f(v[1], v[0]);
    return p < 0 ? (p + 2 * M_PI) : p;
}

float EvalIES(Light const& light, GfVec3f const& wI) {
    IES const& ies = light.shaping.ies;

    if (!ies.iesFile.valid()) {
        // Either none specified or there was an error loading. In either case, just ignore
        return 1.0f;
    }

    // emission direction in light space
    GfVec3f wE = light.xformWorldToLight.TransformDir(wI).GetNormalized();

    float theta = Theta(wE);
    float phi = Phi(wE);

    // apply angle scale to theta, matching Karma
    // if (ies.angleScale > 0) {
    //     theta = Clamp(theta / (1 - ies.angleScale), 0, M_PI);
    // } else if (ies.angleScale < 0) {
    //     theta = Clamp(theta / (1 / (1 + ies.angleScale)), 0, M_PI);
    // }

    float norm = ies.normalize ? ies.iesFile.power() : 1.0f;

    return ies.iesFile.eval(theta, phi, ies.angleScale) / norm;
}

GfVec3f SampleLightTexture(LightTexture const& texture, float s, float t)
{
    if (texture.pixels == nullptr) {
        return GfVec3f(0.0f);
    }

    int x = float(texture.width) * s;
    int y = float(texture.height) * t;

    return texture.pixels[y*texture.width + x];
}

LightSample EvalAreaLight(Light const& light, ShapeSample const& ss, GfVec3f const& position) {
    // Transform PDF from area measure to solid angle measure. We use the inverse PDF
    // here to avoid division by zero when the surface point is behind the light
    GfVec3f wI = ss.pWorld - position;
    const float dist = wI.GetLength();
    wI /= dist;
    const float cosThetaON = PosDot(-wI, ss.nWorld);
    float invPdfW = cosThetaON / Sqr(dist) / ss.pdfA;
    GfVec4f Z = light.xformLightToWorld.GetColumn(2);
    const float cosThetaOZ = PosDot(-wI, GfVec3f(Z[0], Z[1], -Z[2]));

    // Combine the brightness parameters to get initial emission luminance (nits)
    GfVec3f Le = cosThetaON > 0.0f ? light.color * light.intensity * powf(2.0f, light.exposure) : GfVec3f(0);

    // Multiply in blackbody color, if enabled
    if (light.enableColorTemperature) {
        Le = GfCompMult(Le, UsdLuxBlackbodyTemperatureAsRgb(light.colorTemperature));
    }

    // Multiply by the texture, if there is one
    if (light.texture.pixels != nullptr) {
        Le = GfCompMult(Le, SampleLightTexture(light.texture, ss.uv[0], 1.0f - ss.uv[1]));
    }

    // If normalize is enabled, we need to divide the luminance by the surface area of the light,
    // which for an area light is equivalent to multiplying by the area pdf, which is itself the 
    // reciprocal of the surface area 
    if (light.normalize) {
        Le *= ss.pdfA;
    }

    // Apply focus shaping
    if (light.shaping.focus > 0.0f) {
        const float ff = powf(cosThetaOZ, light.shaping.focus);
        const GfVec3f focusTint = Lerp(light.shaping.focusTint, GfVec3f(1), ff);
        Le = GfCompMult(Le, focusTint);
    }

    // Apply cone shaping
    const float thetaC = light.shaping.coneAngle * M_PI / 180.0f;
    const float thetaS = Lerp(thetaC, 0.0f, light.shaping.coneSoftness);
    const float thetaOZ = acosf(cosThetaOZ);
    Le *= 1.0f - Smoothstep(thetaS, thetaC, thetaOZ);

    // Applu IES
    Le *= EvalIES(light, wI);

    return LightSample {
        Le,
        wI,
        dist,
        invPdfW
    };
}

ShapeSample IntersectAreaLight(Light const& light, RTCRayHit const& rayHit) {
    // XXX: just rect lights at the moment, need to do the others
    ShapeSample ss;

    ss.pWorld = GfVec3f(rayHit.ray.org_x + rayHit.ray.tfar * rayHit.ray.dir_x,
            rayHit.ray.org_y + rayHit.ray.tfar * rayHit.ray.dir_y,
            rayHit.ray.org_z + rayHit.ray.tfar * rayHit.ray.dir_z);
    ss.nWorld = GfVec3f(rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z);
    ss.uv = GfVec2f(1.0f - rayHit.hit.u, rayHit.hit.v);
    ss.pdfA = 1.0f / AreaRect(light.xformLightToWorld, light.rect.width, light.rect.height);

    return ss;
}

LightSample SampleAreaLight(Light const& light, GfVec3f const& position, float u1, float u2) {
    // First, sample the shape of the area light in its surface area measure
    ShapeSample ss;
    switch (light.kind) {
    case LightKind::Rect:
        ss = SampleRect(light.xformLightToWorld, light.rect.width, light.rect.height, u1, u2);
        break;
    case LightKind::Sphere:
        ss = SampleSphere(light.xformLightToWorld, light.sphere.radius, u1, u2);
        break;
    case LightKind::Disk:
        ss = SampleDisk(light.xformLightToWorld, light.disk.radius, u1, u2);
        break;
    case LightKind::Cylinder:
        ss = SampleCylinder(light.xformLightToWorld, light.cylinder.radius, light.cylinder.length, u1, u2);
        break;
    case LightKind::Distant:
    case LightKind::Dome:
        // infinite lights handled separately
        break;
    }

    return EvalAreaLight(light, ss, position);
}

LightSample EvalDomeLight(Light const& light, GfVec3f const& direction)
{
    float t = acosf(direction[1]) / M_PI;
    float s = atan2f(direction[0], direction[2]) / (2 * M_PI);
    s = 1.0f - fmodf(s+0.5f, 1.0f);

    GfVec3f Li = light.texture.pixels != nullptr ? 
        SampleLightTexture(light.texture, s, t) 
        : GfVec3f(1.0f);

    return LightSample {
        Li,
        direction,
        std::numeric_limits<float>::max(),
        1.0f / (4*M_PI)
    };
}

void OrthoBasis(GfVec3f const& n, GfVec3f& b1, GfVec3f& b2)
{
    float sign = copysignf(1.0f, n[2]);
    const float a = 1.0f / (sign + n[2]);
    const float b = n[0] * n[1] * a;
    b1 = GfVec3f(1.0f + sign * n[0] * n[0] * a, sign * b, -sign * n[0]).GetNormalized();
    b2 = GfVec3f(b, sign + n[1] * n[1] * a, -n[1]).GetNormalized();
}

LightSample SampleDomeLight(Light const& light, GfVec3f const& W, float u1, float u2)
{
    GfVec3f U, V;
    OrthoBasis(W, U, V);

#if 0
    // Cosine hemisphere sampling for simplicity
    GfVec3f p = SampleDiskPolar(u1, u2);
    const float z = sqrtf(std::max(0.0f, 1 - sqr(p[0]) - sqr(p[1])));

    const GfVec3f wI = (W * z + p[0] * U + p[1] * V).GetNormalized();

    LightSample ls = EvalDomeLight(light, wI);
    ls.invPdfW = M_PI / z;

#else

    float z = u1;
    float r = sqrtf(std::max(0.0f, 1.0f - Sqr(z)));
    float phi = 2 * M_PI * u2;
    
    const GfVec3f wI = (W * z + r * cosf(phi) * U + r * sinf(phi) * V).GetNormalized();

    LightSample ls = EvalDomeLight(light, wI);
    ls.invPdfW = 2 * M_PI;
#endif

    return ls;
}

GfVec4f
HdEmbreeRenderer::_ComputeColor(RTCRayHit const& rayHit,
                                PCG& pcg,
                                GfVec4f const& clearColor)
{
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        // if we missed all geometry in the scene, evaluate the infinite lights directly
        if (_domeIndex >= 0)
        {
            LightSample ls = EvalDomeLight(
                _lights[_domeIndex], 
                GfVec3f(rayHit.ray.dir_x, 
                        rayHit.ray.dir_y, 
                        rayHit.ray.dir_z).GetNormalized()
            );

            return GfVec4f(
                ls.Li[0],
                ls.Li[1],
                ls.Li[2],
                1.0f
            );
        }
        else
        {
            return clearColor;
        }
    }

    if (rayHit.hit.instID[0] == RTC_INVALID_GEOMETRY_ID) {
        // if it's not an instance then it's almost certainly a light
        const HdEmbreeInstanceContext *instanceContext =
            static_cast<HdEmbreeInstanceContext*>(
                    rtcGetGeometryUserData(rtcGetGeometry(_scene, rayHit.hit.geomID)));

        // if we hit a light, just evaluate the light directly
        if (instanceContext->lightIndex != -1) {
            Light const& light = _lights[instanceContext->lightIndex];
            ShapeSample ss = IntersectAreaLight(light, rayHit);
            LightSample ls = EvalAreaLight(light, ss, GfVec3f(rayHit.ray.org_x, rayHit.ray.org_y, rayHit.ray.org_z));

            return GfVec4f(ls.Li[0], ls.Li[1], ls.Li[2], 1);        
        } else {
            // should never get here. magenta warning!
            return GfVec4f(1, 0, 1, 1);        
        }

    }

    // Compute the worldspace location of the rayHit hit.
    GfVec3f hitPos = GfVec3f(rayHit.ray.org_x + rayHit.ray.tfar * rayHit.ray.dir_x,
            rayHit.ray.org_y + rayHit.ray.tfar * rayHit.ray.dir_y,
            rayHit.ray.org_z + rayHit.ray.tfar * rayHit.ray.dir_z);

    // Get the instance and prototype context structures for the hit prim.
    // We don't use embree's multi-level instancing; we
    // flatten everything in hydra. So instID[0] should always be correct.
    const HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
                rtcGetGeometryUserData(rtcGetGeometry(_scene, rayHit.hit.instID[0])));

    const HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
                rtcGetGeometryUserData(rtcGetGeometry(instanceContext->rootScene,rayHit.hit.geomID)));

    // If a normal primvar is present (e.g. from smooth shading), use that
    // for shading; otherwise use the flat face normal.
    GfVec3f normal = -GfVec3f(rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z);
    auto it = prototypeContext->primvarMap.find(HdTokens->normals);
    if (it != prototypeContext->primvarMap.end()) {
        it->second->Sample(
            rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v, &normal);
    }

    // If a color primvar is present, use that as diffuse color; otherwise,
    // use flat grey.
    GfVec3f color = GfVec3f(0.5f, 0.5f, 0.5f);
    if (_enableSceneColors) {
        auto it = prototypeContext->primvarMap.find(HdTokens->displayColor);
        if (it != prototypeContext->primvarMap.end()) {
            it->second->Sample(
                rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v, &color);
        }
    }

    // Transform the normal from object space to world space.
    normal = instanceContext->objectToWorldMatrix.TransformDir(normal);

    // Make sure the normal is unit-length.
    normal.Normalize();

    // If there are no lights, then keep the existing camera light + AO path to be
    // able to inspect the scene
    GfVec3f finalColor(0);
    if (_lights.empty())
    {
        // Lighting model: (camera dot normal), i.e. diffuse-only point light
        // centered on the camera.
        GfVec3f dir = GfVec3f(rayHit.ray.dir_x, rayHit.ray.dir_y, rayHit.ray.dir_z);
        float diffuseLight = fabs(GfDot(-dir, normal)) *
            HdEmbreeConfig::GetInstance().cameraLightIntensity;

        // Lighting gets modulated by an ambient occlusion term.
        float aoLightIntensity =
            _ComputeAmbientOcclusion(hitPos, normal, pcg);

        // XXX: We should support opacity here...

        finalColor = color * diffuseLight * aoLightIntensity;
    }
    else
    {
        // For now just a 100% reflective diffuse BRDF
        float brdf = 1.0f / M_PI;

        // For now just iterate over all lights
        /// XXX: simple uniform sampling may be better here
        for (auto const& light: _lights)
        {
            // Skip light if it's hidden
            if (!light.visible) 
            {
                continue;
            }

            // Sample the light
            LightSample ls;
            switch (light.kind)
            {
            case LightKind::Distant:
                ls = SampleDistantLight(light, hitPos, pcg.uniform(), pcg.uniform());
                break;
            case LightKind::Dome:
                ls = SampleDomeLight(light, normal, pcg.uniform(), pcg.uniform());
                break;
            case LightKind::Rect:
            case LightKind::Sphere:
            case LightKind::Disk:
            case LightKind::Cylinder:
                ls = SampleAreaLight(light, hitPos, pcg.uniform(), pcg.uniform());
                break;
            }

            // Trace shadow
            float vis = _Visibility(hitPos, ls.wI, ls.dist * 0.99f);

            // Add exitant luminance
            finalColor += ls.Li 
                * PosDot(ls.wI, normal) 
                * brdf 
                * vis 
                * ls.invPdfW;
        }
    }

    // Clamp colors to > 0
    GfVec4f output;
    output[0] = std::max(0.0f, finalColor[0]);
    output[1] = std::max(0.0f, finalColor[1]);
    output[2] = std::max(0.0f, finalColor[2]);
    output[3] = 1.0f;
    return output;
}

float
HdEmbreeRenderer::_ComputeAmbientOcclusion(GfVec3f const& position,
                                           GfVec3f const& normal,
                                           PCG& pcg)
{
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
    // for (int i = 0; i < _ambientOcclusionSamples; ++i) {
    //     samples[i][0] = (float(i) + pcg.uniform()) / _ambientOcclusionSamples;
    // }
    // std::shuffle(samples.begin(), samples.end(), random);
    // for (int i = 0; i < _ambientOcclusionSamples; ++i) {
    //     samples[i][1] = (float(i) + pcg.uniform()) / _ambientOcclusionSamples;
    // }

    /// XXX: Do stratified here
    for (int i = 0; i < _ambientOcclusionSamples; ++i) {
        samples[i][0] = pcg.uniform();
        samples[i][1] = pcg.uniform();
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
        shadow.flags = 0;
        _PopulateRay(&shadow, position, shadowDir, 0.001f);
        {
          RTCIntersectContext context;
          rtcInitIntersectContext(&context);
          rtcOccluded1(_scene,&context,&shadow);
        }

        // Record this AO ray's contribution to the occlusion factor: a
        // boolean [In shadow/Not in shadow].
        // shadow is occluded when shadow.ray.tfar < 0.0f
        // notice this is reversed since "it's a visibility ray, and
        // the occlusionFactor is really an ambientLightFactor."
        if (shadow.tfar > 0.0f)
            occlusionFactor += GfDot(shadowDir, normal);
    }
    // Compute the average of the occlusion samples.
    occlusionFactor /= _ambientOcclusionSamples;

    return occlusionFactor;
}

PXR_NAMESPACE_CLOSE_SCOPE
