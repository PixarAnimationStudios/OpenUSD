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
#include <GL/glew.h>
#include "pxr/imaging/hgi/graphicsEncoderDesc.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/graphicsEncoder.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLGraphicsEncoder::HgiGLGraphicsEncoder(
    HgiGraphicsEncoderDesc const& desc)
    : HgiGraphicsEncoder()
{
    TF_VERIFY(desc.width>0 && desc.height>0);
}

HgiGLGraphicsEncoder::~HgiGLGraphicsEncoder()
{
}

void
HgiGLGraphicsEncoder::EndEncoding()
{
}

void
HgiGLGraphicsEncoder::SetViewport(GfVec4i const& vp)
{
    int x = vp[0];
    int y = vp[1];
    int w = vp[2];
    int h = vp[3];
    glViewport(x, y, w, h);
}

void
HgiGLGraphicsEncoder::PushDebugGroup(const char* label)
{
    #if defined(GL_KHR_debug)
        if (GLEW_KHR_debug) {
            glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, label);
        }

    #endif
}

void
HgiGLGraphicsEncoder::PopDebugGroup()
{
    #if defined(GL_KHR_debug)
        if (GLEW_KHR_debug) {
            glPopDebugGroup();
        }
    #endif
}


PXR_NAMESPACE_CLOSE_SCOPE
