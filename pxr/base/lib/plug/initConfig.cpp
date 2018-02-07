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
#include "pxr/base/plug/info.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/symbols.h"

#include <boost/preprocessor/stringize.hpp>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const char* pathEnvVarName      = BOOST_PP_STRINGIZE(PXR_PLUGINPATH_NAME);
const char* buildLocation       = BOOST_PP_STRINGIZE(PXR_BUILD_LOCATION);
const char* pluginBuildLocation = BOOST_PP_STRINGIZE(PXR_PLUGIN_BUILD_LOCATION);

#ifdef PXR_INSTALL_LOCATION
const char* installLocation     = BOOST_PP_STRINGIZE(PXR_INSTALL_LOCATION); 
#endif // PXR_INSTALL_LOCATION

void
_AppendPathList(
    std::vector<std::string>* result, 
    const std::string& paths, const std::string& sharedLibPath)
{
    for (const auto& path: TfStringSplit(paths, ARCH_PATH_LIST_SEP)) {
        if (path.empty()) {
            continue;
        }

        // Anchor all relative paths to the shared library path.
        const bool isLibraryRelativePath = TfIsRelativePath(path);
        if (isLibraryRelativePath) {
            result->push_back(TfStringCatPaths(sharedLibPath, path));
        }
        else {
            result->push_back(path);
        }
    }
}

ARCH_CONSTRUCTOR(Plug_InitConfig, 2, void)
{
    std::vector<std::string> result;

    // Determine the absolute path to the Plug shared library.
    // Any relative paths specified in the plugin search path will be
    // anchored to this directory, to allow for relocatability.
    std::string sharedLibPath;
    if (!ArchGetAddressInfo(
        reinterpret_cast<void*>(&Plug_InitConfig), &sharedLibPath,
            nullptr, nullptr, nullptr)) {
        TF_CODING_ERROR("Unable to determine absolute path for Plug.");
    }

    sharedLibPath = TfGetPathName(sharedLibPath);

    // Environment locations.
    _AppendPathList(&result, TfGetenv(pathEnvVarName), sharedLibPath);

    // Fallback locations.
    _AppendPathList(&result, buildLocation, sharedLibPath);
    _AppendPathList(&result, pluginBuildLocation, sharedLibPath);

#ifdef PXR_INSTALL_LOCATION
    _AppendPathList(&result, installLocation, sharedLibPath);
#endif // PXR_INSTALL_LOCATION

    Plug_SetPaths(result);
}

}

PXR_NAMESPACE_CLOSE_SCOPE
