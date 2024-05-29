//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_PLUGIN_REGISTRY_H
#define PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_PLUGIN_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/imaging/hf/pluginRegistry.h"
#include "pxr/imaging/hdGp/api.h"

#include "pxr/imaging/hdGp/generativeProceduralPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdGpGenerativeProceduralPluginRegistry final : public HfPluginRegistry
{
public:
    ///
    /// Returns the singleton registry for \c HdSceneIndexPlugin
    ///
    HDGP_API
    static HdGpGenerativeProceduralPluginRegistry &GetInstance();

    ///
    /// Entry point for defining an HdSceneIndexPlugin plugin.
    ///
    template<typename T, typename... Bases>
    static void Define();

    HDGP_API
    HdGpGenerativeProcedural *ConstructProcedural(
        const TfToken &proceduralTypeName,
        const SdfPath &proceduralPrimPath);

private:
    friend class TfSingleton<HdGpGenerativeProceduralPluginRegistry>;

    // Singleton gets private constructed
    HdGpGenerativeProceduralPluginRegistry();
    ~HdGpGenerativeProceduralPluginRegistry() override;

};


template<typename T, typename... Bases>
void HdGpGenerativeProceduralPluginRegistry::Define()
{
    HfPluginRegistry::Define<T, HdGpGenerativeProceduralPlugin, Bases...>();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif

