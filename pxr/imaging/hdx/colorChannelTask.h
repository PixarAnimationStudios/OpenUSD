//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HDX_COLORCHANNEL_TASK_H
#define HDX_COLORCHANNEL_TASK_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hgi/graphicsCmds.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdxColorChannelTaskParams;

/// \class HdxColorChannelTask
///
/// A task for choosing a color channel for display.
///
class HdxColorChannelTask : public HdxTask
{
public:
    using TaskParams = HdxColorChannelTaskParams;

    HDX_API
    HdxColorChannelTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxColorChannelTask() override;

    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute the color channel task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

protected:
    /// Sync the render pass resources
    HDX_API
    void _Sync(HdSceneDelegate* delegate,
               HdTaskContext* ctx,
               HdDirtyBits* dirtyBits) override;

private:
    HdxColorChannelTask() = delete;
    HdxColorChannelTask(const HdxColorChannelTask &) = delete;
    HdxColorChannelTask &operator =(const HdxColorChannelTask &) = delete;

    // Utility function to update the shader uniform parameters.
    // Returns true if the values were updated. False if unchanged.
    bool _UpdateParameterBuffer(float screenSizeX, float screenSizeY);

    /// Apply the color channel filtering.
    void _ApplyColorChannel();

    // This struct must match ParameterBuffer in colorChannel.glslfx.
    // Be careful to remember the std430 rules.
    struct _ParameterBuffer
    {
        float screenSize[2];
        int channel;
        bool operator==(const _ParameterBuffer& other) const {
            return channel == other.channel &&
                   screenSize[0] == other.screenSize[0] &&
                   screenSize[1] == other.screenSize[1];
        }
    };

    std::unique_ptr<class HdxFullscreenShader> _compositor;
    _ParameterBuffer _parameterData;

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
