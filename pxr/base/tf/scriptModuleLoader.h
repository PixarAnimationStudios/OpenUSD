//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SCRIPT_MODULE_LOADER_H
#define PXR_BASE_TF_SCRIPT_MODULE_LOADER_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/spinRWMutex.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakBase.h"

// XXX: This include is a hack to avoid build errors due to
// incompatible macro definitions in pyport.h on macOS.
#include <locale>
#include "pxr/external/boost/python/dict.hpp"

#include <deque>
#include <string>
#include <unordered_map>
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
    
    /// Return a python dict containing all currently known modules under
    /// their canonical names.
    TF_API
    pxr_boost::python::dict GetModulesDict() const;
    
    /// Write a graphviz dot-file for the dependency graph of all currently
    /// registered libraries/modules to \a file.
    TF_API
    void WriteDotFile(std::string const &file) const;
    
  private:
    friend class TfSingleton<This>;

    struct _LibInfo {
        _LibInfo() = default;
        _LibInfo(TfToken const &moduleName,
                 std::vector<TfToken> &&predecessors)
            : moduleName(moduleName)
            , predecessors(predecessors) { }
        
        TfToken moduleName;
        std::vector<TfToken> predecessors;
        mutable std::atomic<bool> isLoaded = false;
    };

    using _LibInfoMap =
        std::unordered_map<TfToken, _LibInfo, TfToken::HashFunctor>;

    using _LibAndInfo = _LibInfoMap::value_type;

    TfScriptModuleLoader();
    virtual ~TfScriptModuleLoader();

    _LibInfo const *_FindInfo(TfToken const &lib) const;
    
    void _LoadLibModules(std::vector<_LibAndInfo const *> const &toLoad) const;
    
    _LibInfoMap _libInfo;
    mutable TfSpinRWMutex _mutex;
};

TF_API_TEMPLATE_CLASS(TfSingleton<TfScriptModuleLoader>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SCRIPT_MODULE_LOADER_H
