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
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/debugCodes.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/rprim.h"

#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/info.h"

#include <iostream>

HdxIntersector::HdxIntersector(HdRenderIndexSharedPtr index)
    : _index(index)
{ 
}

void
HdxIntersector::_Init(GfVec2i const& size)
{
    HdRprimCollection col(HdTokens->geometry, HdTokens->hull);
    _renderPass = boost::make_shared<HdRenderPass>(&*_index, col);
    // initialize renderPassState with ID render shader
    _renderPassState = boost::make_shared<HdRenderPassState>(
        boost::make_shared<HdRenderPassShader>(HdxPackageRenderPassIdShader()));


    // Make sure master draw target is always modified on the shared context,
    // so we access it consistently.
    GlfSharedGLContextScopeHolder sharedContextHolder;
    {
        // TODO: determine this size from the incoming projection, we need two
        // different sizes, one for ray picking and one for marquee picking. we
        // could perhaps just use the large size for both.
        _drawTarget = GlfDrawTarget::New(size);

        // Future work: these attachments must match
        // hd/shaders/renderPassShader.glslfx, which is a point of fragility.
        // Ideally, we would declare the output targets here and specify an overlay
        // shader that would direct the output to those targets.  In that world, Hd
        // would know nothing about these outputs, making this code more robust in
        // the face of future changes.

        _drawTarget->Bind();

        // Create initial attachments
        _drawTarget->AddAttachment(
            "primId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
        _drawTarget->AddAttachment(
            "instanceId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
        _drawTarget->AddAttachment(
            "elementId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
        _drawTarget->AddAttachment(
            "depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH24_STENCIL8);
            //"depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);

        _drawTarget->Unbind();
    }
}

void HdxNoDepthMask()
{
    // The depth mask is expected to render something, which is used to
    // condition the stencil buffer -- whatever gets rendered is available for
    // picking. Here, we want everything to be available for picking, so we
    // could render a full-screen primitive, but clearing the stencil buffer is
    // much simpler.
    
    // Clear the stencil buffer to 1.0 to enable all following stencil tests to
    // pass.
    glClearStencil(1);
    glClear(GL_STENCIL_BUFFER_BIT);
    glClearStencil(0);
}

void
HdxIntersector::SetResolution(GfVec2i const& widthHeight)
{
    TRACE_FUNCTION();

    // Make sure we're in a sane GL state before attempting anything.
    if (GlfHasLegacyGraphics()) {
        TF_RUNTIME_ERROR("framebuffer object not supported");
        return;
    }

    if (not _drawTarget) {
        // Initialize the shared draw target late to ensure there is a valid GL
        // context, which may not be the case at constructon time.
        _Init(widthHeight);
        return;
    }

    if (widthHeight == _drawTarget->GetSize()){
        return;
    }

    // Make sure master draw target is always modified on the shared context,
    // so we access it consistently.
    GlfSharedGLContextScopeHolder sharedContextHolder;
    {
        _drawTarget->Bind();
        _drawTarget->SetSize(widthHeight);
        _drawTarget->Unbind();
    }
}

bool
HdxIntersector::Query(HdxIntersector::Params const& params,
                      HdRprimCollection const& col,
                      HdEngine* engine,
                      HdxIntersector::Result* result)
{
    TRACE_FUNCTION();

    // Make sure we're in a sane GL state before attempting anything.
    if (GlfHasLegacyGraphics()) {
        TF_RUNTIME_ERROR("framebuffer object not supported");
        return false;
    }
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (not TF_VERIFY(context)) {
        TF_RUNTIME_ERROR("Invalid GL context");
        return false;
    }
    if (not _drawTarget) {
        // Initialize the shared draw target late to ensure there is a valid GL
        // context, which may not be the case at constructon time.
        _Init(GfVec2i(128,128));
    }

    GfVec2i size(_drawTarget->GetSize());
    GfVec4i viewport(0, 0, size[0], size[1]);

    if (not TF_VERIFY(_renderPass)) {
        return false;
    }
    _renderPass->SetRprimCollection(col);

    // Setup state based on incoming params.
    _renderPassState->SetAlphaThreshold(params.alphaThreshold);
    _renderPassState->SetClipPlanes(params.clipPlanes);
    _renderPassState->SetCullStyle(params.cullStyle);
    _renderPassState->SetCamera(params.viewMatrix, params.projectionMatrix, viewport);
    _renderPassState->SetLightingEnabled(false);

    // Use a separate drawTarget (framebuffer object) for each GL context
    // that uses this renderer, but the drawTargets share attachments/textures.
    GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(size);

    // Clone attachments into this context. Note that this will do a
    // light-weight copy of the textures, it does not produce a full copy of the
    // underlying images.
    drawTarget->Bind();
    drawTarget->CloneAttachments(_drawTarget);

    //
    // Setup GL raster state
    //
    glPushAttrib(GL_VIEWPORT_BIT |
                 GL_ENABLE_BIT |
                 GL_COLOR_BUFFER_BIT |
                 GL_DEPTH_BUFFER_BIT |
                 GL_STENCIL_BUFFER_BIT |
                 GL_TEXTURE_BIT |
                 GL_POLYGON_BIT);

    GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0,
                              GL_COLOR_ATTACHMENT1,
                              GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, drawBuffers);
    
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);  

    glClearColor(0,0,0,0);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    GLF_POST_PENDING_GL_ERRORS();
    
    //
    // Execute the picking pass
    //
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        // Setup stencil state and prevent writes to color buffer.
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 1);
        glStencilOp(GL_KEEP,     // stencil failed
                    GL_KEEP,     // stencil passed, depth failed
                    GL_REPLACE); // stencil passed, depth passed
        // Attempt to protect from GL state corruption.
        glPushAttrib(GL_VIEWPORT_BIT |
                     GL_ENABLE_BIT |
                     GL_COLOR_BUFFER_BIT |
                     GL_DEPTH_BUFFER_BIT |
                     GL_STENCIL_BUFFER_BIT |
                     GL_TEXTURE_BIT |
                     GL_POLYGON_BIT);

        //
        // Condition the stencil buffer.
        //
        params.depthMaskCallback();

        glPopAttrib();
        // Disable stencil updates and setup the stencil test.
        glStencilFunc(GL_LESS, 0, 1);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        // Clear depth incase the depthMaskCallback pollutes the depth buffer.
        glClear(GL_DEPTH_BUFFER_BIT);
        // Restore color outputs & setup state for rendering
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glFrontFace(GL_CCW);

        //
        // Enable conservative rasterization, if available.
        //
        // XXX: This wont work until it's in the Glew build.
        bool convRstr = glewIsSupported("GL_NV_conservative_raster");
        if (convRstr) {
            // XXX: this should come from Glew
            #define GL_CONSERVATIVE_RASTERIZATION_NV 0x9346
            glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
        }

        // 
        // Execute
        //
        // XXX: make intersector a Task
        engine->Draw(*_index, _renderPass, _renderPassState);

        // Restore
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao);
    }

    GLF_POST_PENDING_GL_ERRORS();

    //
    // Capture the result buffers to be resolved later.
    //
    size_t len = size[0] * size[1];
    std::unique_ptr<unsigned char[]> primId(new unsigned char[len*4]);
    std::unique_ptr<unsigned char[]> instanceId(new unsigned char[len*4]);
    std::unique_ptr<unsigned char[]> elementId(new unsigned char[len*4]);
    std::unique_ptr<float[]> depths(new float[len]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("primId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &primId[0]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("instanceId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &instanceId[0]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("elementId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &elementId[0]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("depth")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                    &depths[0]);

    GLF_POST_PENDING_GL_ERRORS();

    if (result) {
        *result = HdxIntersector::Result(
            std::move(primId), std::move(instanceId), std::move(elementId),
            std::move(depths), _index, params, viewport);
    }

    //
    // Restore all modified GL state
    //
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopAttrib(); /* GL_VIEWPORT_BIT |
                      GL_ENABLE_BIT |
                      GL_COLOR_BUFFER_BIT
                      GL_DEPTH_BUFFER_BIT
                      GL_TEXTURE_BIT */
    drawTarget->Unbind();
    GLF_POST_PENDING_GL_ERRORS();

    return true;
}

HdxIntersector::Result::Result()
    : _index(nullptr)
    , _viewport(0,0,0,0)
{
}

HdxIntersector::Result::Result(std::unique_ptr<unsigned char[]> primIds,
                        std::unique_ptr<unsigned char[]> instanceIds,
                        std::unique_ptr<unsigned char[]> elementIds,
                        std::unique_ptr<float[]> depths,
                        HdRenderIndexSharedPtr const& index,
                        HdxIntersector::Params params,
                        GfVec4i viewport)
    : _primIds(std::move(primIds))
    , _instanceIds(std::move(instanceIds))
    , _elementIds(std::move(elementIds))
    , _depths(std::move(depths))
    , _index(index)
    , _params(params)
    , _viewport(viewport)
{
}

HdxIntersector::Result::~Result()
{
    _params = Params();
    _index.reset();
    _viewport = GfVec4i(0,0,0,0);
}

HdxIntersector::Result::Result(Result &&) = default;

HdxIntersector::Result&
HdxIntersector::Result::operator=(Result &&) = default;

bool
HdxIntersector::Result::_ResolveHit(int index, int x, int y, float z,
                                    HdxIntersector::Hit* hit) const
{
    unsigned char const* primIds = _primIds.get();
    unsigned char const* instanceIds = _instanceIds.get();
    unsigned char const* elementIds = _elementIds.get();

    GfVec3d hitPoint(0,0,0);
    gluUnProject(x, y, z,
                 _params.viewMatrix.GetArray(),
                 _params.projectionMatrix.GetArray(),
                 &_viewport[0],
                 &((hitPoint)[0]),
                 &((hitPoint)[1]),
                 &((hitPoint)[2]));

    int idIndex = index*4;

    GfVec4i primIdColor(
        primIds[idIndex+0],
        primIds[idIndex+1],
        primIds[idIndex+2],
        primIds[idIndex+3]);

    GfVec4i instanceIdColor(
        instanceIds[idIndex+0],
        instanceIds[idIndex+1],
        instanceIds[idIndex+2],
        instanceIds[idIndex+3]);

    int instanceIndex = 0;
    hit->objectId = _index->GetPrimPathFromPrimIdColor(primIdColor,
                        instanceIdColor, &instanceIndex);

    if (not hit->IsValid()) {
        return false;
    }

    // XXX: either this should be done in the render index or all render index
    // logic should be moved here. If moved here, the shader logic should move
    // into Hdx as well.
    int elemIndex = ((elementIds[idIndex+0] & 0xff) <<  0) |
                    ((elementIds[idIndex+1] & 0xff) <<  8) |
                    ((elementIds[idIndex+2] & 0xff) << 16);

    HdRprimSharedPtr const& rprim = _index->GetRprim(hit->objectId);
    if (not TF_VERIFY(rprim, "%s\n", hit->objectId.GetText())) {
        return false;
    }

    hit->delegateId = rprim->GetDelegate()->GetDelegateID();
    hit->instancerId = rprim->GetInstancerId();
    hit->worldSpaceHitPoint = GfVec3f(hitPoint);
    hit->ndcDepth = float(z);
    hit->instanceIndex = instanceIndex;
    hit->elementIndex = elemIndex; 

    if (TfDebug::IsEnabled(HDX_INTERSECT)) {
        std::cout << *hit << std::endl;
    }

    return true;
}

size_t
HdxIntersector::Result::_GetHash(int index) const
{
    unsigned char const* primIds = _primIds.get();
    unsigned char const* instanceIds = _instanceIds.get();
    unsigned char const* elementIds = _elementIds.get();

    int idIndex = index*4;

    GfVec4i primIdColor(
        primIds[idIndex+0],
        primIds[idIndex+1],
        primIds[idIndex+2],
        primIds[idIndex+3]);

    GfVec4i instanceIdColor(
        instanceIds[idIndex+0],
        instanceIds[idIndex+1],
        instanceIds[idIndex+2],
        instanceIds[idIndex+3]);

    int elemIndex = ((elementIds[idIndex+0] & 0xff) <<  0) |
                    ((elementIds[idIndex+1] & 0xff) <<  8) |
                    ((elementIds[idIndex+2] & 0xff) << 16);

    size_t hash = 0;
    boost::hash_combine(hash, primIdColor);
    boost::hash_combine(hash, instanceIdColor);
    boost::hash_combine(hash, elemIndex);

    return hash;
}

bool
HdxIntersector::Result::ResolveNearest(HdxIntersector::Hit* hit) const
{
    TRACE_FUNCTION();

    if (not IsValid()) {
        return false;
    }

    int xMin = 0;
    int yMin = 0;
    double zMin = 1.0;
    int zMinIndex = -1;
    float const* depths = _depths.get();

    // Find the smallest value (nearest pixel) in the z buffer
    for (int y=0, i=0; y < _viewport[2]; y++) {
        for (int x=0; x < _viewport[3]; x++, i++) {
            if (depths[i] < zMin) {
                xMin = x;
                yMin = y;
                zMin = depths[i];
                zMinIndex = i;
            }
        }
    }

    if (zMin >= 1.0) {
        return false;
    }

    return _ResolveHit(zMinIndex, xMin, yMin, zMin, hit);
}

bool
HdxIntersector::Result::ResolveAll(HdxIntersector::HitVector* allHits) const
{
    TRACE_FUNCTION();

    if (not IsValid()) { return false; }

    float const* depths = _depths.get();

    int hitCount = 0;
    for (int y=0, i=0; y < _viewport[2]; y++) {
        for (int x=0; x < _viewport[3]; x++, i++) {
            Hit hit;
            if (_ResolveHit(i, x, y, depths[i], &hit)) {
                hitCount++;
                if (allHits) {
                    allHits->push_back(hit);
                }
            }
        }
    }

    return hitCount > 0;
}

bool
HdxIntersector::Result::ResolveUnique(HdxIntersector::HitSet* hitSet) const
{
    TRACE_FUNCTION();

    if (not IsValid()) { return false; }

    int width = _viewport[2];
    int height = _viewport[3];

    std::unordered_map<size_t, int> hitIndices;
    {
        HD_TRACE_SCOPE("unique indices");
        size_t previousHash = 0;
        for (int i = 0; i < width * height; ++i) {
            size_t hash = _GetHash(i);
            // As an optimization, keep track of the previous hash value and
            // reject indices that match it without performing a map lookup.
            // Adjacent indices are likely enough to have the same prim,
            // instance and element ids that this can be a significant
            // improvement.
            if (hitIndices.empty() or hash != previousHash) {
                hitIndices.insert(std::make_pair(hash, i));
                previousHash = hash;
            }
        }
    }

    int hitCount = 0;
    {
        HD_TRACE_SCOPE("resolve");
        float const* depths = _depths.get();

        TF_FOR_ALL(it, hitIndices) {
            int index = it->second;
            int x = index % width;
            int y = index / width;
            Hit hit;
            if (_ResolveHit(index, x, y, depths[index], &hit)) {
                hitCount++;
                if (hitSet) {
                    hitSet->insert(hit);
                }
            }
        }
    }
    return hitCount > 0;
}

size_t
HdxIntersector::Hit::GetHash() const
{
    size_t hash = 0;

    boost::hash_combine(hash, delegateId.GetHash());
    boost::hash_combine(hash, objectId.GetHash());
    boost::hash_combine(hash, instancerId);
    boost::hash_combine(hash, instanceIndex);
    boost::hash_combine(hash, elementIndex);
    boost::hash_combine(hash, worldSpaceHitPoint[0]);
    boost::hash_combine(hash, worldSpaceHitPoint[1]);
    boost::hash_combine(hash, worldSpaceHitPoint[2]);
    boost::hash_combine(hash, ndcDepth);
    
    return hash;
}

size_t 
HdxIntersector::Hit::HitSetHash::operator()(Hit const& hit) const
{
    size_t hash = 0;

    boost::hash_combine(hash, hit.delegateId.GetHash());
    boost::hash_combine(hash, hit.objectId.GetHash());
    boost::hash_combine(hash, hit.instancerId.GetHash());
    boost::hash_combine(hash, hit.instanceIndex);

    // XXX: a workaround of a performance issue
    // of getting unnecessary detailed selection in ResolveUnique
    //
    // boost::hash_combine(hash, hit.elementIndex);

    return hash;
}

bool
HdxIntersector::Hit::HitSetEq::operator()(Hit const& a, Hit const& b) const
{
    return a.delegateId == b.delegateId
       and a.objectId == b.objectId
       and a.instancerId == b.instancerId
       and a.instanceIndex == b.instanceIndex;
    // XXX: a workaround of a performance issue
    // of getting unnecessary detailed selection in ResolveUnique
    //
    // and a.elementIndex == b.elementIndex;
}

bool
HdxIntersector::Hit::operator<(Hit const& lhs) const
{
    return ndcDepth < lhs.ndcDepth;
}

bool
HdxIntersector::Hit::operator==(Hit const& lhs) const
{
    return objectId == lhs.objectId
       and delegateId == lhs.delegateId
       and instancerId == lhs.instancerId
       and instanceIndex == lhs.instanceIndex
       and elementIndex == lhs.elementIndex
       and worldSpaceHitPoint == lhs.worldSpaceHitPoint
       and ndcDepth == lhs.ndcDepth;
}

std::ostream&
operator<<(std::ostream& out, HdxIntersector::Hit const & h)
{
    out << "Delegate: <" << h.delegateId << "> "
        << "Object: <" << h.objectId << "> "
        << "Instancer: <" << h.instancerId << "> "
        << "Instance: [" << h.instanceIndex << "] "
        << "Element: [" << h.instanceIndex << "] "
        << "HitPoint: (" << h.worldSpaceHitPoint << ") "
        << "Depth: (" << h.ndcDepth << ") ";
    return out;
}

