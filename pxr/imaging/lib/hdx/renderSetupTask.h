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
#ifndef HDX_RENDER_SETUP_TASK_H
#define HDX_RENDER_SETUP_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/renderPassState.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdStRenderPassShader> HdStRenderPassShaderSharedPtr;
typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdxRenderSetupTask> HdxRenderSetupTaskSharedPtr;
typedef boost::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;
struct HdxRenderTaskParams;
class HdStRenderPassState;


/// \class HdxRenderSetupTask
///
/// A task for setting up render pass state (camera, renderpass shader, GL
/// states).
///
/// HdxRenderTask depends on the output of this task.  Applications can choose
/// to create a render setup task, and pass it the HdxRenderTaskParams; or they
/// can pass the HdxRenderTaskParams directly to the render task, which will
/// create a render setup task internally.  See the HdxRenderTask documentation
/// for details.
///
class HdxRenderSetupTask : public HdTask {
public:
    HDX_API
    HdxRenderSetupTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxRenderSetupTask();


    // APIs used from HdxRenderTask to manage the sync process.
    HDX_API
    void SyncParams(HdSceneDelegate* delegate,
                    HdxRenderTaskParams const &params);
    HDX_API
    void SyncCamera(HdSceneDelegate* delegate);
    HDX_API
    void SyncAovBindings(HdSceneDelegate* delegate);
    HDX_API
    void SyncRenderPassState(HdSceneDelegate* delegate);

    HdRenderPassStateSharedPtr const &GetRenderPassState() const {
        return _renderPassState;
    }
    TfTokenVector const &GetRenderTags() const {
        return _renderTags;
    }

    /// Sync the render pass resources
    HDX_API
    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override;

    /// Execute render pass task
    HDX_API
    virtual void Execute(HdTaskContext* ctx) override;


private:
    HdRenderPassStateSharedPtr _renderPassState;
    HdStRenderPassShaderSharedPtr _colorRenderPassShader;
    HdStRenderPassShaderSharedPtr _idRenderPassShader;
    GfVec4d _viewport;
    SdfPath _cameraId;
    TfTokenVector _renderTags;
    HdRenderPassAovBindingVector _aovBindings;

    static HdStShaderCodeSharedPtr _overrideShader;

    static void _CreateOverrideShader();

    void _SetHdStRenderPassState(HdxRenderTaskParams const& params,
                                 HdStRenderPassState *renderPassState);

    HdRenderPassStateSharedPtr &_GetRenderPassState(HdSceneDelegate* delegate);


    HdxRenderSetupTask() = delete;
    HdxRenderSetupTask(const HdxRenderSetupTask &) = delete;
    HdxRenderSetupTask &operator =(const HdxRenderSetupTask &) = delete;
};

/// \class HdxRenderTaskParams
///
/// RenderTask parameters (renderpass state).
///
struct HdxRenderTaskParams
{
    HdxRenderTaskParams()
        : overrideColor(0.0)
        , wireframeColor(0.0)
        , maskColor(1.0f, 0.0f, 0.0f, 1.0f)
        , indicatorColor(0.0f, 1.0f, 0.0f, 1.0f)
        , pointColor(GfVec4f(0,0,0,1))
        , pointSize(3.0)
        , pointSelectedSize(3.0)
        , enableLighting(false)
        , enableIdRender(false)
        , alphaThreshold(0.0)
        , enableSceneMaterials(true)
        , renderTags()
        , depthBiasUseDefault(true)
        , depthBiasEnable(false)
        , depthBiasConstantFactor(0.0f)
        , depthBiasSlopeFactor(1.0f)
        , depthFunc(HdCmpFuncLEqual)
        , stencilFunc(HdCmpFuncAlways)
        , stencilRef(0)
        , stencilMask(~0)
        , stencilFailOp(HdStencilOpKeep)
        , stencilZFailOp(HdStencilOpKeep)
        , stencilZPassOp(HdStencilOpKeep)
        , stencilEnable(false)
        , blendColorOp(HdBlendOpAdd)
        , blendColorSrcFactor(HdBlendFactorOne)
        , blendColorDstFactor(HdBlendFactorZero)
        , blendAlphaOp(HdBlendOpAdd)
        , blendAlphaSrcFactor(HdBlendFactorOne)
        , blendAlphaDstFactor(HdBlendFactorZero)
        , blendConstantColor(0.0f, 0.0f, 0.0f, 0.0f)
        , blendEnable(false)
        , cullStyle(HdCullStyleBackUnlessDoubleSided)
        , aovBindings()
        , camera()
        , viewport(0.0)
        {}

    // XXX: Several of the params below should move to global application state.
    GfVec4f overrideColor;
    GfVec4f wireframeColor;
    GfVec4f maskColor;
    GfVec4f indicatorColor;
    GfVec4f pointColor;
    float pointSize;
    float pointSelectedSize;
    bool enableLighting;
    bool enableIdRender;
    float alphaThreshold;
    bool enableSceneMaterials;
    TfTokenVector renderTags;

    // Depth Bias Raster State
    // When use default is true - state
    // is inherited and onther values are
    // ignored.  Otherwise the raster state
    // is set using the values specified.
    bool depthBiasUseDefault;
    bool depthBiasEnable;
    float depthBiasConstantFactor;
    float depthBiasSlopeFactor;

    HdCompareFunction depthFunc;

    // Stencil
    HdCompareFunction stencilFunc;
    int stencilRef;
    int stencilMask;
    HdStencilOp stencilFailOp;
    HdStencilOp stencilZFailOp;
    HdStencilOp stencilZPassOp;
    bool stencilEnable;

    // Blending
    HdBlendOp blendColorOp;
    HdBlendFactor blendColorSrcFactor;
    HdBlendFactor blendColorDstFactor;
    HdBlendOp blendAlphaOp;
    HdBlendFactor blendAlphaSrcFactor;
    HdBlendFactor blendAlphaDstFactor;
    GfVec4f blendConstantColor;
    bool blendEnable;

    // Viewer's Render Style
    HdCullStyle cullStyle;

    // AOV bindings.
    // XXX: As a transitional API, if this is empty it indicates the renderer
    // should write color and depth to the GL framebuffer.
    HdRenderPassAovBindingVector aovBindings;

    // RasterState index objects
    SdfPath camera;
    GfVec4d viewport;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxRenderTaskParams& pv);
HDX_API
bool operator==(const HdxRenderTaskParams& lhs, const HdxRenderTaskParams& rhs);
HDX_API
bool operator!=(const HdxRenderTaskParams& lhs, const HdxRenderTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDX_RENDER_SETUP_TASK_H
