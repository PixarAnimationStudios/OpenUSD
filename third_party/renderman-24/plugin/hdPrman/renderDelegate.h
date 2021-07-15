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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_DELEGATE_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "hdPrman/api.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam;
class HdPrman_RenderPass;
struct HdPrman_Context;
struct HdPrman_InteractiveContext;
class RixParamList;

TF_DEFINE_PRIVATE_TOKENS(
    HdPrmanRenderSettingsTokens,
    ((integrator,                     "integrator"))
    ((integratorName,                 "ri:integrator:name"))
    ((interactiveIntegrator,          "interactiveIntegrator"))
    ((interactiveIntegratorTimeout,   "interactiveIntegratorTimeout"))
    ((dataWindowNDC,                  "dataWindowNDC"))
    ((pixelAspectRatio,               "pixelAspectRatio")) 
    ((resolution,                     "resolution"))
    ((instantaneousShutter,           "instantaneousShutter"))
    ((shutterOpen,                    "shutter:open"))
    ((shutterClose,                   "shutter:close"))
);

#define HDPRMAN_INTEGRATOR_TOKENS \
    (PxrPathTracer)               \
    (PbsPathTracer)               \
    (PxrDirectLighting)

TF_DECLARE_PUBLIC_TOKENS(HdPrmanIntegratorTokens, HDPRMAN_API,
    HDPRMAN_INTEGRATOR_TOKENS);

class HdPrmanRenderDelegate : public HdRenderDelegate 
{
public:
    HDPRMAN_API 
    HdPrmanRenderDelegate(std::shared_ptr<HdPrman_Context> context);
    HDPRMAN_API 
    HdPrmanRenderDelegate(std::shared_ptr<HdPrman_Context> context,
        HdRenderSettingsMap const& settingsMap);
    HDPRMAN_API 
    virtual ~HdPrmanRenderDelegate();

    // HdRenderDelegate API implementation.
    HDPRMAN_API
    HdRenderParam *GetRenderParam() const override;
    HDPRMAN_API 
    const TfTokenVector & GetSupportedRprimTypes() const override;
    HDPRMAN_API 
    const TfTokenVector & GetSupportedSprimTypes() const override;
    HDPRMAN_API 
    const TfTokenVector & GetSupportedBprimTypes() const override;
    HDPRMAN_API 
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    /// Returns a list of user-configurable render settings.
    HDPRMAN_API 
    HdRenderSettingDescriptorList GetRenderSettingDescriptors() const override;

    HDPRMAN_API 
    HdRenderSettingsMap GetRenderSettingsMap() const;        

    HDPRMAN_API 
    HdRenderPassSharedPtr CreateRenderPass(
                HdRenderIndex *index,
                HdRprimCollection const& collection) override;
    HDPRMAN_API 
    HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                 SdfPath const& id) override;
    HDPRMAN_API 
    void DestroyInstancer(HdInstancer *instancer) override;
    HDPRMAN_API 
    HdRprim *CreateRprim(TfToken const& typeId,
                         SdfPath const& rprimId) override;
    HDPRMAN_API 
    void DestroyRprim(HdRprim *rPrim) override;
    HDPRMAN_API 
    HdSprim *CreateSprim(TfToken const& typeId,
                         SdfPath const& sprimId) override;
    HDPRMAN_API 
    HdSprim *CreateFallbackSprim(TfToken const& typeId) override;
    HDPRMAN_API 
    void DestroySprim(HdSprim *sPrim) override;
    HDPRMAN_API 
    HdBprim *CreateBprim(TfToken const& typeId,
                         SdfPath const& bprimId) override;
    HDPRMAN_API 
    HdBprim *CreateFallbackBprim(TfToken const& typeId) override;
    HDPRMAN_API 
    void DestroyBprim(HdBprim *bPrim) override;

    HDPRMAN_API 
    HdAovDescriptor GetDefaultAovDescriptor(TfToken const& name) const override;

    HDPRMAN_API 
    void CommitResources(HdChangeTracker *tracker) override;
    HDPRMAN_API 
    TfToken GetMaterialBindingPurpose() const override;
#if HD_API_VERSION < 41
    HDPRMAN_API 
    TfToken GetMaterialNetworkSelector() const override;
#else
    HDPRMAN_API 
    TfTokenVector GetMaterialRenderContexts() const override;
#endif
    HDPRMAN_API 
    TfTokenVector GetShaderSourceTypes() const override;

    HDPRMAN_API 
    void SetRenderSetting(TfToken const &key, VtValue const &value) override;

    /// NOTE: RenderMan has no notion of pausing the render threads.
    ///       We don't return true, because otherwise start/stop causes
    ///       the renderer to reset to increment zero, which gives a poor
    ///       user experience and poor peformance. 
    HDPRMAN_API 
    bool IsPauseSupported() const override { return false; }

    /// Return true to indicate that stopping and restarting are supported.
    HDPRMAN_API 
    bool IsStopSupported() const override;

    /// Stop background rendering threads.
    HDPRMAN_API 
    bool Stop() override;

    /// Restart background rendering threads.
    HDPRMAN_API 
    bool Restart() override;

private:
    // This class does not support copying.
    HdPrmanRenderDelegate(const HdPrmanRenderDelegate &) = delete;
    HdPrmanRenderDelegate &operator =(const HdPrmanRenderDelegate &) = delete;

    void _Initialize();

    enum RenderMode
    {
        Interactive = 0,
        Offline
    };
    RenderMode _renderMode;

    bool _IsInteractive() const { 
        return (_renderMode == RenderMode::Interactive);
    }

protected:
    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    std::shared_ptr<HdPrman_Context> _context;
    std::shared_ptr<HdPrman_RenderParam> _renderParam;
    HdResourceRegistrySharedPtr _resourceRegistry;
    HdRenderPassSharedPtr _renderPass;
    HdRenderSettingDescriptorList _settingDescriptors;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_DELEGATE_H
