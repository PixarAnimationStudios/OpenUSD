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
#ifndef HF_PLUGIN_DELEGATE_ENTRY_H
#define HF_PLUGIN_DELEGATE_ENTRY_H

class HfPluginDelegateBase;
class HfPluginDelegateDesc;

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/imaging/hf/perfLog.h"
#include <string>
#include <functional>


///
/// Internal class that manages a single Delegate provided by a plug-in.
///
class Hf_PluginDelegateEntry final {
public:
    HF_MALLOC_TAG_NEW("new Hf_PluginDelegateEntry");

    ///
    /// Functor that is used to create a delegate.
    /// This is used instead of using TfType::FactoryBase
    /// as that would require exposing the class hierarchy publicly
    /// due to templating and the idea is that this class is _Factory
    /// below are private.
    ///
    typedef std::function<HfPluginDelegateBase *()> _DelegateFactoryFn;

    ///
    /// Constructors a new Delegate entry from information in the
    /// plugins metadata file.  See HfPluginDelegateRegistry.
    ///
    Hf_PluginDelegateEntry(const TfType &type,
                           const std::string &displayName,
                           int   priority);
    ~Hf_PluginDelegateEntry();

    ///
    /// For containers, allow moving only (no copying)
    ///
    Hf_PluginDelegateEntry(Hf_PluginDelegateEntry &&source);
    Hf_PluginDelegateEntry &operator =(Hf_PluginDelegateEntry &&source);

    ///
    /// Simple Accessors
    ///
    const TfType         &GetType()        const { return _type;             }
    const std::string    &GetDisplayName() const { return _displayName;      }
    int                   GetPriority()    const { return _priority;         }
    HfPluginDelegateBase *GetInstance()    const { return _delegateInstance; }

    ///
    /// Returns the internal name of the delegate that is used by the API's.
    ///
    TfToken            GetId() const;

    ///
    /// Fills in a delegate description structure that is used to communicate
    /// with the application information about this delegate.
    ///
    void               GetDelegateDesc(HfPluginDelegateDesc *desc) const;

    ///
    /// Ref counting the instance of the delegate.  Each delegate is only
    /// Instantiated once.
    ///
    void IncRefCount();
    void DecRefCount();

    ///
    /// For sorting:
    /// Entries are ordered by priority then alphabetical order of type name
    ///
    bool operator <(const Hf_PluginDelegateEntry &other) const;

    ///
    /// Type Factory Creation
    ///
    static void SetFactory(TfType &type, _DelegateFactoryFn &func);

private:
    ///
    /// Factory class used for plugin registration.
    /// Even though this class adds another level of indirection
    /// it's purpose is to abstract away the need to derive the factory
    /// from TfType::FactoryBase, which because of templating was exposing
    /// this class rather than keeping it private.
    ///
    class _Factory final : public TfType::FactoryBase
    {
    public:

        _Factory(_DelegateFactoryFn &func) : _func(func) {}

        HfPluginDelegateBase *New() const { return _func(); }

    private:
        _DelegateFactoryFn _func;
    };


    TfType                _type;
    std::string           _displayName;
    int                   _priority;
    HfPluginDelegateBase *_delegateInstance;
    int                   _refCount;

    ///
    /// Don't allow copying
    ///
    Hf_PluginDelegateEntry()                                           = delete;
    Hf_PluginDelegateEntry(const Hf_PluginDelegateEntry &)             = delete;
    Hf_PluginDelegateEntry &operator =(const Hf_PluginDelegateEntry &) = delete;
};


#endif // HF_PLUGIN_DELEGATE_ENTRY_H
