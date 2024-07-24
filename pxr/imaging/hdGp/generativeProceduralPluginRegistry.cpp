//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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