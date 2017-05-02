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
#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/info.h"

#include "pxr/imaging/hdx/renderSetupTask.h"

#include "pxr/base/tf/stl.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

PXR_NAMESPACE_OPEN_SCOPE



struct _HitData {
    int xMin;
    int yMin;
    double zMin;
    int minIndex;
};
typedef TfHashMap< int32_t, _HitData > _HitDataById;


UsdImagingGLEngine::~UsdImagingGLEngine() { /*nothing*/ }

/*virtual*/
void 
UsdImagingGLEngine::SetCameraState(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projectionMatrix,
                            const GfVec4d& viewport)
{
    // By default, do nothing.
}

void
UsdImagingGLEngine::SetCameraStateFromOpenGL()
{
    GfMatrix4d viewMatrix, projectionMatrix;
    GfVec4d viewport;
    glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix.GetArray());
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix.GetArray());
    glGetDoublev(GL_VIEWPORT, &viewport[0]);

    SetCameraState(viewMatrix, projectionMatrix, viewport);
}

/* virtual */
void
UsdImagingGLEngine::SetLightingStateFromOpenGL()
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::SetLightingState(GlfSimpleLightVector const &lights,
                                     GlfSimpleMaterial const &material,
                                     GfVec4f const &sceneAmbient)
{
    // By default, do nothing.
}


/* virtual */
void
UsdImagingGLEngine::SetRootTransform(GfMatrix4d const& xf)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::SetRootVisibility(bool isVisible)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::SetSelected(SdfPathVector const& paths)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::ClearSelected()
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::AddSelected(SdfPath const &path, int instanceIndex)
{
    // By default, do nothing.
}

/*virtual*/
void
UsdImagingGLEngine::SetSelectionColor(GfVec4f const& color)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::PrepareBatch(const UsdPrim& root, RenderParams params)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::RenderBatch(const SdfPathVector& paths, RenderParams params)
{
    // By default, do nothing.
}

bool
UsdImagingGLEngine::TestIntersection(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const UsdPrim& root, 
    RenderParams params,
    GfVec3d *outHitPoint,
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex,
    int *outHitElementIndex)
{
    // Choose a framebuffer that's large enough to catch thin slice polys.  No
    // need to go too large though, since the depth writes will accumulate to
    // the correct answer.

    // XXX:
    // TidSceneRenderer exposes this as an environment variable; do we care?
    // Have not yet encountered cases where more accuracy is required.
    const int width = 128;
    const int height = width;

    if (GlfHasLegacyGraphics()) {
        TF_RUNTIME_ERROR("framebuffer object not supported");
        return false;
    }

    // Use a separate drawTarget (framebuffer object) for each GL context
    // that uses this renderer, but the drawTargets can share attachments.
    
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (!TF_VERIFY(context)) {
        TF_RUNTIME_ERROR("Invalid GL context");
        return false;
    }

    GfVec2i attachmentSize(width,height);
    GlfDrawTargetRefPtr drawTarget;
    if (!TfMapLookup(_drawTargets, context, &drawTarget)) {

        // Create an instance for use with this GL context
        drawTarget = GlfDrawTarget::New(attachmentSize);

        if (!_drawTargets.empty()) {
            // Share existing attachments
            drawTarget->Bind();
            drawTarget->CloneAttachments(_drawTargets.begin()->second);
            drawTarget->Unbind();
        } else {
            // Need to create initial attachments
            drawTarget->Bind();
            drawTarget->AddAttachment(
                "primId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
            drawTarget->AddAttachment(
                "instanceId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
            drawTarget->AddAttachment(
                "elementId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
            drawTarget->AddAttachment(
                "depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);
            drawTarget->Unbind();
        }

        // This is a good time to clean up any drawTargets no longer in use.
        for (_DrawTargetPerContextMap::iterator
                it = _drawTargets.begin(); it != _drawTargets.end(); ++it) {
            if (!(it->first && it->first->IsValid())) {
                _drawTargets.erase(it);
            }
        }

        _drawTargets[context] = drawTarget;
    }

    // Resize if necessary
    if (drawTarget->GetSize() != attachmentSize) {
        drawTarget->SetSize(attachmentSize);
    }

    drawTarget->Bind();

    glPushAttrib( GL_VIEWPORT_BIT |
                  GL_ENABLE_BIT |
                  GL_COLOR_BUFFER_BIT |
                  GL_DEPTH_BUFFER_BIT |
                  GL_TEXTURE_BIT );

    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);
    
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    // Setup the modelview matrix
    const GfMatrix4d modelViewMatrix = worldToLocalSpace * viewMatrix;

    // Set up camera matrices and viewport. At some point in the future,
    // this may be handled by Hydra itself since we are calling SetCameraState
    // with all of this information so we can support culling
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(projectionMatrix.GetArray());
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(modelViewMatrix.GetArray());
   
    glViewport(0, 0, width, height);

    SetCameraState(modelViewMatrix, projectionMatrix, GfVec4d(0,0,width, height) );

    GLF_POST_PENDING_GL_ERRORS();
    
    // to enable wireframe picking, should respect incoming drawMode
    //params.drawMode = DRAW_GEOM_ONLY;
    Render(root, params);
    
    GLF_POST_PENDING_GL_ERRORS();

    // Restore all gl state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    int xMin = 0;
    int yMin = 0;
    double zMin = 1.0;
    int zMinIndex = -1;

    GLubyte primId[width*height*4];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("primId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, primId);

    GLubyte instanceId[width*height*4];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("instanceId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, instanceId);

    GLubyte elementId[width*height*4];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("elementId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, elementId);

    GLfloat depths[width*height];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("depth")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depths);

    glPopAttrib(); /* GL_VIEWPORT_BIT |
                      GL_ENABLE_BIT |
                      GL_COLOR_BUFFER_BIT
                      GL_DEPTH_BUFFER_BIT
                      GL_TEXTURE_BIT */

    GLF_POST_PENDING_GL_ERRORS();
    
    // Find the smallest value (nearest pixel) in the z buffer
    for (int y=0, i=0; y<height; y++) {
        for (int x=0; x<width; x++, i++) {
            if (depths[i] < zMin) {
	            xMin = x;
	            yMin = y;
	            zMin = depths[i];
                    zMinIndex = i;
            }
        }
    }

    bool didHit = (zMin < 1.0);

    if (didHit) {
        GLint viewport[4] = { 0, 0, width, height };
        GfVec3d hitPoint;

        gluUnProject( xMin, yMin, zMin,
                      viewMatrix.GetArray(),
                      projectionMatrix.GetArray(),
                      viewport,
                      &((*outHitPoint)[0]),
                      &((*outHitPoint)[1]),
                      &((*outHitPoint)[2]));

        if (outHitPrimPath) {
            int idIndex = zMinIndex*4;

            *outHitPrimPath = GetRprimPathFromPrimId(
                    HdxRenderSetupTask::DecodeIDRenderColor(&primId[idIndex]));
            if (outHitInstanceIndex) {
                *outHitInstanceIndex = HdxRenderSetupTask::DecodeIDRenderColor(
                        &instanceId[idIndex]);
            }
            if (outHitElementIndex) {
                *outHitElementIndex = HdxRenderSetupTask::DecodeIDRenderColor(
                        &elementId[idIndex]);
            }

        }
    }

    drawTarget->Unbind();
    GLF_POST_PENDING_GL_ERRORS();

    return didHit;
}

static
uint32_t
_pow2roundup (uint32_t x)
{
    // Round up to next higher power of 2 (return x if it's already a power
    // of 2).
    
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}

bool
UsdImagingGLEngine::TestIntersectionBatch(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const SdfPathVector& paths, 
    RenderParams params,
    unsigned int pickResolution,
    PathTranslatorCallback pathTranslator,
    HitBatch *outHit)
{
    // outHit is not optional
    if (!outHit) {
        return false;
    }
    
    // Choose a framebuffer that's large enough to catch thin slice polys.  No
    // need to go too large though, since the depth writes will accumulate to
    // the correct answer.
    //
    // The incoming pickResolution may not be a power of two, so round up to the
    // nearest fully-support resolution.
    //
    const int width = _pow2roundup(pickResolution);
    const int height = width;

    if (GlfHasLegacyGraphics()) {
        TF_RUNTIME_ERROR("framebuffer object not supported");
        return false;
    }

    // Use a separate drawTarget (framebuffer object) for each GL context
    // that uses this renderer, but the drawTargets can share attachments.
    
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (!TF_VERIFY(context)) {
        TF_RUNTIME_ERROR("Invalid GL context");
        return false;
    }

    GfVec2i attachmentSize(width,height);
    GlfDrawTargetRefPtr drawTarget;
    if (!TfMapLookup(_drawTargets, context, &drawTarget)) {

        // Create an instance for use with this GL context
        drawTarget = GlfDrawTarget::New(attachmentSize);

        if (!_drawTargets.empty()) {
            // Share existing attachments
            drawTarget->Bind();
            drawTarget->CloneAttachments(_drawTargets.begin()->second);
            drawTarget->Unbind();
        } else {
            // Need to create initial attachments
            drawTarget->Bind();
            drawTarget->AddAttachment(
                "primId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
            drawTarget->AddAttachment(
                "instanceId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
            drawTarget->AddAttachment(
                "depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);
            drawTarget->Unbind();
        }

        // This is a good time to clean up any drawTargets no longer in use.
        for (_DrawTargetPerContextMap::iterator
                it = _drawTargets.begin(); it != _drawTargets.end(); ++it) {
            if (!(it->first && it->first->IsValid())) {
                _drawTargets.erase(it);
            }
        }

        _drawTargets[context] = drawTarget;
    }

    // Resize if necessary
    if (drawTarget->GetSize() != attachmentSize) {
        drawTarget->SetSize(attachmentSize);
    }

    drawTarget->Bind();

    glPushAttrib( GL_VIEWPORT_BIT |
                  GL_ENABLE_BIT |
                  GL_COLOR_BUFFER_BIT |
                  GL_DEPTH_BUFFER_BIT |
                  GL_TEXTURE_BIT );

    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);
    
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    // Setup the modelview matrix
    const GfMatrix4d modelViewMatrix = worldToLocalSpace * viewMatrix;

    // Set up camera matrices and viewport. At some point in the future,
    // this may be handled by Hydra itself since we are calling SetCameraState
    // with all of this information so we can support culling
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(projectionMatrix.GetArray());
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(modelViewMatrix.GetArray());
   
    glViewport(0, 0, width, height);

    SetCameraState(modelViewMatrix, projectionMatrix, GfVec4d(0,0,width, height) );

    GLF_POST_PENDING_GL_ERRORS();
    
    // to enable wireframe picking, should respect incoming drawMode
    //params.drawMode = DRAW_GEOM_ONLY;
    RenderBatch(paths, params);
    
    GLF_POST_PENDING_GL_ERRORS();

    // Restore all gl state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    std::vector<GLubyte> primId(width*height*4);
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("primId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, primId.data());

    std::vector<GLubyte> instanceId(width*height*4);
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("instanceId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, instanceId.data());

    std::vector<GLfloat> depths(width*height);
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("depth")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depths.data());

    glPopAttrib(); /* GL_VIEWPORT_BIT |
                      GL_ENABLE_BIT |
                      GL_COLOR_BUFFER_BIT
                      GL_DEPTH_BUFFER_BIT
                      GL_TEXTURE_BIT */

    GLF_POST_PENDING_GL_ERRORS();

    _HitDataById hitResults;
    
    // Find the smallest value (nearest pixel) in the z buffer for each primId
    for (int y=0, i=0; y<height; y++) {
        for (int x=0; x<width; x++, i++) {
            if( depths[i]>=1.0 )
                continue;
            
            // primIdx construction mirrors the underlying prim code,
            // ignoring the A component.
            int32_t primIdx = ((primId[i*4+0] & 0xff) <<  0) | 
                              ((primId[i*4+1] & 0xff) <<  8) |
                              ((primId[i*4+2] & 0xff) << 16);
            
            // Set the iterator to the entry if it exists in hitResults,
            // otherwise insert a new entry with the default values.
            std::pair< _HitDataById::iterator, bool > primEntry =
                    hitResults.insert( {primIdx, {0,0,1.0,-1}} );
            
            _HitData &primHitData = primEntry.first->second;
            if (depths[i] < primHitData.zMin) {
	            primHitData.xMin = x;
	            primHitData.yMin = y;
	            primHitData.zMin = depths[i];
                    primHitData.minIndex = i;
            }
        }
    }

    bool didHit = !hitResults.empty();
    
    TfHashMap<SdfPath,double,SdfPath::Hash> minDistToPath;

    if (didHit) {
        GLint viewport[4] = { 0, 0, width, height };
        
        TF_FOR_ALL( hitEntry, hitResults ) {
            _HitData &primHitData = hitEntry->second;

            int idIndex = primHitData.minIndex*4;

            GfVec4i primIdColor(
                primId[idIndex+0],
                primId[idIndex+1],
                primId[idIndex+2],
                primId[idIndex+3]);

            GfVec4i instanceIdColor(
                instanceId[idIndex+0],
                instanceId[idIndex+1],
                instanceId[idIndex+2],
                instanceId[idIndex+3]);
            
            int hitInstanceIndex;
            SdfPath primPath = GetPrimPathFromPrimIdColor(primIdColor,
                                                          instanceIdColor,
                                                          &hitInstanceIndex);

            // Translate the path. Allows client-side collating of hit prims into
            // useful bins as needed. The simplest translator returns primPath.
            //
            // Note that this non-Hydra implementation has no concept of an
            // instancer path.
            SdfPath hitPath( pathTranslator(primPath, SdfPath(), hitInstanceIndex) );
            
            if( !hitPath.IsEmpty() ) {
                
                double minDist;
                bool exists = TfMapLookup( minDistToPath, hitPath, &minDist );
                if( !exists || primHitData.zMin < minDist ) {

                    GfVec3d hitPoint;
                    gluUnProject( primHitData.xMin, primHitData.yMin, primHitData.zMin,
                                  viewMatrix.GetArray(),
                                  projectionMatrix.GetArray(),
                                  viewport,
                                  &(hitPoint[0]),
                                  &(hitPoint[1]),
                                  &(hitPoint[2]));

                    HitInfo &hitInfo = (*outHit)[hitPath];

                    hitInfo.worldSpaceHitPoint = hitPoint;
                    hitInfo.hitInstanceIndex = hitInstanceIndex;

                    minDistToPath[hitPath] = primHitData.zMin;
                }
            }
        }
    }

    drawTarget->Unbind();
    GLF_POST_PENDING_GL_ERRORS();

    return didHit;
}

/* virtual */
SdfPath
UsdImagingGLEngine::GetRprimPathFromPrimId(int primId) const
{
    return SdfPath();
}

/* virtual */
SdfPath
UsdImagingGLEngine::GetPrimPathFromPrimIdColor(GfVec4i const &primIdColor,
                                             GfVec4i const &instanceIdColor,
                                             int * instanceIndexOut)
{
    unsigned char primIdColorBytes[] =  {
        uint8_t(primIdColor[0]),
        uint8_t(primIdColor[1]),
        uint8_t(primIdColor[2]),
        uint8_t(primIdColor[3])
    };

    int primId = HdxRenderSetupTask::DecodeIDRenderColor(primIdColorBytes);
    SdfPath result = GetRprimPathFromPrimId(primId);
    if (!result.IsEmpty()) {
        if (instanceIndexOut) {
            unsigned char instanceIdColorBytes[] =  {
                uint8_t(instanceIdColor[0]),
                uint8_t(instanceIdColor[1]),
                uint8_t(instanceIdColor[2]),
                uint8_t(instanceIdColor[3])
            };
            *instanceIndexOut = HdxRenderSetupTask::DecodeIDRenderColor(
                    instanceIdColorBytes);
        }
    }
    return result;
}

/* virtual */
SdfPath 
UsdImagingGLEngine::GetPrimPathFromInstanceIndex(
    SdfPath const& protoPrimPath,
    int instanceIndex,
    int *absoluteInstanceIndex,
    SdfPath *rprimPath,
    SdfPathVector *instanceContext)
{
    return SdfPath();
}

/* virtual */
bool
UsdImagingGLEngine::IsConverged() const
{
    // always converges by default.
    return true;
}

/* virtual */
std::vector<TfType>
UsdImagingGLEngine::GetRendererPlugins()
{
    return std::vector<TfType>();
}

/* virtual */
bool
UsdImagingGLEngine::SetRendererPlugin(TfType const &type)
{
    return false;
}

/* virtual */
VtDictionary
UsdImagingGLEngine::GetResourceAllocation() const
{
    return VtDictionary();
}


PXR_NAMESPACE_CLOSE_SCOPE

