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

#include "pxr/pxr.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/debugCodes.h"

#include "pxr/base/arch/threads.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/dl.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tracelite/trace.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyInterpreter.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <mutex>
#include <string>
#include <utility>
#include <vector>

using std::pair;
using std::string;
using std::vector;

// Set this to nonzero to insert a pause for debugging, to make registration
// collisions more likely
#define PXRPLUG_TEST_REGISTRATION_SLEEP_MILLISECS 0

#if PXRPLUG_TEST_REGISTRATION_SLEEP_MILLISECS > 0
#include <chrono>
#include <thread>
#endif // PXRPLUG_TEST_REGISTRATION_SLEEP_MILLISECS

PXR_NAMESPACE_OPEN_SCOPE

typedef PlugPlugin::PluginMap _PluginMap;
typedef TfHashMap< std::string, PlugPluginPtr, TfHash > _WeakPluginMap;
typedef TfHashMap< TfType, PlugPluginPtr, TfHash > _ClassMap;

static TfStaticData<_PluginMap> _allPlugins;
static TfStaticData<_WeakPluginMap> _libraryPluginsByDsoPath;
// XXX -- Use _WeakPluginMap
static TfStaticData<_PluginMap> _allPluginsByDynamicLibraryName;
static TfStaticData<_PluginMap> _allPluginsByModuleName;
static TfStaticData<_PluginMap> _allPluginsByResourceName;
static std::mutex _allPluginsMutex;

static TfStaticData<_ClassMap> _classMap;
static std::mutex _classMapMutex;

pair<PlugPluginPtr, bool>
PlugPlugin::_NewPlugin(const Plug_RegistrationMetadata &metadata,
                       _Type pluginType,
                       const char* pluginTypeName,
                       const std::string& pluginCreationPath,
                       PluginMap& allPluginsByName,
                       PluginMap* allPluginsByCreationPath)
{
    const std::string& path = metadata.pluginPath;
    const std::string& name = metadata.pluginName;

    {
        // Already registered?
        std::lock_guard<std::mutex> lock(_allPluginsMutex);
        _PluginMap::const_iterator it = _allPlugins->find(path);
        if (it != _allPlugins->end())
            return std::make_pair(it->second, false);

        // Already registered with the same name but a different path?  Give
        // priority to the path we've registered already and ignore this one.
        it = allPluginsByName.find(name);
        if (it != allPluginsByName.end()) {
            if (!it->second) {
                // A null PlugPluginPtr is used to signal that another thread is
                // attempting to try to register the same plugin
                TF_DEBUG(PLUG_REGISTRATION).Msg(
                        "Another thread currently attempting to"
                        " register %s plugin '%s' - aborting load of '%s'.\n",
                        pluginTypeName, name.c_str(),
                        pluginCreationPath.c_str());
            }
            else {
                TF_DEBUG(PLUG_REGISTRATION).Msg(
                        "Already registered %s plugin '%s' -"
                        " aborting load of '%s'.\n",
                        pluginTypeName, name.c_str(),
                        pluginCreationPath.c_str());
            }
            return std::make_pair(it->second, false);
        }

        // While we still have the lock, insert a nullptr into
        // _allPluginsByDynamicLibraryName, to prevent race condition
        allPluginsByName[name] = PlugPluginRefPtr();
    }

    // Go ahead and create a plugin.
    TF_DEBUG(PLUG_REGISTRATION).Msg("Registering %s plugin '%s' at '%s'.\n",
                                    pluginTypeName, name.c_str(),
                                    pluginCreationPath.c_str());

#if PXRPLUG_TEST_REGISTRATION_SLEEP_MILLISECS > 0
    // Add a sleep to make collisions more likely, for testing...
    std::this_thread::sleep_for(std::chrono::milliseconds(PXRPLUG_TEST_REGISTRATION_SLEEP_MILLISECS));
#endif // PXRPLUG_TEST_REGISTRATION_SLEEP_MILLISECS

    PlugPluginRefPtr plugin =
            TfCreateRefPtr(new PlugPlugin(pluginCreationPath, name,
                                          metadata.resourcePath,
                                          metadata.plugInfo, pluginType));

    pair<PlugPluginPtr, bool> result;
    {
        std::lock_guard<std::mutex> lock(_allPluginsMutex);
        pair<_PluginMap::iterator, bool> iresult =
                _allPlugins->insert(make_pair(path, plugin));
        result.first = iresult.first->second;
        result.second = iresult.second;

        // If successfully inserted, add to allPluginsByName too.
        if (iresult.second) {
            allPluginsByName[name] = plugin;
            // Wanted to have this just handled in _NewDynamicLibraryPlugin,
            // after calling _NewPlugin, but that would have required
            // re-accquiring the lock, which would potentially be slower
            if (allPluginsByCreationPath != nullptr) {
                (*allPluginsByCreationPath)[pluginCreationPath] = plugin;
            }
        }
        // If we failed, delete the nullptr we inserted earlier
        else {
            TF_DEBUG(PLUG_REGISTRATION).Msg(
                    "Failed registering %s plugin '%s' at '%s'...\n",
                    pluginTypeName, name.c_str(), pluginCreationPath.c_str());
            auto it = allPluginsByName.find(name);
            if (it != allPluginsByName.end() && !it->second) {
                allPluginsByName.erase(it);
            }
        }
    }

    return result;
}


