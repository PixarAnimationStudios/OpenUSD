//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hdSt/drawTarget.h"
#include "pxr/imaging/hdSt/drawTargetAttachmentDescArray.h"
#include "pxr/imaging/hdSt/drawTargetTextureResource.h"
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/imaging/glf/glContext.h"

#include "pxr/base/tf/stl.h"

PXR_NAMESPACE_OPEN_SCOPE


static const std::string DEPTH_ATTACHMENT_NAME = "depth";

TF_DEFINE_PUBLIC_TOKENS(HdStDrawTargetTokens, HDST_DRAW_TARGET_TOKENS);

HdStDrawTarget::HdStDrawTarget(SdfPath const &id)
    : HdSprim(id)
    , _version(1) // Clients tacking start at 0.
    , _enabled(true)
    , _cameraId()
    , _resolution(512, 512)
    , _collections()
    , _renderPassState()
    , _drawTargetContext()
    , _drawTarget()

{
}

HdStDrawTarget::~HdStDrawTarget()
{
}

/*virtual*/
void
HdStDrawTarget::Sync(HdSceneDelegate *sceneDelegate,
                     HdRenderParam   *renderParam,
                     HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    SdfPath const &id = GetId();
    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    HdDirtyBits bits = *dirtyBits;

    if (bits & DirtyDTEnable) {
        VtValue vtValue =  sceneDelegate->Get(id, HdStDrawTargetTokens->enable);

        // Optional attribute.
        _enabled = vtValue.GetWithDefault<bool>(true);
    }

    if (bits & DirtyDTCamera) {
        VtValue vtValue =  sceneDelegate->Get(id, HdStDrawTargetTokens->camera);
        _cameraId = vtValue.Get<SdfPath>();
        _renderPassState.SetCamera(_cameraId);
    }

    if (bits & DirtyDTResolution) {
        VtValue vtValue =
                        sceneDelegate->Get(id, HdStDrawTargetTokens->resolution);

        _resolution = vtValue.Get<GfVec2i>();

        // No point in Resizing the textures if new ones are going to
        // be created (see _SetAttachments())
        if (_drawTarget && ((bits & DirtyDTAttachment) == Clean)) {
            _ResizeDrawTarget();
        }
    }

    if (bits & DirtyDTAttachment) {
        // Depends on resolution being set correctly.
        VtValue vtValue =
                       sceneDelegate->Get(id, HdStDrawTargetTokens->attachments);


        const HdStDrawTargetAttachmentDescArray &attachments =
            vtValue.GetWithDefault<HdStDrawTargetAttachmentDescArray>(
                    HdStDrawTargetAttachmentDescArray());

        _SetAttachments(sceneDelegate, attachments);
    }


    if (bits & DirtyDTDepthClearValue) {
        VtValue vtValue =
                   sceneDelegate->Get(id, HdStDrawTargetTokens->depthClearValue);

        float depthClearValue = vtValue.GetWithDefault<float>(1.0f);

        _renderPassState.SetDepthClearValue(depthClearValue);
    }

    if (bits & DirtyDTCollection) {
        VtValue vtValue =
                        sceneDelegate->Get(id, HdStDrawTargetTokens->collection);

        const HdRprimCollectionVector &collections =
                vtValue.GetWithDefault<HdRprimCollectionVector>(
                                                     HdRprimCollectionVector());

        _collections = collections;
        size_t newColSize     = collections.size();
        for (size_t colNum = 0; colNum < newColSize; ++colNum) {
            TfToken const &currentName = _collections[colNum].GetName();

            HdChangeTracker& changeTracker =
                             sceneDelegate->GetRenderIndex().GetChangeTracker();

            changeTracker.MarkCollectionDirty(currentName);
        }

        if (newColSize > 0)
        {
            // XXX:  Draw Targets currently only support a single collection right
            // now as each collect requires it's own render pass and then
            // it becomes a complex matrix of values as we have race needing to
            // know the number of attachments and number of render passes to
            // handle clear color and keeping that all in sync
            if (_collections.size() != 1) {
                TF_CODING_ERROR("Draw targets currently supports only a "
                                "single collection");
            }

            _renderPassState.SetRprimCollection(_collections[0]);
        }
    }

    *dirtyBits = Clean;
}

// virtual
HdDirtyBits
HdStDrawTarget::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

