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
#include "usdMaya/chaserRegistry.h"

#include "usdMaya/debugCodes.h"

#include "pxr/base/tf/instantiateSingleton.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE


UsdMayaChaserRegistry::FactoryContext::FactoryContext(
        const UsdStagePtr& stage,
        const DagToUsdMap& dagToUsdMap,
        const UsdMayaJobExportArgs& jobArgs)
    : _stage(stage)
    , _dagToUsdMap(dagToUsdMap)
    , _jobArgs(jobArgs)
{
}

UsdStagePtr
UsdMayaChaserRegistry::FactoryContext::GetStage() const
{
    return _stage;
}

const UsdMayaChaserRegistry::FactoryContext::DagToUsdMap&
UsdMayaChaserRegistry::FactoryContext::GetDagToUsdMap() const
{
    return _dagToUsdMap;
}

const UsdMayaJobExportArgs&
UsdMayaChaserRegistry::FactoryContext::GetJobArgs() const
{
    return _jobArgs;
}

TF_INSTANTIATE_SINGLETON(UsdMayaChaserRegistry);

std::map<std::string, UsdMayaChaserRegistry::FactoryFn> _factoryRegistry;

bool
UsdMayaChaserRegistry::RegisterFactory(
        const std::string& name,
        FactoryFn fn)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg("registering chaser '%s'.\n", name.c_str());
    auto ret = _factoryRegistry.insert(std::make_pair(name, fn));
    return ret.second;
}

UsdMayaChaserRefPtr
UsdMayaChaserRegistry::Create(
        const std::string& name,
        const FactoryContext& context) const
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaChaserRegistry>();
    if (UsdMayaChaserRegistry::FactoryFn fn = _factoryRegistry[name]) {
        return TfCreateRefPtr(fn(context));
    }
    else {
        return TfNullPtr;
    }
}

std::vector<std::string>
UsdMayaChaserRegistry::GetAllRegisteredChasers() const 
{
    std::vector<std::string> ret;
    for (const auto& p : _factoryRegistry) {
        ret.push_back(p.first);
    }
    return ret;
}

// static
UsdMayaChaserRegistry& 
UsdMayaChaserRegistry::GetInstance()
{
    return TfSingleton<UsdMayaChaserRegistry>::GetInstance();
}

UsdMayaChaserRegistry::UsdMayaChaserRegistry()
{
}

UsdMayaChaserRegistry::~UsdMayaChaserRegistry()
{
}


PXR_NAMESPACE_CLOSE_SCOPE

