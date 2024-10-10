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
    ((aspectRatioConformPolicy,       "aspectRatioConformPolicy"))     \
    ((pixelAspectRatio,               "pixelAspectRatio"))             \
    ((resolution,                     "resolution"))                   \
                                                                       \
    /* \deprecated Use disableMotionBlur instead */                    \
    ((instantaneousShutter,           "instantaneousShutter"))         \
    ((disableMotionBlur,              "disableMotionBlur"))            \
    ((disableDepthOfField,            "disableDepthOfField"))            \
    ((shutterOpen,                    "shutter:open"))                 \
    ((shutterClose,                   "shutter:close"))                \
    ((experimentalRenderSpec,         "experimental:renderSpec"))      \
    ((renderVariant,                  "renderVariant"))                \
    ((xpuCpuConfig,                   "xpuCpuConfig"))                 \
    ((xpuGpuConfig,                   "xpuGpuConfig"))                 \
    ((delegateRenderProducts,         "delegateRenderProducts"))       \
    ((projection,                     "projection"))                   \
    ((projectionName,                 "ri:projection:name"))           \
    ((enableInteractive,              "enableInteractive"))            \
    ((batchCommandLine,               "batchCommandLine"))             \
    ((houdiniFrame,                   "houdini:frame"))                \
    ((checkpointInterval,             "ri:checkpoint:interval"))       \
    ((pixelFilter,                    "ri:Ri:PixelFilterName"))        \
    ((pixelFilterWidth,               "ri:Ri:PixelFilterWidth"))

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
    (PxrDirectLighting)           \
    (PxrUnified)

#define HDPRMAN_PROJECTION_TOKENS \
    (PxrPerspective)              \
    (PxrOrthographic)             

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

TF_DECLARE_PUBLIC_TOKENS(HdPrmanProjectionTokens, HDPRMAN_API,
    HDPRMAN_PROJECTION_TOKENS);

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

#if PXR_VERSION <= 2308
/* Aspect Ratio Conform Policy Tokens used on render settings prims 
 * Note that these mirror the conform policy tokens in UsdRenderTokens */
#define HD_ASPECT_RATIO_CONFORM_POLICY                       \
    (adjustApertureWidth)                                    \
    (adjustApertureHeight)                                   \
    (expandAperture)                                         \
    (cropAperture)                                           \
    (adjustPixelAspectRatio)                                 \

TF_DECLARE_PUBLIC_TOKENS(HdAspectRatioConformPolicyTokens, 
                        HD_ASPECT_RATIO_CONFORM_POLICY);
#endif

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
    VtDictionary GetRenderStats() const override;

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
    HDPRMAN_API 
    TfTokenVector GetMaterialRenderContexts() const override;
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

    HDPRMAN_API 
    bool IsPauseSupported() const override { return true; }
    bool Pause() override;
    bool Resume() override;

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

    std::string _GetRenderVariant(const HdRenderSettingsMap &settingsMap);

    static
    int _GetCpuConfig(const HdRenderSettingsMap &settingsMap);

    static
    std::vector<int> _GetGpuConfig(const HdRenderSettingsMap &settingsMap);

protected:
    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    std::shared_ptr<class HdPrman_RenderParam> _renderParam;

    // _rileySceneIndices holds on to _renderParam, so it needs to
    // be after _renderParam so that we destroy it before _renderParam.
    struct _RileySceneIndices;
    std::unique_ptr<_RileySceneIndices> _rileySceneIndices;

    HdResourceRegistrySharedPtr _resourceRegistry;
    HdRenderPassSharedPtr _renderPass;
    HdRenderSettingDescriptorList _settingDescriptors;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_DELEGATE_H
