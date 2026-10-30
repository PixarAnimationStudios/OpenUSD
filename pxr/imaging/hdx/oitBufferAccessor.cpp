//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/oitBufferAccessor.h"

#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/envSetting.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDX_ENABLE_OIT, true,
                      "Enable order independent translucency");

TF_DEFINE_ENV_SETTING(HDX_ENABLE_OIT_PACKED_DATA, false,
    "Pack color, transmission and depth into smaller "
    "buffers, trading accuracy for memory.");

/* static */
bool
HdxOitBufferAccessor::IsOitEnabled()
{
    return TfGetEnvSetting(HDX_ENABLE_OIT);
}

/* static */
bool
HdxOitBufferAccessor::IsOitPackedDepthEnabled()
{
    return TfGetEnvSetting(HDX_ENABLE_OIT_PACKED_DATA);
}

HdxOitBufferAccessor::HdxOitBufferAccessor(HdTaskContext *ctx)
    : _ctx(ctx)
{
}

void
HdxOitBufferAccessor::RequestOitBuffers()
{
    (*_ctx)[HdxTokens->oitRequestFlag] = VtValue(true);
}

HdBufferArrayRangeSharedPtr const &
HdxOitBufferAccessor::_GetBar(const TfToken &name)
{
    const auto it = _ctx->find(name);
    if (it == _ctx->end()) {
        static HdBufferArrayRangeSharedPtr n;
        return n;
    }

    const VtValue &v = it->second;
    return v.Get<HdBufferArrayRangeSharedPtr>();
}

bool
HdxOitBufferAccessor::AddOitBufferBindings(
    const HdStRenderPassShaderSharedPtr &shader)
{
    HdBufferArrayRangeSharedPtr const & counterBar =
        _GetBar(HdxTokens->oitCounterBufferBar);
    HdBufferArrayRangeSharedPtr const & dataBar =
        _GetBar(HdxTokens->oitDataBufferBar);
    HdBufferArrayRangeSharedPtr const & jointBar =
        _GetBar(HdxTokens->oitJointBufferBar);
    HdBufferArrayRangeSharedPtr const & uniformBar =
        _GetBar(HdxTokens->oitUniformBar);

    if (counterBar && dataBar && jointBar && uniformBar) {

        HdStBufferArrayRangeSharedPtr jointBar_ =
            std::static_pointer_cast<HdStBufferArrayRange>(jointBar);

        shader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::SSBO,
                               HdxTokens->oitCounterBufferBar,
                               counterBar,
                               /*interleave = */ false,
                               /*writable = */ true,
                               /*arraySize = */ 0,
                               /*concatenateNames = */ false,
                               /*visibility = */ HgiShaderStageFragment));

        shader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::SSBO,
                               HdxTokens->oitDataBufferBar,
                               dataBar,
                               /*interleave = */ false,
                               /*writable = */ true,
                               /*arraySize = */ 0,
                               /*concatenateNames = */ false,
                               /*visibility = */ HgiShaderStageFragment));

        shader->AddBufferBinding(
                HdStBindingRequest(HdStBinding::SSBO,
                                   HdxTokens->oitJointBufferBar,
                                   jointBar_,
                                   /*interleave = */ true,
                                   /*writable = */ true,
                                   /*arraySize = */ 0,
                                   /*concatenateNames = */ false,
                                   /*visibility = */ HgiShaderStageFragment));

        shader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::UBO,
                               HdxTokens->oitUniformBar,
                               uniformBar,
                               /*interleave = */ true));
        return true;
    } else {
        shader->RemoveBufferBinding(HdxTokens->oitCounterBufferBar);
        shader->RemoveBufferBinding(HdxTokens->oitDataBufferBar);
        shader->RemoveBufferBinding(HdxTokens->oitJointBufferBar);
        shader->RemoveBufferBinding(HdxTokens->oitUniformBar);
        return false;
    }
}

