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
#ifndef HDX_SIMPLE_LIGHT_BYPASS_TASK_H
#define HDX_SIMPLE_LIGHT_BYPASS_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


//
//  This class exists to isolate code churn of Hd/Hdx/UsdImaging from exisiting
//  UsdImaging usecase in Presto.
//  Until Phd completely takes over all imaging system, we need to support the
//  existing scheme that Glim/UsdBatch owns all lighting information
//  including shadow maps. This HdTask can be used for simply passing the
//  lighting context down to following HdxRenderTask, which is internally
//  constructed in UsdImagingHdEngine.
//

class HdRenderIndex;
class HdSceneDelegate;
class HdxCamera;

typedef boost::shared_ptr<class HdxSimpleLightingShader> HdxSimpleLightingShaderSharedPtr;


class HdxSimpleLightBypassTask : public HdSceneTask {
public:
    HDX_API
    HdxSimpleLightBypassTask(HdSceneDelegate* delegate, SdfPath const& id);

protected:
    /// Execute render pass task
    HDX_API
    virtual void _Execute(HdTaskContext* ctx);

    /// Sync the render pass resources
    HDX_API
    virtual void _Sync(HdTaskContext* ctx);

private:
    const HdxCamera *_camera;
    HdxSimpleLightingShaderSharedPtr _lightingShader;

    GlfSimpleLightingContextRefPtr _simpleLightingContext;
};

struct HdxSimpleLightBypassTaskParams {
    SdfPath cameraPath;
    GlfSimpleLightingContextRefPtr simpleLightingContext;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out,
                         const HdxSimpleLightBypassTaskParams& pv);
HDX_API
bool operator==(const HdxSimpleLightBypassTaskParams& lhs,
                const HdxSimpleLightBypassTaskParams& rhs);
HDX_API
bool operator!=(const HdxSimpleLightBypassTaskParams& lhs,
                const HdxSimpleLightBypassTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDX_SIMPLE_LIGHT_BYPASS_TASK_H
