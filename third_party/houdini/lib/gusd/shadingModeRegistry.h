//
// Copyright 2018 Pixar
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
#ifndef __GUSD_SHADEROUTPUTREGISTRY_H__
#define __GUSD_SHADEROUTPUTREGISTRY_H__

#include <pxr/pxr.h>

#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/singleton.h>
#include <pxr/base/tf/weakPtr.h>

#include "gusd/api.h"

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <UT/UT_Map.h>

#include <vector>
#include <tuple>
#include <functional>

class OP_Node;
class OP_OperatorTable;

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(GusdShaderOutputRegistry);

/// Class for registering and querying shader exporters.
/// To make sure the plugins will be loaded before creating
/// the user interface for USD Output, the plugin registry
/// loads registered plugins.
/// Add the following snippet to plugInfo.json to tell the registry which
/// plugin to load.
/// "Info" : {
///     "UsdHoudini" : {
///         "ShadingModePlugin" : true
///     }
/// },
/// Plugin loads happen via Houdini, so the library needs to be a valid
/// Houdini plugin. Example:
/// #include <SYS/SYS_Version.h>
/// #include <UT/UT_DSOVersion.h>
/// #include <OP/OP_OperatorTable.h>
/// 
/// extern "C" {
///     void newDriverOperator(OP_OperatorTable* operators) { }
/// }
class GusdShadingModeRegistry : public TfWeakBase
{
private:
    GusdShadingModeRegistry() = default;
public:
    using HouMaterialMap = UT_Map<std::string, std::vector<SdfPath>>;
    using ExporterFn = std::function<
        void(OP_Node*, const UsdStagePtr&, const SdfPath&,
             const HouMaterialMap&, const std::string&)>;
    using ExporterList = std::vector<std::tuple<TfToken, TfToken>>;

    static ExporterFn
    getExporter(const TfToken& name) {
        return getInstance()._getExporter(name);
    }

    static ExporterList
    listExporters() {
        return getInstance()._listExporters();
    }

    GUSD_API
    static GusdShadingModeRegistry& getInstance();

    GUSD_API
    bool registerExporter(
        const std::string& name,
        const std::string& label,
        ExporterFn fn);

    GUSD_API
    static void loadPlugins(OP_OperatorTable* table);
private:
    friend class TfSingleton<GusdShadingModeRegistry>;

    ExporterFn _getExporter(const TfToken& name);
    ExporterList _listExporters();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_SHADEROUTPUTREGISTRY_H__
