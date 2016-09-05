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
#include "pxr/base/arch/env.h"
#include "pxr/base/arch/error.h"

std::string ArchGetEnv(const std::string &name)
{
#if defined(ARCH_OS_WINDOWS)
    size_t requiredSize;

    getenv_s(&requiredSize, NULL, 0, name.c_str());
    if (requiredSize)
    {
        std::string result;

        if (requiredSize < result.size())
            result.resize(requiredSize);

        getenv_s(&requiredSize, &result[0], requiredSize, name.c_str());
        return result;
    }
    else
        return nullptr;

#else
    return getenv(name.c_str());
#endif
}

bool ArchSetEnv(const std::string &name, const std::string &value, int overwrite)
{
#if defined(ARCH_OS_WINDOWS)
    if (_putenv_s(name.c_str(), value.c_str()) == 0)
#else
    if (setenv(name.c_str(), value.c_str(), overwrite) == 0)
#endif
        return true;

    ARCH_WARNING("Error setting an environment variable");
    return false;
}

bool ArchRemoveEnv(const std::string &name)
{
#if defined(ARCH_OS_WINDOWS)
    if (_putenv_s(name.c_str(), "") == 0)
#else
    if (unsetenv(name.c_str()) == 0)
#endif
        return true;

    return false;
}