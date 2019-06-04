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
#ifndef HDPRMAN_RENDER_DELEGATE_H
#define HDPRMAN_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "hdPrman/api.h"
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam;
class HdPrman_RenderPass;
struct HdPrman_Context;

#define HDPRMAN_RENDER_SETTINGS_TOKENS \
    (integrator)

TF_DECLARE_PUBLIC_TOKENS(HdPrmanRenderSettingsTokens, HDPRMAN_API,
    HDPRMAN_RENDER_SETTINGS_TOKENS);

class HdPrmanRenderDelegate : public HdRenderDelegate {
public:
    HDPRMAN_API HdPrmanRenderDelegate(std::shared_ptr<HdPrman_Context> context);
    HDPRMAN_API HdPrmanRenderDelegate(std::shared_ptr<HdPrman_Context> context,
        HdRenderSettingsMap const& settingsMap);
    HDPRMAN_API virtual ~HdPrmanRenderDelegate();

    // HdRenderDelegate API implementation.
    HDPRMAN_API virtual HdRenderParam *GetRenderParam() const override;
    HDPRMAN_API virtual const TfTokenVector &
    GetSupportedRprimTypes() const override;
    HDPRMAN_API virtual const TfTokenVector &
    GetSupportedSprimTypes() const override;
    HDPRMAN_API virtual const TfTokenVector &
    GetSupportedBprimTypes() const override;
    HDPRMAN_API virtual HdResourceRegistrySharedPtr
    GetResourceRegistry() const override;

    /// Returns a list of user-configurable render settings.
    HDPRMAN_API virtual HdRenderSettingDescriptorList
        GetRenderSettingDescriptors() const override;

    HDPRMAN_API virtual HdRenderPassSharedPtr CreateRenderPass(
                HdRenderIndex *index,
                HdRprimCollection const& collection) override;
    HDPRMAN_API virtual HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id,
                                         SdfPath const& instancerId) override;
    HDPRMAN_API virtual void DestroyInstancer(HdInstancer *instancer) override;
    HDPRMAN_API virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId,
                                 SdfPath const& instancerId) override;
    HDPRMAN_API virtual void DestroyRprim(HdRprim *rPrim) override;
    HDPRMAN_API virtual HdSprim *CreateSprim(TfToken const& typeId,
                                 SdfPath const& sprimId) override;
    HDPRMAN_API virtual HdSprim *CreateFallbackSprim(
                                 TfToken const& typeId) override;
    HDPRMAN_API virtual void DestroySprim(HdSprim *sPrim) override;
    HDPRMAN_API virtual HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override;
    HDPRMAN_API virtual HdBprim *CreateFallbackBprim(
                                 TfToken const& typeId) override;
    HDPRMAN_API virtual void DestroyBprim(HdBprim *bPrim) override;
    HDPRMAN_API virtual void CommitResources(HdChangeTracker *tracker) override;
    HDPRMAN_API virtual TfToken GetMaterialBindingPurpose() const override;
    HDPRMAN_API virtual TfToken GetMaterialNetworkSelector() const override;
    HDPRMAN_API virtual TfTokenVector GetShaderSourceTypes() const override;

private:
    // This class does not support copying.
    HdPrmanRenderDelegate(const HdPrmanRenderDelegate &)             = delete;
    HdPrmanRenderDelegate &operator =(const HdPrmanRenderDelegate &) = delete;

    void _Initialize();

protected: // data
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

#endif // HDPRMAN_RENDER_DELEGATE_H
