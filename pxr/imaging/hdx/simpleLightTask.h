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
#ifndef PXR_IMAGING_HDX_SIMPLE_LIGHT_TASK_H
#define PXR_IMAGING_HDX_SIMPLE_LIGHT_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"

#include "pxr/imaging/hd/task.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"

#include "pxr/imaging/cameraUtil/framing.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/declarePtrs.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdSceneDelegate;
class HdCamera;

using HdRenderPassSharedPtr = std::shared_ptr<class HdRenderPass>;
using HdStSimpleLightingShaderSharedPtr =
    std::shared_ptr<class HdStSimpleLightingShader>;
using HdxShadowMatrixComputationSharedPtr =
    std::shared_ptr<class HdxShadowMatrixComputation>;

TF_DECLARE_REF_PTRS(GlfSimpleShadowArray);


class HdxSimpleLightTask : public HdTask
{
public:
    HDX_API
    HdxSimpleLightTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxSimpleLightTask() override;

    /// Sync the render pass resources
    HDX_API
    void Sync(HdSceneDelegate* delegate,
              HdTaskContext* ctx,
              HdDirtyBits* dirtyBits) override;

    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

private:
    std::vector<GfMatrix4d> _ComputeShadowMatrices(
        const HdCamera * camera,
        HdxShadowMatrixComputationSharedPtr const &computation) const;

    SdfPath _cameraId;
    std::map<TfToken, SdfPathVector> _lightIds;
    SdfPathVector _lightIncludePaths;
    SdfPathVector _lightExcludePaths;
    size_t _numLights;

    // Should be weak ptrs
    HdStSimpleLightingShaderSharedPtr _lightingShader;
    bool _enableShadows;
    GfVec4f _viewport;
    CameraUtilFraming _framing;
    std::pair<bool, CameraUtilConformWindowPolicy> _overrideWindowPolicy;

    // XXX: compatibility hack for passing some unit tests until we have
    //      more formal material plumbing.
    GlfSimpleMaterial _material;
    GfVec4f _sceneAmbient;

    // For now these are only valid for the lifetime of a single pass of
    // the render graph.  Maybe long-term these could be change-tracked.
    GlfSimpleLightVector _glfSimpleLights;

    size_t _AppendLightsOfType(HdRenderIndex &renderIndex,
                               std::vector<TfToken> const &lightTypes,
                               SdfPathVector const &lightIncludePaths,
                               SdfPathVector const &lightExcludePaths,
                               std::map<TfToken, SdfPathVector> *lights);

    HdxSimpleLightTask() = delete;
    HdxSimpleLightTask(const HdxSimpleLightTask &) = delete;
    HdxSimpleLightTask &operator =(const HdxSimpleLightTask &) = delete;
};

struct HdxSimpleLightTaskParams {
    HdxSimpleLightTaskParams()
        : cameraPath()
        , lightIncludePaths(1, SdfPath::AbsoluteRootPath())
        , lightExcludePaths()
        , enableShadows(false)
        , viewport(0.0f)
        , overrideWindowPolicy{false, CameraUtilFit}
        , material()
        , sceneAmbient(0) 
        {}

    SdfPath cameraPath;
    SdfPathVector lightIncludePaths;
    SdfPathVector lightExcludePaths;
    bool enableShadows;
    GfVec4f viewport;
    CameraUtilFraming framing;
    std::pair<bool, CameraUtilConformWindowPolicy> overrideWindowPolicy;
    
    // XXX: compatibility hack for passing some unit tests until we have
    //      more formal material plumbing.
    GlfSimpleMaterial material;
    GfVec4f sceneAmbient;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxSimpleLightTaskParams& pv);
HDX_API
bool operator==(
    const HdxSimpleLightTaskParams& lhs, 
    const HdxSimpleLightTaskParams& rhs);
HDX_API
bool operator!=(
    const HdxSimpleLightTaskParams& lhs, 
    const HdxSimpleLightTaskParams& rhs);

struct HdxShadowParams {
    HdxShadowParams()
        : shadowMatrix()
        , bias(0.0)
        , blur(0.0)
        , resolution(0)
        , enabled(false)
        {}

    HdxShadowMatrixComputationSharedPtr shadowMatrix;
    double bias;
    double blur;
    int resolution;
    bool enabled;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxShadowParams& pv);
HDX_API
bool operator==(const HdxShadowParams& lhs, const HdxShadowParams& rhs);
HDX_API
bool operator!=(const HdxShadowParams& lhs, const HdxShadowParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_SIMPLE_LIGHT_TASK_H
