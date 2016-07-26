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
#include "pxr/imaging/hd/drawTarget.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/conversions.h"
#include "pxr/imaging/hd/camera.h"

#include "pxr/imaging/hd/drawTargetAttachmentDescArray.h"

#include "pxr/base/tf/stl.h"


#include "pxr/imaging/glf/glContext.h"

static const std::string DEPTH_ATTACHMENT_NAME = "depth";

HdDrawTarget::HdDrawTarget(HdSceneDelegate *delegate, SdfPath const &id)
    : _delegate(delegate)
    , _id(id)
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

HdDrawTarget::~HdDrawTarget()
{
}

void
HdDrawTarget::Sync()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_delegate)) {
        return;  
    } 

    HdChangeTracker& changeTracker = 
                                _delegate->GetRenderIndex().GetChangeTracker();
    HdChangeTracker::DirtyBits bits = changeTracker.GetDrawTargetDirtyBits(_id);


    if (bits & HdChangeTracker::DirtyDTEnable) {
        VtValue vtValue =  _delegate->Get(_id, HdTokens->enable);

        // Optional attribute.
        _enabled = vtValue.GetWithDefault<bool>(true);
    }

    if (bits & HdChangeTracker::DirtyDTCamera) {
        VtValue vtValue =  _delegate->Get(_id, HdTokens->camera);
        _cameraId = vtValue.Get<SdfPath>();
        _renderPassState.SetCamera(_cameraId);
    }

    if (bits & HdChangeTracker::DirtyDTResolution) {
        VtValue vtValue =  _delegate->Get(_id, HdTokens->resolution);

        _resolution = vtValue.Get<GfVec2i>();

        // No point in Resizing the textures if new ones are going to
        // be created (see _SetAttachments())
        if (_drawTarget &&
            HdChangeTracker::IsClean(bits & HdChangeTracker::DirtyDTAttachment)) {
            _ResizeDrawTarget();
        }
    }

    if (bits & HdChangeTracker::DirtyDTAttachment) {
        // Depends on resolution being set correctly.
        VtValue vtValue =  _delegate->Get(_id, HdTokens->attachments);


        const HdDrawTargetAttachmentDescArray &attachments =
            vtValue.GetWithDefault<HdDrawTargetAttachmentDescArray>(
                    HdDrawTargetAttachmentDescArray());

        _SetAttachments(attachments);
    }


    if (bits & HdChangeTracker::DirtyDTDepthClearValue) {
        VtValue vtValue =  _delegate->Get(_id, HdTokens->depthClearValue);

        float depthClearValue = vtValue.GetWithDefault<float>(1.0f);

        _renderPassState.SetDepthClearValue(depthClearValue);
    }

    if (bits & HdChangeTracker::DirtyDTCollection) {
        VtValue vtValue =  _delegate->Get(_id, HdTokens->collection);

        const HdRprimCollectionVector &collections =
                vtValue.GetWithDefault<HdRprimCollectionVector>(
                                                     HdRprimCollectionVector());

        _collections = collections;
        size_t newColSize     = collections.size();
        for (size_t colNum = 0; colNum < newColSize; ++colNum) {
            TfToken const &currentName = _collections[colNum].GetName();

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
}


bool
HdDrawTarget::WriteToFile(const std::string &attachment,
                          const std::string &path)
{
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

    HdCameraSharedPtr camera = _GetCamera();
    if (!camera) {
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
HdDrawTarget::_SetAttachments(const HdDrawTargetAttachmentDescArray &attachments)
{
    if (!_drawTargetContext) {
        // Use one of the shared contexts as the master.
        _drawTargetContext = GlfGLContext::GetSharedGLContext();
    }

    // Make sure all draw target operations happen on the same
    // context.
    GlfGLContextSharedPtr oldContext = GlfGLContext::GetCurrentGLContext();

    GlfGLContext::MakeCurrent(_drawTargetContext);

    // XXX: Discard old draw target and create a new one
    // This is necessary because a we have to clone the draw target into each
    // gl context.
    _drawTarget = GlfDrawTarget::New(_resolution);

    size_t numAttachments = attachments.GetNumAttachments();
    _renderPassState.SetNumColorAttachments(numAttachments);

    _drawTarget->Bind();

    for (size_t attachmentNum = 0; attachmentNum < numAttachments;
                                                              ++attachmentNum) {
      const HdDrawTargetAttachmentDesc &desc =
                                       attachments.GetAttachment(attachmentNum);

        GLenum format = GL_RGBA;
        GLenum type   = GL_BYTE;
        GLenum internalFormat = GL_RGBA8;
        HdConversions::GetGlFormat(desc.GetFormat(),
                                   &format, &type, &internalFormat);


        _drawTarget->AddAttachment(desc.GetName(),
                                   format,
                                   type,
                                   internalFormat);

        _renderPassState.SetColorClearValue(attachmentNum, desc.GetClearColor());
    }

    // Always add depth texture
    // XXX: GlfDrawTarget requires the depth texture be added last,
    // otherwise the draw target indexes are off-by-1.
    _drawTarget->AddAttachment(DEPTH_ATTACHMENT_NAME,
                               GL_DEPTH_COMPONENT,
                               GL_FLOAT,
                               GL_DEPTH_COMPONENT32F);

   _drawTarget->Unbind();

   GlfGLContext::MakeCurrent(oldContext);

   // The texture bindings have changed so increment the version
   ++_version;
}


HdCameraSharedPtr
HdDrawTarget::_GetCamera() const
{
    return _delegate->GetRenderIndex().GetCamera(_cameraId);
}

void
HdDrawTarget::_ResizeDrawTarget()
{
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
