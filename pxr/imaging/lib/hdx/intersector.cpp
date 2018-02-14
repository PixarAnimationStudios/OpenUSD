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
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/info.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


HdxIntersector::HdxIntersector(HdRenderIndex *index)
    : _index(index)
{ 
}

void
HdxIntersector::_Init(GfVec2i const& size)
{
    // The collection created below is purely for satisfying the HdRenderPass
    // constructor. The collections for the render passes are set in Query(..)
    HdRprimCollection col(HdTokens->geometry, HdTokens->hull);
    _pickableRenderPass = 
        _index->GetRenderDelegate()->CreateRenderPass(&*_index, col);
    _unpickableRenderPass = 
        _index->GetRenderDelegate()->CreateRenderPass(&*_index, col);

    // initialize renderPassState with ID render shader
    _pickableRenderPassState = boost::make_shared<HdStRenderPassState>(
        boost::make_shared<HdStRenderPassShader>(HdxPackageRenderPassIdShader()));

    // Turn off color writes for the unpickables (we only want to condition the
    // depth buffer).
    // XXX: This is a quick alternative to using a different  shader mixin for
    // the unpickables.
    _unpickableRenderPassState = boost::make_shared<HdStRenderPassState>(
        boost::make_shared<HdStRenderPassShader>(HdxPackageRenderPassIdShader()));
    _unpickableRenderPassState->SetColorMaskUseDefault(false);
    _unpickableRenderPassState->SetColorMask(HdRenderPassState::ColorMaskNone);

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
            "edgeId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
        _drawTarget->AddAttachment(
            "depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH24_STENCIL8);
            //"depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);

        _drawTarget->Unbind();
    }
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

    if (!_drawTarget) {
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

void
HdxIntersector::_ConditionStencilWithGLCallback(DepthMaskCallback maskCallback)
{
    // Setup stencil state and prevent writes to color buffer.
    // We don't use the pickable/unpickable render pass state below, since
    // the callback uses immediate mode GL, and doesn't conform to Hydra's
    // command buffer based execution philosophy.
    {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 1);
        glStencilOp(GL_KEEP,     // stencil failed
                    GL_KEEP,     // stencil passed, depth failed
                    GL_REPLACE); // stencil passed, depth passed
    }
    
    //
    // Condition the stencil buffer.
    //
    maskCallback();

    // We expect any GL state changes are restored.
    {
        // Clear depth incase the depthMaskCallback pollutes the depth buffer.
        glClear(GL_DEPTH_BUFFER_BIT);
        // Restore color outputs & setup state for rendering
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glFrontFace(GL_CCW);
    }
    
    // Update the stencil state for the render passes
    {
        HdRenderPassStateSharedPtr states[] = {_pickableRenderPassState,
                                               _unpickableRenderPassState};
        for (auto& state : states) {
            state->SetStencilEnabled(true);
            state->SetStencil(HdCmpFuncLess,
                            /*ref=*/0,
                            /*mask=*/1,
                            /*sFail*/HdStencilOpKeep,
                            /*sPassZFail*/HdStencilOpKeep,
                            /*sPassZPass*/HdStencilOpKeep);
        }
    }
}

class HdxIntersector_DrawTask final : public HdTask
{
public:
    HdxIntersector_DrawTask(HdRenderPassSharedPtr const &renderPass,
                HdRenderPassStateSharedPtr const &renderPassState,
                TfTokenVector const &renderTags)
    : HdTask()
    , _renderPass(renderPass)
    , _renderPassState(renderPassState)
    , _renderTags(renderTags)
    {
    }

protected:
    virtual void _Sync(HdTaskContext* ctx) override
    {
        _renderPass->Sync();
        _renderPassState->Sync(
            _renderPass->GetRenderIndex()->GetResourceRegistry());
    }

    virtual void _Execute(HdTaskContext* ctx) override
    {
        // Try to extract render tags from the context in case
        // there are render tags passed to the graph that 
        // we should be using while rendering the id buffer
        // XXX If this was a task (in the render graph) we could
        // just connect it to the render pass setup which receives
        // its rendertags from the viewer.
        if(_renderTags.empty()) {
            _GetTaskContextData(ctx, HdxTokens->renderTags, &_renderTags);
        }

        _renderPassState->Bind();
        if(_renderTags.size()) {
            _renderPass->Execute(_renderPassState, _renderTags);
        } else {
            _renderPass->Execute(_renderPassState);
        }
        _renderPassState->Unbind();
    }

private:
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
    TfTokenVector _renderTags;
};

bool
HdxIntersector::Query(HdxIntersector::Params const& params,
                      HdRprimCollection const& pickablesCol,
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
    if (!TF_VERIFY(context)) {
        TF_RUNTIME_ERROR("Invalid GL context");
        return false;
    }
    if (!_drawTarget) {
        // Initialize the shared draw target late to ensure there is a valid GL
        // context, which may not be the case at constructon time.
        _Init(GfVec2i(128,128));
    }
    if (!TF_VERIFY(_pickableRenderPass) || 
        !TF_VERIFY(_unpickableRenderPass)) {
        return false;
    }

    GfVec2i size(_drawTarget->GetSize());
    GfVec4i viewport(0, 0, size[0], size[1]);

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
    // XXX: We should use the pickMode param to bind only the attachments
    // that are necessary. This should affect the shader code generated as well.
    GLenum drawBuffers[4] = { GL_COLOR_ATTACHMENT0,
                              GL_COLOR_ATTACHMENT1,
                              GL_COLOR_ATTACHMENT2,
                              GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, drawBuffers);
    
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

        bool needStencilConditioning = (params.depthMaskCallback != nullptr);

        if (needStencilConditioning) {
            _ConditionStencilWithGLCallback(params.depthMaskCallback);
        } else {
            // disable stencil
            _pickableRenderPassState->SetStencilEnabled(false);
            _unpickableRenderPassState->SetStencilEnabled(false);
        }
        

        // Update render pass states based on incoming params.
        HdRenderPassStateSharedPtr states[] = {_pickableRenderPassState,
                                               _unpickableRenderPassState};
        for (auto& state : states) {
            state->SetAlphaThreshold(params.alphaThreshold);
            state->SetClipPlanes(params.clipPlanes);
            state->SetCullStyle(params.cullStyle);
            state->SetCamera(params.viewMatrix, params.projectionMatrix, viewport);
            state->SetLightingEnabled(false);
        }

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

        HdTaskSharedPtrVector tasks;
        //
        // Unpickable prims (condition the depth buffer so they can occlude,
        // but don't write to the attachments of the draw target)
        //
        if (pickablesCol.GetExcludePaths().size() > 0)
        {
            HdRprimCollection unpickablesCol = 
                pickablesCol.CreateInverseCollection();
            _unpickableRenderPass->SetRprimCollection(unpickablesCol);

            tasks.push_back(boost::make_shared<HdxIntersector_DrawTask>(
                    _unpickableRenderPass,
                    _unpickableRenderPassState,
                    params.renderTags));
        }
        
        // 
        // Pickable prims (id render)
        //
        // XXX: make intersector a Task
        _pickableRenderPass->SetRprimCollection(pickablesCol);
        tasks.push_back(boost::make_shared<HdxIntersector_DrawTask>(
                _pickableRenderPass,
                _pickableRenderPassState,
                params.renderTags));

        engine->Execute(*_index, tasks);

        glDisable(GL_STENCIL_TEST);

        if (convRstr) {
            // XXX: this should come from Glew
            #define GL_CONSERVATIVE_RASTERIZATION_NV 0x9346
            glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
        }

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
    std::unique_ptr<unsigned char[]> edgeId(new unsigned char[len*4]);
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
        drawTarget->GetAttachments().at("edgeId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &edgeId[0]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("depth")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                    &depths[0]);

    glBindTexture(GL_TEXTURE_2D, 0);

    GLF_POST_PENDING_GL_ERRORS();

    if (result) {
        *result = HdxIntersector::Result(
            std::move(primId), std::move(instanceId), std::move(elementId),
            std::move(edgeId), std::move(depths), _index, params, viewport);
    }

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
                        std::unique_ptr<unsigned char[]> edgeIds,
                        std::unique_ptr<float[]> depths,
                        HdRenderIndex const *index,
                        HdxIntersector::Params params,
                        GfVec4i viewport)
    : _primIds(std::move(primIds))
    , _instanceIds(std::move(instanceIds))
    , _elementIds(std::move(elementIds))
    , _edgeIds(std::move(edgeIds))
    , _depths(std::move(depths))
    , _index(index)
    , _params(params)
    , _viewport(viewport)
{
}

HdxIntersector::Result::~Result()
{
    _params = Params();
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
    unsigned char const* edgeIds = _edgeIds.get();

    GfVec3d hitPoint(0,0,0);
    gluUnProject(x, y, z,
                 _params.viewMatrix.GetArray(),
                 _params.projectionMatrix.GetArray(),
                 &_viewport[0],
                 &((hitPoint)[0]),
                 &((hitPoint)[1]),
                 &((hitPoint)[2]));

    int idIndex = index*4;

    int primId = HdxRenderSetupTask::DecodeIDRenderColor(&primIds[idIndex]);
    hit->objectId = _index->GetRprimPathFromPrimId(primId);

    if (!hit->IsValid()) {
        return false;
    }

    int instanceIndex = HdxRenderSetupTask::DecodeIDRenderColor(
            &instanceIds[idIndex]);
    int elementIndex = HdxRenderSetupTask::DecodeIDRenderColor(
            &elementIds[idIndex]);
    int edgeIndex = HdxRenderSetupTask::DecodeIDRenderColor(
            &edgeIds[idIndex]);

    bool rprimValid = _index->GetSceneDelegateAndInstancerIds(hit->objectId,
                                                           &(hit->delegateId),
                                                           &(hit->instancerId));

    if (!TF_VERIFY(rprimValid, "%s\n", hit->objectId.GetText())) {
        return false;
    }

    hit->worldSpaceHitPoint = GfVec3f(hitPoint);
    hit->ndcDepth = float(z);
    hit->instanceIndex = instanceIndex;
    hit->elementIndex = elementIndex; 
    hit->edgeIndex = edgeIndex; 

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
    unsigned char const* edgeIds = _edgeIds.get();

    int idIndex = index*4;

    int primId = HdxRenderSetupTask::DecodeIDRenderColor(
            &primIds[idIndex]);
    int instanceIndex = HdxRenderSetupTask::DecodeIDRenderColor(
            &instanceIds[idIndex]);
    int elementIndex = HdxRenderSetupTask::DecodeIDRenderColor(
            &elementIds[idIndex]);
    int edgeIndex = HdxRenderSetupTask::DecodeIDRenderColor(
            &edgeIds[idIndex]);

    size_t hash = 0;
    boost::hash_combine(hash, primId);
    boost::hash_combine(hash, instanceIndex);
    boost::hash_combine(hash, elementIndex);
    boost::hash_combine(hash, size_t(edgeIndex));

    return hash;
}

bool
HdxIntersector::Result::ResolveNearest(HdxIntersector::Hit* hit) const
{
    TRACE_FUNCTION();

    if (!IsValid()) {
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

    if (!IsValid()) { return false; }

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

    if (!IsValid()) { return false; }

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
            if (hitIndices.empty() || hash != previousHash) {
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
    boost::hash_combine(hash, edgeIndex);
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
    boost::hash_combine(hash, hit.elementIndex);
    boost::hash_combine(hash, hit.edgeIndex);

    return hash;
}

bool
HdxIntersector::Hit::HitSetEq::operator()(Hit const& a, Hit const& b) const
{
    return a.delegateId == b.delegateId
       && a.objectId == b.objectId
       && a.instancerId == b.instancerId
       && a.instanceIndex == b.instanceIndex
       && a.elementIndex == b.elementIndex
       && a.edgeIndex == b.edgeIndex;
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
       && delegateId == lhs.delegateId
       && instancerId == lhs.instancerId
       && instanceIndex == lhs.instanceIndex
       && elementIndex == lhs.elementIndex
       && edgeIndex == lhs.edgeIndex
       && worldSpaceHitPoint == lhs.worldSpaceHitPoint
       && ndcDepth == lhs.ndcDepth;
}

std::ostream&
operator<<(std::ostream& out, HdxIntersector::Hit const & h)
{
    out << "Delegate: <" << h.delegateId << "> "
        << "Object: <" << h.objectId << "> "
        << "Instancer: <" << h.instancerId << "> "
        << "Instance: [" << h.instanceIndex << "] "
        << "Element: [" << h.elementIndex << "] "
        << "Edge: [" << h.edgeIndex  << "] "
        << "HitPoint: (" << h.worldSpaceHitPoint << ") "
        << "Depth: (" << h.ndcDepth << ") ";
    return out;
}


PXR_NAMESPACE_CLOSE_SCOPE

