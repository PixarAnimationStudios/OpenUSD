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
#ifndef HDX_COLORCHANNEL_TASK_H
#define HDX_COLORCHANNEL_TASK_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/garch/gl.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class HdStGLSLProgram;
typedef boost::shared_ptr<class HdStGLSLProgram> HdStGLSLProgramSharedPtr;
typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;


/// \class HdxColorChannelTask
///
/// A task for choosing a color channel for display.
///
class HdxColorChannelTask : public HdTask
{
public:
    HDX_API
    HdxColorChannelTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxColorChannelTask();

    /// Sync the render pass resources
    HDX_API
    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override;

    /// Prepare the tasks resources
    HDX_API
    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override;

    /// Execute the color channel task
    HDX_API
    virtual void Execute(HdTaskContext* ctx) override;

private:
    HdxColorChannelTask() = delete;
    HdxColorChannelTask(const HdxColorChannelTask &) = delete;
    HdxColorChannelTask &operator =(const HdxColorChannelTask &) = delete;

    // Utility function to create the GL program for color channel
    bool _CreateShaderResources();

    // Utility function to create buffer resources.
    bool _CreateBufferResources();

    // Utility function to setup the copy-framebuffer
    bool _CreateFramebufferResources();

    // Copies the client framebuffer texture into ours
    void _CopyTexture();

    /// Apply the color channel filtering to the currently bound framebuffer.
    void _ApplyColorChannel();
    
    // Get an integer that represents the color channel. This can be used to
    // pass the color channel option as a uniform uint argument of the glsl
    // shader (see the `#define CHANNEL_*` lines in the shader). 
    // If _channel contains an invalid entry this will return 'color'.
    GLint _GetChannelAsGLint();

    HdStGLSLProgramSharedPtr _shaderProgram;
    GLuint _texture;
    GfVec2i _textureSize;
    GLint _locations[5];
    GLuint _vertexBuffer;

    // XXX: Removed due to slowness in the IsCurrent() call when multiple
    //      gl contexts are registered in GlfGLContextRegistry.
    // GlfGLContextSharedPtr _owningContext;

    GLuint _copyFramebuffer;
    GfVec2i _framebufferSize;

    // The color channel to be rendered (see HdxColorChannelTokens for the
    // possible values).
    TfToken _channel;
};


/// \class HdxColorChannelTaskParams
///
/// ColorChannelTask parameters.
///
struct HdxColorChannelTaskParams
{
    HdxColorChannelTaskParams() {}
    
    // Resolution of bound framebuffer we are color correcting.
    // This must be set if the viewport and framebuffer do not match.
    GfVec2i framebufferSize = GfVec2i(0);

    // Specifies which output color channel should be drawn. Defaults to 'color'
    // (untouched RGBA).
    TfToken channel = HdxColorChannelTokens->color;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxColorChannelTaskParams& pv);
HDX_API
bool operator==(const HdxColorChannelTaskParams& lhs,
                const HdxColorChannelTaskParams& rhs);
HDX_API
bool operator!=(const HdxColorChannelTaskParams& lhs,
                const HdxColorChannelTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
