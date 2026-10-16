//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pySingleton.h"
#include "pxr/base/tf/stringUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

typedef TfWeakPtr<PlugRegistry> PlugRegistryPtr;

static PlugPluginPtrVector
_RegisterPlugins(PlugRegistryPtr self, string path)
{
    return self->RegisterPlugins(path);
}

static PlugPluginPtrVector
_RegisterPluginsList(PlugRegistryPtr self, vector<string> paths)
{
    return self->RegisterPlugins(paths);
}

// Disambiguate vs. the template
static PlugPluginPtr
_GetPluginForType(PlugRegistry &reg, const TfType &t)
{
    return reg.GetPluginForType(t);
}

static std::string
_GetStringFromPluginMetaData(PlugRegistry &reg, const TfType &type,
    const std::string &key) 
{
    return reg.GetStringFromPluginMetaData(type,key);
}

static std::vector<TfType>
_GetAllDerivedTypes(TfType const &type)
{
    std::set<TfType> types;
    PlugRegistry::GetAllDerivedTypes(type, &types);
    return vector<TfType>(types.begin(), types.end());
}

// For testing -- load plugins in parallel.

typedef bool PluginPredicateSig(PlugPluginPtr);
typedef std::function<PluginPredicateSig> PluginPredicateFn;

struct SharedState {

    SharedState() = default;
    SharedState(SharedState const&) = delete;
    SharedState& operator=(SharedState const&) = delete;

    void ThreadTask() {
        while (true) {
            // Try to take the next plugin to load.
            size_t cur = nextAvailable;
            while (cur != plugins.size() &&
                   !nextAvailable.compare_exchange_strong(cur, cur+1)) {
                cur = nextAvailable;
            }

            // No more plugins to load?
            if (cur == plugins.size())
                return;

            // Otherwise we load plugins[cur].
            printf("Loading '%s'\n", plugins[cur]->GetName().c_str());
            plugins[cur]->Load();
        }
    }       
  
    PlugPluginPtrVector plugins;
    std::atomic<size_t> nextAvailable;
};

template <class It>
string PluginNames(const It begin, const It end) {
    using std::distance;
    vector<string> names(distance(begin, end));
    transform(begin, end, names.begin(),
              [](PlugPluginPtr const &plug) { return plug->GetName(); });
    return TfStringJoin(names.begin(), names.end(), ", ");
}

void _LoadPluginsConcurrently(PluginPredicateFn pred,
                              size_t numThreads,
                              bool verbose)
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Take all unloaded plugins for which pred(plugin) is true.
    PlugPluginPtrVector plugins = PlugRegistry::GetInstance().GetAllPlugins();

    // Remove all plugins which fail the predicate.
    plugins.erase(partition(plugins.begin(), plugins.end(), pred),
                  plugins.end());

    // Shuffle all already loaded plugins to the end.
    PlugPluginPtrVector::iterator alreadyLoaded =
        partition(plugins.begin(), plugins.end(),
                  [](PlugPluginPtr const &plug) { return !plug->IsLoaded(); });

    // Report any already loaded plugins as skipped.
    if (verbose && alreadyLoaded != plugins.end()) {
        printf("Skipping already-loaded plugins: %s\n",
               PluginNames(alreadyLoaded, plugins.end()).c_str());
    }

    // Trim the already loaded plugins from the vector.
    plugins.erase(alreadyLoaded, plugins.end());

    if (plugins.empty()) {
        if (verbose)
            printf("No plugins to load.\n");
        return;
    }

    // Determine number of threads to use.  If caller specified a value, use it.
    // Otherwise use the min of the machine's physical threads and the number of
    // plugins we're loading.
    unsigned int hwThreads = std::thread::hardware_concurrency();
    numThreads = numThreads ? numThreads :
        std::min(hwThreads, (unsigned int)plugins.size());

    // Report what we're doing.
    if (verbose) {
        printf("Loading %zu plugins concurrently: %s\n",
               plugins.size(),
               PluginNames(std::cbegin(plugins), std::cend(plugins)).c_str());
    }

    // Establish shared state.
    SharedState state;
    state.plugins.swap(plugins);
    state.nextAvailable = 0;

    // Load in multiple threads.
    std::vector<std::thread> threads;
    for (size_t i = 0; i != numThreads; ++i) {
        threads.emplace_back([&state](){ state.ThreadTask(); });
    }

    // Wait for threads.
    for (auto& thread: threads) {
        thread.join();
    }

    if (verbose) {
        printf("Used %zu threads.\n", numThreads);
    }
}

} // anonymous namespace 

void wrapRegistry(nanobind::module_& m)
{
    namespace nb = nanobind;
    typedef PlugRegistry This;

    nb::class_<This> cls(m, "Registry",
                         nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbSingleton(cls);
    cls.def("RegisterPlugins", &_RegisterPlugins)
        .def("RegisterPlugins", &_RegisterPluginsList)
        .def("GetStringFromPluginMetaData", &_GetStringFromPluginMetaData)
        .def("GetPluginWithName", &This::GetPluginWithName)
        .def("GetPluginForType", &_GetPluginForType)
        .def("GetAllPlugins", &This::GetAllPlugins)

        .def_static("FindTypeByName", &This::FindTypeByName)

        .def_static("FindDerivedTypeByName",
                    (TfType (*)(TfType, std::string const &))
                    &This::FindDerivedTypeByName)

        .def_static("GetDirectlyDerivedTypes", [](TfType baseType) {
            auto v = This::GetDirectlyDerivedTypes(baseType);
            nb::tuple res = nb::steal<nb::tuple>(PyTuple_New(v.size()));
            for (size_t i = 0; i < v.size(); ++i) {
                PyTuple_SET_ITEM(res.ptr(), i, nb::cast(v[i]).release().ptr());
            }
            return res;
        })

        .def_static("GetAllDerivedTypes", [](TfType const &type) {
            auto v = _GetAllDerivedTypes(type);
            nb::tuple res = nb::steal<nb::tuple>(PyTuple_New(v.size()));
            for (size_t i = 0; i < v.size(); ++i) {
                PyTuple_SET_ITEM(res.ptr(), i, nb::cast(v[i]).release().ptr());
            }
            return res;
        })
        ;

    m.def("_LoadPluginsConcurrently",
          &_LoadPluginsConcurrently,
          nb::arg("predicate"),
          nb::arg("numThreads") = 0,
          nb::arg("verbose") = false);
}
