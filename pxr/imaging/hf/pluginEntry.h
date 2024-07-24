//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HF_PLUGIN_ENTRY_H
#define PXR_IMAGING_HF_PLUGIN_ENTRY_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/imaging/hf/perfLog.h"
#include <string>
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class HfPluginBase;
struct HfPluginDesc;



///
/// Internal class that manages a single plugin.
///
class Hf_PluginEntry final {
public:
    HF_MALLOC_TAG_NEW("new Hf_PluginEntry");

    ///
    /// Functor that is used to create a plugin.
    /// This is used instead of using TfType::FactoryBase
    /// as that would require exposing the class hierarchy publicly
    /// due to templating and the idea is that this class is _Factory
    /// below are private.
    ///
    typedef std::function<HfPluginBase *()> _PluginFactoryFn;

    ///
    /// Constructors a new plugin entry from information in the
    /// plugins metadata file.  See HfPluginRegistry.
    ///
    Hf_PluginEntry(const TfType &type,
                   const std::string &displayName,
                   int   priority);
    ~Hf_PluginEntry();

    ///
    /// For containers, allow moving only (no copying)
    ///
    Hf_PluginEntry(Hf_PluginEntry &&source);
    Hf_PluginEntry &operator =(Hf_PluginEntry &&source);

    ///
    /// Simple Accessors
    ///
    const TfType         &GetType()        const { return _type;             }
    const std::string    &GetDisplayName() const { return _displayName;      }
    int                   GetPriority()    const { return _priority;         }
    HfPluginBase         *GetInstance()    const { return _instance;         }

    ///
    /// Returns the internal name of the plugin that is used by the API's.
    ///
    TfToken            GetId() const;

    ///
    /// Fills in a plugin description structure that is used to communicate
    /// with the application information about this plugin.
    ///
    void               GetDesc(HfPluginDesc *desc) const;

    ///
    /// Ref counting the instance of the plugin.  Each plugin is only
    /// instantiated once.
    ///
    void IncRefCount();
    void DecRefCount();

    ///
    /// For sorting:
    /// Entries are ordered by priority then alphabetical order of type name
    ///
    bool operator <(const Hf_PluginEntry &other) const;

    ///
    /// Type Factory Creation
    ///
    static void SetFactory(TfType &type, _PluginFactoryFn &func);

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

        _Factory(_PluginFactoryFn &func) : _func(func) {}

        HfPluginBase *New() const { return _func(); }

    private:
        _PluginFactoryFn _func;
    };


    TfType                _type;
    std::string           _displayName;
    int                   _priority;
    HfPluginBase         *_instance;
    int                   _refCount;

    ///
    /// Don't allow copying
    ///
    Hf_PluginEntry()                                   = delete;
    Hf_PluginEntry(const Hf_PluginEntry &)             = delete;
    Hf_PluginEntry &operator =(const Hf_PluginEntry &) = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HF_PLUGIN_ENTRY_H
