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
#ifndef HDX_RENDER_PLUGIN_REGISTRY_H
#define HDX_RENDER_PLUGIN_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/imaging/hf/pluginDelegateRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdxRendererPlugin;

class HdxRendererPluginRegistry final  : public HfPluginDelegateRegistry
{
public:
    ///
    /// Returns the singleton registry for \c HdxRendererPlugin
    ///
    HDX_API
    static HdxRendererPluginRegistry &GetInstance();

    ///
    /// Entry point for defining an HdxRendererPlugin plugin.
    ///
    template<typename T, typename... Bases>
    static void Define();

    ///
    /// Returns the renderer plugin for the given id or null
    /// if not found.  The reference count on the returned
    /// delegate is incremented.
    ///
    HDX_API
    HdxRendererPlugin *GetRendererPlugin(const TfToken &pluginId);

private:
    // Friend required by TfSingleton to access constructor (as it is private).
    friend class TfSingleton<HdxRendererPluginRegistry>;

    // Singleton gets private constructed
    HdxRendererPluginRegistry();
    virtual ~HdxRendererPluginRegistry();

    //
    /// This class is not intended to be copied.
    ///
    HdxRendererPluginRegistry(const HdxRendererPluginRegistry &)            = delete;
    HdxRendererPluginRegistry &operator=(const HdxRendererPluginRegistry &) = delete;
};


template<typename T, typename... Bases>
void HdxRendererPluginRegistry::Define()
{
    HfPluginDelegateRegistry::Define<T, HdxRendererPlugin, Bases...>();
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_RENDER_PLUGIN_REGISTRY_H