bool
HdStDrawTarget::WriteToFile(const HdRenderIndex &renderIndex,
                           const std::string &attachment,
                           const std::string &path) const
{
    HF_MALLOC_TAG_FUNCTION();

    // Check the draw targets been allocated
    if (!_drawTarget || !_drawTargetContext) {
        TF_WARN("Missing draw target");
        return false;
    }

    // XXX: The GlfDrawTarget will throw an error if attachment is invalid,
    // so need to check that it is valid first.
    //
    // This ends in a double-search of the map, but this path is for
    // debug and testing and not meant to be a performance path.
    if (!_drawTarget->GetAttachment(attachment)) {
        TF_WARN("Missing attachment\n");
        return false;
    }

    const HdCamera *camera = _GetCamera(renderIndex);
    if (camera == nullptr) {
        TF_WARN("Missing camera\n");
        return false;
    }


    // embed camera matrices into metadata
    VtValue viewMatrixVt  = camera->Get(HdShaderTokens->worldToViewMatrix);
    VtValue projMatrixVt  = camera->Get(HdShaderTokens->projectionMatrix);
    const GfMatrix4d &viewMatrix = viewMatrixVt.Get<GfMatrix4d>();
    const GfMatrix4d &projMatrix = projMatrixVt.Get<GfMatrix4d>();

    // Make sure all draw target operations happen on the same
    // context.
    GlfGLContextSharedPtr oldContext = GlfGLContext::GetCurrentGLContext();
    GlfGLContext::MakeCurrent(_drawTargetContext);

    bool result = _drawTarget->WriteToFile(attachment, path,
                                           viewMatrix, projMatrix);

    GlfGLContext::MakeCurrent(oldContext);

    return result;
}

void
HdStDrawTarget::_SetAttachments(
                           HdSceneDelegate *sceneDelegate,
                           const HdStDrawTargetAttachmentDescArray &attachments)
{
    HF_MALLOC_TAG_FUNCTION();

    if (!_drawTargetContext) {
        // Use one of the shared contexts as the master.
        _drawTargetContext = GlfGLContext::GetSharedGLContext();
    }

    // Clear out old texture resources for the attachments.
    _colorTextureResources.clear();
    _depthTextureResource.reset();


    // Make sure all draw target operations happen on the same
    // context.
    GlfGLContextSharedPtr oldContext = GlfGLContext::GetCurrentGLContext();

    GlfGLContext::MakeCurrent(_drawTargetContext);


    if (_drawTarget) {
        // If we had a prior draw target, we need to garbage collect
        // to clean up it's resources.
        HdChangeTracker& changeTracker =
                         sceneDelegate->GetRenderIndex().GetChangeTracker();

        changeTracker.SetGarbageCollectionNeeded();
    }

    // XXX: Discard old draw target and create a new one
    // This is necessary because a we have to clone the draw target into each
    // gl context.
    // XXX : All draw targets in Hydra are currently trying to create MSAA
    // buffers (as long as they are allowed by the environment variables) 
    // because we need alpha to coverage for transparent object.
    _drawTarget = GlfDrawTarget::New(_resolution, /* MSAA */ true);

    size_t numAttachments = attachments.GetNumAttachments();
    _renderPassState.SetNumColorAttachments(numAttachments);

    _drawTarget->Bind();

    _colorTextureResources.resize(numAttachments);

    for (size_t attachmentNum = 0; attachmentNum < numAttachments;
                                                              ++attachmentNum) {
      const HdStDrawTargetAttachmentDesc &desc =
                                       attachments.GetAttachment(attachmentNum);

        GLenum format = GL_RGBA;
        GLenum type   = GL_BYTE;
        GLenum internalFormat = GL_RGBA8;
        HdStGLConversions::GetGlFormat(desc.GetFormat(),
                                   &format, &type, &internalFormat);

        const std::string &name = desc.GetName();
        _drawTarget->AddAttachment(name,
                                   format,
                                   type,
                                   internalFormat);

        _renderPassState.SetColorClearValue(attachmentNum, desc.GetClearColor());

        _RegisterTextureResource(sceneDelegate,
                                 name,
                                 &_colorTextureResources[attachmentNum]);

        HdSt_DrawTargetTextureResource *resource =
                static_cast<HdSt_DrawTargetTextureResource *>(
                                 _colorTextureResources[attachmentNum].get());

        resource->SetAttachment(_drawTarget->GetAttachment(name));
        resource->SetSampler(desc.GetWrapS(),
                             desc.GetWrapT(),
                             desc.GetMinFilter(),
                             desc.GetMagFilter());

    }

    // Always add depth texture
    // XXX: GlfDrawTarget requires the depth texture be added last,
    // otherwise the draw target indexes are off-by-1.
    _drawTarget->AddAttachment(DEPTH_ATTACHMENT_NAME,
                               GL_DEPTH_COMPONENT,
                               GL_FLOAT,
                               GL_DEPTH_COMPONENT32F);

    _RegisterTextureResource(sceneDelegate,
                             DEPTH_ATTACHMENT_NAME,
                             &_depthTextureResource);


    HdSt_DrawTargetTextureResource *depthResource =
                    static_cast<HdSt_DrawTargetTextureResource *>(
                                                  _depthTextureResource.get());

    depthResource->SetAttachment(_drawTarget->GetAttachment(DEPTH_ATTACHMENT_NAME));
    depthResource->SetSampler(attachments.GetDepthWrapS(),
                              attachments.GetDepthWrapT(),
                              attachments.GetDepthMinFilter(),
                              attachments.GetDepthMagFilter());
   _drawTarget->Unbind();

   _renderPassState.SetDepthPriority(attachments.GetDepthPriority());

   GlfGLContext::MakeCurrent(oldContext);

   // The texture bindings have changed so increment the version
   ++_version;
}


