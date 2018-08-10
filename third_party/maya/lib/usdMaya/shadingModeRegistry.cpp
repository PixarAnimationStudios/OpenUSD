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
#include "usdMaya/shadingModeRegistry.h"

#include "usdMaya/registryHelper.h"
#include "usdMaya/shadingModeExporter.h"
#include "usdMaya/shadingModeExporterContext.h"
#include "usdMaya/shadingModeImporter.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

#include <map>
#include <string>
#include <utility>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(UsdMayaShadingModeTokens,
    PXRUSDMAYA_SHADINGMODE_TOKENS);


typedef std::map<TfToken, UsdMayaShadingModeExporterCreator> _ExportRegistry;
static _ExportRegistry _exportReg;

bool
UsdMayaShadingModeRegistry::RegisterExporter(
        const std::string& name,
        UsdMayaShadingModeExporterCreator fn)
{
    std::pair<_ExportRegistry::const_iterator, bool> insertStatus =
        _exportReg.insert(
            std::make_pair(TfToken(name), fn));
    return insertStatus.second;
}

UsdMayaShadingModeExporterCreator
UsdMayaShadingModeRegistry::_GetExporter(const TfToken& name)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    const auto it = _exportReg.find(name);
    return it == _exportReg.end() ? nullptr : it->second;
}

typedef std::map<TfToken, UsdMayaShadingModeImporter> _ImportRegistry;
static _ImportRegistry _importReg;

bool
UsdMayaShadingModeRegistry::RegisterImporter(
        const std::string& name,
        UsdMayaShadingModeImporter fn)
{
    std::pair<_ImportRegistry::const_iterator, bool> insertStatus =
        _importReg.insert(
            std::make_pair(TfToken(name), fn));
    return insertStatus.second;
}

UsdMayaShadingModeImporter
UsdMayaShadingModeRegistry::_GetImporter(const TfToken& name)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeImportContext>();
    return _importReg[name];
}

TfTokenVector
UsdMayaShadingModeRegistry::_ListExporters() {
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    TfTokenVector ret;
    ret.reserve(_exportReg.size());
    for (const auto& e : _exportReg) {
        ret.push_back(e.first);
    }
    return ret;
}

TfTokenVector
UsdMayaShadingModeRegistry::_ListImporters() {
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeImportContext>();
    TfTokenVector ret;
    ret.reserve(_importReg.size());
    for (const auto& e : _importReg) {
        ret.push_back(e.first);
    }
    return ret;
}

TF_INSTANTIATE_SINGLETON(UsdMayaShadingModeRegistry);

UsdMayaShadingModeRegistry&
UsdMayaShadingModeRegistry::GetInstance()
{
    return TfSingleton<UsdMayaShadingModeRegistry>::GetInstance();
}

UsdMayaShadingModeRegistry::UsdMayaShadingModeRegistry()
{
}

UsdMayaShadingModeRegistry::~UsdMayaShadingModeRegistry()
{
}


PXR_NAMESPACE_CLOSE_SCOPE
