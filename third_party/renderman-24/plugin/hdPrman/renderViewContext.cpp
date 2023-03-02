//
// Copyright 2021 Pixar
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
#include "hdPrman/renderViewContext.h"
#include "hdPrman/renderDelegate.h"

#include "hdPrman/rixStrings.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_RenderViewDesc::RenderOutputDesc::RenderOutputDesc()
  : type(riley::RenderOutputType::k_Color)
  , rule(RixStr.k_filter)
  , filter(RixStr.k_box)
  , filterWidth(1.0f, 1.0f)
  , relativePixelVariance(1.0f)
{ }

HdPrman_RenderViewContext::HdPrman_RenderViewContext() = default;

void
HdPrman_RenderViewContext::CreateRenderView(
    const HdPrman_RenderViewDesc &desc,
    riley::Riley * const riley)
{
    _DestroyRenderView(riley);

    using RenderOutputDesc = HdPrman_RenderViewDesc::RenderOutputDesc;

    for (const RenderOutputDesc &outputDesc : desc.renderOutputDescs) {
        const riley::FilterSize filterWidth = { outputDesc.filterWidth[0],
                                                outputDesc.filterWidth[1] };
        
        _renderOutputIds.push_back(
            riley->CreateRenderOutput(
                riley::UserId(stats::AddDataLocation(outputDesc.name.CStr()).GetValue()),
                outputDesc.name,
                outputDesc.type,
                outputDesc.sourceName,
                outputDesc.rule,
                outputDesc.filter,
                filterWidth,
                outputDesc.relativePixelVariance,
                outputDesc.params));
    }
    
    const riley::Extent rtResolution = {
        static_cast<uint32_t>(desc.resolution[0]),
        static_cast<uint32_t>(desc.resolution[1]),
        1 };
    
    static const RtUString rtWeighted("weighted");

    _renderTargetId =
        riley->CreateRenderTarget(
            riley::UserId(stats::AddDataLocation("/renderTarget").GetValue()),
            { static_cast<uint32_t>(_renderOutputIds.size()),
              _renderOutputIds.data() },
            rtResolution,
            rtWeighted,
            1.0f,
            RtParamList());

    using DisplayDesc = HdPrman_RenderViewDesc::DisplayDesc;
    for (const DisplayDesc &displayDesc : desc.displayDescs) {
        std::vector<riley::RenderOutputId> displayRenderOutputIds;
        displayRenderOutputIds.reserve(displayDesc.renderOutputIndices.size());
        for (const size_t renderOutputIndex : displayDesc.renderOutputIndices) {
            displayRenderOutputIds.push_back(
                _renderOutputIds[renderOutputIndex]);
        }
        _displayIds.push_back(
            riley->CreateDisplay(
                riley::UserId(
                    stats::AddDataLocation(
                        displayDesc.name.CStr()).GetValue()),
                _renderTargetId,
                displayDesc.name,
                displayDesc.driver,
                { static_cast<uint32_t>(displayRenderOutputIds.size()),
                  displayRenderOutputIds.data()},
                displayDesc.params));
    }
    
    _renderViewId =
        riley->CreateRenderView(
            riley::UserId(stats::AddDataLocation("/renderView").GetValue()),
            _renderTargetId,
            desc.cameraId,
            desc.integratorId,
            desc.displayFilterList,
            desc.sampleFilterList,
            RtParamList());
}

void
HdPrman_RenderViewContext::_DestroyRenderView(
    riley::Riley * const riley)
{
    if (_renderViewId != riley::RenderViewId::InvalidId()) {
        riley->DeleteRenderView(_renderViewId);
        _renderViewId = riley::RenderViewId::InvalidId();
    }

    for (const riley::DisplayId id : _displayIds) {
        riley->DeleteDisplay(id);
    }
    _displayIds.clear();
    
    if (_renderTargetId != riley::RenderTargetId::InvalidId()) {
        riley->DeleteRenderTarget(_renderTargetId);
        _renderTargetId = riley::RenderTargetId::InvalidId();
    }

    for (const riley::RenderOutputId id : _renderOutputIds) {
        riley->DeleteRenderOutput(id);
    }
    _renderOutputIds.clear();
}

void
HdPrman_RenderViewContext::SetIntegratorId(
    const riley::IntegratorId id,
    riley::Riley * const riley)
{
    if (_renderViewId == riley::RenderViewId::InvalidId()) {
        return;
    }
    
    riley->ModifyRenderView(
        _renderViewId, nullptr, nullptr, &id, nullptr, nullptr, nullptr);        
}

void
HdPrman_RenderViewContext::SetResolution(
    const GfVec2i &resolution,
    riley::Riley * const riley)
{
    if (_renderTargetId == riley::RenderTargetId::InvalidId()) {
        return;
    }

    const riley::Extent extent = {
        static_cast<uint32_t>(resolution[0]),
        static_cast<uint32_t>(resolution[1]),
        1 };

    riley->ModifyRenderTarget(
        _renderTargetId, nullptr, &extent, nullptr, nullptr, nullptr);
}

PXR_NAMESPACE_CLOSE_SCOPE
