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
#ifndef EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HDX_PRMAN_RENDERER_PLUGIN_H
#define EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HDX_PRMAN_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdxPrmanRendererPlugin final : public HdRendererPlugin {
public:
    HdxPrmanRendererPlugin() = default;
    virtual ~HdxPrmanRendererPlugin() = default;

    virtual HdRenderDelegate *CreateRenderDelegate() override;
    virtual HdRenderDelegate *CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap) override;
    virtual void DeleteRenderDelegate(HdRenderDelegate *) override;
    virtual bool IsSupported() const override;

private:
    // This class does not support copying.
    HdxPrmanRendererPlugin(const HdxPrmanRendererPlugin&)             = delete;
    HdxPrmanRendererPlugin &operator =(const HdxPrmanRendererPlugin&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HDX_PRMAN_RENDERER_PLUGIN_H
