//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_PLUG_PLUGIN_H
#define PXR_BASE_PLUG_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/base/plug/api.h"

#include "pxr/base/js/types.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/weakBase.h"

#include <atomic>
#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(PlugPlugin);

class Plug_RegistrationMetadata;
class TfType;

/// \class PlugPlugin
///
/// Defines an interface to registered plugins.
///
/// Plugins are registered using the interfaces in \c PlugRegistry.
///
/// For each registered plugin, there is an instance of
/// \c PlugPlugin which can be used to load and unload
/// the plugin and to retrieve information about the
/// classes implemented by the plugin.
///
class PlugPlugin : public TfWeakBase {
public:
    PLUG_API ~PlugPlugin();

    /// Loads the plugin.
    /// This is a noop if the plugin is already loaded.
    PLUG_API bool Load();

    /// Returns \c true if the plugin is currently loaded.  Resource
    /// plugins always report as loaded.
    PLUG_API bool IsLoaded() const;

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    /// Returns \c true if the plugin is a python module.
    PLUG_API bool IsPythonModule() const;
#endif // PXR_PYTHON_SUPPORT_ENABLED

    /// Returns \c true if the plugin is resource-only.
    PLUG_API bool IsResource() const;

    /// Returns the dictionary containing meta-data for the plugin.
    PLUG_API JsObject GetMetadata();

    /// Returns the metadata sub-dictionary for a particular type.
    PLUG_API JsObject GetMetadataForType(const TfType &type);

    /// Returns the dictionary containing the dependencies for the plugin.
    PLUG_API JsObject GetDependencies();

    /// Returns true if \p type is declared by this plugin. 
    /// If \p includeSubclasses is specified, also returns true if any 
    /// subclasses of \p type have been declared.
    PLUG_API bool DeclaresType(const TfType& type, bool includeSubclasses = false) const;

    /// Returns the plugin's name.
    std::string const &GetName() const {
        return _name;
    }

    /// Returns the plugin's filesystem path.
    std::string const &GetPath() const {
        return _path;
    }

    /// Returns the plugin's resources filesystem path.
    std::string const &GetResourcePath() const {
        return _resourcePath;
    }

    /// Build a plugin resource path by returning a given absolute path or
    /// combining the plugin's resource path with a given relative path.
    PLUG_API std::string MakeResourcePath(const std::string& path) const;

    /// Find a plugin resource by absolute or relative path optionally
    /// verifying that file exists.  If verification fails an empty path
    /// is returned.  Relative paths are relative to the plugin's resource
    /// path.
    PLUG_API std::string FindPluginResource(const std::string& path, bool verify = true) const;

private:
    enum _Type {
        LibraryType, 
        PythonType, 
        ResourceType 
    };

    // Private ctor, plugins are constructed only by PlugRegistry.
    PLUG_LOCAL
    PlugPlugin(const std::string & path,
               const std::string & name,
               const std::string & resourcePath,
               const JsObject & plugInfo,
               _Type type);

    PLUG_LOCAL
    static PlugPluginPtr _GetPluginForType(const TfType & type);

    PLUG_LOCAL
    static void _RegisterAllPlugins();
    PLUG_LOCAL
    static PlugPluginPtr _GetPluginWithName(const std::string& name);
    PLUG_LOCAL
    static PlugPluginPtrVector _GetAllPlugins();

    template <class PluginMap>
    PLUG_LOCAL
    static std::pair<PlugPluginPtr, bool>
    _NewPlugin(const Plug_RegistrationMetadata &metadata,
               _Type pluginType,
               const std::string& pluginCreationPath,
               PluginMap *allPluginsByNamePtr);

    PLUG_LOCAL
    static std::pair<PlugPluginPtr, bool> 
    _NewDynamicLibraryPlugin(const Plug_RegistrationMetadata& metadata);

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    PLUG_LOCAL
    static std::pair<PlugPluginPtr, bool> 
    _NewPythonModulePlugin(const Plug_RegistrationMetadata& metadata);
#endif // PXR_PYTHON_SUPPORT_ENABLED

    PLUG_LOCAL
    static std::pair<PlugPluginPtr, bool> 
    _NewResourcePlugin(const Plug_RegistrationMetadata& metadata);

    PLUG_LOCAL
    bool _Load();

    PLUG_LOCAL
    void _DeclareAliases( TfType t, const JsObject & metadata );
    PLUG_LOCAL
    void _DeclareTypes();
    PLUG_LOCAL
    void _DeclareType(const std::string &name, const JsObject &dict);
    PLUG_LOCAL
    static void _DefineType( TfType t );

    struct _SeenPlugins;
    PLUG_LOCAL
    bool _LoadWithDependents(_SeenPlugins * seenPlugins);

    PLUG_LOCAL
    static void _UpdatePluginMaps( const TfType & baseType );

    PLUG_LOCAL
    static constexpr char const *_GetPluginTypeDisplayName(_Type type);

private:
    std::string _name;
    std::string _path;
    std::string _resourcePath;
    JsObject _dict;
    void *_handle;      // the handle returned by ArchLibraryOpen() is a void*
    std::atomic<bool> _isLoaded;
    _Type _type;

    friend class PlugRegistry;
};

/// Find a plugin's resource by absolute or relative path optionally
/// verifying that file exists.  If \c plugin is \c NULL or verification
/// fails an empty path is returned.  Relative paths are relative to the
/// plugin's resource path.
PLUG_API
std::string
PlugFindPluginResource(const PlugPluginPtr& plugin,
                       const std::string& path, bool verify = true);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_PLUG_PLUGIN_H
