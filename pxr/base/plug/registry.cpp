//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/plug/registry.h"

#include "pxr/base/plug/debugCodes.h"
#include "pxr/base/plug/notice.h"
#include "pxr/base/plug/info.h"
#include "pxr/base/plug/plugin.h"

#include "pxr/base/arch/attributes.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/work/withScopedParallelism.h"

#include <tbb/concurrent_vector.h>
#include <tbb/spin_mutex.h>

#include <functional>

using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON( PlugRegistry );

PlugRegistry &
PlugRegistry::GetInstance()
{
    return TfSingleton< This >::GetInstance();
}

PlugRegistry::PlugRegistry()
{
    TfSingleton< This >::SetInstanceConstructed(*this);
}

bool
PlugRegistry::_InsertRegisteredPluginPath(const std::string &path)
{
    static tbb::spin_mutex mutex;
    tbb::spin_mutex::scoped_lock lock(mutex);
    return _registeredPluginPaths.insert(path).second;
}

template <class ConcurrentVector>
void
PlugRegistry::_RegisterPlugin(
    const Plug_RegistrationMetadata& metadata,
    ConcurrentVector* newPlugins)
{
    std::pair<PlugPluginPtr, bool> newPlugin(TfNullPtr, false);
    switch (metadata.type) {
    default:
    case Plug_RegistrationMetadata::UnknownType:
        TF_CODING_ERROR("Tried to register a plugin of unknown type "
                        "(maybe from %s)", metadata.pluginPath.c_str());
        break;

    case Plug_RegistrationMetadata::LibraryType:
        newPlugin = PlugPlugin::_NewDynamicLibraryPlugin(metadata);
        break;
#ifdef PXR_PYTHON_SUPPORT_ENABLED
    case Plug_RegistrationMetadata::PythonType:
        newPlugin = PlugPlugin::_NewPythonModulePlugin(metadata);
        break;
#endif // PXR_PYTHON_SUPPORT_ENABLED
    case Plug_RegistrationMetadata::ResourceType:
        newPlugin = PlugPlugin::_NewResourcePlugin(metadata);
        break;
    }

    if (newPlugin.second) {
        newPlugins->push_back(newPlugin.first);
    }
}

PlugPluginPtrVector
PlugRegistry::RegisterPlugins(const std::string & pathToPlugInfo)
{
    return RegisterPlugins(vector<string>(1, pathToPlugInfo));
}

PlugPluginPtrVector
PlugRegistry::RegisterPlugins(const std::vector<std::string> & pathsToPlugInfo)
{
    const bool pathsAreOrdered = true;
    PlugPluginPtrVector result =
        _RegisterPlugins(pathsToPlugInfo, pathsAreOrdered);
    if (!result.empty()) {
        PlugNotice::DidRegisterPlugins(result).Send(TfCreateWeakPtr(this));
    }
    return result;
}

PlugPluginPtrVector
PlugRegistry::_RegisterPlugins(const std::vector<std::string>& pathsToPlugInfo,
                               bool pathsAreOrdered)
{
    TF_DESCRIBE_SCOPE("Registering plugins");
    TfAutoMallocTag2 tag2("Plug", "PlugRegistry::RegisterPlugins");

    typedef tbb::concurrent_vector<PlugPluginPtr> NewPluginsVec;
    NewPluginsVec newPlugins;
    {
        Plug_TaskArena taskArena;
        // XXX -- Is this mutex really needed?
        std::lock_guard<std::mutex> lock(_mutex);
        WorkWithScopedParallelism([&]() {
                Plug_ReadPlugInfo(
                    pathsToPlugInfo,
                    pathsAreOrdered,
                    std::bind(
                        &PlugRegistry::_InsertRegisteredPluginPath,
                        this, std::placeholders::_1),
                    std::bind(
                        &PlugRegistry::_RegisterPlugin<NewPluginsVec>,
                        this, std::placeholders::_1, &newPlugins),
                    &taskArena);
        }, /*dropPythonGIL=*/false);
        // We explicitly do not drop the GIL here because of sad stories like
        // the following. A shared library loads and during its initialization,
        // it wants to look up information from plugins, and thus invokes this
        // code to do first-time plugin registration. The dynamic loader holds
        // its own lock while it loads the shared library. If this code holds
        // the GIL (say the library is being loaded due to a python 'import')
        // and was to drop it during the parallelism, then other Python-based
        // threads can take the GIL and wind up calling, dlsym() for example.
        // This will wait on the dynamic loader's lock, but this thread will
        // never release it since it will wait to reacquire the GIL. This causes
        // a deadlock between the dynamic loader's lock and the Python GIL.
        // Retaining the GIL here prevents this scenario.
    }

    if (!newPlugins.empty()) {
        PlugPluginPtrVector v(newPlugins.begin(), newPlugins.end());
        for (const auto& plug : v) {
            plug->_DeclareTypes();
        }
        return v;
    }
    return PlugPluginPtrVector();
}