void
HdxOitBufferAccessor::AllocateOitBuffersIfNecessary(HdRenderIndex *renderIndex)
{
    // If bars are already in context (e.g. from a previous frame), nothing to do.
    if (_ctx->find(HdxTokens->oitCounterBufferBar) != _ctx->end()) {
        return;
    }

    HdStResourceRegistrySharedPtr const hdStResourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex->GetResourceRegistry());

    // Counter buffer
    HdBufferSpecVector counterSpecs{
        { HdxTokens->hdxOitCounterBuffer, HdTupleType{HdTypeInt32, 1} }
    };
    (*_ctx)[HdxTokens->oitCounterBufferBar] =
        hdStResourceRegistry->AllocateSingleBufferArrayRange(
            HdxTokens->oitCounter, counterSpecs,
            HdBufferArrayUsageHintBitsStorage);

    // Index + depth (joint) buffer
    HdBufferSpecVector jointSpecs{
        { HdxTokens->hdxOitIndexBuffer, HdTupleType{HdTypeInt32, 1} }
    };
    if (IsOitPackedDepthEnabled()) {
        jointSpecs.emplace_back(HdxTokens->hdxOitTransmissionDepthBuffer,
                                HdTupleType{HdTypeUInt32, 1});
    } else {
        jointSpecs.emplace_back(HdxTokens->hdxOitDepthBuffer,
                                HdTupleType{HdTypeFloat, 1});
    }
    (*_ctx)[HdxTokens->oitJointBufferBar] =
        hdStResourceRegistry->AllocateShaderStorageBufferArrayRange(
            HdxTokens->oitJointBuffer, jointSpecs,
            HdBufferArrayUsageHintBitsStorage);

    // Data (color) buffer
    HdBufferSpecVector dataSpecs{};
    if (IsOitPackedDepthEnabled()) {
        dataSpecs.emplace_back(HdxTokens->hdxOitColorBuffer,
                               HdTupleType{HdTypeUInt32, 1});
    } else {
        dataSpecs.emplace_back(HdxTokens->hdxOitDataBuffer,
                               HdTupleType{HdTypeFloatVec4, 1});
    }
    (*_ctx)[HdxTokens->oitDataBufferBar] =
        hdStResourceRegistry->AllocateSingleBufferArrayRange(
            HdxTokens->oitData, dataSpecs,
            HdBufferArrayUsageHintBitsStorage);

    // Uniform buffer (screen size)
    HdBufferSpecVector uniformSpecs{
        { HdxTokens->oitScreenSize, HdTupleType{HdTypeInt32Vec2, 1} },
        { HdxTokens->oitMaxDataBufferSize, HdTupleType{HdTypeInt32, 1} }
    };
    (*_ctx)[HdxTokens->oitUniformBar] =
        hdStResourceRegistry->AllocateUniformBufferArrayRange(
            HdxTokens->oitUniforms, uniformSpecs,
            HdBufferArrayUsageHintBitsUniform);
}

