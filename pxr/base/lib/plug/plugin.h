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
#ifndef PLUG_PLUGIN_H
#define PLUG_PLUGIN_H

#include "pxr/base/plug/api.h"

#include "pxr/base/js/types.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"

#include <boost/noncopyable.hpp>
#include <atomic>
#include <string>
#include <utility>
#include <vector>

TF_DECLARE_WEAK_AND_REF_PTRS(PlugPlugin);

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
class PlugPlugin : public TfRefBase, public TfWeakBase {
public:
	PLUG_API virtual ~PlugPlugin();

    /// Loads the plugin.
    /// This is a noop if the plugin is already loaded.
	PLUG_API bool Load();

    /// Returns \c true if the plugin is currently loaded.  Resource
    /// plugins always report as loaded.
	PLUG_API bool IsLoaded() const;

    /// Returns \c true if the plugin is a python module.
	PLUG_API bool IsPythonModule() const;

    /// Returns \c true if the plugin is resource-only.
	PLUG_API bool IsResource() const;

    /// Returns the dictionary containing meta-data for the plugin.
	PLUG_API JsObject GetMetadata();

    /// Returns the metdata sub-dictionary for a particular type.
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

    /// Build a plugin resource path by returing a given absolute path or
    /// combining the plugin's resource path with a given relative path.
	PLUG_API std::string MakeResourcePath(const std::string& path) const;

    /// Find a plugin resource by absolute or relative path optionally
    /// verifying that file exists.  If verification fails an empty path
    /// is returned.  Relative paths are relative to the plugin's resource
    /// path.
    PLUG_API std::string FindPluginResource(const std::string& path, bool verify = true) const;

private:
    enum _Type { LibraryType, PythonType, ResourceType };

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

    PLUG_LOCAL
    static std::pair<PlugPluginPtr, bool> 
    _NewDynamicLibraryPlugin(const std::string & path,
                             const std::string & name,
                             const std::string & dsoPath,
                             const std::string & resourcePath,
                             const JsObject & plugInfo);

    PLUG_LOCAL
    static std::pair<PlugPluginPtr, bool> 
    _NewPythonModulePlugin(const std::string & path,
                           const std::string & name,
                           const std::string & resourcePath,
                           const JsObject & plugInfo);

    PLUG_LOCAL
    static std::pair<PlugPluginPtr, bool> 
    _NewResourcePlugin(const std::string & path,
                       const std::string & name,
                       const std::string & resourcePath,
                       const JsObject & plugInfo);

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

#endif // PLUG_PLUGIN_H