PlugPluginPtr
PlugRegistry::GetPluginForType(TfType t) const
{
    if (t.IsUnknown()) {
        TF_CODING_ERROR("Unknown base type");
        return TfNullPtr;
    }
    return PlugPlugin::_GetPluginForType(t);
}

PlugPluginPtrVector
PlugRegistry::GetAllPlugins() const
{
    return PlugPlugin::_GetAllPlugins();
}

PlugPluginPtr
PlugRegistry::GetPluginWithName(const string& name) const
{
    return PlugPlugin::_GetPluginWithName(name);
}

JsValue
PlugRegistry::GetDataFromPluginMetaData(TfType type, const string &key) const
{
    JsValue result;

    string typeName = type.GetTypeName();
    PlugPluginPtr plugin = GetPluginForType(type);
    if (plugin) {
        JsObject dict = plugin->GetMetadataForType(type);
        TfMapLookup(dict,key,&result);
    }
    return result;
}

string
PlugRegistry::GetStringFromPluginMetaData(TfType type, const string &key) const
{
    JsValue v = GetDataFromPluginMetaData(type, key);
    return v.IsString() ? v.GetString() : string();
}

TfType 
PlugRegistry::FindTypeByName(std::string const &typeName)
{
    PlugPlugin::_RegisterAllPlugins();
    return TfType::FindByName(typeName);
}

TfType 
PlugRegistry::FindDerivedTypeByName(TfType base, std::string const &typeName)
{
    PlugPlugin::_RegisterAllPlugins();
    return base.FindDerivedByName(typeName);
}

std::vector<TfType>
PlugRegistry::GetDirectlyDerivedTypes(TfType base)
{
    PlugPlugin::_RegisterAllPlugins();
    return base.GetDirectlyDerivedTypes();
}

void
PlugRegistry::GetAllDerivedTypes(TfType base, std::set<TfType> *result)
{
    PlugPlugin::_RegisterAllPlugins();
    base.GetAllDerivedTypes(result);
}

namespace {

struct PathsInfo {
    std::vector<std::string> paths;
    std::vector<std::string> debugMessages;
    bool pathsAreOrdered = true;
};

// Return a static vector<string> that holds the bootstrap plugin paths.
static
PathsInfo&
Plug_GetPathsInfo()
{
    // This is a static local variable since the function is called from
    // ARCH_CONSTRUCTOR methods, potentially before module-level static
    // initialization.
    static PathsInfo pathsInfo;
    return pathsInfo;
}

}

void
Plug_SetPaths(const std::vector<std::string>& paths,
              const std::vector<std::string>& debugMessages,
              bool pathsAreOrdered)
{
    auto& pathsInfo = Plug_GetPathsInfo();
    pathsInfo.paths = paths;
    pathsInfo.debugMessages = debugMessages;
    pathsInfo.pathsAreOrdered = pathsAreOrdered;
}

// This is here so plugin.cpp doesn't have to include info.h or registry.h.
void
PlugPlugin::_RegisterAllPlugins()
{
    PlugPluginPtrVector result;

    static std::once_flag once;
    std::call_once(once, [&result](){
        PlugRegistry &registry = PlugRegistry::GetInstance();

        if (!TfGetenvBool("PXR_DISABLE_STANDARD_PLUG_SEARCH_PATH", false)) {
            // Emit any debug messages first, then call _RegisterPlugins.
            for (std::string const &msg:
                     Plug_GetPathsInfo().debugMessages) {
                TF_DEBUG(PLUG_INFO_SEARCH).Msg("%s", msg.c_str());
            }
            // Register plugins in the tree. This declares TfTypes.
            result = registry._RegisterPlugins(
                Plug_GetPathsInfo().paths,
                Plug_GetPathsInfo().pathsAreOrdered);
        }
    });


    // Send a notice outside of the call_once.  We don't want to be holding
    // a lock (even an implicit one) when sending a notice.
    if (!result.empty()) {
        PlugNotice::DidRegisterPlugins(result).Send(
            TfCreateWeakPtr(&PlugRegistry::GetInstance()));
    }
}

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<PlugRegistry>();
}

PXR_NAMESPACE_CLOSE_SCOPE
