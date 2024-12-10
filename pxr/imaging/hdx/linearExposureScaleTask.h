//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HDX_LINEAREXPOSURESCALE_TASK_H
#define HDX_LINEAREXPOSURESCALE_TASK_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hgi/graphicsCmds.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxLinearExposureScaleTask
///
/// A task for applying an exposure scale for display.
///
class HdxLinearExposureScaleTask : public HdxTask
{
public:
    HDX_API
    HdxLinearExposureScaleTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxLinearExposureScaleTask() override;

    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute the exposure scale task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

protected:
    /// Sync the render pass resources
    HDX_API
    void _Sync(HdSceneDelegate* delegate,
               HdTaskContext* ctx,
               HdDirtyBits* dirtyBits) override;

private:
    HdxLinearExposureScaleTask() = delete;
    HdxLinearExposureScaleTask(const HdxLinearExposureScaleTask &) = delete;
    HdxLinearExposureScaleTask &operator =(const HdxLinearExposureScaleTask &) = delete;

    // Utility function to update the shader uniform parameters.
    // Returns true if the values were updated. False if unchanged.
    bool _UpdateParameterBuffer(float screenSizeX, float screenSizeY);

    // This struct must match ParameterBuffer in linearExposureScale.glslfx.
    // Be careful to remember the std430 rules.
    struct _ParameterBuffer
    {
        float screenSize[2];
        float linearExposureScale;
        bool operator==(const _ParameterBuffer& other) const {
            return linearExposureScale == other.linearExposureScale &&
                   screenSize[0] == other.screenSize[0] &&
                   screenSize[1] == other.screenSize[1];
        }
    };

    std::unique_ptr<class HdxFullscreenShader> _compositor;
    _ParameterBuffer _parameterData;

    SdfPath _cameraPath;

    // The multiplier to be applied to the displayed pixels
    float _linearExposureScale = 1.0f;
};


/// \class HdxLinearExposureScaleTaskParams
///
/// LinearExposureScaleTask parameters.
///
struct HdxLinearExposureScaleTaskParams
{
    SdfPath cameraPath;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxLinearExposureScaleTaskParams& pv);
HDX_API
bool operator==(const HdxLinearExposureScaleTaskParams& lhs,
                const HdxLinearExposureScaleTaskParams& rhs);
HDX_API
bool operator!=(const HdxLinearExposureScaleTaskParams& lhs,
                const HdxLinearExposureScaleTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
