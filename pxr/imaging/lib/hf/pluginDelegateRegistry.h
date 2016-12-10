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
#ifndef HF_PLUGIN_DELEGATE_REGISTRY_H
#define HF_PLUGIN_DELEGATE_REGISTRY_H

#include "pxr/base/tf/type.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hf/pluginDelegateDesc.h"
#include <map>

class HfPluginDelegateBase;
class Hf_PluginDelegateEntry;


///
/// \class HfPluginDelegateRegistry
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
///            "bases": ["BaseDelegateTypeName"],
///            "displayName": "Human Readable Name",
///            "priority" : 0
///       }
///    }
///}
///
class HfPluginDelegateRegistry
{

public:
    ///
    /// Returns the id of delegate to use as the default
    ///
    TfToken GetDefaultDelegateId();

    ///
    /// Returns an ordered list of all registered delegates.
    /// The delegates are ordered by priority then alphabetically
    ///
    void GetDelegateDescs(HfPluginDelegateDescVector *delegates);

    ///
    /// Increment the reference count on an existing delegate.
    ///
    void AddDelegateReference(HfPluginDelegateBase *delegate);

    ///
    /// Decrement the reference count on the delegate.  If the
    /// reference count get to 0, the delegate is freed.
    ///
    void ReleaseDelegate(HfPluginDelegateBase *delegate);

    ///
    /// Returns true if a delegate has been registered for the given id.
    /// The delegate may not be loaded or been actually created yet.
    ///
    bool IsRegisteredDelegate(const TfToken &delegateId) const;

protected:
    // Must be derived.

    ///
    /// Constructs a Delegate Registry.
    /// delegateBaseType is the TfType of the class derived from
    /// HfPluginDelegate that provides the delegate API.
    ///
    HfPluginDelegateRegistry(const TfType &delegateBaseType);
    virtual ~HfPluginDelegateRegistry();

    ///
    /// Returns the delegate from the given delegateId.
    /// The reference count on the deletegate is automatically increased.
    ///
    HfPluginDelegateBase *GetDelegate(const TfToken &delegateId);

    ///
    /// Entry point for registering a types implementation.
    /// T is the delegate being registered.
    /// DelegateBaseType is the HfPluginDelegate derived class
    /// that specifies the API (the same one the TfType is for in
    /// the constructor).
    ///
    /// Bases optionally specifies other classes that T is derived from.
    ///
    template<typename T, typename DelegateBaseType, typename... Bases>
    static void Define();

private:
    typedef std::vector<Hf_PluginDelegateEntry> _PluginDelegateEntryVector;
    typedef std::map<TfToken, size_t> _TokenMap;

    //
    // The purpose of this group of functions is to provide a factory
    // to create the registered plugin delegate with the type system
    // without exposing the internal class _PluginDelegateEntry
    ///
    typedef std::function<HfPluginDelegateBase *()> _FactoryFn;

    template<typename T>
    static HfPluginDelegateBase *_CreateDelegate();
    static void _SetFactory(TfType &type, _FactoryFn &func);

    TfType                     _delegateBaseType;

    //
    // Plugins are stored in a ordered list (as a vector).  The token
    // map converts from plugin id into an index in the list.
    //
    _PluginDelegateEntryVector _delegateEntries;
    _TokenMap                  _delegateIndex;

    // Plugin discovery is deferred until first use.
    bool                       _delegateCachePopulated;

    // Use the Plug system to discover delegates from the meta data.
    void _DiscoverDelegates();

    // Find the plugin entry for the given delegate
    Hf_PluginDelegateEntry *_GetEntryForDelegate(HfPluginDelegateBase *delegate);

    ///
    /// This class is not intended to be copied.
    ///
    HfPluginDelegateRegistry()                                            = delete;
    HfPluginDelegateRegistry(const HfPluginDelegateRegistry &)            = delete;
    HfPluginDelegateRegistry &operator=(const HfPluginDelegateRegistry &) = delete;
};

template<typename T>
HfPluginDelegateBase *
HfPluginDelegateRegistry::_CreateDelegate()
{
    HF_MALLOC_TAG_FUNCTION();
    return new T;
}


template<typename T, typename DelegateBaseType, typename... Bases>
void
HfPluginDelegateRegistry::Define()
{
    TfType type = TfType::Define<T,
                                  TfType::Bases<DelegateBaseType, Bases...> >();

    _FactoryFn func = &_CreateDelegate<T>;
    _SetFactory(type, func);
}


#endif //HF_PLUGIN_DELEGATE_REGISTRY_H

