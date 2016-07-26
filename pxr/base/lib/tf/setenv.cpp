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
#include "pxr/base/tf/setenv.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/arch/errno.h"

bool
TfSetenv(const std::string & name, const std::string & value)
{
    if (TfPyIsInitialized())
        return TfPySetenv(name, value);

    if (setenv(name.c_str(), value.c_str(), /* overwrite */ 1) == 0)
        return true;

    TF_WARN("Error setting '%s': %s", name.c_str(), ArchStrerror().c_str());
    return false;
}

bool
TfUnsetenv(const std::string & name)
{
    if (TfPyIsInitialized())
        return TfPyUnsetenv(name);

    if (unsetenv(name.c_str()) == 0)
        return true;

    TF_WARN("Error unsetting '%s': %s", name.c_str(), ArchStrerror().c_str());
    return false;
}

