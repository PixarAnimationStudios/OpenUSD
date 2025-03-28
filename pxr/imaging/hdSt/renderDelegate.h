//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
using HdStDrawItemsCacheUniquePtr =
    std::unique_ptr<class HdSt_DrawItemsCache>;
using HdStDrawItemsCachePtr = HdSt_DrawItemsCache *;

using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<class HdStResourceRegistry>;

///
/// HdStRenderDelegate
///
/// The Storm Render Delegate provides a rasterizer renderer to draw the scene.
/// While it currently has some ties to GL, the goal is to use Hgi to allow
/// it to be graphics API agnostic.
///
class HdStRenderDelegate final : public HdRenderDelegate
{
public:
    HDST_API
    HdStRenderDelegate();
    HDST_API
    HdStRenderDelegate(HdRenderSettingsMap const& settingsMap);

    HDST_API
    ~HdStRenderDelegate() override;
    
    // ---------------------------------------------------------------------- //
    /// \name HdRenderDelegate virtual API
    // ---------------------------------------------------------------------- //

    HDST_API
    void SetDrivers(HdDriverVector const& drivers) override;

    HDST_API
    HdRenderParam *GetRenderParam() const override;

    HDST_API
    const TfTokenVector &GetSupportedRprimTypes() const override;
    HDST_API
    const TfTokenVector &GetSupportedSprimTypes() const override;
    HDST_API
    const TfTokenVector &GetSupportedBprimTypes() const override;
    HDST_API
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    HDST_API
    HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex *index,
                HdRprimCollection const& collection) override;
    HDST_API
    HdRenderPassStateSharedPtr CreateRenderPassState() const override;

    HDST_API
    HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                 SdfPath const& id) override;

    HDST_API
    void DestroyInstancer(HdInstancer *instancer) override;

    HDST_API
    HdRprim *CreateRprim(TfToken const& typeId,
                         SdfPath const& rprimId) override;
    HDST_API
    void DestroyRprim(HdRprim *rPrim) override;

    HDST_API
    HdSprim *CreateSprim(TfToken const& typeId,
                         SdfPath const& sprimId) override;
    HDST_API
    HdSprim *CreateFallbackSprim(TfToken const& typeId) override;
    HDST_API
    void DestroySprim(HdSprim *sPrim) override;

    HDST_API
    HdBprim *CreateBprim(TfToken const& typeId,
                         SdfPath const& bprimId) override;
    HDST_API
    HdBprim *CreateFallbackBprim(TfToken const& typeId) override;
    HDST_API
    void DestroyBprim(HdBprim *bPrim) override;

    HDST_API
    void CommitResources(HdChangeTracker *tracker) override;

    HDST_API
    TfTokenVector GetMaterialRenderContexts() const override;

    HDST_API
    TfTokenVector GetShaderSourceTypes() const override;

    HDST_API
    bool IsPrimvarFilteringNeeded() const override;

    HDST_API
    HdRenderSettingDescriptorList
        GetRenderSettingDescriptors() const override;

    HDST_API
    VtDictionary GetRenderStats() const override;

    HDST_API
    HdAovDescriptor
        GetDefaultAovDescriptor(TfToken const& name) const override;
    
    // ---------------------------------------------------------------------- //
    /// \name Misc public API
    // ---------------------------------------------------------------------- //

    // Returns whether or not HdStRenderDelegate can run on the current
    // hardware.
    HDST_API
    static bool IsSupported();

    // Returns a raw pointer to the draw items cache owned (solely) by the
    // render delegate.
    HDST_API
    HdStDrawItemsCachePtr GetDrawItemsCache() const;

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

    HdStDrawItemsCacheUniquePtr _drawItemsCache;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_RENDER_DELEGATE_H
