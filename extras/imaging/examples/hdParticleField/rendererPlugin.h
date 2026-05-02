//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef HDPARTICLEFIELD_HDPARTICLEFIELDRENDERPLUGIN_H
#define HDPARTICLEFIELD_HDPARTICLEFIELDRENDERPLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"
#include "pxr/imaging/hd/version.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdParticleFieldRendererPlugin final : public HdRendererPlugin {
  public:
    HdParticleFieldRendererPlugin()           = default;
    ~HdParticleFieldRendererPlugin() override = default;

    /// Create a new gaussian splats render delegate instance.
    HdRenderDelegate* CreateRenderDelegate() override;

    /// Construct a new gaussian splats render delegate instance.
    HdRenderDelegate* CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap) override;

    /// Delete a gaussian splats render delegate instance.
    void DeleteRenderDelegate(HdRenderDelegate* renderDelegate) override;

    /// Is this plugin supported?
#if HD_API_VERSION >= 83
    bool IsSupported(
        HdRendererCreateArgs const &rendererCreateArgs,
        std::string *reasonWhyNot = nullptr) const override;
#else
    bool IsSupported(bool gpuEnabled = true) const override;
#endif

#if HD_API_VERSION >= 98
    bool IsSupported(
        const HdRendererCreateArgsSchema &rendererCreateArgs,
        std::string *reasonWhyNot = nullptr) const override;
#endif

  private:
    /// Cannot copy.
    HdParticleFieldRendererPlugin(
        const HdParticleFieldRendererPlugin&) = delete;
    HdParticleFieldRendererPlugin& operator=(
        const HdParticleFieldRendererPlugin&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_HDPARTICLEFIELDRENDERPLUGIN_H
