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
#ifndef EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HDX_PRMAN_RENDER_DELEGATE_H
#define EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HDX_PRMAN_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "hdPrman/renderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam;
struct HdPrman_Context;
class HdxPrman_RenderPass;
struct HdxPrman_InteractiveContext;

class HdxPrmanRenderDelegate final : public HdPrmanRenderDelegate {
public:
    HdxPrmanRenderDelegate(std::shared_ptr<HdPrman_Context> context);
    HdxPrmanRenderDelegate(std::shared_ptr<HdPrman_Context> context,
        HdRenderSettingsMap const& settingsMap);
    virtual ~HdxPrmanRenderDelegate();

    // HdRenderDelegate API implementation.
    virtual HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex *index,
                HdRprimCollection const& collection) override;
    virtual HdSprim *CreateSprim(TfToken const& typeId,
                                 SdfPath const& sprimId) override;
    virtual void DestroySprim(HdSprim *sPrim) override;

    virtual const TfTokenVector &GetSupportedBprimTypes() const override;
    virtual HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override;
    virtual HdBprim *CreateFallbackBprim(TfToken const& typeId) override;

    virtual HdAovDescriptor GetDefaultAovDescriptor(
                                TfToken const& name) const override;

    /// Return true to indicate that pausing and resuming are supported.
    virtual bool IsPauseSupported() const override;

    /// Pause background rendering threads.
    virtual bool Pause() override;

    /// Resume background rendering threads.
    virtual bool Resume() override;

private:
    // This class does not support copying.
    HdxPrmanRenderDelegate(const HdxPrmanRenderDelegate &)             = delete;
    HdxPrmanRenderDelegate &operator =(const HdxPrmanRenderDelegate &) = delete;

    void _Initialize(std::shared_ptr<HdPrman_Context> context);

private: // data
    HdRenderPassSharedPtr _renderPass;
    std::shared_ptr<HdxPrman_InteractiveContext> _interactiveContext;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HDX_PRMAN_RENDER_DELEGATE_H
