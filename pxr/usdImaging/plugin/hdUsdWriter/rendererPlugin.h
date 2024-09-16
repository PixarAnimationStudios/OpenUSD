//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#ifndef HD_USD_WRITER_RENDERER_PLUGIN_H
#define HD_USD_WRITER_RENDERER_PLUGIN_H

#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"
#include "pxr/imaging/hd/rendererPlugin.h"
#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdUsdWriterRendererPlugin
///
/// A registered child of HdRendererPlugin, this is the class that gets
/// loaded when a Hydra application asks to draw with a certain renderer.
/// It supports rendering via creation/destruction of renderer-specific
/// classes. The render delegate is the Hydra-facing entrypoint into the
/// renderer; it's responsible for creating specialized implementations of Hydra
/// prims (which translate scene data into drawable representations) and Hydra
/// renderpasses (which draw the scene to the framebuffer).
///
class HdUsdWriterRendererPlugin : public HdRendererPlugin
{
public:
    HDUSDWRITER_API
    HdUsdWriterRendererPlugin() = default;

    HDUSDWRITER_API
    virtual ~HdUsdWriterRendererPlugin() = default;

    /// Construct a new render delegate of type HdUsdWriterRenderDelegate.
    HDUSDWRITER_API
    virtual HdRenderDelegate* CreateRenderDelegate() override;

    /// Construct a new render delegate of type HdUsdWriterRenderDelegate.
    ///
    ///   \param settingsMap Render settings when creating the render delegate.
    HDUSDWRITER_API
    virtual HdRenderDelegate* CreateRenderDelegate(HdRenderSettingsMap const& settingsMap) override;

    /// Destroy a render delegate created by this class's CreateRenderDelegate.
    ///
    ///   \param renderDelegate The render delegate to delete.
    HDUSDWRITER_API
    virtual void DeleteRenderDelegate(HdRenderDelegate* renderDelegate) override;

    /// Checks to see if the plugin is supported on the running system.
    ///
    ///   \return True if supported, false otherwise.
    HDUSDWRITER_API
    virtual bool IsSupported(bool gpuEnabled = true) const override;

private:
    // This class does not support copying.
    HdUsdWriterRendererPlugin(const HdUsdWriterRendererPlugin&) = delete;
    HdUsdWriterRendererPlugin& operator=(const HdUsdWriterRendererPlugin&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
