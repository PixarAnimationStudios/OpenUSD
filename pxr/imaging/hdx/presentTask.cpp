//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/presentTask.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE

HdxPresentTask::HdxPresentTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxTask(id)
{
}

HdxPresentTask::~HdxPresentTask() = default;

void
HdxPresentTask::_Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxPresentTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            if (_params.destinationParams != params.destinationParams) {
                _present = nullptr;
            }

            _params = params;
        }
    }
    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxPresentTask::Prepare(HdTaskContext* ctx, HdRenderIndex *renderIndex)
{
    if (!_present) {
        _present = std::make_unique<HgiPresent>(HgiPresent::Create(_GetHgi(),
            _params.destinationParams));
    }
}

void
HdxPresentTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // The present task can be disabled in case an application does offscreen
    // rendering or doesn't use Hgi interop (e.g. directly access AOV results).
    // But we still need to call Hgi::EndFrame.

    if (_params.enabled && _HasTaskContextData(ctx, HdAovTokens->color)) {
        // The color and depth aovs have the results we want to blit to the
        // application. Depth is optional. When we are previewing a custom aov
        // we may not have a depth buffer.

        HgiTextureHandle colorTexture;
        _GetTaskContextData(ctx, HdAovTokens->color, &colorTexture);
        if (colorTexture) {
            HgiTextureDesc texDesc = colorTexture->GetDescriptor();
            if (!_present->IsFormatSupported(texDesc.format)) {
                // Warn, but don't bail.
                TF_WARN("Aov texture format %d may not be correctly supported "
                        "for presentation via HgiInterop.", texDesc.format);
            }
        }

        HgiTextureHandle depthTexture;
        if (_HasTaskContextData(ctx, HdAovTokens->depth)) {
            _GetTaskContextData(ctx, HdAovTokens->depth, &depthTexture);
        }

        // Present might blit to a window and make the AOV immediately visible,
        // or it might compose to an externally managed framebuffer for further
        // processing. We may render with an Hgi backend that differs from the
        // rendering system used by a particular application. for example,
        // using Vulkan to render, and OpenGL to display in an application.
        _present->Present(colorTexture, depthTexture);
    }

    // Wrap one HdEngine::Execute frame with Hgi StartFrame and EndFrame.
    // StartFrame is currently called in the AovInputTask.
    // This is important for Hgi garbage collection to run.
    _GetHgi()->EndFrame();
}

bool
HdxPresentTask::IsFormatSupported(HgiFormat colorFormat) const
{
    return _present && _present->IsFormatSupported(colorFormat);
}


// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxPresentTaskParams& pv)
{
    out << "PresentTask Params: (...) "
        << pv.enabled;

    return out;
}

bool operator==(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    return lhs.destinationParams == rhs.destinationParams &&
           lhs.enabled == rhs.enabled;
}

bool operator!=(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
