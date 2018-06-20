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
#include "usdKatana/bootstrap.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/dl.h"
#include "pxr/base/tf/stringUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnGroupBuilder.h>
#include <FnLogging/FnLogging.h>
#include <FnPluginManager/FnPluginManager.h>

#include <mutex>

typedef FnPluginManagerHostSuite_v1 const*
        (*GetFnPluginManagerHostSuite)(char const* apiName,
                                       unsigned int apiVersion);

PXR_NAMESPACE_OPEN_SCOPE

FnLogSetup("PxrUsdKatanaBootstrap");

void PxrUsdKatanaBootstrap()
{
    static std::once_flag once;
    std::call_once(once, []()
    {
        // Path of the katana process (without filename).
        std::string path = TfGetPathName(ArchGetExecutablePath());

        // FnAttribute::Bootstrap() appends 'bin', so remove it here.
        std::string const binPrefix("bin" ARCH_PATH_SEP);
        if (TfStringEndsWith(path, binPrefix))
            path.erase(path.length() - binPrefix.length());

        // Boostrap FnAttribute.
        FnAttribute::Bootstrap(path);

        // Load Katana's Plugin Manager dynamic library.
        path += binPrefix + "FnPluginSystem" ARCH_LIBRARY_SUFFIX;
        std::string dlError;
        void* handle = TfDlopen(path, ARCH_LIBRARY_NOW, &dlError);
        if (!handle)
        {
            FnLogError("Failed to open " << path << " to bootstrap Katana");
            return;
        }

        // Find the symbol.
        void* symbol = TfDlfindSymbol(handle, "FnPluginSystemGetHostSuite");
        if (!symbol)
        {
            FnLogError("Failed to symbol " << path << " to bootstrap Katana");
            return;
        }

        GetFnPluginManagerHostSuite pfnGetFnPluginManagerHostSuite =
            reinterpret_cast<GetFnPluginManagerHostSuite>(symbol);

        // Get the Host Suite.
        FnPluginManagerHostSuite_v1 const* hostSuite =
            pfnGetFnPluginManagerHostSuite("PluginManager", 1);

        if (hostSuite)
        {
            FnPluginHost* host = hostSuite->getHost();

            FnAttribute::Attribute::setHost(host);
            FnAttribute::GroupBuilder::setHost(host);
            Foundry::Katana::PluginManager::setHost(host);
        }

        TfDlclose(handle);
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
