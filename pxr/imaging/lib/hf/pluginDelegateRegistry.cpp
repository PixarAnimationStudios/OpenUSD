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
#include "pxr/imaging/hf/pluginDelegateRegistry.h"
#include "pxr/imaging/hf/pluginDelegateBase.h"
#include "pxr/imaging/hf/pluginDelegateEntry.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

PXR_NAMESPACE_OPEN_SCOPE


//
// Plugin Metadata Keys
//
static const char *DISPLAY_NAME = "displayName";
static const char *PRIORITY     = "priority";

HfPluginDelegateRegistry::HfPluginDelegateRegistry(const TfType &delegateBaseType)
 : _delegateBaseType(delegateBaseType)
 , _delegateEntries()
 , _delegateIndex()
 , _delegateCachePopulated(false)
{
}

HfPluginDelegateRegistry::~HfPluginDelegateRegistry()
{
}

TfToken
HfPluginDelegateRegistry::GetDefaultDelegateId()
{
    if (!_delegateCachePopulated) {
        _DiscoverDelegates();
    }

    if (!TF_VERIFY(!_delegateEntries.empty())) {
        return TfToken();
    }


    return _delegateEntries[0].GetId();
}

void
HfPluginDelegateRegistry::GetDelegateDescs(HfPluginDelegateDescVector *delegates)
{
    if (!_delegateCachePopulated) {
        _DiscoverDelegates();
    }

    // Expect the delegate list to be empty
    if (!TF_VERIFY(delegates->empty())) {
        delegates->clear();
    }

    delegates->resize(_delegateEntries.size());
    for (size_t index = 0; index < _delegateEntries.size(); ++index) {
        const Hf_PluginDelegateEntry &entry = _delegateEntries[index];
        HfPluginDelegateDesc *desc =  &(*delegates)[index];

        entry.GetDelegateDesc(desc);
    }
}

void
HfPluginDelegateRegistry::AddDelegateReference(HfPluginDelegateBase *delegate)
{
    Hf_PluginDelegateEntry *entry = _GetEntryForDelegate(delegate);
    if (entry == nullptr) {
        return;
    }

    entry->IncRefCount();
}

void
HfPluginDelegateRegistry::ReleaseDelegate(HfPluginDelegateBase *delegate)
{
    if (delegate == nullptr) {
        return;
    }

    Hf_PluginDelegateEntry *entry = _GetEntryForDelegate(delegate);
    if (entry == nullptr) {
        return;
    }

    entry->DecRefCount();
}

bool
HfPluginDelegateRegistry::IsRegisteredDelegate(const TfToken &delegateId)
{
    if (!_delegateCachePopulated) {
        _DiscoverDelegates();
    }

    _TokenMap::const_iterator it = _delegateIndex.find(delegateId);
    return (it != _delegateIndex.end());
}

HfPluginDelegateBase *
HfPluginDelegateRegistry::GetDelegate(const TfToken &delegateId)
{
    if (!_delegateCachePopulated) {
        _DiscoverDelegates();
    }

    _TokenMap::const_iterator it = _delegateIndex.find(delegateId);
    if (it == _delegateIndex.end()) {
        return nullptr;
    }

    Hf_PluginDelegateEntry &entry = _delegateEntries[it->second];

    if (entry.GetInstance() == nullptr) {
        // If instance has not be created yet, make sure plugin is loaded
        PlugRegistry &pluginRegistry = PlugRegistry::GetInstance();

        PlugPluginPtr plugin = pluginRegistry.GetPluginForType(entry.GetType());
        if (!TF_VERIFY(plugin)) {
            return nullptr;
        }

        if (!plugin->Load()) {
            return nullptr;
        }
    }

    // This will create the instance if nessasary.
    entry.IncRefCount();


    return entry.GetInstance();
}

// static
void
HfPluginDelegateRegistry::_SetFactory(TfType &type, _FactoryFn &func)
{
    Hf_PluginDelegateEntry::SetFactory(type, func);
}

void
HfPluginDelegateRegistry::_DiscoverDelegates()
{
    // This should only be done once on an empty cache.
    // If not empty name clashes may occur, but it means
    // new information will not be picked up.
    TF_VERIFY(_delegateEntries.empty());

    typedef std::set<TfType> TfTypeSet;

    PlugRegistry &pluginRegistry = PlugRegistry::GetInstance();

    TfTypeSet delegateTypes;
    pluginRegistry.GetAllDerivedTypes(_delegateBaseType, &delegateTypes);

    _delegateEntries.reserve(delegateTypes.size());

    for (TfTypeSet::const_iterator it  = delegateTypes.begin();
                                   it != delegateTypes.end();
                                 ++it) {
        const TfType &delegateType = *it;

        const std::string &displayName =
                pluginRegistry.GetStringFromPluginMetaData(delegateType,
                                                           DISPLAY_NAME);
        const JsValue &priorityValue =
                pluginRegistry.GetDataFromPluginMetaData(delegateType,
                                                         PRIORITY);

        if (displayName.empty() || !priorityValue.IsInt()) {
            TF_WARN("Delegate Pluging %s plugin type information incomplete",
                    delegateType.GetTypeName().c_str());
        } else {
            int priority = priorityValue.GetInt();

            _delegateEntries.emplace_back(delegateType,
                                          displayName,
                                          priority);
        }
    }

    // Sort entries according to policy (in operator <)
    std::sort(_delegateEntries.begin(), _delegateEntries.end());

    // Now sorted create index for fast lookup
    for (size_t index = 0; index < _delegateEntries.size(); ++index) {
        TfToken id(_delegateEntries[index].GetId());

        _delegateIndex.emplace(id, index);
    }

    _delegateCachePopulated = true;
}

Hf_PluginDelegateEntry *
HfPluginDelegateRegistry::_GetEntryForDelegate(HfPluginDelegateBase *delegate)
{
    const TfType &type = TfType::Find(delegate);
    if (!TF_VERIFY(!type.IsUnknown())) {
        return nullptr;
    }

    TfToken machineName(type.GetTypeName());

    _TokenMap::const_iterator it = _delegateIndex.find(machineName);
    if (!TF_VERIFY(it != _delegateIndex.end())) {
        return nullptr;
    }

    Hf_PluginDelegateEntry &entry = _delegateEntries[it->second];

    if (!TF_VERIFY(entry.GetInstance() == delegate)) {
        return nullptr;
    }

    return &entry;
}

PXR_NAMESPACE_CLOSE_SCOPE

