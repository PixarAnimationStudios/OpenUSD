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
#include "pxr/imaging/hdSt/hgiConversions.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/dynamicUvTextureObject.h"
#include "pxr/imaging/hdSt/subtextureIdentifier.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/imaging/glf/glContext.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(HDST_USE_STORM_TEXTURE_SYSTEM_FOR_DRAW_TARGETS, true,
                      "Use Storm texture system for draw targets.");

TF_DEFINE_PUBLIC_TOKENS(HdStDrawTargetTokens, HDST_DRAW_TARGET_TOKENS);

HdStDrawTarget::HdStDrawTarget(SdfPath const &id)
    : HdSprim(id)
    , _version(1) // Clients tacking start at 0.
    , _enabled(true)
    , _resolution(512, 512)
    , _depthClearValue(1.0)
{
}

HdStDrawTarget::~HdStDrawTarget() = default;

/*static*/
bool
HdStDrawTarget::GetUseStormTextureSystem()
{
    static bool result =
        TfGetEnvSetting(HDST_USE_STORM_TEXTURE_SYSTEM_FOR_DRAW_TARGETS);
    return result;
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

    const HdDirtyBits bits = *dirtyBits;

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
        const VtValue vtValue =
            sceneDelegate->Get(id, HdStDrawTargetTokens->resolution);
        
        // The resolution is needed to set the viewport and compute the
        // camera projection matrix (more precisely, to do the aspect ratio
        // adjustment).
        //
        // Note that it is also stored in the render buffers. This is
        // somewhat redundant but it would be complicated for the draw
        // target to reach through to the render buffers to get the
        // resolution and that conceptually, the view port and camera
        // projection matrix are different from the texture
        // resolution.
        _resolution = vtValue.Get<GfVec2i>();

        if (!GetUseStormTextureSystem()) {
            // No point in Resizing the textures if new ones are going to
            // be created (see _SetAttachments())
            if (_drawTarget && ((bits & DirtyDTAttachment) == Clean)) {
                _ResizeDrawTarget();
            }
        }
    }

    if (GetUseStormTextureSystem()) {
        if (bits & DirtyDTAovBindings) {
            const VtValue aovBindingsValue =
                sceneDelegate->Get(id, HdStDrawTargetTokens->aovBindings);
            _renderPassState.SetAovBindings(
                aovBindingsValue.GetWithDefault<HdRenderPassAovBindingVector>(
                    {}));
        }

        if (bits & DirtyDTDepthPriority) {
            const VtValue depthPriorityValue =
                sceneDelegate->Get(id, HdStDrawTargetTokens->depthPriority);
            _renderPassState.SetDepthPriority(
                depthPriorityValue.GetWithDefault<HdDepthPriority>(
                    HdDepthPriorityNearest));
        }
    } else {
        if (bits & DirtyDTAttachment) {
            // Depends on resolution being set correctly.
            const VtValue vtValue =
                sceneDelegate->Get(id, HdStDrawTargetTokens->attachments);
            
            const HdStDrawTargetAttachmentDescArray &attachments =
                vtValue.GetWithDefault<HdStDrawTargetAttachmentDescArray>(
                    HdStDrawTargetAttachmentDescArray());
            
            _SetAttachments(sceneDelegate, attachments);
        }

        if (bits & DirtyDTDepthClearValue) {
            const VtValue vtValue =
                sceneDelegate->Get(id, HdStDrawTargetTokens->depthClearValue);
            
            _depthClearValue = vtValue.GetWithDefault<float>(1.0f);
            _renderPassState.SetDepthClearValue(_depthClearValue);
        }
    }
        
    if (bits & DirtyDTCollection) {
        VtValue vtValue =
                       sceneDelegate->Get(id, HdStDrawTargetTokens->collection);

        HdRprimCollection collection = vtValue.Get<HdRprimCollection>();

        TfToken const &collectionName = collection.GetName();

        HdChangeTracker& changeTracker =
                         sceneDelegate->GetRenderIndex().GetChangeTracker();

        if (_collection.GetName() != collectionName) {
            // Make sure collection has been added to change tracker
            changeTracker.AddCollection(collectionName);
        }

        // Always mark collection dirty even if added - as we don't
        // know if this is a re-add.
        changeTracker.MarkCollectionDirty(collectionName);

        _renderPassState.SetRprimCollection(collection);
        _collection = collection;
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

    const HdCamera * const camera = _GetCamera(renderIndex);
    if (camera == nullptr) {
        TF_WARN("Missing camera\n");
        return false;
    }


    // embed camera matrices into metadata
    const GfMatrix4d &viewMatrix = camera->GetViewMatrix();
    const GfMatrix4d &projMatrix = camera->GetProjectionMatrix();

    // Make sure all draw target operations happen on the same
    // context.
    GlfGLContextSharedPtr const oldContext =
        GlfGLContext::GetCurrentGLContext();
    GlfGLContext::MakeCurrent(_drawTargetContext);

    const bool result = _drawTarget->WriteToFile(attachment, path,
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
    _colorTextureResourceHandles.clear();
    _depthTextureResourceHandle.reset();


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
    // XXX : All draw targets in Storm are currently trying to create MSAA
    // buffers (as long as they are allowed by the environment variables) 
    // because we need alpha to coverage for transparent object.
    _drawTarget = GlfDrawTarget::New(_resolution, /* MSAA */ true);

    size_t numAttachments = attachments.GetNumAttachments();
    _renderPassState.SetNumColorAttachments(numAttachments);

    _drawTarget->Bind();

    _colorTextureResourceHandles.resize(numAttachments);

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

        _RegisterTextureResourceHandle(sceneDelegate,
                                 name,
                                 &_colorTextureResourceHandles[attachmentNum]);

        HdSt_DrawTargetTextureResource *resource =
                static_cast<HdSt_DrawTargetTextureResource *>(
                    _colorTextureResourceHandles[attachmentNum]->
                        GetTextureResource().get());

        resource->SetAttachment(_drawTarget->GetAttachment(name));
        resource->SetSampler(desc.GetWrapS(),
                             desc.GetWrapT(),
                             desc.GetMinFilter(),
                             desc.GetMagFilter());

    }

    // Always add depth texture
    // XXX: GlfDrawTarget requires the depth texture be added last,
    // otherwise the draw target indexes are off-by-1.
    _drawTarget->AddAttachment(HdStDrawTargetTokens->depth.GetString(),
                               GL_DEPTH_COMPONENT,
                               GL_FLOAT,
                               GL_DEPTH_COMPONENT32F);

    _RegisterTextureResourceHandle(sceneDelegate,
                                   HdStDrawTargetTokens->depth.GetString(),
                                   &_depthTextureResourceHandle);


    HdSt_DrawTargetTextureResource *depthResource =
                static_cast<HdSt_DrawTargetTextureResource *>(
                    _depthTextureResourceHandle->GetTextureResource().get());

    depthResource->SetAttachment(
        _drawTarget->GetAttachment(
            HdStDrawTargetTokens->depth.GetString()));
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
HdStDrawTarget::_RegisterTextureResourceHandle(
        HdSceneDelegate *sceneDelegate,
        const std::string &name,
        HdStTextureResourceHandleSharedPtr *handlePtr)
{
    HF_MALLOC_TAG_FUNCTION();

    HdStResourceRegistrySharedPtr const& resourceRegistry =
         std::static_pointer_cast<HdStResourceRegistry>(
             sceneDelegate->GetRenderIndex().GetResourceRegistry());

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
    HdInstance<HdStTextureResourceSharedPtr> texInstance =
                resourceRegistry->RegisterTextureResource(texKey);

    if (texInstance.IsFirstInstance()) {
        texInstance.SetValue(HdStTextureResourceSharedPtr(
                                         new HdSt_DrawTargetTextureResource()));
    }

    HdStTextureResourceSharedPtr texResource = texInstance.GetValue();

    HdResourceRegistry::TextureKey handleKey =
        HdStTextureResourceHandle::GetHandleKey(&renderIndex, resourcePath);
    HdInstance<HdStTextureResourceHandleSharedPtr> handleInstance =
                resourceRegistry->RegisterTextureResourceHandle(handleKey);
    if (handleInstance.IsFirstInstance()) {
        handleInstance.SetValue(HdStTextureResourceHandleSharedPtr(   
                                          new HdStTextureResourceHandle(
                                              texResource)));
    } else {
        handleInstance.GetValue()->SetTextureResource(texResource);
    }
    *handlePtr = handleInstance.GetValue();
}


/*static*/
void
HdStDrawTarget::GetDrawTargets(HdRenderIndex* const renderIndex,
                               HdStDrawTargetPtrVector * const drawTargets)
{
    HF_MALLOC_TAG_FUNCTION();

    if (!renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->drawTarget)) {
        return;
    }

    const SdfPathVector paths = renderIndex->GetSprimSubtree(
        HdPrimTypeTokens->drawTarget, SdfPath::AbsoluteRootPath());

    for (const SdfPath &path : paths) {
        if (HdSprim * const drawTarget =
                renderIndex->GetSprim(HdPrimTypeTokens->drawTarget, path)) {
            drawTargets->push_back(static_cast<HdStDrawTarget *>(drawTarget));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

