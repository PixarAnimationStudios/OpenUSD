//
// Copyright 2020 benmalartre
//
// Unlicensed 
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/drawTarget.h"
#include "pxr/imaging/plugin/LoFi/drawTargetAttachmentDescArray.h"
#include "pxr/imaging/plugin/LoFi/drawTargetTextureResource.h"
#include "pxr/imaging/plugin/LoFi/glConversions.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/imaging/glf/glContext.h"

#include "pxr/base/tf/stl.h"

PXR_NAMESPACE_OPEN_SCOPE


static const std::string DEPTH_ATTACHMENT_NAME = "depth";

TF_DEFINE_PUBLIC_TOKENS(LoFiDrawTargetTokens, LOFI_DRAW_TARGET_TOKENS);

LoFiDrawTarget::LoFiDrawTarget(SdfPath const &id)
    : HdSprim(id)
    , _version(1) // Clients tacking start at 0.
    , _enabled(true)
    , _cameraId()
    , _resolution(512, 512)
    , _collection()
    , _renderPassState()
    , _drawTargetContext()
    , _drawTarget()

{
}

LoFiDrawTarget::~LoFiDrawTarget()
{
}

/*virtual*/
void
LoFiDrawTarget::Sync(HdSceneDelegate *sceneDelegate,
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
        VtValue vtValue =  sceneDelegate->Get(id, LoFiDrawTargetTokens->enable);

        // Optional attribute.
        _enabled = vtValue.GetWithDefault<bool>(true);
    }

    if (bits & DirtyDTCamera) {
        VtValue vtValue =  sceneDelegate->Get(id, LoFiDrawTargetTokens->camera);
        _cameraId = vtValue.Get<SdfPath>();
        _renderPassState.SetCamera(_cameraId);
    }

    if (bits & DirtyDTResolution) {
        VtValue vtValue =
                        sceneDelegate->Get(id, LoFiDrawTargetTokens->resolution);

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
                       sceneDelegate->Get(id, LoFiDrawTargetTokens->attachments);


        const LoFiDrawTargetAttachmentDescArray &attachments =
            vtValue.GetWithDefault<LoFiDrawTargetAttachmentDescArray>(
                    LoFiDrawTargetAttachmentDescArray());

        _SetAttachments(sceneDelegate, attachments);
    }


    if (bits & DirtyDTDepthClearValue) {
        VtValue vtValue =
                  sceneDelegate->Get(id, LoFiDrawTargetTokens->depthClearValue);

        float depthClearValue = vtValue.GetWithDefault<float>(1.0f);

        _renderPassState.SetDepthClearValue(depthClearValue);
    }

    if (bits & DirtyDTCollection) {
        VtValue vtValue =
                       sceneDelegate->Get(id, LoFiDrawTargetTokens->collection);

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
LoFiDrawTarget::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

bool
LoFiDrawTarget::WriteToFile(const HdRenderIndex &renderIndex,
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
    const GfMatrix4d &viewMatrix = camera->GetViewMatrix();
    const GfMatrix4d &projMatrix = camera->GetProjectionMatrix();

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
LoFiDrawTarget::_SetAttachments(
                           HdSceneDelegate *sceneDelegate,
                           const LoFiDrawTargetAttachmentDescArray &attachments)
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
      const LoFiDrawTargetAttachmentDesc &desc =
                                       attachments.GetAttachment(attachmentNum);

        GLenum format = GL_RGBA;
        GLenum type   = GL_BYTE;
        GLenum internalFormat = GL_RGBA8;
        LoFiGLConversions::GetGlFormat(desc.GetFormat(),
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

        LoFiDrawTargetTextureResource *resource =
                static_cast<LoFiDrawTargetTextureResource *>(
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
    _drawTarget->AddAttachment(DEPTH_ATTACHMENT_NAME,
                               GL_DEPTH_COMPONENT,
                               GL_FLOAT,
                               GL_DEPTH_COMPONENT32F);

    _RegisterTextureResourceHandle(sceneDelegate,
                             DEPTH_ATTACHMENT_NAME,
                             &_depthTextureResourceHandle);


    LoFiDrawTargetTextureResource* depthResource =
                static_cast<LoFiDrawTargetTextureResource*>(
                    _depthTextureResourceHandle->GetTextureResource().get());

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
LoFiDrawTarget::_GetCamera(const HdRenderIndex &renderIndex) const
{
    return static_cast<const HdCamera *>(
            renderIndex.GetSprim(HdPrimTypeTokens->camera, _cameraId));
}

void
LoFiDrawTarget::_ResizeDrawTarget()
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
LoFiDrawTarget::_RegisterTextureResourceHandle(
        HdSceneDelegate *sceneDelegate,
        const std::string &name,
        LoFiTextureResourceHandleSharedPtr *handlePtr)
{
    HF_MALLOC_TAG_FUNCTION();

    LoFiResourceRegistrySharedPtr const& resourceRegistry =
         std::static_pointer_cast<LoFiResourceRegistry>(
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
    HdInstance<LoFiTextureResourceSharedPtr> texInstance =
                resourceRegistry->RegisterTextureResource(texKey);

    if (texInstance.IsFirstInstance()) {
        texInstance.SetValue(LoFiTextureResourceSharedPtr(
                                         new LoFiDrawTargetTextureResource()));
    }

    LoFiTextureResourceSharedPtr texResource = texInstance.GetValue();

    HdResourceRegistry::TextureKey handleKey =
        LoFiTextureResourceHandle::GetHandleKey(&renderIndex, resourcePath);
    HdInstance<LoFiTextureResourceHandleSharedPtr> handleInstance =
                resourceRegistry->RegisterTextureResourceHandle(handleKey);
    if (handleInstance.IsFirstInstance()) {
        handleInstance.SetValue(LoFiTextureResourceHandleSharedPtr(   
                                          new LoFiTextureResourceHandle(
                                              texResource)));
    } else {
        handleInstance.GetValue()->SetTextureResource(texResource);
    }
    *handlePtr = handleInstance.GetValue();
}


/*static*/
void
LoFiDrawTarget::GetDrawTargets(HdRenderIndex* renderIndex,
                               LoFiDrawTargetPtrConstVector *drawTargets)
{
    HF_MALLOC_TAG_FUNCTION();

    SdfPathVector sprimPaths;

    if (renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->drawTarget)) {
        sprimPaths = renderIndex->GetSprimSubtree(HdPrimTypeTokens->drawTarget,
            SdfPath::AbsoluteRootPath());
    }

    TF_FOR_ALL (it, sprimPaths) {
        HdSprim const *drawTarget =
                        renderIndex->GetSprim(HdPrimTypeTokens->drawTarget, *it);

        if (drawTarget != nullptr)
        {
            drawTargets->push_back(static_cast<LoFiDrawTarget const *>(drawTarget));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

