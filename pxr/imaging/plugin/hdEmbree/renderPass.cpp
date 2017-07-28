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

#include "pxr/imaging/hdEmbree/renderPass.h"

#include "pxr/imaging/hdEmbree/config.h"
#include "pxr/imaging/hdEmbree/context.h"
#include "pxr/imaging/hdEmbree/mesh.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"

#include <embree2/rtcore_ray.h>
#include <random>

PXR_NAMESPACE_OPEN_SCOPE

HdEmbreeRenderPass::HdEmbreeRenderPass(HdRenderIndex *index,
                                       HdRprimCollection const &collection,
                                       RTCScene scene)
    : HdRenderPass(index, collection)
    , _pendingResetImage(false)
    , _width(0)
    , _height(0)
    , _scene(scene)
    , _inverseViewMatrix(1.0f) // == identity
    , _inverseProjMatrix(1.0f) // == identity
    , _clearColor(0.0707f, 0.0707f, 0.0707f)
{
}

HdEmbreeRenderPass::~HdEmbreeRenderPass()
{
}

void
HdEmbreeRenderPass::ResetImage()
{
    // Set a flag to clear the sample buffer the next time Execute() is called.
    _pendingResetImage = true;
}

bool
HdEmbreeRenderPass::IsConverged() const
{
    // A super simple heuristic: consider ourselves converged after N
    // samples. Since we currently uniformly sample the framebuffer, we can
    // use the sample count from pixel(0,0).
    unsigned int samplesToConvergence =
        HdEmbreeConfig::GetInstance().samplesToConvergence;
    return (_sampleBuffer[3] >= samplesToConvergence);
}

void
HdEmbreeRenderPass::_MarkCollectionDirty()
{
    // If the drawable collection changes, we should reset the sample buffer.
    _pendingResetImage = true;
}

void
HdEmbreeRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState,
                             TfTokenVector const &renderTags)
{
    // XXX: Add collection and renderTags support.
    // XXX: Add clip planes support.

    _inverseViewMatrix = renderPassState->GetWorldToViewMatrix().GetInverse();
    _inverseProjMatrix = renderPassState->GetProjectionMatrix().GetInverse();

    // If the viewport has changed, resize the sample buffer.
    GfVec4f vp = renderPassState->GetViewport();
    if (_width != vp[2] || _height != vp[3]) {
        _width = vp[2];
        _height = vp[3];
        _sampleBuffer.resize(_width*_height*4);
        _colorBuffer.resize(_width*_height*4);
        _pendingResetImage = true;
    }

    // Reset the sample buffer if it's been requested.
    if (_pendingResetImage) {
        memset(&_sampleBuffer[0], 0, _width*_height*4*sizeof(float));
        _pendingResetImage = false;
    }
    
    // Render the image. Each call to _Render() adds a sample per pixel (with
    // jittered ray direction); we can optionally render multiple samples per
    // frame, or render the same image cross multiple frames (progressive
    // rendering), to produce a more accurate and less noisy image.
    unsigned int samplesPerFrame =
        HdEmbreeConfig::GetInstance().samplesPerFrame;
    for (unsigned int i = 0; i < samplesPerFrame; ++i) {
        _Render();
    }

    // Resolve the image buffer: find the average color per pixel by
    // dividing the summed color by the number of samples;
    // and convert the image into a GL-compatible format.
    for (unsigned int i = 0; i < _width * _height; ++i) {
        float r = 1.0f/_sampleBuffer[i*4+3];
        _colorBuffer[i*4+0] = (uint8_t)(255.0f*_sampleBuffer[i*4+0]*r);
        _colorBuffer[i*4+1] = (uint8_t)(255.0f*_sampleBuffer[i*4+1]*r);
        _colorBuffer[i*4+2] = (uint8_t)(255.0f*_sampleBuffer[i*4+2]*r);
        _colorBuffer[i*4+3] = 255;
    }

    // Blit!
    glDrawPixels(_width, _height, GL_RGBA, GL_UNSIGNED_BYTE, &_colorBuffer[0]);
}

