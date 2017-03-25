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
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pySingleton.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/bind.hpp>
#include <boost/range.hpp>
#include <boost/python.hpp>

#include <algorithm>
#include <atomic>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using std::make_pair;
using std::pair;
using std::string;
using std::vector;

using namespace boost::python;

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
typedef boost::function<PluginPredicateSig> PluginPredicateFn;

struct SharedState : boost::noncopyable {

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

template <class Range>
string PluginNames(Range const &range) {
    using std::distance;
    vector<string> names(distance(boost::begin(range), boost::end(range)));
    transform(boost::begin(range), boost::end(range), names.begin(),
              boost::bind(&PlugPlugin::GetName, _1));
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
                  !boost::bind(&PlugPlugin::IsLoaded, _1));

    // Report any already loaded plugins as skipped.
    if (verbose && alreadyLoaded != plugins.end()) {
        printf("Skipping already-loaded plugins: %s\n",
               PluginNames(make_pair(alreadyLoaded, plugins.end())).c_str());
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
               plugins.size(), PluginNames(plugins).c_str());
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

void wrapRegistry()
{

    typedef PlugRegistry This;

    class_<This, TfWeakPtr<This>, boost::noncopyable>
        ("Registry", no_init)
        .def(TfPySingleton())
        .def("RegisterPlugins", &_RegisterPlugins,
            return_value_policy<TfPySequenceToList>())
        .def("RegisterPlugins", &_RegisterPluginsList,
            return_value_policy<TfPySequenceToList>())
        .def("GetStringFromPluginMetaData", &_GetStringFromPluginMetaData)
        .def("GetPluginWithName", &This::GetPluginWithName)
        .def("GetPluginForType", &_GetPluginForType)
        .def("GetAllPlugins", &This::GetAllPlugins,
             return_value_policy<TfPySequenceToList>())

        .def("FindTypeByName", This::FindTypeByName,
             return_value_policy<return_by_value>())
        .staticmethod("FindTypeByName")

        .def("FindDerivedTypeByName", (TfType (*)(TfType, std::string const &))
             This::FindDerivedTypeByName)
        .staticmethod("FindDerivedTypeByName")

        .def("GetDirectlyDerivedTypes", This::GetDirectlyDerivedTypes,
             return_value_policy<TfPySequenceToTuple>())
        .staticmethod("GetDirectlyDerivedTypes")

        .def("GetAllDerivedTypes", _GetAllDerivedTypes,
             return_value_policy<TfPySequenceToTuple>())
        .staticmethod("GetAllDerivedTypes")

        ;

    TfPyFunctionFromPython<PluginPredicateSig>();
    def("_LoadPluginsConcurrently",
        _LoadPluginsConcurrently,
        (arg("predicate"), arg("numThreads")=0, arg("verbose")=false));
}
