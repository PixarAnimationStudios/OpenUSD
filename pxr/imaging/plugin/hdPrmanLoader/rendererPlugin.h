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
#ifndef PXR_IMAGING_PLUGIN_HD_PRMAN_LOADER_RENDERER_PLUGIN_H
#define PXR_IMAGING_PLUGIN_HD_PRMAN_LOADER_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanLoaderRendererPlugin final : public HdRendererPlugin {
public:
    HdPrmanLoaderRendererPlugin();
    virtual ~HdPrmanLoaderRendererPlugin();

    virtual HdRenderDelegate *CreateRenderDelegate() override;
    virtual HdRenderDelegate *CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap) override;
    virtual void DeleteRenderDelegate(HdRenderDelegate *) override;
    virtual bool IsSupported() const override;

private:
    // This class does not support copying.
    HdPrmanLoaderRendererPlugin(
        const HdPrmanLoaderRendererPlugin&) = delete;
    HdPrmanLoaderRendererPlugin &operator =(
        const HdPrmanLoaderRendererPlugin&) = delete;
};

// These macros are used to shim the actual hdPrman delegate implementation
#define HDPRMAN_LOADER_CREATE_DELEGATE \
    extern "C" ARCH_EXPORT HdRenderDelegate* HdPrmanLoaderCreateDelegate( \
        HdRenderSettingsMap const& settingsMap)
#define HDPRMAN_LOADER_DELETE_DELEGATE \
    extern "C" ARCH_EXPORT void HdPrmanLoaderDeleteDelegate( \
        HdRenderDelegate *renderDelegate)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_PRMAN_LOADER_RENDERER_PLUGIN_H
