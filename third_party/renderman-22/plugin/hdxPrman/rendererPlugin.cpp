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
#include "hdxPrman/rendererPlugin.h"
#include "hdxPrman/context.h"
#include "hdxPrman/renderDelegate.h"
#include "pxr/imaging/hdx/rendererPluginRegistry.h"

#include "pxr/base/tf/getenv.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    HdxRendererPluginRegistry::Define<HdxPrmanRendererPlugin>();
}

HdRenderDelegate*
HdxPrmanRendererPlugin::CreateRenderDelegate()
{
    std::shared_ptr<HdxPrman_InteractiveContext> context =
        std::make_shared<HdxPrman_InteractiveContext>();
    return new HdxPrmanRenderDelegate(context);
}

HdRenderDelegate*
HdxPrmanRendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    std::shared_ptr<HdxPrman_InteractiveContext> context =
        std::make_shared<HdxPrman_InteractiveContext>();
    return new HdxPrmanRenderDelegate(context, settingsMap);
}

void
HdxPrmanRendererPlugin::DeleteRenderDelegate(HdRenderDelegate *renderDelegate)
{
    // The HdxPrman_InteractiveContext is owned the by delegate and
    // will be automatically destroyed by ref-counting, shutting
    // down the attached PRMan instance.
    delete renderDelegate;
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
