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
#include "rendererPlugin.h"
#include "context.h"
#include "renderDelegate.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"

#include "pxr/base/tf/getenv.h"

PXR_NAMESPACE_OPEN_SCOPE

static HdxPrmanRenderDelegate* s_currentDelegate = nullptr;

TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdxPrmanRendererPlugin>();
}

HdRenderDelegate*
HdxPrmanRendererPlugin::CreateRenderDelegate()
{
    // Prman only supports one delegate at a time
    if (s_currentDelegate) {
        return nullptr;
    }
    std::shared_ptr<HdxPrman_InteractiveContext> context =
        std::make_shared<HdxPrman_InteractiveContext>();
    s_currentDelegate = new HdxPrmanRenderDelegate(context);

    return s_currentDelegate;
}

HdRenderDelegate*
HdxPrmanRendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    // Prman only supports one delegate at a time
    if (s_currentDelegate) {
        return nullptr;
    }
    std::shared_ptr<HdxPrman_InteractiveContext> context =
        std::make_shared<HdxPrman_InteractiveContext>();
    s_currentDelegate = new HdxPrmanRenderDelegate(context, settingsMap);

    return s_currentDelegate;
}

void
HdxPrmanRendererPlugin::DeleteRenderDelegate(HdRenderDelegate *renderDelegate)
{
    // The HdxPrman_InteractiveContext is owned the by delegate and
    // will be automatically destroyed by ref-counting, shutting
    // down the attached PRMan instance.
    if (s_currentDelegate == renderDelegate) {
        delete renderDelegate;
        s_currentDelegate = nullptr;
    }
}

bool
HdxPrmanRendererPlugin::IsSupported() const
{
    if (TfGetenv("RMANTREE").empty()) {
        // HdxPrman_InteractiveContext::Begin() requires RMANTREE to be set
        // in order to connect to renderman.  Setting RMANTREE from code is
        // already too late, since libloadprman.a has library ctor entries that
        // read the environment when loaded.
        TF_WARN("The Hydra-Riley backend requires $RMANTREE to be "
                "set before startup.");
        return false;
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
