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
#ifndef PLUG_INFO_H
#define PLUG_INFO_H

#include "pxr/pxr.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/js/value.h"

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class JsValue;

/// Data describing the plugin itself.
struct Plug_RegistrationMetadata {
    enum Type {
        UnknownType,
        LibraryType,
#ifdef PXR_PYTHON_SUPPORT_ENABLED
        PythonType,
#endif // PXR_PYTHON_SUPPORT_ENABLED
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
    void Run(const boost::function<void()>& fn);

    /// Wait for all scheduled tasks to complete.
    void Wait();

private:
    class _Impl;
    boost::scoped_ptr<_Impl> _impl;
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
    const boost::function<bool (const std::string&)>& addVisitedPath,
    const boost::function<void (const Plug_RegistrationMetadata&)>& addPlugin,
    Plug_TaskArena* taskArena);

/// Sets the paths to the bootstrap plug-path JSON files.
void Plug_SetPaths(const std::vector<std::string>&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PLUG_INFO_H
