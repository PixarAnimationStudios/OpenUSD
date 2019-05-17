//
// Copyright 2019 Pixar
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

#include "pxr/imaging/hdx/pickTask.h"

#include "pxr/imaging/hdx/debugCodes.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/shaderCode.h"

#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/info.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdxPickTokens, HDX_PICK_TOKENS);

static HdRenderPassStateSharedPtr
_InitIdRenderPassState(HdRenderIndex *index)
{
    HdRenderPassStateSharedPtr rps =
        index->GetRenderDelegate()->CreateRenderPassState();

    if (HdStRenderPassState* extendedState =
            dynamic_cast<HdStRenderPassState*>(
                rps.get())) {
        extendedState->SetRenderPassShader(
            boost::make_shared<HdStRenderPassShader>(
                HdxPackageRenderPassPickingShader()));
    }

    return rps;
}

static bool
_IsStreamRenderingBackend(HdRenderIndex *index)
{
    if(!dynamic_cast<HdStRenderDelegate*>(index->GetRenderDelegate())) {
        return false;
    }

    return true;
}

HdxPickTask::HdxPickTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdTask(id)
    , _index(nullptr)
{
}

HdxPickTask::~HdxPickTask()
{
}

void
HdxPickTask::_Init(GfVec2i const& size)
{
    // The collection created below is purely for satisfying the HdRenderPass
    // constructor. The collections for the render passes are set in Query(..)
    HdRprimCollection col(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));
    _pickableRenderPass = 
        _index->GetRenderDelegate()->CreateRenderPass(&*_index, col);
    _occluderRenderPass = 
        _index->GetRenderDelegate()->CreateRenderPass(&*_index, col);

    // initialize renderPassStates with ID render shader
    _pickableRenderPassState = _InitIdRenderPassState(_index);
    _occluderRenderPassState = _InitIdRenderPassState(_index);
    // Turn off color writes for the occluders, wherein we want to only
    // condition the depth buffer and not write out any IDs.
    // XXX: This is a hacky alternative to using a different shader mixin to
    // accomplish the same thing.
    _occluderRenderPassState->SetColorMaskUseDefault(false);
    _occluderRenderPassState->SetColorMask(HdRenderPassState::ColorMaskNone);

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
            "pointId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
        _drawTarget->AddAttachment(
            "neye", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
        _drawTarget->AddAttachment(
            "depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH24_STENCIL8);
            //"depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);

        _drawTarget->Unbind();
    }
}

void
HdxPickTask::_ConfigureSceneMaterials(bool enableSceneMaterials,
    HdStRenderPassState *renderPassState)
{
    if (enableSceneMaterials) {
        renderPassState->SetOverrideShader(HdStShaderCodeSharedPtr());
    } else {
        if (!_overrideShader) {
            _overrideShader = HdStShaderCodeSharedPtr(new HdStGLSLFXShader(
                HioGlslfxSharedPtr(new HioGlslfx(
                    HdStPackageFallbackSurfaceShader()))));
        }
        renderPassState->SetOverrideShader(_overrideShader);
    }
}

void
HdxPickTask::_SetResolution(GfVec2i const& widthHeight)
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
HdxPickTask::_ConditionStencilWithGLCallback(
    HdxPickTaskContextParams::DepthMaskCallback maskCallback)
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
}

bool
HdxPickTask::_UseOcclusionPass() const
{
    return _contextParams.doUnpickablesOcclude &&
           !_contextParams.collection.GetExcludePaths().empty();
}

