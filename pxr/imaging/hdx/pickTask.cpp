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
#include "pxr/imaging/garch/glApi.h"

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
#include "pxr/imaging/glf/contextCaps.h"

#include <boost/functional/hash.hpp>

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
            std::make_shared<HdStRenderPassShader>(
                HdxPackageRenderPassPickingShader()));
    }

    return rps;
}

static bool
_IsStormRenderer(HdRenderDelegate *renderDelegate)
{
    if(!dynamic_cast<HdStRenderDelegate*>(renderDelegate)) {
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------- //
// HdxPickTask
// -------------------------------------------------------------------------- //
HdxPickTask::HdxPickTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdTask(id)
    , _renderTags()
    , _index(nullptr)
{
}

HdxPickTask::~HdxPickTask() = default;

// Assumes that there is a valid OpenGL 2.0 or later context.
//
// Uses _drawTarget and _drawTarget->GetSize() to determine whether
// initialization is necessary.
//
void
HdxPickTask::_InitIfNeeded(GfVec2i const& size)
{
    if (_drawTarget) {
        if (size != _drawTarget->GetSize()) {
            GlfSharedGLContextScopeHolder sharedContextHolder;

            _drawTarget->Bind();
            _drawTarget->SetSize(size);
            _drawTarget->Unbind();
        }
        return;
    }

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
    _occluderRenderPassState->SetColorMasks({HdRenderPassState::ColorMaskNone});

    // Make sure master draw target is always modified on the shared context,
    // so we access it consistently.
    {
        GlfSharedGLContextScopeHolder sharedContextHolder;

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
    GLF_GROUP_FUNCTION();

    if (!_IsStormRenderer( delegate->GetRenderIndex().GetRenderDelegate() )) {
        return;
    }

    // Gather params from the scene and the task context.
    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        _GetTaskParams(delegate, &_params);
    }

    if ((*dirtyBits) & HdChangeTracker::DirtyRenderTags) {
        _renderTags = _GetTaskRenderTags(delegate);
    }

    _GetTaskContextData(ctx, HdxPickTokens->pickParams, &_contextParams);

    // Store the render index so we can map ids to paths in Execute()...
    _index = &(delegate->GetRenderIndex());

    // Make sure we're in a sane GL state before attempting anything.
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (!TF_VERIFY(context)) {
        TF_RUNTIME_ERROR("Invalid GL context");
        return;
    }

    // Make sure the GL context is at least OpenGL 2.0.
    if (GlfContextCaps::GetInstance().glVersion < 200) {
        TF_RUNTIME_ERROR("framebuffer object not supported");
        return;
    }

    // Uses _drawTarget and _drawTarget->GetSize() to determine whether
    // initialization is necessary.
    _InitIfNeeded(_contextParams.resolution);

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

        state->SetEnableDepthTest(true);
        state->SetEnableDepthMask(true);
        state->SetDepthFunc(HdCmpFuncLEqual);

        // Make sure translucent pixels can be picked by not discarding them
        state->SetAlphaThreshold(0.0f);
        state->SetAlphaToCoverageEnabled(false);
        state->SetBlendEnabled(false);
        state->SetCullStyle(_params.cullStyle);
        state->SetLightingEnabled(false);

        // If scene materials are disabled in this environment then 
        // let's setup the override shader
        if (HdStRenderPassState* extState =
                dynamic_cast<HdStRenderPassState*>(state.get())) {
            extState->SetCameraFramingState(
                _contextParams.viewMatrix, 
                _contextParams.projectionMatrix,
                viewport,
                _contextParams.clipPlanes);
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

    *dirtyBits = HdChangeTracker::Clean;
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
    GLF_GROUP_FUNCTION();

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
    bool convRstr = GARCH_GLAPI_HAS(NV_conservative_raster);
    if (convRstr) {
        glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
    }

    if (_UseOcclusionPass()) {
        _occluderRenderPass->Execute(_occluderRenderPassState,
                                     GetRenderTags());
    }
    _pickableRenderPass->Execute(_pickableRenderPassState,
                                 GetRenderTags());

    glDisable(GL_STENCIL_TEST);

    if (convRstr) {
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

    // For un-projection, get the current depth range.
    GLfloat p[2];
    glGetFloatv(GL_DEPTH_RANGE, &p[0]);
    GfVec2f depthRange(p[0], p[1]);

    // HdxPickResult takes a subrect, which is the region of the id buffer to
    // search over; this task always uses the whole buffer...
    GfVec4i subRect(0, 0, size[0], size[1]);

    HdxPickResult result(
            primIds.get(), instanceIds.get(), elementIds.get(),
            edgeIds.get(), pointIds.get(), neyes.get(), depths.get(),
            _index, _contextParams.pickTarget, _contextParams.viewMatrix,
            _contextParams.projectionMatrix, depthRange, size, subRect);

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

const TfTokenVector &
HdxPickTask::GetRenderTags() const
{
    return _renderTags;
}

// -------------------------------------------------------------------------- //
// HdxPickResult
// -------------------------------------------------------------------------- //
HdxPickResult::HdxPickResult(
        int const* primIds,
        int const* instanceIds,
        int const* elementIds,
        int const* edgeIds,
        int const* pointIds,
        int const* neyes,
        float const* depths,
        HdRenderIndex const *index,
        TfToken const& pickTarget,
        GfMatrix4d const& viewMatrix,
        GfMatrix4d const& projectionMatrix,
        GfVec2f const& depthRange,
        GfVec2i const& bufferSize,
        GfVec4i const& subRect)
    : _primIds(primIds)
    , _instanceIds(instanceIds)
    , _elementIds(elementIds)
    , _edgeIds(edgeIds)
    , _pointIds(pointIds)
    , _neyes(neyes)
    , _depths(depths)
    , _index(index)
    , _pickTarget(pickTarget)
    , _depthRange(depthRange)
    , _bufferSize(bufferSize)
    , _subRect(subRect)
{
    // Clamp _subRect [x,y,w,h] to render buffer [0,0,w,h]
    _subRect[0] = std::max(0, _subRect[0]);
    _subRect[1] = std::max(0, _subRect[1]);
    _subRect[2] = std::min(_bufferSize[0]-_subRect[0], _subRect[2]);
    _subRect[3] = std::min(_bufferSize[1]-_subRect[1], _subRect[3]);

    _eyeToWorld = viewMatrix.GetInverse();
    _ndcToWorld = (viewMatrix * projectionMatrix).GetInverse();
}

HdxPickResult::~HdxPickResult() = default;

HdxPickResult::HdxPickResult(HdxPickResult &&) = default;

HdxPickResult&
HdxPickResult::operator=(HdxPickResult &&) = default;

bool
HdxPickResult::IsValid() const
{
    // Make sure we have at least a primId buffer and a depth buffer.
    if (!_depths || !_primIds) {
        return false;
    }

    return true;
}

GfVec3f
HdxPickResult::_GetNormal(int index) const
{
    GfVec3f normal = GfVec3f(0);
    if (_neyes != nullptr) {
        GfVec3f neye =
            HdVec4f_2_10_10_10_REV(_neyes[index]).GetAsVec<GfVec3f>();
        normal = _eyeToWorld.TransformDir(neye);
    }
    return normal;
}

bool
HdxPickResult::_ResolveHit(int index, int x, int y, float z,
                           HdxPickHit* hit) const
{
    int primId = _GetPrimId(index);
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

    // Calculate the hit location in NDC, then transform to worldspace.
    GfVec3d ndcHit(
        ((double)x / _bufferSize[0]) * 2.0 - 1.0,
        ((double)y / _bufferSize[1]) * 2.0 - 1.0,
        ((z - _depthRange[0]) / (_depthRange[1] - _depthRange[0])) * 2.0 - 1.0);
    hit->worldSpaceHitPoint = GfVec3f(_ndcToWorld.Transform(ndcHit));
    hit->worldSpaceHitNormal = _GetNormal(index);
    hit->normalizedDepth =
        (z - _depthRange[0]) / (_depthRange[1] - _depthRange[0]);

    hit->instanceIndex = _GetInstanceId(index);
    hit->elementIndex = _GetElementId(index);
    hit->edgeIndex = _GetEdgeId(index);
    hit->pointIndex = _GetPointId(index);

    if (TfDebug::IsEnabled(HDX_INTERSECT)) {
        std::cout << *hit << std::endl;
    }

    return true;
}

size_t
HdxPickResult::_GetHash(int index) const
{
    size_t hash = 0;
    boost::hash_combine(hash, _GetPrimId(index));
    boost::hash_combine(hash, _GetInstanceId(index));
    if (_pickTarget == HdxPickTokens->pickFaces) {
        boost::hash_combine(hash, _GetElementId(index));
    }
    if (_pickTarget == HdxPickTokens->pickEdges) {
        boost::hash_combine(hash, _GetEdgeId(index));
    }
    if (_pickTarget == HdxPickTokens->pickPoints) {
        boost::hash_combine(hash, _GetPointId(index));
    }
    return hash;
}

bool
HdxPickResult::_IsValidHit(int index) const
{
    // Inspect the id buffers to determine if the pixel index is a valid hit
    // by accounting for the pick target when picking points and edges.
    // This allows the hit(s) returned to be relevant.
    bool validPrim = (_GetPrimId(index) != -1);
    bool invalidTargetEdgePick = (_pickTarget == HdxPickTokens->pickEdges)
        && (_GetEdgeId(index) == -1);
    bool invalidTargetPointPick = (_pickTarget == HdxPickTokens->pickPoints)
        && (_GetPointId(index) == -1);

    return validPrim
           && !invalidTargetEdgePick
           && !invalidTargetPointPick;
}

void
HdxPickResult::ResolveNearestToCamera(HdxPickHitVector* allHits) const
{
    TRACE_FUNCTION();

    if (!IsValid() || !allHits) {
        return;
    }

    int xMin = 0;
    int yMin = 0;
    double zMin = 0;
    int zMinIndex = -1;

    // Find the smallest value (nearest pixel) in the z buffer that is a valid
    // prim. The last part is important since the depth buffer may be
    // populated with occluders (which aren't picked, and thus won't update any
    // of the ID buffers)
    for (int y = _subRect[1]; y < _subRect[1] + _subRect[3]; ++y) {
        for (int x = _subRect[0]; x < _subRect[0] + _subRect[2]; ++x) {
            int i = y * _bufferSize[0] + x;
            if (_IsValidHit(i) && (zMinIndex == -1 || _depths[i] < zMin)) {
                xMin = x;
                yMin = y;
                zMin = _depths[i];
                zMinIndex = i;
            }
        }
    }

    if (zMinIndex == -1) {
        // We didn't find any valid hits.
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

    if (!IsValid() || !allHits) {
        return;
    }

    int width = _subRect[2];
    int height = _subRect[3];

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
    for (int w = midW, h = midH; w >= 0 && h >= 0; w--, h--) {
        for (int ww = w; ww < width-w; ww++) {
            for (int hh = h; hh < height-h; hh++) {
                int x = ww + _subRect[0];
                int y = hh + _subRect[1];
                int i = y * _bufferSize[0] + x;
                if (_IsValidHit(i)) {
                    HdxPickHit hit;
                    if (_ResolveHit(i, x, y, _depths[i], &hit)) {
                        allHits->push_back(hit);
                    }
                    return;
                }
                // Skip pixels we've already visited and jump to the boundary
                if (!(ww == w || ww == width-w-1) && hh == h) {
                    hh = std::max(hh, height-h-2);
                }
            }
        }
    }
}

void
HdxPickResult::ResolveAll(HdxPickHitVector* allHits) const
{
    TRACE_FUNCTION();

    if (!IsValid() || !allHits) {
        return;
    }

    for (int y = _subRect[1]; y < _subRect[1] + _subRect[3]; ++y) {
        for (int x = _subRect[0]; x < _subRect[0] + _subRect[2]; ++x) {
            int i = y * _bufferSize[0] + x;
            if (!_IsValidHit(i)) continue;

            HdxPickHit hit;
            if (_ResolveHit(i, x, y, _depths[i], &hit)) {
                allHits->push_back(hit);
            }
        }
    }
}

void
HdxPickResult::ResolveUnique(HdxPickHitVector* allHits) const
{
    TRACE_FUNCTION();

    if (!IsValid() || !allHits) {
        return;
    }

    std::unordered_map<size_t, GfVec2i> hitIndices;
    {
        HD_TRACE_SCOPE("unique indices");
        size_t previousHash = 0;
        for (int y = _subRect[1]; y < _subRect[1] + _subRect[3]; ++y) {
            for (int x = _subRect[0]; x < _subRect[0] + _subRect[2]; ++x) {
                int i = y * _bufferSize[0] + x;
                if (!_IsValidHit(i)) continue;

                size_t hash = _GetHash(i);
                // As an optimization, keep track of the previous hash value and
                // reject indices that match it without performing a map lookup.
                // Adjacent indices are likely enough to have the same prim,
                // instance and if relevant, the same subprim ids, that this can
                // be a significant improvement.
                if (hitIndices.empty() || hash != previousHash) {
                    hitIndices.insert(std::make_pair(hash, GfVec2i(x,y)));
                    previousHash = hash;
                }
            }
        }
    }

    {
        HD_TRACE_SCOPE("resolve");

        for (auto const& pair : hitIndices) {
            int x = pair.second[0];
            int y = pair.second[1];
            int i = y * _bufferSize[0] + x;

            HdxPickHit hit;
            if (_ResolveHit(i, x, y, _depths[i], &hit)) {
                allHits->push_back(hit);
            }
        }
    }
}

// -------------------------------------------------------------------------- //
// HdxPickHit
// -------------------------------------------------------------------------- //
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
    boost::hash_combine(hash, normalizedDepth);
    
    return hash;
}

// -------------------------------------------------------------------------- //
// Comparison, equality and logging
// -------------------------------------------------------------------------- //
bool
operator<(HdxPickHit const& lhs, HdxPickHit const& rhs)
{
    return lhs.normalizedDepth < rhs.normalizedDepth;
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
        && lhs.normalizedDepth == rhs.normalizedDepth;
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
        << "Depth: (" << h.normalizedDepth << ") ";
    return out;
}

bool
operator==(HdxPickTaskParams const& lhs, HdxPickTaskParams const& rhs)
{
    return lhs.cullStyle == rhs.cullStyle
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
        << p.cullStyle << " "
        << p.enableSceneMaterials;
    return out;
}

bool
operator==(HdxPickTaskContextParams const& lhs,
           HdxPickTaskContextParams const& rhs)
{
    using RawDepthMaskCallback = void (*) ();
    const RawDepthMaskCallback *lhsDepthMaskPtr =
        lhs.depthMaskCallback.target<RawDepthMaskCallback>();
    const RawDepthMaskCallback *rhsDepthMaskPtr =
        rhs.depthMaskCallback.target<RawDepthMaskCallback>();
    const RawDepthMaskCallback lhsDepthMask =
        lhsDepthMaskPtr ? *lhsDepthMaskPtr : nullptr;
    const RawDepthMaskCallback rhsDepthMask =
        rhsDepthMaskPtr ? *rhsDepthMaskPtr : nullptr;

    return lhs.resolution == rhs.resolution
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
    using RawDepthMaskCallback = void (*) ();
    const RawDepthMaskCallback *depthMaskPtr =
        p.depthMaskCallback.target<RawDepthMaskCallback>();
    const RawDepthMaskCallback depthMask =
        depthMaskPtr ? *depthMaskPtr : nullptr;

    out << "PickTask Context Params: (...) "
        << p.resolution << " "
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
