//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_DELEGATE_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/version.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDPRMAN_RENDER_SETTINGS_TOKENS                                 \
    ((rileyVariant,                   "ri:variant"))                   \
    ((xpuDevices,                     "ri:xpudevices"))                \
    ((integrator,                     "integrator"))                   \
    ((integratorName,                 "ri:integrator:name"))           \
    ((interactiveIntegrator,          "interactiveIntegrator"))        \
    ((interactiveIntegratorTimeout,   "interactiveIntegratorTimeout")) \
    ((dataWindowNDC,                  "dataWindowNDC"))                \
    ((pixelAspectRatio,               "pixelAspectRatio"))             \
    ((resolution,                     "resolution"))                   \
                                                                       \
    /* \deprecated Use disableMotionBlur instead */                    \
    ((instantaneousShutter,           "instantaneousShutter"))         \
    ((disableMotionBlur,              "disableMotionBlur"))            \
    ((shutterOpen,                    "shutter:open"))                 \
    ((shutterClose,                   "shutter:close"))                \
    ((experimentalRenderSpec,         "experimental:renderSpec"))      \
    ((delegateRenderProducts,         "delegateRenderProducts"))       \
    ((batchCommandLine,               "batchCommandLine"))             \
    ((houdiniFrame,                   "houdini:frame"))                \
    ((checkpointInterval,             "ri:checkpoint:interval"))

TF_DECLARE_PUBLIC_TOKENS(HdPrmanRenderSettingsTokens, HDPRMAN_API,
    HDPRMAN_RENDER_SETTINGS_TOKENS);

#define HDPRMAN_EXPERIMENTAL_RENDER_SPEC_TOKENS \
    (renderProducts)               \
    (renderVars)                   \
    (renderVarIndices)             \
    (name)                         \
    (sourceName)                   \
    (sourceType)                   \
    (type)                         \
    (params)                       \
    (camera)

TF_DECLARE_PUBLIC_TOKENS(HdPrmanExperimentalRenderSpecTokens, HDPRMAN_API,
    HDPRMAN_EXPERIMENTAL_RENDER_SPEC_TOKENS);

#define HDPRMAN_INTEGRATOR_TOKENS \
    (PxrPathTracer)               \
    (PbsPathTracer)               \
    (PxrDirectLighting)

TF_DECLARE_PUBLIC_TOKENS(HdPrmanIntegratorTokens, HDPRMAN_API,
    HDPRMAN_INTEGRATOR_TOKENS);

#define HDPRMAN_RENDER_PRODUCT_TOKENS \
    (productName) \
    (productType) \
    (orderedVars) \
    (sourcePrim)

TF_DECLARE_PUBLIC_TOKENS(
    HdPrmanRenderProductTokens, HDPRMAN_API,
    HDPRMAN_RENDER_PRODUCT_TOKENS);

#define HDPRMAN_AOV_SETTINGS_TOKENS \
    ((dataType,                       "dataType"))                   \
    ((sourceName,                     "sourceName"))                 \
    ((sourceType,                     "sourceType"))                 \
    ((format,                         "aovDescriptor.format"))       \
    ((multiSampled,                   "aovDescriptor.multiSampled")) \
    ((aovSettings,                    "aovDescriptor.aovSettings"))  \
    ((clearValue,                     "aovDescriptor.clearValue"))

TF_DECLARE_PUBLIC_TOKENS(
    HdPrmanAovSettingsTokens, HDPRMAN_API,
    HDPRMAN_AOV_SETTINGS_TOKENS);

class HdPrmanRenderDelegate : public HdRenderDelegate 
{
public:
    HDPRMAN_API 
    HdPrmanRenderDelegate(HdRenderSettingsMap const& settingsMap);
    HDPRMAN_API 
    ~HdPrmanRenderDelegate() override;

    // ------------------------------------------------------------------------
    // Satisfying HdRenderDelegate
    // ------------------------------------------------------------------------
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

#if HD_API_VERSION > 46
    HDPRMAN_API 
    TfTokenVector GetRenderSettingsNamespaces() const override;
#endif

#if HD_API_VERSION >= 60
    HDPRMAN_API
    HdContainerDataSourceHandle GetCapabilities() const override;
#endif

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

    /// Return true to indicate whether or not the rendering threads are active.
    HDPRMAN_API 
    bool IsStopped() const override;

    /// Stop background rendering threads.
    HDPRMAN_API 
    bool Stop(bool blocking) override;

    /// Restart background rendering threads.
    HDPRMAN_API 
    bool Restart() override;

#if HD_API_VERSION >= 55

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Hydra 2.0 API
    ///
    ////////////////////////////////////////////////////////////////////////////

    HDPRMAN_API
    void SetTerminalSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex) override;

    HDPRMAN_API
    void Update() override;

#endif

    // ------------------------------------------------------------------------
    // Public (non-virtual) API
    // ------------------------------------------------------------------------

    HDPRMAN_API
    HdRenderSettingsMap GetRenderSettingsMap() const;    

    HDPRMAN_API
    bool IsInteractive() const;

    HDPRMAN_API
    HdRenderIndex* GetRenderIndex() const;

private:
    // This class does not support copying.
    HdPrmanRenderDelegate(const HdPrmanRenderDelegate &) = delete;
    HdPrmanRenderDelegate &operator =(const HdPrmanRenderDelegate &) = delete;

    void _Initialize();

protected:
    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    std::shared_ptr<class HdPrman_RenderParam> _renderParam;

#if HD_API_VERSION >= 55
    std::unique_ptr<class HdPrman_TerminalSceneIndexObserver>
        _terminalObserver;
#endif

    struct _RileySceneIndices;
    std::unique_ptr<_RileySceneIndices> _rileySceneIndices;

    HdResourceRegistrySharedPtr _resourceRegistry;
    HdRenderPassSharedPtr _renderPass;
    HdRenderSettingDescriptorList _settingDescriptors;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_DELEGATE_H
