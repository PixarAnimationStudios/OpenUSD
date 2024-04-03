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

