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
#ifndef PXR_BASE_TF_SCRIPT_MODULE_LOADER_H
#define PXR_BASE_TF_SCRIPT_MODULE_LOADER_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakBase.h"

// XXX: This include is a hack to avoid build errors due to
// incompatible macro definitions in pyport.h on macOS.
#include <locale>
#include <boost/python/dict.hpp>

#include <deque>
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfScriptModuleLoader
///
/// Provides low-level facilities for shared libraries with script bindings to
/// register themselves with their dependences, and provides a mechanism
/// whereby those script modules will be loaded when necessary. Currently,
/// this is when one of our script modules is loaded, when TfPyInitialize is
/// called, and when Plug opens shared libraries.
///
/// Generally, user code will not make use of this.
///
class TfScriptModuleLoader : public TfWeakBase {

  public:

    typedef TfScriptModuleLoader This;

    /// Return the singleton instance.
    TF_API static This &GetInstance() {
        return TfSingleton<This>::GetInstance();
    } 

    /// Register a library named \a name and with script module \a moduleName
    /// and libraries which must be loaded first \a predecessors. The script
    /// module will be loaded when necessary. This should generally not be
    /// called by user code.
    TF_API
    void RegisterLibrary(TfToken const &name, TfToken const &moduleName,
                         std::vector<TfToken> const &predecessors);

    /// Load all the script modules for any libraries registered using \a
    /// RegisterLibrary if necessary. Loads the modules in dependency order as
    /// defined in \a RegisterLibrary.
    TF_API
    void LoadModules();

    /// Load all the script modules for any libraries registered using \a
    /// RegisterLibrary that depend on library \a name.
    TF_API
    void LoadModulesForLibrary(TfToken const &name);

    /// Return a list of all currently known modules in a valid dependency
    /// order.
    TF_API
    std::vector<std::string> GetModuleNames() const;

    /// Return a python dict containing all currently known modules under
    /// their canonical names.
    TF_API
    boost::python::dict GetModulesDict() const;
    
    /// Write a graphviz dot-file for the dependency graph of all. currently
    /// known libraries/modules to \a file.
    TF_API
    void WriteDotFile(std::string const &file) const;
    
  private:

    struct _LibInfo {
        _LibInfo() {}
        std::vector<TfToken> predecessors, successors;
    };

    typedef TfHashMap<TfToken, _LibInfo, TfToken::HashFunctor>
    _TokenToInfoMap;

    typedef TfHashMap<TfToken, TfToken, TfToken::HashFunctor>
    _TokenToTokenMap;
    
    typedef TfHashSet<TfToken, TfToken::HashFunctor>
    _TokenSet;
    
    TfScriptModuleLoader();
    virtual ~TfScriptModuleLoader();
    friend class TfSingleton<This>;
    
    void _AddSuccessor(TfToken const &lib, TfToken const &successor);
    void _LoadModulesFor(TfToken const &name);
    void _LoadUpTo(TfToken const &name);
    void _GetOrderedDependenciesRecursive(TfToken const &lib,
                                          TfToken::HashSet *seenLibs,
                                          std::vector<TfToken> *result) const;
    void _GetOrderedDependencies(std::vector<TfToken> const &input,
                                 std::vector<TfToken> *result) const;
    void _TopologicalSort(std::vector<TfToken> *result) const;

    bool _HasTransitiveSuccessor(TfToken const &predecessor,
                                 TfToken const &successor) const;

    _TokenToInfoMap _libInfo;
    _TokenToTokenMap _libsToModules;
    _TokenSet _loadedSet;

    // This is only used to handle reentrant loading requests.
    std::deque<TfToken> _remainingLoadWork;
};

TF_API_TEMPLATE_CLASS(TfSingleton<TfScriptModuleLoader>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SCRIPT_MODULE_LOADER_H
