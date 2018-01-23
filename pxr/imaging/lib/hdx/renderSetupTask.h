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

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

PXR_NAMESPACE_OPEN_SCOPE


typedef std::shared_ptr<class HdStRenderPassShader> HdStRenderPassShaderSharedPtr;
typedef std::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef std::shared_ptr<class HdxRenderSetupTask> HdxRenderSetupTaskSharedPtr;
typedef std::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;
struct HdxRenderTaskParams;
class HdStRenderPassState;


/// \class HdxRenderSetupTask
///
/// A task for setting up render pass state (camera, renderpass shader, GL
/// states).
///
class HdxRenderSetupTask : public HdSceneTask {
public:
    HDX_API
    HdxRenderSetupTask(HdSceneDelegate* delegate, SdfPath const& id);

    // compatibility APIs used from HdxRenderTask
    HDX_API
    void SyncParams(HdxRenderTaskParams const &params);
    HDX_API
    void SyncCamera();

    HdRenderPassStateSharedPtr const &GetRenderPassState() const {
        return _renderPassState;
    }
    TfTokenVector const &GetRenderTags() const {
        return _renderTags;
    }

    /// The ID render pass encodes the ID as color in a specific order.
    /// Use this method to ensure the read back is done in an endian
    /// correct fashion.
    ///
    /// As packing of IDs may change in the future we encapuslate the
    /// correct behavior here.
    /// \param idColor a byte buffer of length 4.
    static inline int DecodeIDRenderColor(unsigned char const idColor[4]) {
        return (int32_t(idColor[0] & 0xff) << 0)  |
               (int32_t(idColor[1] & 0xff) << 8)  |
               (int32_t(idColor[2] & 0xff) << 16) |
               (int32_t(idColor[3] & 0xff) << 24);
    }

protected:
    /// Execute render pass task
    HDX_API
    virtual void _Execute(HdTaskContext* ctx);

    /// Sync the render pass resources
    HDX_API
    virtual void _Sync(HdTaskContext* ctx);

private:
    HdRenderPassStateSharedPtr _renderPassState;
    HdStRenderPassShaderSharedPtr _colorRenderPassShader;
    HdStRenderPassShaderSharedPtr _idRenderPassShader;
    GfVec4d _viewport;
    SdfPath _cameraId;
    TfTokenVector _renderTags;

    static HdStShaderCodeSharedPtr _overrideShader;

    static void _CreateOverrideShader();

    void _SetHdStRenderPassState(HdxRenderTaskParams const& params,
                                 HdStRenderPassState *renderPassState);
};

/// \class HdxRenderTaskParams
///
/// RenderTask parameters (renderpass state).
///
struct HdxRenderTaskParams : public HdTaskParams
{
    HdxRenderTaskParams()
        : overrideColor(0.0)
        , wireframeColor(0.0)
        , enableLighting(false)
        , enableIdRender(false)
        , alphaThreshold(0.0)
        , tessLevel(1.0)
        , drawingRange(0.0, -1.0)
        , enableHardwareShading(true)
        , renderTags()
        , depthBiasUseDefault(true)
        , depthBiasEnable(false)
        , depthBiasConstantFactor(0.0f)
        , depthBiasSlopeFactor(1.0f)
        , depthFunc(HdCmpFuncLEqual)
        , cullStyle(HdCullStyleBackUnlessDoubleSided)
        , geomStyle(HdGeomStylePolygons)
        , complexity(HdComplexityLow)
        , hullVisibility(false)
        , surfaceVisibility(true)
        , camera()
        , viewport(0.0)
        {}

    // RasterState
    GfVec4f overrideColor;
    GfVec4f wireframeColor;
    bool enableLighting;
    bool enableIdRender;
    float alphaThreshold;
    float tessLevel;
    GfVec2f drawingRange;
    bool enableHardwareShading;
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

    // Viewer's Render Style
    HdCullStyle cullStyle;
    HdGeomStyle geomStyle;
    HdComplexity complexity;
    bool hullVisibility;
    bool surfaceVisibility;

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
