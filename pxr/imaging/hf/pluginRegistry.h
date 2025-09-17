//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HF_PLUGIN_REGISTRY_H
#define PXR_IMAGING_HF_PLUGIN_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hf/api.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hf/pluginDesc.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/type.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE


class HfPluginBase;
class Hf_PluginEntry;


///
/// \class HfPluginRegistry
///
/// Base class for registering Hydra plugins using the plug mechanism.
/// It is expected that each plugin has a pluginfo.json file that contains
/// a list of types, where each type provides a list of base classes,
/// displayName and priority.
///
/// The priority is used to order plugins, with the plugin with the highest
/// priority being at the front of the order.  priority is a signed integer.
/// In the event of two plugins having the same priority, the plugins are sorted
/// alphabetically on the type name.
///
/// The plugin sorted to the front is used as the default plugin, when not
/// specified.
///
/// Example:
///
///{
///    "Types": {
///        "CPPTypeName": {
///            "bases": ["BaseTypeName"],
///            "displayName": "Human Readable Name",
///            "priority" : 0
///       }
///    }
///}
///
class HfPluginRegistry
{

public:
    ///
    /// Returns an ordered list of all registered plugins.
    /// The plugins are ordered by priority then alphabetically
    ///
    HF_API
    void GetPluginDescs(HfPluginDescVector *plugins);

    ///
    /// Returns the description for the given plugin id.
    /// The plugin may not be loaded or been actually created yet.
    ///
    HF_API
    bool GetPluginDesc(const TfToken &pluginId, HfPluginDesc *desc);

    ///
    /// Increment the reference count on an existing plugin.
    ///
    HF_API
    void AddPluginReference(HfPluginBase *plugin);

    ///
    /// Decrement the reference count on the plugin.  If the
    /// reference count get to 0, the plugin is freed.
    ///
    HF_API
    void ReleasePlugin(HfPluginBase *plugin);

    ///
    /// Returns true if a plugin has been registered for the given id.
    /// The plugin may not be loaded or been actually created yet.
    ///
    HF_API
    bool IsRegisteredPlugin(const TfToken &pluginId);

    HF_API
    TfToken GetPluginId(const HfPluginBase *plugin) const;

protected:
    // Must be derived.

    ///
    /// Constructs a Plugin Registry.
    /// pluginBaseType is the TfType of the class derived from
    /// HfPluginBase that provides the plugin API.
    ///
    HF_API
    HfPluginRegistry(const TfType &pluginBaseType);
    HF_API
    virtual ~HfPluginRegistry();

    ///
    /// Returns the plugin from the given pluginId.
    /// The reference count on the plugin is automatically increased.
    ///
    HF_API
    HfPluginBase *GetPlugin(const TfToken &pluginId);

    ///
    /// Entry point for registering a types implementation.
    /// T is the plugin being registered.
    /// PluginBaseType is the HfPluginBase derived class
    /// that specifies the API (the same one the TfType is for in
    /// the constructor).
    ///
    /// Bases optionally specifies other classes that T is derived from.
    ///
    template<typename T, typename PluginBaseType, typename... Bases>
    static void Define();

    /// Gives subclasses an opportunity to inspect plugInfo-based metadata
    /// at the time of discovery.
    HF_API
    virtual void _CollectAdditionalMetadata(
        const PlugRegistry &plugRegistry, const TfType &pluginType);

private:
    typedef std::vector<Hf_PluginEntry> _PluginEntryVector;
    typedef std::map<TfToken, size_t> _TokenMap;

    //
    // The purpose of this group of functions is to provide a factory
    // to create the registered plugin with the type system
    // without exposing the internal class _PluginEntry
    ///
    typedef std::function<HfPluginBase *()> _FactoryFn;

    template<typename T>
    static HfPluginBase *_CreatePlugin();

    HF_API
    static void _SetFactory(TfType &type, _FactoryFn &func);

    TfType                     _pluginBaseType;

    //
    // Plugins are stored in a ordered list (as a vector).  The token
    // map converts from plugin id into an index in the list.
    //
    _PluginEntryVector         _pluginEntries;
    _TokenMap                  _pluginIndex;

    // Plugin discovery is deferred until first use.
    bool                       _pluginCachePopulated;

    // Use the Plug system to discover plugins from the meta data.
    void _DiscoverPlugins();

    // Find the plugin entry for the given plugin object.
    Hf_PluginEntry *_GetEntryForPlugin(HfPluginBase *plugin);

    ///
    /// This class is not intended to be copied.
    ///
    HfPluginRegistry()                                    = delete;
    HfPluginRegistry(const HfPluginRegistry &)            = delete;
    HfPluginRegistry &operator=(const HfPluginRegistry &) = delete;
};

template<typename T>
HfPluginBase *
HfPluginRegistry::_CreatePlugin()
{
    HF_MALLOC_TAG_FUNCTION();
    return new T;
}


template<typename T, typename PluginBaseType, typename... Bases>
void
HfPluginRegistry::Define()
{
    TfType type = TfType::Define<T,
                                  TfType::Bases<PluginBaseType, Bases...> >();

    _FactoryFn func = &_CreatePlugin<T>;
    _SetFactory(type, func);
}



PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HF_PLUGIN_REGISTRY_H

