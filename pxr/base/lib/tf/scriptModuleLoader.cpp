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
#include "pxr/base/tf/scriptModuleLoader.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/tf/staticData.h"

#include "pxr/base/arch/fileSystem.h"

#include <boost/python/borrowed.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/handle.hpp>

#include <deque>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(TfScriptModuleLoader);

using std::deque;
using std::string;
using std::vector;

using boost::python::borrowed;
using boost::python::dict;
using boost::python::handle;
using boost::python::object;


TfScriptModuleLoader::TfScriptModuleLoader()
{
}

// CODE_COVERAGE_OFF_GCOV_BUG
TfScriptModuleLoader::~TfScriptModuleLoader()
// CODE_COVERAGE_ON_GCOV_BUG
{
}

void
TfScriptModuleLoader::
RegisterLibrary(TfToken const &name, TfToken const &moduleName,
                vector<TfToken> const &predecessors)
{

    if (TfDebug::IsEnabled(TF_SCRIPT_MODULE_LOADER)) {
        TF_DEBUG(TF_SCRIPT_MODULE_LOADER)
            .Msg("Registering library %s with predecessors: ", name.GetText());
        TF_FOR_ALL(pred, predecessors) {
            TF_DEBUG(TF_SCRIPT_MODULE_LOADER).Msg("%s, ", pred->GetText());
        }
        TF_DEBUG(TF_SCRIPT_MODULE_LOADER).Msg("\n");
    }

    // Add library with predecessors.
    vector<TfToken> &predsInTable = _libInfo[name].predecessors;
    predsInTable = predecessors;
    std::sort(predsInTable.begin(), predsInTable.end());
    _libsToModules[name] = moduleName;

    // Add this library as a successor to all predecessors.
    TF_FOR_ALL(pred, predecessors)
        _AddSuccessor(*pred, name);
}

vector<string>
TfScriptModuleLoader::GetModuleNames() const
{
    vector<TfToken> order;
    vector<string> ret;
    _TopologicalSort(&order);
    ret.reserve(order.size());
    TF_FOR_ALL(lib, order) {
        _TokenToTokenMap::const_iterator i = _libsToModules.find(*lib);
        if (i != _libsToModules.end())
            ret.push_back(i->second.GetString());
    }
    return ret;
}

dict
TfScriptModuleLoader::GetModulesDict() const
{
    if (!TfPyIsInitialized()) {
        TF_CODING_ERROR("Python is not initialized!");
        return dict();
    }

    // Kick the registry function so any loaded libraries with script
    // bindings register themselves with the script module loading system.
    TfRegistryManager::GetInstance().SubscribeTo<TfScriptModuleLoader>();

    TfPyLock lock;

    // Get the sys.modules dict from python, so we can see if modules are
    // already loaded.
    dict modulesDict(handle<>(borrowed(PyImport_GetModuleDict())));

    vector<TfToken> order;
    dict ret;
    _TopologicalSort(&order);
    TF_FOR_ALL(lib, order) {
        _TokenToTokenMap::const_iterator i = _libsToModules.find(*lib);
        if (i != _libsToModules.end() &&
            modulesDict.has_key(i->second.GetText())) {
            handle<> modHandle(PyImport_ImportModule(const_cast<char *>
                                                     (i->second.GetText())));

            // Use the upper-cased form of the library name as
            // the Python module name.
            //
            // XXX This does not seem quite right.  For one thing,
            //     we should be using the Python module names here,
            //     not the C++ library names. However, after the
            //     shared_code unification, some Python modules
            //     are now submodules ("Tf" becomes "pixar.Tf").
            //     To preserve compatibility with eval'ing reprs
            //     of python types via this function, we have two
            //     options:
            //
            //     1) strip the new "pixar." prefix off module names
            //     2) upper-case the library name (tf -> Tf)
            //
            //     Also, neither of these two options correctly
            //     handles Ext submodules.  This will provide a
            //     binding as "ExtShippedFoo", when really we
            //     should have a binding to the "ExtShipped" module
            //     with "Foo" imported underneath.
            //
            // For now, we just upper-case the library name.
            //
            string moduleName = TfStringCapitalize(lib->GetString());

            ret[moduleName] = object(modHandle);
        }
    }
    return ret;
}

void
TfScriptModuleLoader::WriteDotFile(string const &file) const
{
    FILE *out = ArchOpenFile(file.c_str(), "w");
    if (!out) {
        TF_RUNTIME_ERROR("Could not open '%s' for writing.\n", file.c_str());
        return;
    }

    fprintf(out, "digraph Modules {\n");
    
    TF_FOR_ALL(info, _libInfo) {
        TF_FOR_ALL(successor, info->second.successors) {
            fprintf(out, "\t%s -> %s;\n", info->first.GetText(),
                    successor->GetText());
        }
    }

    fprintf(out, "}\n");
    fclose(out);
}

void
TfScriptModuleLoader::LoadModules()
{
    _LoadModulesFor(TfToken());
}

void
TfScriptModuleLoader::LoadModulesForLibrary(TfToken const &name) {
    _LoadModulesFor(name);
}

bool
TfScriptModuleLoader::_HasTransitiveSuccessor(TfToken const &predecessor,
                                              TfToken const &successor) const
{
    // This function does a simple DFS of the dependency dag, to determine if \a
    // predecessor has \a successor somewhere in the transitive closure.

    vector<TfToken> predStack(1, predecessor);
    TfToken::HashSet seenPreds;

    while (!predStack.empty()) {
        TfToken pred = predStack.back();
        predStack.pop_back();

        // A name counts as its own successor.
        if (pred == successor)
            return true;

        // Look up successors, and push ones not yet visted as possible
        // predecessors.
        _TokenToInfoMap::const_iterator i = _libInfo.find(pred);
        if (i != _libInfo.end()) {
            // Walk all the successors.
            TF_FOR_ALL(j, i->second.successors) {
                // Push those that haven't yet been visited on the stack.
                if (seenPreds.insert(pred).second)
                    predStack.push_back(pred);
            }
        }
    }
    return false;
}

static bool _DidPyErrorOccur()
{
    TfPyLock pyLock;
    return PyErr_Occurred();
}

void
TfScriptModuleLoader::_LoadUpTo(TfToken const &name)
{
    static size_t indent = 0;
    string indentString;
    char const *indentTxt = 0;

    if (TfDebug::IsEnabled(TF_SCRIPT_MODULE_LOADER)) {
        indentString = std::string(indent * 2, ' ');
        indentTxt = indentString.c_str();
    }

    // Don't do anything if the name isn't empty and it's not a name we know
    // about.
    if (!name.IsEmpty() && !_libInfo.count(name)) {
        TF_DEBUG(TF_SCRIPT_MODULE_LOADER)
            .Msg("%s*** Not loading modules for unknown lib '%s'\n",
                 indentTxt, name.GetText());
        return;
    }

    // Otherwise load modules in topological dependency order until we
    // encounter the requested module.
    vector<TfToken> order;
    if (name.IsEmpty()) {
        _TopologicalSort(&order);
    } else {
        _GetOrderedDependencies(vector<TfToken>(1, name), &order);
    }

    TF_DEBUG(TF_SCRIPT_MODULE_LOADER)
        .Msg("%s_LoadUpTo('%s') {\n", indentTxt, name.GetText());
    TF_FOR_ALL(lib, order) {
        // If we encounter the library we're loading on behalf of, quit.
        // Mostly this is the last library in the order, but it may not be.
        if (*lib == name)
            break;

        if (_libsToModules.count(*lib) && !_loadedSet.count(*lib)) {
            TF_DEBUG(TF_SCRIPT_MODULE_LOADER)
                .Msg("%s  Load('%s');\n", indentTxt, lib->GetText());
            _loadedSet.insert(*lib);
            ++indent;
            Tf_PyLoadScriptModule(_libsToModules[*lib]);
            --indent;
        }

        if (_DidPyErrorOccur()) {
            TF_DEBUG(TF_SCRIPT_MODULE_LOADER).Msg("%s  *error*\n", indentTxt);
            break;
        }
    }

    TF_DEBUG(TF_SCRIPT_MODULE_LOADER).Msg("%s}\n", indentTxt);
}

