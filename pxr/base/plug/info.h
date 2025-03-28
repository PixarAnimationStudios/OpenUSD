//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_PLUG_INFO_H
#define PXR_BASE_PLUG_INFO_H

#include "pxr/pxr.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/js/value.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class JsValue;

/// Data describing the plugin itself.
class Plug_RegistrationMetadata {
public:
    enum Type {
        UnknownType,
        LibraryType,
        PythonType,
        ResourceType
    };

    Plug_RegistrationMetadata() : type(UnknownType) { }
    Plug_RegistrationMetadata(const JsValue&,
                              const std::string& valuePathname,
                              const std::string& locationForErrorReporting);

    Type type;
    std::string pluginName;
    std::string pluginPath;
    JsObject plugInfo;
    std::string libraryPath;
    std::string resourcePath;
};

/// A task arena for reading plug info.
class Plug_TaskArena {
public:
    class Synchronous { };  // For single-threaded debugging.
    Plug_TaskArena();
    Plug_TaskArena(Synchronous);
    ~Plug_TaskArena();

    /// Schedule \p fn to run.
    template <class Fn>
    void Run(Fn const &fn);

    /// Wait for all scheduled tasks to complete.
    void Wait();

private:
    class _Impl;
    std::unique_ptr<_Impl> _impl;
};

/// Reads several plugInfo files, recursively loading any included files.
/// \p addPlugin is invoked each time a plugin is found.  The order in
/// which plugins is discovered is undefined.  \p addPlugin is invoked
/// by calling \c Run() on \p taskArena.
///
/// \p addVisitedPath is called each time a plug info file is found;  if it
/// returns \c true then the file is processed, otherwise it is ignored. 
/// Clients should return \c true or \c false the first time a given path
/// is passed and \c false all subsequent times.
void
Plug_ReadPlugInfo(
    const std::vector<std::string>& pathnames,
    bool pathsAreOrdered,
    const std::function<bool (const std::string&)>& addVisitedPath,
    const std::function<void (const Plug_RegistrationMetadata&)>& addPlugin,
    Plug_TaskArena* taskArena);

/// Sets the paths to the bootstrap plugInfo JSON files, also any diagnostic
/// messages that should be reported when plugins are registered (if any).
/// The priority order of elements of the path is honored if pathsAreOrdered.
/// Defined in registry.cpp.
void Plug_SetPaths(const std::vector<std::string>&,
                   const std::vector<std::string>&,
                   bool pathsAreOrdered);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_PLUG_INFO_H
