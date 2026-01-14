//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef HDPARTICLEFIELD_HDPARTICLEFIELDRENDERDELEGATE_H
#define HDPARTICLEFIELD_HDPARTICLEFIELDRENDERDELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/resourceRegistry.h"

#include "renderer.h"
#include "gsRenderer.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdParticleFieldRenderParam;

/// \class HdParticleFieldRenderDelegate
///
/// Hydra renderer interface for the gaussian splats renderer.
class HdParticleFieldRenderDelegate final : public HdRenderDelegate {
  public:
    /// Default constructor.
    HdParticleFieldRenderDelegate();

    /// Constructor with render settings.
    HdParticleFieldRenderDelegate(const HdRenderSettingsMap& settingsMap);

    /// Destrucutor.
    ~HdParticleFieldRenderDelegate() override;

    /// Query supported hydra prim types.
    const TfTokenVector& GetSupportedRprimTypes() const override;
    const TfTokenVector& GetSupportedSprimTypes() const override;
    const TfTokenVector& GetSupportedBprimTypes() const override;

    /// Return this delegate's render param, which provides top-level scene
    /// state.
    ///   \return An instance of HdGaussianSplatsRenderParam.
    HdRenderParam* GetRenderParam() const override;

    /// Returns a list of user-configurable render settings, available in the
    /// UI.
    HdRenderSettingDescriptorList GetRenderSettingDescriptors() const override;

    /// Get the resource registry.
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    /// Create render pass.
    virtual HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex* index,
        const HdRprimCollection& collection) override;

    /// Create an instancer.
    HdInstancer* CreateInstancer(
        HdSceneDelegate* delegate, const SdfPath& id) override;

    /// Destroy an instancer.
    void DestroyInstancer(HdInstancer* instancer) override;

    /// Create a new Rprim.
    HdRprim* CreateRprim(
        const TfToken& typeId, const SdfPath& rprimId) override;

    void DestroyRprim(HdRprim* rprim) override;

    /// Create a new Sprim.
    HdSprim* CreateSprim(
        const TfToken& typeId, const SdfPath& sprimId) override;

    HdSprim* CreateFallbackSprim(const TfToken& typeId) override;

    /// Destroy an existing Sprim.
    void DestroySprim(HdSprim* sprim) override;

    /// Create a new buffer prim.
    HdBprim* CreateBprim(
        const TfToken& typeId, const SdfPath& bprimId) override;

    /// Create a fallback buffer prim.
    HdBprim* CreateFallbackBprim(const TfToken& typeId) override;

    /// Destroy an existing Bprim.
    void DestroyBprim(HdBprim* bprim) override;

    /// Do work.
    void CommitResources(HdChangeTracker* tracker) override;

    /// Return the AOV description for \param aovName.
    /// This will be used to initialize the aov buffers.
    HdAovDescriptor GetDefaultAovDescriptor(
        const TfToken& aovName) const override;

    /// Return true to indicate that pausing and resuming are supported.
    bool IsPauseSupported() const override;

    /// Pause background rendering threads.
    bool Pause() override;

    /// Resume background rendering threads.
    bool Resume() override;

  private:
    // Setup routine (used in both constructors).
    void _Setup();

    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    // A background render thread for running the actual renders in. The
    // render thread object manages synchronization between the scene data
    // and the background-threaded renderer.
    HdRenderThread _renderThread;

    HdParticleFieldRenderer _renderer;

    // A version counter for edits to _scene.
    std::atomic<int> _sceneVersion;

    std::unique_ptr<HdParticleFieldRenderParam> _renderParam;
    HdRenderSettingDescriptorList _settingDescriptors;

    HdResourceRegistrySharedPtr _resourceRegistry;

    /// Cannot copy.
    HdParticleFieldRenderDelegate(
        const HdParticleFieldRenderDelegate&) = delete;
    HdParticleFieldRenderDelegate& operator=(
        const HdParticleFieldRenderDelegate&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_HDPARTICLEFIELDRENDERDELEGATE_H