const HdCamera *
HdStDrawTarget::_GetCamera(const HdRenderIndex &renderIndex) const
{
    return static_cast<const HdCamera *>(
            renderIndex.GetSprim(HdPrimTypeTokens->camera, _cameraId));
}

void
HdStDrawTarget::_ResizeDrawTarget()
{
    HF_MALLOC_TAG_FUNCTION();

    // Make sure all draw target operations happen on the same
    // context.
    GlfGLContextSharedPtr oldContext = GlfGLContext::GetCurrentGLContext();

    GlfGLContext::MakeCurrent(_drawTargetContext);

    _drawTarget->Bind();
    _drawTarget->SetSize(_resolution);
    _drawTarget->Unbind();

    // The texture bindings might have changed so increment the version
    ++_version;

    GlfGLContext::MakeCurrent(oldContext);
}

void
HdStDrawTarget::_RegisterTextureResource(
                                        HdSceneDelegate *sceneDelegate,
                                        const std::string &name,
                                        HdTextureResourceSharedPtr *resourcePtr)
{
    HF_MALLOC_TAG_FUNCTION();

    HdResourceRegistrySharedPtr const& resourceRegistry =
        sceneDelegate->GetRenderIndex().GetResourceRegistry();

    // Create Path for the texture resource
    SdfPath resourcePath = GetId().AppendProperty(TfToken(name));

    // Ask delegate for an ID for this tex
    HdTextureResource::ID texID =
                              sceneDelegate->GetTextureResourceID(resourcePath);

    // Use render index to convert local texture id into global
    // texture key.  This is because the instance registry is shared by
    // multiple render indexes, but the scene delegate generated
    // texture id's are only unique to the scene.  (i.e. two draw
    // targets at the same path in the scene are likely to produce the
    // same texture id, even though they refer to textures on different
    // render indexes).
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdResourceRegistry::TextureKey texKey = renderIndex.GetTextureKey(texID);


    // Add to resource registry
    HdInstance<HdResourceRegistry::TextureKey,
               HdTextureResourceSharedPtr> texInstance;
    std::unique_lock<std::mutex> regLock =
                  resourceRegistry->RegisterTextureResource(texKey,
                                                            &texInstance);

    if (texInstance.IsFirstInstance()) {
        texInstance.SetValue(HdTextureResourceSharedPtr(
                                         new HdSt_DrawTargetTextureResource()));
    }

    *resourcePtr =  texInstance.GetValue();
}


/*static*/
void
HdStDrawTarget::GetDrawTargets(HdSceneDelegate *sceneDelegate,
                               HdStDrawTargetPtrConstVector *drawTargets)
{
    HF_MALLOC_TAG_FUNCTION();

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    SdfPathVector sprimPaths;

    if (renderIndex.IsSprimTypeSupported(HdPrimTypeTokens->drawTarget)) {
        sprimPaths = renderIndex.GetSprimSubtree(HdPrimTypeTokens->drawTarget,
            SdfPath::AbsoluteRootPath());
    }

    TF_FOR_ALL (it, sprimPaths) {
        HdSprim const *drawTarget =
                        renderIndex.GetSprim(HdPrimTypeTokens->drawTarget, *it);

        if (drawTarget != nullptr)
        {
            drawTargets->push_back(static_cast<HdStDrawTarget const *>(drawTarget));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