bool
HdxOitBufferAccessor::ResizeOitBuffersIfNecessary(
    HdRenderIndex *renderIndex,
    GfVec2i const &screenSize,
    GfVec2i *prevScreenSize)
{
    static const int numSamples = 8; // Must match glslfx files

    if (screenSize[0] < 0 || screenSize[1] < 0) {
        TF_CODING_ERROR("Invalid screen size for OIT buffer resize");
        return false;
    }

    const size_t numPixels = static_cast<size_t>(screenSize[0]) *
                             static_cast<size_t>(screenSize[1]);

    // The counter buffer is sized based on the size of the screen and uses
    // fragCoord to index into the buffers. We use numPixels + 1 for the size
    // because element 0 is an atomic counter pointing at the next free index
    // into the data/joint buffers.
    const size_t numCounterElements = numPixels + 1;

    // If the counter buffer won't fit on the GPU, we can't safely use OIT
    // at all.
    const size_t maxCounterElements =
        _GetBar(HdxTokens->oitCounterBufferBar)->GetMaxNumElements();
    if (numCounterElements > maxCounterElements) {
        TF_WARN("OIT counter buffer for screen size (%d, %d) needs 0x%zx "
                "elements, which exceeds the hardware maximum of 0x%zx; "
                "disabling OIT.",
                screenSize[0], screenSize[1],
                numCounterElements, maxCounterElements);
        return false;
    }

    const size_t oldNumPixels = static_cast<size_t>((*prevScreenSize)[0]) *
                                static_cast<size_t>((*prevScreenSize)[1]);
    *prevScreenSize = screenSize;

    HdStResourceRegistrySharedPtr const hdStResourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex->GetResourceRegistry());

    // We only ever need to grow this buffer; if it's too big,
    // we can just use part of it and don't need to shrink it.
    if (oldNumPixels < numPixels) {
        // Update counterBar since we've already checked its length.
        _GetBar(HdxTokens->oitCounterBufferBar)->Resize(numCounterElements);

        // The data/joint buffer elements are allocated as-needed to represent
        // transparent pixels, so these can be safely clamped if they are too
        // big (and data buffer, holding the color samples, is likely to be the
        // first to hit the size limit). Clamping here might mean we throw
        // away transparent pixels for complex scenes rendered at high res, so
        // warn before clamping.
        auto clampedResize = [&](
                HdBufferArrayRangeSharedPtr const &bar,
                const char *name) {
            const size_t numElements = numPixels * numSamples;
            const size_t maxElements = bar->GetMaxNumElements();
            if (numElements > maxElements) {
                TF_WARN("OIT %s needs 0x%zx elements, which exceeds the "
                        "hardware maximum of 0x%zx; clamping.",
                        name, numElements, maxElements);
                bar->Resize(maxElements);
            } else {
                bar->Resize(numElements);
            }
        };

        clampedResize(_GetBar(HdxTokens->oitJointBufferBar), "joint buffer");
        clampedResize(_GetBar(HdxTokens->oitDataBufferBar), "data buffer");
    }

    hdStResourceRegistry->AddSource(
        _GetBar(HdxTokens->oitUniformBar),
        std::make_shared<HdVtBufferSource>(
            HdxTokens->oitScreenSize,
            VtValue(screenSize)));

    // Compute the maximum addressable sample (across dataBar/jointBar) so that
    // the shader can properly do bounds checking.
    const size_t maxDataElements = std::min({
        _GetBar(HdxTokens->oitDataBufferBar)->GetNumElements(),
        _GetBar(HdxTokens->oitJointBufferBar)->GetNumElements(),
        numPixels * numSamples});

    hdStResourceRegistry->AddSource(
        _GetBar(HdxTokens->oitUniformBar),
        std::make_shared<HdVtBufferSource>(
            HdxTokens->oitMaxDataBufferSize,
            VtValue(static_cast<int>(maxDataElements))));

    return true;
}

void
HdxOitBufferAccessor::InitializeOitBuffersIfNecessary(Hgi *hgi)
{
    // If the OIT buffers were already cleared earlier, skip and do not
    // clear them again.
    VtValue &clearFlag = (*_ctx)[HdxTokens->oitClearedFlag];
    if (!clearFlag.IsEmpty()) {
        return;
    }

    // Mark OIT buffers as cleared.
    clearFlag = true;

    // Clear counter buffer.

    // The shader determines what elements in each buffer are used based on
    // finding -1 in the counter buffer. We can skip clearing the other buffers.

    HdStBufferArrayRangeSharedPtr stCounterBar =
        std::dynamic_pointer_cast<HdStBufferArrayRange>(
            _GetBar(HdxTokens->oitCounterBufferBar));

    if (!stCounterBar) {
        TF_CODING_ERROR(
            "No OIT counter buffer allocateed when trying to clear it");
        return;
    }

    HdStBufferResourceSharedPtr stCounterResource =
        stCounterBar->GetResource(HdxTokens->hdxOitCounterBuffer);

    // We want to fill the buffer with int -1 but the FillBuffer interface
    // supports uint8_t (due to a limitation in the Metal API which we can later
    // revisit to find a workaround). A buffer filled with uint8_t 0xff is the
    // same as a buffer filled with int 0xffffffff.
    const uint8_t clearCounter = -1;

    HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
    blitCmds->PushDebugGroup("Clear OIT buffers");
    blitCmds->FillBuffer(stCounterResource->GetHandle(), clearCounter);
    blitCmds->PopDebugGroup();
    hgi->SubmitCmds(blitCmds.get());
}

PXR_NAMESPACE_CLOSE_SCOPE
