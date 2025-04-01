//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_PRESENT_TASK_H
#define PXR_IMAGING_HDX_PRESENT_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/imaging/hgiPresent/present.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxPresentTaskParams
///
/// PresentTask parameters.
///
struct HdxPresentTaskParams
{
    /// Parameters for the presentation destination. Forwarded to hgiPresent.
    HgiPresentDestinationParams destinationParams;

    /// When not enabled, present task does not execute, but still calls
    /// Hgi::EndFrame.
    bool enabled = false;
};

/// \class HdxPresentTask
///
/// A task for taking the final result of the aovs and compositing it over the 
/// currently bound framebuffer.
/// This task uses the 'color' and optionally 'depth' aov's in the task
/// context. The 'color' aov is expected to use non-integer 
/// (i.e., float or norm) types to keep the interop step simple.
///
class HdxPresentTask : public HdxTask
{
public:
    using TaskParams = HdxPresentTaskParams;

    HDX_API
    HdxPresentTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxPresentTask() override;

    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    HDX_API
    void Execute(HdTaskContext* ctx) override;

    /// Returns true if the format is supported for presentation. This is useful
    /// for upstream tasks to prepare the AOV data accordingly, and keeps the
    /// api-interoperable presentation simple.
    HDX_API
    bool IsFormatSupported(HgiFormat colorFormat) const;

protected:
    HDX_API
    void _Sync(HdSceneDelegate* delegate,
               HdTaskContext* ctx,
               HdDirtyBits* dirtyBits) override;

private:
    HdxPresentTaskParams _params;
    std::unique_ptr<HgiPresent> _present;

    HdxPresentTask() = delete;
    HdxPresentTask(const HdxPresentTask &) = delete;
    HdxPresentTask &operator =(const HdxPresentTask &) = delete;
};



// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxPresentTaskParams& pv);
HDX_API
bool operator==(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs);
HDX_API
bool operator!=(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
