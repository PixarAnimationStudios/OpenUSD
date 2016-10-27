//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hd/renderDelegateRegistry.h"

#include "pxr/imaging/hd/renderDelegate.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include "pxr/base/tf/instantiateSingleton.h"

TF_INSTANTIATE_SINGLETON( HdRenderDelegateRegistry );

HdRenderDelegateRegistry &
HdRenderDelegateRegistry::GetInstance()
{
    return TfSingleton< HdRenderDelegateRegistry >::GetInstance();
}

HdRenderDelegateRegistry::HdRenderDelegateRegistry()
    : _pluginsLoaded(false)
{
}

HdRenderDelegatePtrVector 
HdRenderDelegateRegistry::GetAllRenderDelegates()
{
    // Make sure all the plugins are loaded
    _LoadPlugins();

    // This is not particularily efficient, but:
    //
    //   1. We expect this function to be called very rarely
    //   2. We expect a small number of render delegates
    //
    return HdRenderDelegatePtrVector(
        _renderDelegates.begin(), _renderDelegates.end());
}

void 
HdRenderDelegateRegistry::_LoadPlugins()
{
    if (_pluginsLoaded)
        return;

    std::set<TfType> result;
    PlugRegistry::GetInstance().GetAllDerivedTypes<HdRenderDelegate>(&result);

    // Note that we load all the plugins in this function.
    // XXX: This can be improved by only loading the plugins that the client
    // has asked for.
    for (const TfType &t : result) {
        if (PlugPluginPtr p = PlugRegistry::GetInstance().GetPluginForType(t)) {

            if (!p->Load()) {
                TF_WARN("Failed to load HdRenderDelegate plugin at path %s",
                    p->GetPath().c_str());
                continue;
            }

            HdRenderDelegateRefPtr renderDelegate;
            if (FactoryBase *factory = t.GetFactory<FactoryBase>()) {
                renderDelegate = factory->New();
            } else {
                TF_WARN("Failed to find HdRenderDelegate factory for plugin %s"
                        ", at path %s",
                    p->GetName().c_str(),
                    p->GetPath().c_str());
            }

            if (renderDelegate) {
                _renderDelegates.push_back(renderDelegate);
            }
        }
    }

    _pluginsLoaded = true;
}