pair<PlugPluginPtr, bool>
PlugPlugin::_NewDynamicLibraryPlugin(const Plug_RegistrationMetadata& metadata)
{
    return _NewPlugin(metadata, LibraryType, "dso", metadata.libraryPath,
                      *_allPluginsByDynamicLibraryName,
                      &(*_allPluginsByDynamicLibraryName));
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
pair<PlugPluginPtr, bool>
PlugPlugin::_NewPythonModulePlugin(const Plug_RegistrationMetadata& metadata)
{
    return _NewPlugin(metadata, PythonType, "python", metadata.pluginPath,
                      *_allPluginsByModuleName);
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

std::pair<PlugPluginPtr, bool> 
PlugPlugin::_NewResourcePlugin(const Plug_RegistrationMetadata& metadata)
{
    return _NewPlugin(metadata, ResourceType, "resource", metadata.pluginPath,
                      *_allPluginsByResourceName);
}

PlugPlugin::PlugPlugin(const std::string & path,
                       const std::string & name,
                       const std::string & resourcePath,
                       const JsObject & plugInfo,
                       _Type type) :

    _name(name),
    _path(path),
    _resourcePath(resourcePath),
    _dict(plugInfo),
    _handle(0),
    _isLoaded(type == ResourceType),
    _type(type)
{
    // Do nothing
}

// CODE_COVERAGE_OFF -- we don't destroy plugins once registered
PlugPlugin::~PlugPlugin()
{
}
// CODE_COVERAGE_ON

JsObject
PlugPlugin::GetMetadata()
{
    return _dict;
}

JsObject
PlugPlugin::GetDependencies()
{
    JsObject::iterator depend = _dict.find("PluginDependencies");
    if (depend == _dict.end() || 
        !depend->second.IsObject())
        return JsObject();

    return depend->second.GetJsObject();
}

bool
PlugPlugin::_Load()
{
    TRACE_FUNCTION();
    TF_DESCRIBE_SCOPE("Loading plugin '%s'", TfGetBaseName(_name).c_str());
    TF_DEBUG(PLUG_LOAD).Msg("Loading plugin '%s'.\n", _name.c_str());

    bool isLoaded = true;

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    if (IsPythonModule()) {
        string cmd = TfStringPrintf("import %s\n", _name.c_str());
        if (TfPyRunSimpleString(cmd) != 0) {
            TF_CODING_ERROR("Load of %s for %s failed",
                            _name.c_str(), _name.c_str());
            isLoaded = false;
        }
#else
    if (false) {
#endif // PXR_PYTHON_SUPPORT_ENABLED
    } else if (!IsResource()) {
        // XXX -- This is a hack to handle a static/non-monolithic library
        // build.  In this case some "plugins" will be static libraries
        // directly linked into the executable and can't be dynamically
        // loaded.  Just skip these since they're already loaded.
        if (!TfStringEndsWith(_path, ARCH_STATIC_LIBRARY_SUFFIX)) {
            string dsoError;
            _handle = TfDlopen(_path.c_str(), ARCH_LIBRARY_NOW, &dsoError);
            if (!_handle ) {
                TF_CODING_ERROR("Load of '%s' for '%s' failed: %s",
                                _path.c_str(), _name.c_str(), dsoError.c_str());
                isLoaded = false;
            }
        }
    }
    // Set _isLoaded at the end to make sure that we've fully loaded since
    // other threads may ask whether or not we're loaded (e.g., in
    // _LoadWithDependents) and we don't want to tell them that we are 
    // before we actually are loaded.
    return (_isLoaded = isLoaded);
}

struct PlugPlugin::_SeenPlugins {
    TfHashSet< std::string, TfHash > plugins;
};

bool
PlugPlugin::_LoadWithDependents(_SeenPlugins *seenPlugins)
{
    if (!_isLoaded) {
        // Take note of each plugin we've visited and bail if there is a cycle.
        if (seenPlugins->plugins.count(_name)) {
            TF_CODING_ERROR("Load failed because of cyclic dependency for '%s'",
                            _name.c_str());
            return false;
        }
        seenPlugins->plugins.insert(_name);

        // Load any dependencies.
        JsObject dependencies = GetDependencies();
        TF_FOR_ALL(i, dependencies) {
            string baseTypeName = i->first;
            TfType baseType = TfType::FindByName(baseTypeName);

            // Check that each base class type is defined.
            if (baseType.IsUnknown()) {
                TF_CODING_ERROR("Load failed: unknown base class '%s'",
                                baseTypeName.c_str());
                return false;
            }

            // Get dependencies, as typenames
            typedef vector<string> TypeNames;
            if (!i->second.IsArrayOf<string>()) {
                TF_CODING_ERROR("Load failed: dependency list has wrong type");
                return false;
            }
            const TypeNames & dependents = i->second.GetArrayOf<string>();

            // Load the dependencies for each base class.
            TF_FOR_ALL(j, dependents) {
                const string & dependName = *j;
                TfType dependType = TfType::FindByName(dependName);

                if (dependType.IsUnknown()) {
                    TF_CODING_ERROR("Load failed: unknown dependent class '%s'",
                                    dependName.c_str());
                    return false;
                }

                PlugPluginPtr dependPlugin = _GetPluginForType(dependType);
                if (!dependPlugin) {
                    TF_CODING_ERROR("Load failed: unknown dependent "
                                    "plugin '%s'", dependName.c_str());
                    return false;
                }
                if (!dependPlugin->_LoadWithDependents(seenPlugins)) {
                    TF_CODING_ERROR("Load failed: unable to load dependent "
                                    "plugin '%s'", dependName.c_str());
                    return false;
                }
            }
        }
        // Finally, load ourself.
        return _Load();
    }
    return true;
}

bool
PlugPlugin::Load()
{
    static std::recursive_mutex loadMutex;

    bool result = false;
    bool loadedInSecondaryThread = false;
    {
        // Drop the GIL if we have it, otherwise we can deadlock if another
        // thread has the plugin loadMutex and is waiting on the GIL (for
        // example if we're concurrently loading a python plugin in another
        // thread).
        TF_PY_ALLOW_THREADS_IN_SCOPE();

        std::lock_guard<std::recursive_mutex> lock(loadMutex);
        loadedInSecondaryThread = !_isLoaded && !ArchIsMainThread();
        _SeenPlugins seenPlugins;
        result = _LoadWithDependents(&seenPlugins);
    }

    if (loadedInSecondaryThread) {
        TF_DEBUG(PLUG_LOAD_IN_SECONDARY_THREAD).Msg(
            "Loaded plugin '%s' in a secondary thread.\n", _name.c_str());
    }

    return result;
}

bool
PlugPlugin::IsLoaded() const
{
    return _isLoaded;
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
bool
PlugPlugin::IsPythonModule() const
{
    return _type == PythonType;
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

bool
PlugPlugin::IsResource() const
{
    return _type == ResourceType;
}

std::string
PlugPlugin::MakeResourcePath(const std::string& path) const
{
    std::string result = path;
    if (result.empty()) {
        return result;
    }
    if (result[0] != '/') {
        result = TfStringCatPaths(GetResourcePath(), path);
    }
    return result;
}

std::string
PlugPlugin::FindPluginResource(const std::string& path, bool verify) const
{
    std::string result = MakeResourcePath(path);
    if (verify && !TfPathExists(result)) {
        result.clear();
    }
    return result;
}

// Defined in registry.cpp.
//void PlugPlugin::_RegisterAllPlugins();

PlugPluginPtr
PlugPlugin::_GetPluginWithName(const std::string& name)
{
    // Register all plugins first. We can't associate a plugin with a name
    // until it's registered.
    _RegisterAllPlugins();

    std::lock_guard<std::mutex> lock(_allPluginsMutex);

    auto idso = _allPluginsByDynamicLibraryName->find(name);
    if (idso != _allPluginsByDynamicLibraryName->end()) {
        return idso->second;
    }

    auto imod = _allPluginsByModuleName->find(name);
    if (imod != _allPluginsByModuleName->end()) {
        return imod->second;
    }

    auto ires = _allPluginsByResourceName->find(name);
    if (ires != _allPluginsByResourceName->end()) {
        return ires->second;
    }

    return nullptr;
}

PlugPluginPtrVector
PlugPlugin::_GetAllPlugins()
{
    _RegisterAllPlugins();

    std::lock_guard<std::mutex> lock(_allPluginsMutex);
    PlugPluginPtrVector plugins;
    plugins.reserve(_allPlugins->size());
    TF_FOR_ALL(it, *_allPlugins) {
        plugins.push_back(it->second);
    }
    return plugins;
}

PlugPluginPtr
PlugPlugin::_GetPluginForType( const TfType & type )
{
    // Ensure that plugins are registered, since even though the library that
    // defines \a type might be loaded, we might not have loaded its plugin
    // information, if it's loaded as a regular library dependency.
    _RegisterAllPlugins();

    std::lock_guard<std::mutex> lock(_classMapMutex);
    _ClassMap::iterator it = _classMap->find(type);
    if (it != _classMap->end())
        return it->second;
    return TfNullPtr;
}

JsObject
PlugPlugin::GetMetadataForType(const TfType &type)
{
    JsValue types;
    TfMapLookup(_dict,"Types",&types);
    if (!types.IsObject()) {
        return JsObject();
    }

    const JsObject &typesDict = types.GetJsObject();
    JsValue result;
    TfMapLookup(typesDict,type.GetTypeName(),&result);
    if (result.IsObject()) {
        return result.GetJsObject();
    }
    return JsObject();
}

bool 
PlugPlugin::DeclaresType(const TfType& type, bool includeSubclasses) const
{
    if (const JsValue* typesEntry = TfMapLookupPtr(_dict, "Types")) {
        if (typesEntry->IsObject()) {
            const JsObject& typesDict = typesEntry->GetJsObject();
            TF_FOR_ALL(it, typesDict) {
                const TfType typeFromPlugin = TfType::FindByName(it->first);
                const bool match = 
                    (includeSubclasses ? 
                     typeFromPlugin.IsA(type) : (typeFromPlugin == type));
                if (match) {
                    return true;
                }
            }
        }
    }

    return false;
}

void
PlugPlugin::_DefineType( TfType t )
{
    PlugPluginPtr plug;
    {
        std::lock_guard<std::mutex> lock(_classMapMutex);
        _ClassMap::const_iterator it = _classMap->find(t);
        if (it == _classMap->end()) {
            // CODE_COVERAGE_OFF - This cannot be hit by the public API for
            // this class.
            TF_CODING_ERROR("unknown plugin type %s",
                            t.GetTypeName().c_str());
            return;
            // CODE_COVERAGE_ON
        }
        plug = it->second;
    }
    plug->Load();
}

void
PlugPlugin::_DeclareAliases( TfType t, const JsObject & metadata )
{
    JsObject::const_iterator i = metadata.find("alias");

    if (i == metadata.end() || !i->second.IsObject())
        return;

    const JsObject& aliasDict = i->second.GetJsObject();

    TF_FOR_ALL(aliasIt, aliasDict) {

        if (!aliasIt->second.IsString()) {
            TF_WARN("Expected string for alias name, but found %s",
                    aliasIt->second.GetTypeName().c_str() );
            continue;
        }

        const string& aliasName = aliasIt->second.GetString();
        TfType aliasBase = TfType::Declare(aliasIt->first);

        t.AddAlias( aliasBase, aliasName );
    }
}

void
PlugPlugin::_DeclareTypes()
{
    JsValue typesValue;
    TfMapLookup(_dict,"Types",&typesValue);
    if (typesValue.IsObject()) {
        const JsObject& types = typesValue.GetJsObject();

        // Declare TfTypes for all the types found in the plugin.
        TF_FOR_ALL(i, types) {
            if (i->second.IsObject()) {
                _DeclareType(i->first, i->second.GetJsObject());
            }
        }
    }
}

void
PlugPlugin::_DeclareType(
    const std::string &typeName,
    const JsObject &typeDict)
{
    TfType::DefinitionCallback cb( &_DefineType );

    // Get the base types, declaring them if necessary.
    vector<TfType> basesVec;
    JsValue bases;
    TfMapLookup(typeDict,"bases",&bases);
    if (bases.IsArrayOf<string>()) {
        for (const auto& name : bases.GetArrayOf<string>()) {
            basesVec.push_back(TfType::Declare(name));
        }
    } else if (!bases.IsNull()) {
        TF_CODING_ERROR("Invalid bases for type %s specified by plugin %s. "
            "Expected list of strings.", typeName.c_str(), _name.c_str());
    }

    // Declare the type.
    TfType type = TfType::Declare(typeName);

    // We need to handle the case of a plugin already having
    // been loaded (ex: via an explicit 'import') -- in which case
    // the type will have already been declared with a full set of
    // bases.  Since it is an error to re-declare a TfType with
    // fewer bases, we check if the type has already been declared
    // with bases -- if it has, we just make sure that the base
    // mentioned in the plugin is among them.

    std::vector<TfType> existingBases = type.GetBaseTypes();
    if (existingBases.empty()) {
        // If there were no bases previously declared, simply declare with known
        // bases.
        TfType::Declare(typeName, basesVec, cb);
    } else {
        // Make sure that the bases mentioned in the plugin
        // metadata are among them.
        TF_FOR_ALL(i, basesVec) {
            TfType base = *i;
            std::string const &baseName = base.GetTypeName();
            if (std::find(existingBases.begin(), existingBases.end(),
                          base) == existingBases.end()) {
                // Our expected base was not found.
                std::string basesStr;
                TF_FOR_ALL(j, existingBases)
                    basesStr += j->GetTypeName() + " ";
                TF_CODING_ERROR(
                    "The metadata for plugin '%s' defined in %s declares "
                    "type '%s' with base type '%s', but the type has "
                    "already been declared with a different set of bases "
                    "that does not include that type.  The existing "
                    "bases are: (%s).  Please fix the plugin.",
                    _name.c_str(),
                    GetPath().c_str(),
                    typeName.c_str(),
                    baseName.c_str(),
                    basesStr.c_str() );
            }
        }
    }

    // Ensure that no other plugin declared that it provides this
    // type.  This is to guard against errors in plugin metadata
    // introducing subtle cycles.
    {
        std::lock_guard<std::mutex> lock(_classMapMutex);
        if (_classMap->count(type)) {
            PlugPluginPtr other((*_classMap)[type]);
            TF_CODING_ERROR("Plugin '%s' defined in %s has metadata "
                            "claiming that it provides type %s, but this " 
                            "was previously provided by plugin '%s' "
                            "defined in %s.",
                            GetName().c_str(),
                            GetPath().c_str(),
                            typeName.c_str(),
                            other->GetName().c_str(),
                            other->GetPath().c_str());
            return;
        }

        (*_classMap)[type] = TfCreateWeakPtr(this);
    }

    // Find type aliases.
    _DeclareAliases( type, typeDict);
}


TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<PlugPlugin>();
}

std::string
PlugFindPluginResource(
    const PlugPluginPtr& plugin,
    const std::string& path,
    bool verify)
{
    return plugin ? plugin->FindPluginResource(path, verify) : std::string();
}

PXR_NAMESPACE_CLOSE_SCOPE
