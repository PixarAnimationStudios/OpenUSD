//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hf/pluginRegistry.h"
#include "pxr/imaging/hf/pluginBase.h"
#include "pxr/imaging/hf/pluginEntry.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/plug/plugin.h"

PXR_NAMESPACE_OPEN_SCOPE


//
// Plugin Metadata Keys
//
static const char *DISPLAY_NAME = "displayName";
static const char *PRIORITY     = "priority";

HfPluginRegistry::HfPluginRegistry(const TfType &pluginBaseType)
 : _pluginBaseType(pluginBaseType)
 , _pluginEntries()
 , _pluginIndex()
 , _pluginCachePopulated(false)
{
}

HfPluginRegistry::~HfPluginRegistry()
{
}

void
HfPluginRegistry::GetPluginDescs(HfPluginDescVector *plugins)
{
    if (!_pluginCachePopulated) {
        _DiscoverPlugins();
    }

    // Expect the plugin list to be empty
    if (!TF_VERIFY(plugins->empty())) {
        plugins->clear();
    }

    plugins->resize(_pluginEntries.size());
    for (size_t index = 0; index < _pluginEntries.size(); ++index) {
        const Hf_PluginEntry &entry = _pluginEntries[index];
        HfPluginDesc *desc =  &(*plugins)[index];

        entry.GetDesc(desc);
    }
}

bool
HfPluginRegistry::GetPluginDesc(const TfToken &pluginId, HfPluginDesc *desc)
{
    if (!_pluginCachePopulated) {
        _DiscoverPlugins();
    }

    _TokenMap::const_iterator it = _pluginIndex.find(pluginId);
    if (it == _pluginIndex.end()) {
        return false;
    }

    Hf_PluginEntry &entry = _pluginEntries[it->second];
    entry.GetDesc(desc);

    return true;
}

void
HfPluginRegistry::AddPluginReference(HfPluginBase *plugin)
{
    Hf_PluginEntry *entry = _GetEntryForPlugin(plugin);
    if (entry == nullptr) {
        return;
    }

    entry->IncRefCount();
}

void
HfPluginRegistry::ReleasePlugin(HfPluginBase *plugin)
{
    if (plugin == nullptr) {
        return;
    }

    Hf_PluginEntry *entry = _GetEntryForPlugin(plugin);
    if (entry == nullptr) {
        return;
    }

    entry->DecRefCount();
}

bool
HfPluginRegistry::IsRegisteredPlugin(const TfToken &pluginId)
{
    if (!_pluginCachePopulated) {
        _DiscoverPlugins();
    }

    _TokenMap::const_iterator it = _pluginIndex.find(pluginId);
    return (it != _pluginIndex.end());
}

HfPluginBase *
HfPluginRegistry::GetPlugin(const TfToken &pluginId)
{
    if (!_pluginCachePopulated) {
        _DiscoverPlugins();
    }

    _TokenMap::const_iterator it = _pluginIndex.find(pluginId);
    if (it == _pluginIndex.end()) {
        return nullptr;
    }

    Hf_PluginEntry &entry = _pluginEntries[it->second];

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
HfPluginRegistry::_SetFactory(TfType &type, _FactoryFn &func)
{
    Hf_PluginEntry::SetFactory(type, func);
}

void
HfPluginRegistry::_CollectAdditionalMetadata(
    const PlugRegistry &plugRegistry, const TfType &pluginType)
{
    // base implementation does nothing.
}

void
HfPluginRegistry::_DiscoverPlugins()
{
    // This should only be done once on an empty cache.
    // If not empty name clashes may occur, but it means
    // new information will not be picked up.
    TF_VERIFY(_pluginEntries.empty());

    typedef std::set<TfType> TfTypeSet;

    PlugRegistry &pluginRegistry = PlugRegistry::GetInstance();

    TfTypeSet pluginTypes;
    pluginRegistry.GetAllDerivedTypes(_pluginBaseType, &pluginTypes);

    _pluginEntries.reserve(pluginTypes.size());

    for (TfTypeSet::const_iterator it  = pluginTypes.begin();
                                   it != pluginTypes.end();
                                 ++it) {
        const TfType &pluginType = *it;

        const std::string &displayName =
                pluginRegistry.GetStringFromPluginMetaData(pluginType,
                                                           DISPLAY_NAME);
        const JsValue &priorityValue =
                pluginRegistry.GetDataFromPluginMetaData(pluginType,
                                                         PRIORITY);

        if (displayName.empty() || !priorityValue.IsInt()) {
            TF_WARN("Plugin %s type information incomplete",
                    pluginType.GetTypeName().c_str());
        } else {
            int priority = priorityValue.GetInt();

            _pluginEntries.emplace_back(pluginType,
                                        displayName,
                                        priority);
        }

        _CollectAdditionalMetadata(pluginRegistry, pluginType);
    }

    // Sort entries according to policy (in operator <)
    std::sort(_pluginEntries.begin(), _pluginEntries.end());

    // Now sorted create index for fast lookup
    for (size_t index = 0; index < _pluginEntries.size(); ++index) {
        TfToken id(_pluginEntries[index].GetId());

        _pluginIndex.emplace(id, index);
    }

    _pluginCachePopulated = true;
}

Hf_PluginEntry *
HfPluginRegistry::_GetEntryForPlugin(HfPluginBase *plugin)
{
    const TfType &type = TfType::Find(plugin);
    if (!TF_VERIFY(!type.IsUnknown())) {
        return nullptr;
    }

    TfToken machineName(type.GetTypeName());

    _TokenMap::const_iterator it = _pluginIndex.find(machineName);
    if (!TF_VERIFY(it != _pluginIndex.end())) {
        return nullptr;
    }

    Hf_PluginEntry &entry = _pluginEntries[it->second];

    if (!TF_VERIFY(entry.GetInstance() == plugin)) {
        return nullptr;
    }

    return &entry;
}

TfToken
HfPluginRegistry::GetPluginId(const HfPluginBase * plugin) const
{
    for (const Hf_PluginEntry &entry : _pluginEntries) {
        if (entry.GetInstance() == plugin) {
            return entry.GetId();
        }
    }
    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE

