//
// Copyright 2019 Pixar
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
#include "renderDelegate.h"
#include "renderBuffer.h"
#include "renderParam.h"
#include "renderPass.h"
#include "resourceRegistry.h"
#include "context.h"

#include "hdPrman/instancer.h"
#include "hdPrman/light.h"
#include "hdPrman/material.h"

#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxPrmanRenderDelegate::HdxPrmanRenderDelegate(
    std::shared_ptr<HdPrman_Context> context)
    : HdPrmanRenderDelegate(context)
{
    _Initialize(context);
}

HdxPrmanRenderDelegate::HdxPrmanRenderDelegate(
    std::shared_ptr<HdPrman_Context> context,
    HdRenderSettingsMap const& settingsMap)
    : HdPrmanRenderDelegate(context, settingsMap)
{
    _Initialize(context);
}

void
HdxPrmanRenderDelegate::_Initialize(std::shared_ptr<HdPrman_Context> context)
{
    // Check if this is an interactive context.
    _interactiveContext =
        std::dynamic_pointer_cast<HdxPrman_InteractiveContext>(context);

    if (_interactiveContext) {
        _renderParam = std::make_shared<HdxPrman_RenderParam>(
            _interactiveContext);
        _interactiveContext->Begin(this);
    }

    _resourceRegistry = std::make_shared<HdxPrman_ResourceRegistry>(
        _interactiveContext);
}

HdxPrmanRenderDelegate::~HdxPrmanRenderDelegate()
{
    if (_interactiveContext) {
        _interactiveContext->End();
    }
}

HdRenderPassSharedPtr
HdxPrmanRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                                        HdRprimCollection const& collection)
{
    if (!_renderPass) {
        _renderPass = HdRenderPassSharedPtr(
            new HdxPrman_RenderPass(index, collection, _context));
    }
    return _renderPass;
}

HdSprim *
HdxPrmanRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    HdSprim *sprim = HdPrmanRenderDelegate::CreateSprim(typeId, sprimId);
    if (dynamic_cast<HdPrmanLight*>(sprim)) {
        // Disregard fallback prims in count.
        if (sprim->GetId() != SdfPath()) {
            _interactiveContext->sceneLightCount++;
        }
    }
    return sprim;
}

void 
HdxPrmanRenderDelegate::DestroySprim(HdSprim *sprim)
{
    if (dynamic_cast<HdPrmanLight*>(sprim)) {
        // Disregard fallback prims in count.
        if (sprim->GetId() != SdfPath()) {
            _interactiveContext->sceneLightCount--;
        }
    }
    HdPrmanRenderDelegate::DestroySprim(sprim);
}

static TfTokenVector _PushBack(TfTokenVector const& vec,
                               TfToken elem)
{
    TfTokenVector vec2 = vec;
    vec2.push_back(elem);
    return vec2;
}

const TfTokenVector&
HdxPrmanRenderDelegate::GetSupportedBprimTypes() const
{
    static TfTokenVector types = _PushBack(
        HdPrmanRenderDelegate::GetSupportedBprimTypes(),
        HdPrimTypeTokens->renderBuffer);
    return types;
}

HdBprim*
HdxPrmanRenderDelegate::CreateBprim(TfToken const& typeId,
                                    SdfPath const& bprimId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdxPrmanRenderBuffer(bprimId);
    }
    return HdPrmanRenderDelegate::CreateBprim(typeId, bprimId);
}

HdBprim*
HdxPrmanRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdxPrmanRenderBuffer(SdfPath::EmptyPath());
    }
    return HdPrmanRenderDelegate::CreateFallbackBprim(typeId);
}

HdAovDescriptor
HdxPrmanRenderDelegate::GetDefaultAovDescriptor(
                            TfToken const& name) const
{
    if (name == HdAovTokens->color) {
        return HdAovDescriptor(HdFormatFloat32Vec4, false,
                               VtValue(GfVec4f(0.0f)));
    } else if (name == HdAovTokens->depth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(1.0f));
    } else if (name == HdAovTokens->primId ||
               name == HdAovTokens->instanceId ||
               name == HdAovTokens->elementId) {
        return HdAovDescriptor(HdFormatInt32, false, VtValue(-1));
    }

    return HdAovDescriptor(HdFormatFloat32Vec3, false,
                           VtValue(GfVec3f(0.0f)));
}

bool
HdxPrmanRenderDelegate::IsStopSupported() const
{
    return true;
}

bool
HdxPrmanRenderDelegate::Stop()
{
    _interactiveContext->StopRender();
    return true;
}
bool
HdxPrmanRenderDelegate::Restart()
{
    // Next call into HdxPrman_RenderPass::_Execute will do a StartRender
    _interactiveContext->sceneVersion++;
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