void
HdxPickTask::Sync(HdSceneDelegate* delegate,
                  HdTaskContext* ctx,
                  HdDirtyBits* dirtyBits)
{
    if (!_IsStreamRenderingBackend(&(delegate->GetRenderIndex()))) {
        return;
    }

    // Gather params from the scene and the task context.
    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        _GetTaskParams(delegate, &_params);
        *dirtyBits = HdChangeTracker::Clean;
    }
    _GetTaskContextData(ctx, HdxPickTokens->pickParams, &_contextParams);

    // Store the render index so we can map ids to paths in Execute()...
    _index = &(delegate->GetRenderIndex());

    // Make sure we're in a sane GL state before attempting anything.
    if (GlfHasLegacyGraphics()) {
        TF_RUNTIME_ERROR("framebuffer object not supported");
        return;
    }
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (!TF_VERIFY(context)) {
        TF_RUNTIME_ERROR("Invalid GL context");
        return;
    }

    if (!_drawTarget) {
        // Initialize the shared draw target late to ensure there is a valid GL
        // context, which may not be the case at constructon time.
        _Init(_contextParams.resolution);
    } else {
        _SetResolution(_contextParams.resolution);
    }

    if (!TF_VERIFY(_pickableRenderPass) || 
        !TF_VERIFY(_occluderRenderPass)) {
        return;
    }
    
    HdRenderPassStateSharedPtr states[] = {_pickableRenderPassState,
                                           _occluderRenderPassState};

    // Are we using stencil conditioning?
    bool needStencilConditioning =
        (_contextParams.depthMaskCallback != nullptr);

    // Calculate the viewport
    GfVec2i size(_drawTarget->GetSize());
    GfVec4i viewport(0, 0, size[0], size[1]);

    // Update the renderpass states.
    for (auto& state : states) {
        if (needStencilConditioning) {
            state->SetStencilEnabled(true);
            state->SetStencil(HdCmpFuncLess,
                            /*ref=*/0,
                            /*mask=*/1,
                            /*sFail*/HdStencilOpKeep,
                            /*sPassZFail*/HdStencilOpKeep,
                            /*sPassZPass*/HdStencilOpKeep);
        } else {
            state->SetStencilEnabled(false);
        }
        state->SetAlphaThreshold(_params.alphaThreshold);
        state->SetClipPlanes(_contextParams.clipPlanes);
        state->SetCullStyle(_params.cullStyle);
        state->SetCamera(_contextParams.viewMatrix, 
                         _contextParams.projectionMatrix, viewport);
        state->SetLightingEnabled(false);

        // If scene materials are disabled in this environment then 
        // let's setup the override shader
        if (HdStRenderPassState* extState =
                dynamic_cast<HdStRenderPassState*>(state.get())) {
            _ConfigureSceneMaterials(_params.enableSceneMaterials, extState);
        }
    }

    // Update the collections
    //
    // The picking operation is composed of one or more conceptual passes:
    // (i) [optional] depth-only pass for "unpickable" prims: This ensures 
    // that occlusion stemming for unpickable prims is honored during 
    // picking.
    //
    // (ii) [mandatory] id render for "pickable" prims: This writes out the
    // various id's for prims that pass the depth test.
    if (_UseOcclusionPass()) {
        // Pass (i) from above
        HdRprimCollection occluderCol =
            _contextParams.collection.CreateInverseCollection();
        _occluderRenderPass->SetRprimCollection(occluderCol);
    }

    // Pass (ii) from above
    _pickableRenderPass->SetRprimCollection(_contextParams.collection);

    if (_UseOcclusionPass()) {
        _occluderRenderPass->Sync();
    }
    _pickableRenderPass->Sync();
}

void
HdxPickTask::Prepare(HdTaskContext* ctx,
                     HdRenderIndex* renderIndex)
{
    if (!_drawTarget) {
        return;
    }

    if (_UseOcclusionPass()) {
        _occluderRenderPassState->Prepare(renderIndex->GetResourceRegistry());
    }
    _pickableRenderPassState->Prepare(renderIndex->GetResourceRegistry());
}