void
HdEmbreeRenderPass::_Render()
{
    unsigned int tileSize = HdEmbreeConfig::GetInstance().tileSize;
    const unsigned int numTilesX = (_width + tileSize-1) / tileSize;
    const unsigned int numTilesY = (_height + tileSize-1) / tileSize;

    // Render by scheduling square tiles of the sample buffer in a parallel
    // for loop.
    WorkParallelForN(numTilesX*numTilesY,
        std::bind(&HdEmbreeRenderPass::_RenderTiles, this,
                  std::placeholders::_1, std::placeholders::_2));
}

void
HdEmbreeRenderPass::_RenderTiles(size_t tileStart, size_t tileEnd)
{
    unsigned int tileSize =
        HdEmbreeConfig::GetInstance().tileSize;
    const unsigned int numTilesX = (_width + tileSize-1) / tileSize;

    // _RenderTiles gets a range of tiles; iterate through them.
    for (unsigned int tile = tileStart; tile < tileEnd; ++tile) {

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

                // Initialize the RNG with a fixed seed or with entropy,
                // depending on environment settings.
                std::default_random_engine random(0x12345678);
                if (!HdEmbreeConfig::GetInstance().fixRandomSeed) {
                    random.seed(std::chrono::system_clock::now().
                        time_since_epoch().count());
                }

                // Jitter the camera ray direction.
                std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
                GfVec2f jitter(uniform(random), uniform(random));

                // Un-transform the pixel's NDC coordinates through the
                // projection matrix to get the trace of the camera ray in the
                // near plane.
                GfVec3f ndc = GfVec3f(2 * ((x + jitter[0]) / _width) - 1,
                                      2 * ((y + jitter[1]) / _height) - 1,
                                      -1);
                GfVec3f nearPlaneTrace = _inverseProjMatrix.Transform(ndc);

                GfVec3f origin, dir;
                if (fabsf(nearPlaneTrace[2]) < 0.0001f) {
                    // If the near plane is co-planar with the camera origin,
                    // assume we're doing an orthographic projection: trace
                    // parallel rays from the near plane trace.
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

                // Trace the ray to get pixel color.
                GfVec3f color = _TraceRay(origin, dir);

                // Add the pixel sample to the sample buffer.
                int idx = y*_width+x;
                _sampleBuffer[idx*4+0] += color[0];
                _sampleBuffer[idx*4+1] += color[1];
                _sampleBuffer[idx*4+2] += color[2];
                _sampleBuffer[idx*4+3] += 1.0f;
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

/// Generate a uniformly random direction ray.
template <typename T>
static GfVec3f
_RandomDirection(T& random_engine)
{
    GfVec3f dir;
    std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
    float theta = 2.0f * M_PI * uniform(random_engine);
    float phi = acosf(2.0f * uniform(random_engine) - 1.0f);
    float sinphi = sinf(phi);
    dir[0] = cosf(theta) * sinphi;
    dir[1] = sinf(theta) * sinphi;
    dir[2] = cosf(phi);
    return dir;
}


GfVec3f
HdEmbreeRenderPass::_TraceRay(GfVec3f const &origin, GfVec3f const &dir)
{
    // Intersect the camera ray.
    RTCRay ray;
    _PopulateRay(&ray, origin, dir, 0.0f);
    rtcIntersect(_scene, ray);

    if (ray.geomID == RTC_INVALID_GEOMETRY_ID) {
        // Ray miss gets the clear color.
        return _clearColor;
    } else {
        
        // Get the instance and prototype context structures for the hit prim.
        HdEmbreeInstanceContext *instanceContext =
            static_cast<HdEmbreeInstanceContext*>(
                rtcGetUserData(_scene, ray.instID));

        HdEmbreePrototypeContext *prototypeContext =
            static_cast<HdEmbreePrototypeContext*>(
                rtcGetUserData(instanceContext->rootScene, ray.geomID));

        // Compute the worldspace location of the ray hit.
        GfVec3f hitPos = GfVec3f(ray.org[0] + ray.tfar * ray.dir[0],
                                 ray.org[1] + ray.tfar * ray.dir[1],
                                 ray.org[2] + ray.tfar * ray.dir[2]);

        // If a normal primvar is present (e.g. from smooth shading), use that
        // for shading; otherwise use the flat face normal.
        GfVec3f normal = -GfVec3f(ray.Ng[0], ray.Ng[1], ray.Ng[2]);
        if (prototypeContext->primvarMap.count(HdTokens->normals) > 0) {
            prototypeContext->primvarMap[HdTokens->normals]->Sample(
                ray.primID, ray.u, ray.v, &normal);
        }

        // If a color primvar is present, use that as diffuse color; otherwise,
        // use flat white.
        GfVec4f color = GfVec4f(1.0f, 1.0f, 1.0f, 1.0f);
        if (HdEmbreeConfig::GetInstance().useFaceColors &&
            prototypeContext->primvarMap.count(HdTokens->color) > 0) {
            prototypeContext->primvarMap[HdTokens->color]->Sample(
                ray.primID, ray.u, ray.v, &color);
        }

        // Transform the normal from object space to world space.
        GfVec4f expandedNormal(normal[0], normal[1], normal[2], 0.0f);
        expandedNormal = expandedNormal * instanceContext->objectToWorldMatrix;
        normal = GfVec3f(expandedNormal[0], expandedNormal[1],
            expandedNormal[2]);

        // Make sure the normal is unit-length.
        normal.Normalize();

        // Lighting model: (camera dot normal), i.e. diffuse-only point light
        // centered on the camera.
        float diffuseLight = fabs(GfDot(-dir, normal)) *
            HdEmbreeConfig::GetInstance().cameraLightIntensity;

        // Lighting gets modulated by an ambient occlusion term.
        float aoLightIntensity =
            (1.0f - _ComputeAmbientOcclusion(hitPos, normal));
            
        // Return color.xyz * diffuseLight * aoLightIntensity.
        // XXX: Transparency?
        GfVec3f finalColor = GfVec3f(color[0], color[1], color[2]) *
            diffuseLight * aoLightIntensity;

        // Clamp colors to [0,1].
        finalColor[0] = std::max(0.0f, std::min(1.0f, finalColor[0]));
        finalColor[1] = std::max(0.0f, std::min(1.0f, finalColor[1]));
        finalColor[2] = std::max(0.0f, std::min(1.0f, finalColor[2]));
        return finalColor;
    }
}

float
HdEmbreeRenderPass::_ComputeAmbientOcclusion(GfVec3f const& position,
                                             GfVec3f const& normal)
{
    // 0 ambient occlusion samples means disable the ambient occlusion term.
    unsigned int ambientOcclusionSamples =
        HdEmbreeConfig::GetInstance().ambientOcclusionSamples;
    if (ambientOcclusionSamples < 1) {
        return 0.0f;
    }

    // Initialize the RNG with a fixed seed or with entropy, depending on
    // environment settings.
    std::default_random_engine random(0x12345678);
    if (!HdEmbreeConfig::GetInstance().fixRandomSeed) {
        random.seed(std::chrono::system_clock::now().
            time_since_epoch().count());
    }

    float occlusionFactor = 0.0f;

    // Trace ambient occlusion rays. The occlusion factor is the fraction of
    // the hemisphere that's occluded when rays are traced to infinity,
    // computed by uniform random sampling over the hemisphere.
    for (unsigned int i = 0; i < ambientOcclusionSamples; i++)
    {
        // We sample in the hemisphere centered on the face normal.
        GfVec3f shadowDir = _RandomDirection(random);
        if (GfDot(shadowDir, normal) < 0) shadowDir = -shadowDir;

        // Trace shadow ray, using the fast interface (rtcOccluded) since
        // we only care about intersection status, not intersection id.
        RTCRay shadow;
        _PopulateRay(&shadow, position, shadowDir, 0.001f);
        rtcOccluded(_scene, shadow);

        // Record this AO ray's contribution to the occlusion factor: a
        // boolean [In shadow/Not in shadow].
        if (shadow.geomID != RTC_INVALID_GEOMETRY_ID)
            occlusionFactor += 1.0f;
    }
    // Compute the average of the occlusion samples.
    occlusionFactor /= ambientOcclusionSamples;

    return occlusionFactor;
}

PXR_NAMESPACE_CLOSE_SCOPE