void
TfScriptModuleLoader::_LoadModulesFor(TfToken const &inName)
{
    // Don't load anything if python isn't initialized.
    if (!TfPyIsInitialized())
        return;
    if (_DidPyErrorOccur())
        return;

    //////////////////////////////////////////////////////////////////////////
    // This function handles requests to load modules in topological dependency
    // order up to \a inName.  Doing so may cause reentrant calls to this
    // function.  Handling reentrancy works as follows.  There are two cases to
    // consider.
    //
    // -----------------------------------------------------------------------
    // Case 1: The reentrant request is for a module that depends (transitively)
    // on the module we're currently loading (or is a request to load all
    // modules).
    //
    //     In this case, we defer until after the current request is completed.
    //     We do this because the dependent library may attempt to load the
    //     library we're currently loading, which would fail.
    //
    // ------------------------------------------------------------------------
    // Case 2: The reentrant request is for a module that does not depend on the
    // module we're currently loading.
    //
    //     In this case, we immediately load the module and its dependencies.
    //     This is a dynamically discovered dependency of the module we're
    //     currently loading.
    //
    // We achieve this by keeping a queue of requests.  We always push the
    // request on the back of the queue.  If we're the outermost call (i.e. not
    // reentrant) we loop, processing the queue from the front to the back.  For
    // a reentrant call, in case 1 (above), we return immediately, deferring the
    // work.  In case 2 (above) we process the element on the back of the queue
    // immediately and pop it.
    //

    // Add this request to the remaining work to do.
    _remainingLoadWork.push_back(inName);

    // Kick the registry function so any loaded libraries with script bindings
    // register themselves with the script module loading system.
    TfRegistryManager::GetInstance().SubscribeTo<TfScriptModuleLoader>();

    // If this is the outermost caller initiating loading, start processing the
    // queue.
    if (_remainingLoadWork.size() == 1) {
        while (!_remainingLoadWork.empty() && !_DidPyErrorOccur()) {
            // Note: we copy the front of the deque here, since _LoadUpTo may
            // add items into the deque.  _LoadUpTo currently doesn't access the
            // reference-to-token it's passed after it might have modified the
            // deque, but this is good defensive coding in case it's changed in
            // the future to do so.
            TfToken name = _remainingLoadWork.front();
            _LoadUpTo(name);
            // We must pop the queue *after* processing, since reentrant calls
            // need to see \a name on the front of the queue.  See the access of
            // _remainingLoadWork.front() below.
            _remainingLoadWork.pop_front();
        }

        // Otherwise, this is a reentrant load request.  If the reentrant
        // request is not to load everything (empty token) and it's also not a
        // (transitive) dependency of the library we're currently working on,
        // then load it immediately.
    } else if (!_remainingLoadWork.back().IsEmpty() &&
               !_HasTransitiveSuccessor(_remainingLoadWork.front(),
                                        _remainingLoadWork.back())) {
        TfToken name = _remainingLoadWork.back();
        _remainingLoadWork.pop_back();
        _LoadUpTo(name);
    }
}

void
TfScriptModuleLoader::
_AddSuccessor(TfToken const &lib, TfToken const &successor)
{
    if (ARCH_UNLIKELY(lib == successor)) {
        // CODE_COVERAGE_OFF Can only happen if there's a bug.
        TF_FATAL_ERROR("Library '%s' cannot depend on itself.", lib.GetText());
        return;
        // CODE_COVERAGE_ON
    }

    // Add dependent as a dependent of lib.
    vector<TfToken> *successors = &(_libInfo[lib].successors);
    successors->insert(std::lower_bound(successors->begin(),
                                        successors->end(), successor),
                       successor);
}

void
TfScriptModuleLoader
::_GetOrderedDependenciesRecursive(TfToken const &lib,
                                   TfToken::HashSet *seenLibs,
                                   vector<TfToken> *result) const
{
    // If we've not yet visited this library, then visit its predecessors, and
    // finally add it to the order.
    if (seenLibs->insert(lib).second) {
        TF_FOR_ALL(i, _libInfo.find(lib)->second.predecessors)
            _GetOrderedDependenciesRecursive(*i, seenLibs, result);

        result->push_back(lib);
    }
}

void
TfScriptModuleLoader::
_GetOrderedDependencies(vector<TfToken> const &input,
                        vector<TfToken> *result) const
{
    TfToken::HashSet seenLibs;
    TF_FOR_ALL(i, input) {
        // If we haven't seen the current input yet, add its predecessors (and
        // their dependencies) to the result.
        if (seenLibs.insert(*i).second) {
            TF_FOR_ALL(j, _libInfo.find(*i)->second.predecessors)
                _GetOrderedDependenciesRecursive(*j, &seenLibs, result);
        }
    }
}


void
TfScriptModuleLoader::
_TopologicalSort(vector<TfToken> *result) const
{
    // Find all libs with no successors, then produce all ordered dependencies
    // from them.
    vector<TfToken> leaves;
    TF_FOR_ALL(i, _libInfo) {
        if (i->second.successors.empty())
            leaves.push_back(i->first);
    }

    // Sort to ensure determinism.
    std::sort(leaves.begin(), leaves.end());

    // Find all the leaves' dependencies.
    _GetOrderedDependencies(leaves, result);

    // Add the leaves themselves, at the end.
    result->insert(result->end(), leaves.begin(), leaves.end());
}

PXR_NAMESPACE_CLOSE_SCOPE
