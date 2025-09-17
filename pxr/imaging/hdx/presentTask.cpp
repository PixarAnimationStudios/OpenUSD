//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/presentTask.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE

bool
_IsIntegerFormat(HgiFormat format)
{
    return (format == HgiFormatUInt16 ||
            format == HgiFormatUInt16Vec2 ||
            format == HgiFormatUInt16Vec3 ||
            format == HgiFormatUInt16Vec4 ||
            format == HgiFormatInt32 ||
            format == HgiFormatInt32Vec2 ||
            format == HgiFormatInt32Vec3 ||
            format == HgiFormatInt32Vec4);
}

/*static*/
bool
HdxPresentTask::IsFormatSupported(HgiFormat aovFormat)
{
    // Integer formats are not supported (this requires the GL interop to
    // support additional sampler types), nor are compressed formats.
    return !_IsIntegerFormat(aovFormat) && !HgiIsCompressed(aovFormat);
}   

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
            _params = params;
        }
    }
    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxPresentTask::Prepare(HdTaskContext* ctx, HdRenderIndex *renderIndex)
{
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

        HgiTextureHandle aovTexture;
        _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);
        if (aovTexture) {
            HgiTextureDesc texDesc = aovTexture->GetDescriptor();
            if (!IsFormatSupported(texDesc.format)) {
                // Warn, but don't bail.
                TF_WARN("Aov texture format %d may not be correctly supported "
                        "for presentation via HgiInterop.", texDesc.format);
            }
        }

        HgiTextureHandle depthTexture;
        if (_HasTaskContextData(ctx, HdAovTokens->depth)) {
            _GetTaskContextData(ctx, HdAovTokens->depth, &depthTexture);
        }

        // Use HgiInterop to composite the Hgi textures over the application's
        // framebuffer contents.
        // Eg. This allows us to render with HgiMetal and present the images
        // into a opengl based application (such as usdview).
        _interop.TransferToApp(
            _GetHgi(),
            aovTexture, depthTexture,
            _params.dstApi,
            _params.dstFramebuffer, _params.dstRegion);
    }

    // Wrap one HdEngine::Execute frame with Hgi StartFrame and EndFrame.
    // StartFrame is currently called in the AovInputTask.
    // This is important for Hgi garbage collection to run.
    _GetHgi()->EndFrame();
}


// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxPresentTaskParams& pv)
{
    out << "PresentTask Params: (...) "
        << pv.dstApi;
    return out;
}

bool operator==(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    return lhs.dstApi == rhs.dstApi &&
           lhs.dstFramebuffer == rhs.dstFramebuffer &&
           lhs.dstRegion == rhs.dstRegion &&
           lhs.enabled == rhs.enabled;
}

bool operator!=(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
