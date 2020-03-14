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
#include "pxr/base/arch/env.h"
#include "pxr/base/tf/getenv.h"
#include <ctype.h>
#include <string>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

string
TfGetenv(const string& envName, const string& defaultValue)
{
    string value = ArchGetEnv(envName);

    if (value.empty())
        return defaultValue;
    else 
        return value;
}

int
TfGetenvInt(const string& envName, int defaultValue)
{
    string value = ArchGetEnv(envName);

    if (value.empty())
        return defaultValue;
    else 
        return std::stoi(value);
}

bool
TfGetenvBool(const string& envName, bool defaultValue)
{
    string value = ArchGetEnv(envName);

    if (value.empty())
        return defaultValue;
    else {
        for (char& c: value)
            c = tolower(c);

        return value == "true" ||
               value == "yes"  ||
               value == "on"   ||
               value == "1";
    }
}

double
TfGetenvDouble(const string& envName, double defaultValue)
{
    string value = ArchGetEnv(envName);

    if (value.empty())
        return defaultValue;
    else
        return std::stod(value);
}

PXR_NAMESPACE_CLOSE_SCOPE
