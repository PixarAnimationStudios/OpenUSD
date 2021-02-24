//
// Copyright 2017 Pixar
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
#ifndef PXR_IMAGING_HD_ST_RENDER_DELEGATE_H
#define PXR_IMAGING_HD_ST_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/renderDelegate.h"

#include <memory>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HdStRenderParam;

using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<class HdStResourceRegistry>;

///
/// HdStRenderDelegate
///
/// The Storm Render Delegate provides a rasterizer renderer to draw the scene.
/// While it currently has some ties to GL, the goal is to use Hgi to allow
/// it to be graphics API agnostic.
///
class HdStRenderDelegate final : public HdRenderDelegate {
public:
    HDST_API
    HdStRenderDelegate();
    HDST_API
    HdStRenderDelegate(HdRenderSettingsMap const& settingsMap);

    HDST_API
    virtual ~HdStRenderDelegate();
    
    // ---------------------------------------------------------------------- //
    /// \name HdRenderDelegate virtual API
    // ---------------------------------------------------------------------- //

    HDST_API
    virtual void SetDrivers(HdDriverVector const& drivers) override;

    HDST_API
    virtual HdRenderParam *GetRenderParam() const override;

    HDST_API
    virtual const TfTokenVector &GetSupportedRprimTypes() const override;
    HDST_API
    virtual const TfTokenVector &GetSupportedSprimTypes() const override;
    HDST_API
    virtual const TfTokenVector &GetSupportedBprimTypes() const override;
    HDST_API
    virtual HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    HDST_API
    virtual HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex *index,
                HdRprimCollection const& collection) override;
    HDST_API
    virtual HdRenderPassStateSharedPtr CreateRenderPassState() const override;

    HDST_API
    virtual HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id) override;

    HDST_API
    virtual void DestroyInstancer(HdInstancer *instancer) override;

    HDST_API
    virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId) override;
    HDST_API
    virtual void DestroyRprim(HdRprim *rPrim) override;

    HDST_API
    virtual HdSprim *CreateSprim(TfToken const& typeId,
                                 SdfPath const& sprimId) override;
    HDST_API
    virtual HdSprim *CreateFallbackSprim(TfToken const& typeId) override;
    HDST_API
    virtual void DestroySprim(HdSprim *sPrim) override;

    HDST_API
    virtual HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override;
    HDST_API
    virtual HdBprim *CreateFallbackBprim(TfToken const& typeId) override;
    HDST_API
    virtual void DestroyBprim(HdBprim *bPrim) override;

    HDST_API
    virtual void CommitResources(HdChangeTracker *tracker) override;

    HDST_API
    virtual TfToken GetMaterialNetworkSelector() const override;

    HDST_API
    virtual TfTokenVector GetShaderSourceTypes() const override;

    HDST_API
    virtual HdRenderSettingDescriptorList
        GetRenderSettingDescriptors() const override;

    HDST_API
    virtual VtDictionary GetRenderStats() const override;

    HDST_API
    virtual HdAovDescriptor
        GetDefaultAovDescriptor(TfToken const& name) const override;
    
    // ---------------------------------------------------------------------- //
    /// \name Misc public API
    // ---------------------------------------------------------------------- //

    // Returns whether or not HdStRenderDelegate can run on the current
    // hardware.
    HDST_API
    static bool IsSupported();

    // Returns Hydra graphics interface
    HDST_API
    Hgi* GetHgi();

private:
    void _ApplyTextureSettings();
    HdSprim *_CreateFallbackMaterialPrim();

    HdStRenderDelegate(const HdStRenderDelegate &)             = delete;
    HdStRenderDelegate &operator =(const HdStRenderDelegate &) = delete;

    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;

    // Resource registry used in this render delegate
    HdStResourceRegistrySharedPtr _resourceRegistry;

    HdRenderSettingDescriptorList _settingDescriptors;

    Hgi* _hgi;

    std::unique_ptr<HdStRenderParam> _renderParam;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_RENDER_DELEGATE_H
