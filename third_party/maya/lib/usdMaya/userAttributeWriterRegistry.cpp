//
// Copyright 2019 Pixar
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

#include "usdMaya/userAttributeWriterRegistry.h"

#include "usdMaya/registryHelper.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    using _WriterRegistry = std::map<TfToken, UsdMayaUserAttributeWriterRegistry::UserAttributeWriter>;
    _WriterRegistry _writerReg;
}

TfTokenVector UsdMayaUserAttributeWriterRegistry::_ListWriters()
{
    UsdMaya_RegistryHelper::LoadUserAttributeWriterPlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaUserAttributeWriterRegistry>();
    TfTokenVector ret;
    ret.reserve(_writerReg.size());
    for (const auto& nameAndWriter : _writerReg) {
        ret.push_back(nameAndWriter.first);
    }
    return ret;
}

void UsdMayaUserAttributeWriterRegistry::RegisterWriter(
    const std::string& name,
    const UserAttributeWriter& fn)
{
    _writerReg.insert(std::make_pair(TfToken(name), fn));
}

UsdMayaUserAttributeWriterRegistry::UserAttributeWriter UsdMayaUserAttributeWriterRegistry::_GetWriter(const TfToken& name)
{
    UsdMaya_RegistryHelper::LoadUserAttributeWriterPlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaUserAttributeWriterRegistry>();
    const auto it = _writerReg.find(name);
    return it == _writerReg.end() ? nullptr : it->second;
}

TF_INSTANTIATE_SINGLETON(UsdMayaUserAttributeWriterRegistry);

UsdMayaUserAttributeWriterRegistry&
UsdMayaUserAttributeWriterRegistry::GetInstance()
{
    return TfSingleton<UsdMayaUserAttributeWriterRegistry>::GetInstance();
}

UsdMayaUserAttributeWriterRegistry::UsdMayaUserAttributeWriterRegistry()
{

}

UsdMayaUserAttributeWriterRegistry::~UsdMayaUserAttributeWriterRegistry()
{

}

PXR_NAMESPACE_CLOSE_SCOPE
