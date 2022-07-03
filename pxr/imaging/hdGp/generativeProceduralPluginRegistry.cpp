//
// Copyright 2022 Pixar
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
#include "generativeProceduralPluginRegistry.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/diagnostic.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdGpGenerativeProceduralPluginRegistry);

HdGpGenerativeProceduralPluginRegistry &
HdGpGenerativeProceduralPluginRegistry::GetInstance()
{
    return TfSingleton<HdGpGenerativeProceduralPluginRegistry>::GetInstance();
}

HdGpGenerativeProceduralPluginRegistry::HdGpGenerativeProceduralPluginRegistry()
 : HfPluginRegistry(TfType::Find<HdGpGenerativeProceduralPlugin>())
{
    TfSingleton<HdGpGenerativeProceduralPluginRegistry
        >::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<
        HdGpGenerativeProceduralPluginRegistry>();

    // for tests and debugging
    std::string epp = TfGetenv("PXR_HDGP_TEST_PLUGIN_PATH");
    if (!epp.empty()) {
        TF_STATUS("PXR_HDGP_TEST_PLUGIN_PATH set to %s", epp.c_str());
        PlugRegistry::GetInstance().RegisterPlugins(epp);
    }

    // Force discovery at instantiation time
    std::vector<HfPluginDesc> descs;
    HdGpGenerativeProceduralPluginRegistry::GetInstance()
        .GetPluginDescs(&descs);
}

HdGpGenerativeProceduralPluginRegistry::
    ~HdGpGenerativeProceduralPluginRegistry() = default;

HdGpGenerativeProcedural *
HdGpGenerativeProceduralPluginRegistry::ConstructProcedural(
    const TfToken &proceduralTypeName,
    const SdfPath &proceduralPrimPath)
{
    TfToken pluginId = proceduralTypeName;

    HfPluginDescVector descs;
    GetPluginDescs(&descs);
    for (const HfPluginDesc &desc : descs) {
        if (desc.displayName == proceduralTypeName) {
            pluginId = desc.id;
            break;
        }
    }

    if (HdGpGenerativeProceduralPlugin *plugin =
            static_cast<HdGpGenerativeProceduralPlugin*>(
                GetPlugin(pluginId))) {
        return plugin->Construct(proceduralPrimPath);
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE