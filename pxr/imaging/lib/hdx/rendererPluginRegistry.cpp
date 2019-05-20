//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hdx/rendererPluginRegistry.h"
#include "pxr/imaging/hdx/rendererPlugin.h"

#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_INSTANTIATE_SINGLETON( HdxRendererPluginRegistry );

HdxRendererPluginRegistry &
HdxRendererPluginRegistry::GetInstance()
{
    return TfSingleton< HdxRendererPluginRegistry >::GetInstance();
}


HdxRendererPluginRegistry::HdxRendererPluginRegistry()
 : HfPluginRegistry(TfType::Find<HdxRendererPlugin>())
{
}


HdxRendererPluginRegistry::~HdxRendererPluginRegistry()
{
}

TfToken 
HdxRendererPluginRegistry::GetDefaultPluginId()
{
    // Get all the available plugins to see if any of them is supported on this
    // platform and use the first one as the default.
    // 
    // Important note, we want to avoid loading plugins as much as possible, 
    // we would prefer to only load plugins when the user asks for them.  So
    // we will only load plugins until we find the first one that works.
    HfPluginDescVector pluginDescriptors;
    GetPluginDescs(&pluginDescriptors);
    for (const HfPluginDesc &desc : pluginDescriptors) {
        
        HdxRendererPlugin *plugin = HdxRendererPluginRegistry::GetInstance().
            GetRendererPlugin(desc.id);

        // Important to bail out as soon as we found a plugin that works to
        // avoid loading plugins unnecessary as that can be arbitrarily
        // expensive.
        if (plugin && plugin->IsSupported()) {
            HdxRendererPluginRegistry::GetInstance().ReleasePlugin(plugin);
            return desc.id;
        }

        HdxRendererPluginRegistry::GetInstance().ReleasePlugin(plugin);
    }

    return TfToken();
}

HdxRendererPlugin *
HdxRendererPluginRegistry::GetRendererPlugin(const TfToken &pluginId)
{
    return static_cast<HdxRendererPlugin *>(GetPlugin(pluginId));
}


PXR_NAMESPACE_CLOSE_SCOPE