void
HdxPickTask::Execute(HdTaskContext* ctx)
{
    if (!_drawTarget) {
        return;
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
    // XXX: We could use the pick target to set some of these to GL_NONE,
    // as a potential optimization.
    GLenum drawBuffers[6] = { GL_COLOR_ATTACHMENT0,
                              GL_COLOR_ATTACHMENT1,
                              GL_COLOR_ATTACHMENT2,
                              GL_COLOR_ATTACHMENT3,
                              GL_COLOR_ATTACHMENT4,
                              GL_COLOR_ATTACHMENT5};
    glDrawBuffers(6, drawBuffers);
    
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    // Clear all color channels to 1, so when cast as int, an unwritten pixel
    // is encoded as -1.
    glClearColor(1,1,1,1);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    GLF_POST_PENDING_GL_ERRORS();

    //
    // Execute the picking pass
    //
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    if (_contextParams.depthMaskCallback != nullptr) {
        _ConditionStencilWithGLCallback(_contextParams.depthMaskCallback);
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

    if (_UseOcclusionPass()) {
        _occluderRenderPassState->Bind();
        if (_params.renderTags.size() > 0) {
            _occluderRenderPass->Execute(_occluderRenderPassState,
                                         _params.renderTags);
        } else {
            _occluderRenderPass->Execute(_occluderRenderPassState);
        }
        _occluderRenderPassState->Unbind();
    }
    _pickableRenderPassState->Bind();
    if (_params.renderTags.size() > 0) {
        _pickableRenderPass->Execute(_pickableRenderPassState,
                                     _params.renderTags);
    } else {
        _pickableRenderPass->Execute(_pickableRenderPassState);
    }
    _pickableRenderPassState->Unbind();

    glDisable(GL_STENCIL_TEST);

    if (convRstr) {
        // XXX: this should come from Glew
        #define GL_CONSERVATIVE_RASTERIZATION_NV 0x9346
        glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
    }

    // Restore
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);

    GLF_POST_PENDING_GL_ERRORS();

    // Capture the result buffers.
    size_t len = size[0] * size[1];
    std::unique_ptr<int[]> primIds(new int[len]);
    std::unique_ptr<int[]> instanceIds(new int[len]);
    std::unique_ptr<int[]> elementIds(new int[len]);
    std::unique_ptr<int[]> edgeIds(new int[len]);
    std::unique_ptr<int[]> pointIds(new int[len]);
    std::unique_ptr<int[]> neyes(new int[len]);
    std::unique_ptr<float[]> depths(new float[len]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("primId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &primIds[0]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("instanceId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &instanceIds[0]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("elementId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &elementIds[0]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("edgeId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &edgeIds[0]);
    
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("pointId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pointIds[0]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("neye")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &neyes[0]);

    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("depth")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                    &depths[0]);

    glBindTexture(GL_TEXTURE_2D, 0);
    drawTarget->Unbind();

    GLF_POST_PENDING_GL_ERRORS();

    HdxPickResult result(
            std::move(primIds), std::move(instanceIds), std::move(elementIds),
            std::move(edgeIds), std::move(pointIds), std::move(neyes),
            std::move(depths),
            _index, _contextParams.pickTarget, _contextParams.viewMatrix,
            _contextParams.projectionMatrix, viewport);

    // Resolve!
    if (_contextParams.resolveMode ==
            HdxPickTokens->resolveNearestToCenter) {
        result.ResolveNearestToCenter(_contextParams.outHits);
    } else if (_contextParams.resolveMode ==
            HdxPickTokens->resolveNearestToCamera) {
        result.ResolveNearestToCamera(_contextParams.outHits);
    } else if (_contextParams.resolveMode ==
            HdxPickTokens->resolveUnique) {
        result.ResolveUnique(_contextParams.outHits);
    } else if (_contextParams.resolveMode ==
            HdxPickTokens->resolveAll) {
        result.ResolveAll(_contextParams.outHits);
    } else {
        TF_CODING_ERROR("Unrecognized interesection mode '%s'",
            _contextParams.resolveMode.GetText());
    }
}

HdxPickResult::HdxPickResult()
    : _index(nullptr)
    , _viewport(0,0,0,0)
{
}

HdxPickResult::HdxPickResult(
        std::unique_ptr<int[]> primIds,
        std::unique_ptr<int[]> instanceIds,
        std::unique_ptr<int[]> elementIds,
        std::unique_ptr<int[]> edgeIds,
        std::unique_ptr<int[]> pointIds,
        std::unique_ptr<int[]> neyes,
        std::unique_ptr<float[]> depths,
        HdRenderIndex const *index,
        TfToken const& pickTarget,
        GfMatrix4d const& viewMatrix,
        GfMatrix4d const& projectionMatrix,
        GfVec4i viewport)
    : _primIds(std::move(primIds))
    , _instanceIds(std::move(instanceIds))
    , _elementIds(std::move(elementIds))
    , _edgeIds(std::move(edgeIds))
    , _pointIds(std::move(pointIds))
    , _neyes(std::move(neyes))
    , _depths(std::move(depths))
    , _index(index)
    , _pickTarget(pickTarget)
    , _viewMatrix(viewMatrix)
    , _projectionMatrix(projectionMatrix)
    , _viewport(viewport)
{
}

HdxPickResult::~HdxPickResult()
{
}

HdxPickResult::HdxPickResult(HdxPickResult &&) = default;

HdxPickResult&
HdxPickResult::operator=(HdxPickResult &&) = default;

bool
HdxPickResult::_ResolveHit(int index, int x, int y, float z,
                           HdxPickHit* hit) const
{
    int primId = _primIds[index];
    hit->objectId = _index->GetRprimPathFromPrimId(primId);

    if (!hit->IsValid()) {
        return false;
    }

    bool rprimValid = _index->GetSceneDelegateAndInstancerIds(hit->objectId,
                                                           &(hit->delegateId),
                                                           &(hit->instancerId));

    if (!TF_VERIFY(rprimValid, "%s\n", hit->objectId.GetText())) {
        return false;
    }

    GfVec3d hitPoint(0,0,0);
    gluUnProject(x, y, z,
                 _viewMatrix.GetArray(),
                 _projectionMatrix.GetArray(),
                 &_viewport[0],
                 &((hitPoint)[0]),
                 &((hitPoint)[1]),
                 &((hitPoint)[2]));

    hit->worldSpaceHitPoint = GfVec3f(hitPoint);
    hit->ndcDepth = float(z);

    GfMatrix4d eyeToWorld = _viewMatrix.GetInverse();
    GfVec3f neye = HdVec4f_2_10_10_10_REV(_neyes[index]).GetAsVec<GfVec3f>();
    hit->worldSpaceHitNormal = eyeToWorld.TransformDir(neye);

    int instanceIndex = _instanceIds[index];
    hit->instanceIndex = instanceIndex;

    int elementIndex = _elementIds[index];
    hit->elementIndex = elementIndex;

    int edgeIndex = _edgeIds[index];
    hit->edgeIndex = edgeIndex;

    int pointIndex = _pointIds[index];
    hit->pointIndex = pointIndex;

    if (TfDebug::IsEnabled(HDX_INTERSECT)) {
        std::cout << *hit << std::endl;
    }

    return true;
}

size_t
HdxPickResult::_GetHash(int index) const
{
    int primId = _primIds[index];
    int instanceIndex = _instanceIds[index];
    int elementIndex = _elementIds[index];
    int edgeIndex = _edgeIds[index];
    int pointIndex = _pointIds[index];

    size_t hash = 0;
    boost::hash_combine(hash, primId);
    boost::hash_combine(hash, instanceIndex);
    boost::hash_combine(hash, elementIndex);
    boost::hash_combine(hash, size_t(edgeIndex));
    boost::hash_combine(hash, size_t(pointIndex));

    return hash;
}

bool
HdxPickResult::_IsValidHit(int index) const
{
    // Inspect the id buffers to determine if the pixel index is a valid hit
    // by accounting for the pick target when picking points and edges.
    // This allows the hit(s) returned to be relevant.
    bool validPrim = (_primIds[index] != -1);
    bool invalidTargetEdgePick = (_pickTarget == HdxPickTokens->pickEdges)
        && (_edgeIds[index] == -1);
    bool invalidTargetPointPick = (_pickTarget == HdxPickTokens->pickPoints)
        && (_pointIds[index] == -1);

    return validPrim
           && !invalidTargetEdgePick
           && !invalidTargetPointPick;
}

void
HdxPickResult::ResolveNearestToCamera(HdxPickHitVector* allHits) const
{
    TRACE_FUNCTION();

    if (!IsValid() || allHits == nullptr) {
        return;
    }

    int width = _viewport[2];
    int height = _viewport[3];
    float const* depths = _depths.get();
    int xMin = 0;
    int yMin = 0;
    double zMin = 1.0;
    int zMinIndex = -1;

    // Find the smallest value (nearest pixel) in the z buffer that is a valid
    // prim. The last part is important since the depth buffer may be
    // populated with occluders (which aren't picked, and thus won't update any
    // of the ID buffers)
    for (int y=0, i=0; y < height; y++) {
        for (int x=0; x < width; x++, i++) {
            if (_IsValidHit(i) && depths[i] < zMin) {
                xMin = x;
                yMin = y;
                zMin = depths[i];
                zMinIndex = i;
            }
        }
    }

    if (zMin >= 1.0) {
        return;
    }

    HdxPickHit hit;
    if (_ResolveHit(zMinIndex, xMin, yMin, zMin, &hit)) {
        allHits->push_back(hit);
    }
}

void
HdxPickResult::ResolveNearestToCenter(HdxPickHitVector* allHits) const
{
    TRACE_FUNCTION();

    if (!IsValid() || allHits == nullptr) {
        return;
    }

    int width = _viewport[2];
    int height = _viewport[3];
    float const* depths = _depths.get();

    int midH = height/2;
    int midW = width /2;
    if (height%2 == 0) {
        midH--;
    }
    if (width%2 == 0) {
        midW--;
    }
    
    // Return the first valid hit that's closest to the center of the draw
    // target by walking from the center outwards.
    for (int x = midW, y = midH; x >= 0 && y >= 0; x--, y--) {
        for (int xx = x; xx < width-x; xx++) {
            for (int yy = y; yy < height-y; yy++) {
                int index = xx + yy*width;
                if (_IsValidHit(index)) {
                    HdxPickHit hit;
                    if (_ResolveHit(index, index%width, index/width,
                                    depths[index], &hit)) {
                        allHits->push_back(hit);
                        return;
                    }
                }
                // Skip pixels we've already visited and jump to the boundary
                if (!(xx == x || xx == width-x-1) && yy == y) {
                    yy = std::max(yy, height-y-2);
                }
            }
        }
    }
}

void
HdxPickResult::ResolveAll(HdxPickHitVector* allHits) const
{
    TRACE_FUNCTION();

    if (!IsValid() || allHits == nullptr) { return; }

    int width = _viewport[2];
    int height = _viewport[3];
    float const* depths = _depths.get();

    for (int y=0, i=0; y < height; y++) {
        for (int x=0; x < width; x++, i++) {
            if (!_IsValidHit(i)) continue;

            HdxPickHit hit;
            if (_ResolveHit(i, x, y, depths[i], &hit)) {
                allHits->push_back(hit);
            }
        }
    }
}

void
HdxPickResult::ResolveUnique(HdxPickHitVector* allHits) const
{
    TRACE_FUNCTION();

    if (!IsValid() || allHits == nullptr) { return; }

    int width = _viewport[2];
    int height = _viewport[3];

    std::unordered_map<size_t, int> hitIndices;
    {
        HD_TRACE_SCOPE("unique indices");
        size_t previousHash = 0;
        for (int i = 0; i < width * height; ++i) {
            if (!_IsValidHit(i)) continue;
           
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

    {
        HD_TRACE_SCOPE("resolve");
        float const* depths = _depths.get();

        TF_FOR_ALL(it, hitIndices) {
            int index = it->second;
            int x = index % width;
            int y = index / width;
            HdxPickHit hit;
            if (_ResolveHit(index, x, y, depths[index], &hit)) {
                // _GetHash has done the uniqueifying for us here.
                allHits->push_back(hit);
            }
        }
    }
}

size_t
HdxPickHit::GetHash() const
{
    size_t hash = 0;

    boost::hash_combine(hash, delegateId.GetHash());
    boost::hash_combine(hash, objectId.GetHash());
    boost::hash_combine(hash, instancerId);
    boost::hash_combine(hash, instanceIndex);
    boost::hash_combine(hash, elementIndex);
    boost::hash_combine(hash, edgeIndex);
    boost::hash_combine(hash, pointIndex);
    boost::hash_combine(hash, worldSpaceHitPoint[0]);
    boost::hash_combine(hash, worldSpaceHitPoint[1]);
    boost::hash_combine(hash, worldSpaceHitPoint[2]);
    boost::hash_combine(hash, worldSpaceHitNormal[0]);
    boost::hash_combine(hash, worldSpaceHitNormal[1]);
    boost::hash_combine(hash, worldSpaceHitNormal[2]);
    boost::hash_combine(hash, ndcDepth);
    
    return hash;
}

bool
operator<(HdxPickHit const& lhs, HdxPickHit const& rhs)
{
    return lhs.ndcDepth < rhs.ndcDepth;
}

bool
operator==(HdxPickHit const& lhs, HdxPickHit const& rhs)
{
    return lhs.objectId == rhs.objectId
        && lhs.delegateId == rhs.delegateId
        && lhs.instancerId == rhs.instancerId
        && lhs.instanceIndex == rhs.instanceIndex
        && lhs.elementIndex == rhs.elementIndex
        && lhs.edgeIndex == rhs.edgeIndex
        && lhs.pointIndex == rhs.pointIndex
        && lhs.worldSpaceHitPoint == rhs.worldSpaceHitPoint
        && lhs.worldSpaceHitNormal == rhs.worldSpaceHitNormal
        && lhs.ndcDepth == rhs.ndcDepth;
}

bool
operator!=(HdxPickHit const& lhs, HdxPickHit const& rhs)
{
    return !(lhs==rhs);
}

std::ostream&
operator<<(std::ostream& out, HdxPickHit const& h)
{
    out << "Delegate: <" << h.delegateId << "> "
        << "Object: <" << h.objectId << "> "
        << "Instancer: <" << h.instancerId << "> "
        << "Instance: [" << h.instanceIndex << "] "
        << "Element: [" << h.elementIndex << "] "
        << "Edge: [" << h.edgeIndex  << "] "
        << "Point: [" << h.pointIndex  << "] "
        << "HitPoint: (" << h.worldSpaceHitPoint << ") "
        << "HitNormal: (" << h.worldSpaceHitNormal << ") "
        << "Depth: (" << h.ndcDepth << ") ";
    return out;
}

bool
operator==(HdxPickTaskParams const& lhs, HdxPickTaskParams const& rhs)
{
    return lhs.alphaThreshold == rhs.alphaThreshold
        && lhs.cullStyle == rhs.cullStyle
        && lhs.renderTags == rhs.renderTags
        && lhs.enableSceneMaterials == rhs.enableSceneMaterials;
}

bool
operator!=(HdxPickTaskParams const& lhs, HdxPickTaskParams const& rhs)
{
    return !(lhs==rhs);
}

std::ostream&
operator<<(std::ostream& out, HdxPickTaskParams const& p)
{
    out << "PickTask Params: (...) "
        << p.alphaThreshold << " "
        << p.cullStyle << " "
        << p.enableSceneMaterials;
    for (auto const& a : p.renderTags) {
        out << a << " ";
    }
    return out;
}

bool
operator==(HdxPickTaskContextParams const& lhs,
           HdxPickTaskContextParams const& rhs)
{
    typedef void (*RawDepthMaskCallback)(void);
    const RawDepthMaskCallback *lhsDepthMaskPtr =
        lhs.depthMaskCallback.target<RawDepthMaskCallback>();
    const RawDepthMaskCallback *rhsDepthMaskPtr =
        rhs.depthMaskCallback.target<RawDepthMaskCallback>();
    const RawDepthMaskCallback lhsDepthMask =
        lhsDepthMaskPtr ? *lhsDepthMaskPtr : nullptr;
    const RawDepthMaskCallback rhsDepthMask =
        rhsDepthMaskPtr ? *rhsDepthMaskPtr : nullptr;

    return lhs.resolution == rhs.resolution
        && lhs.hitMode == rhs.hitMode
        && lhs.pickTarget == rhs.pickTarget
        && lhs.resolveMode == rhs.resolveMode
        && lhs.doUnpickablesOcclude == rhs.doUnpickablesOcclude
        && lhs.viewMatrix == rhs.viewMatrix
        && lhs.projectionMatrix == rhs.projectionMatrix
        && lhs.clipPlanes == rhs.clipPlanes
        && lhsDepthMask == rhsDepthMask
        && lhs.collection == rhs.collection
        && lhs.outHits == rhs.outHits;
}

bool
operator!=(HdxPickTaskContextParams const& lhs,
           HdxPickTaskContextParams const& rhs)
{
    return !(lhs==rhs);
}

std::ostream&
operator<<(std::ostream& out, HdxPickTaskContextParams const& p)
{
    typedef void (*RawDepthMaskCallback)(void);
    const RawDepthMaskCallback *depthMaskPtr =
        p.depthMaskCallback.target<RawDepthMaskCallback>();
    const RawDepthMaskCallback depthMask =
        depthMaskPtr ? *depthMaskPtr : nullptr;

    out << "PickTask Context Params: (...) "
        << p.resolution << " "
        << p.hitMode << " "
        << p.pickTarget << " "
        << p.resolveMode << " "
        << p.doUnpickablesOcclude << " "
        << p.viewMatrix << " "
        << p.projectionMatrix << " "
        << depthMask << " "
        << p.collection << " "
        << p.outHits;
    for (auto const& a : p.clipPlanes) {
        out << a << " ";
    }
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
