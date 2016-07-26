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

#include "pxr/base/tf/instantiateSingleton.h"

TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaShadingModeTokens, PXRUSDMAYA_SHADINGMODE_TOKENS);

typedef std::map<TfToken, PxrUsdMayaShadingModeExporter> _ExportRegistry;
static _ExportRegistry _exportReg;

bool
PxrUsdMayaShadingModeRegistry::RegisterExporter(
        const std::string& name,
        PxrUsdMayaShadingModeExporter fn)
{
    std::pair<_ExportRegistry::const_iterator, bool> insertStatus = _exportReg.insert(
            std::make_pair(TfToken(name), fn));
    return insertStatus.second;
}

PxrUsdMayaShadingModeExporter 
PxrUsdMayaShadingModeRegistry::_GetExporter(const TfToken& name)
{
    TfRegistryManager::GetInstance().SubscribeTo<PxrUsdMayaShadingModeExportContext>();
    return _exportReg[name];
}

typedef std::map<TfToken, PxrUsdMayaShadingModeImporter> _ImportRegistry;
static _ImportRegistry _importReg;
bool
PxrUsdMayaShadingModeRegistry::RegisterImporter(
        const std::string& name,
        PxrUsdMayaShadingModeImporter fn)
{
    std::pair<_ImportRegistry::const_iterator, bool> insertStatus = _importReg.insert(
            std::make_pair(TfToken(name), fn));
    return insertStatus.second;
}

PxrUsdMayaShadingModeImporter 
PxrUsdMayaShadingModeRegistry::_GetImporter(const TfToken& name)
{
    TfRegistryManager::GetInstance().SubscribeTo<PxrUsdMayaShadingModeImportContext>();
    return _importReg[name];
}

TF_INSTANTIATE_SINGLETON(PxrUsdMayaShadingModeRegistry);

PxrUsdMayaShadingModeRegistry&
PxrUsdMayaShadingModeRegistry::GetInstance() 
{
    return TfSingleton<PxrUsdMayaShadingModeRegistry>::GetInstance();
}

PxrUsdMayaShadingModeRegistry::PxrUsdMayaShadingModeRegistry()
{
}

PxrUsdMayaShadingModeRegistry::~PxrUsdMayaShadingModeRegistry()
{
}

