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

#include "pxr/imaging/hdx/drawTargetResolveTask.h"
#include "pxr/imaging/hdx/drawTarget.h"

#include "pxr/imaging/glf/drawTarget.h"

HdxDrawTargetResolveTask::HdxDrawTargetResolveTask(HdSceneDelegate* delegate,
                                                   SdfPath const& id)
 : HdSceneTask(delegate, id)
{
}

void
HdxDrawTargetResolveTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();
}

void
HdxDrawTargetResolveTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    HdSceneDelegate* delegate = GetDelegate();

    HdxDrawTargetSharedPtrVector drawTargets;
    HdxDrawTarget::GetDrawTargets(delegate, &drawTargets);

    size_t numDrawTargets = drawTargets.size();

    if (numDrawTargets > 0) {
    
        // Store the current framebuffers so we can set them back after blitting
        GLuint rfb, dfb;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint*)&rfb);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&dfb);

        // XXX : This can be optimized if we pass an array of current 
        // draw targets from drawTargetTask.cpp via the HdxTaskContext
        for (size_t i = 0; i < numDrawTargets; ++i) {
            HdxDrawTargetSharedPtr drawTarget = drawTargets[i];

            if (drawTarget and drawTarget->IsEnabled()) {
                
                // If the fbo has msaa attachments let's resolve them now
                GlfDrawTargetRefPtr glfdrawtarget = drawTarget->GetGlfDrawTarget();
                if (glfdrawtarget->HasMSAA()) {
                    GlfDrawTargetRefPtr contextFriendlyDrawTarget = 
                        GlfDrawTarget::New(glfdrawtarget);
                    contextFriendlyDrawTarget->Resolve();
                }
            }
        }

        // Restore the FBOs
        glBindFramebuffer(GL_READ_FRAMEBUFFER, rfb);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dfb);
    }
}
