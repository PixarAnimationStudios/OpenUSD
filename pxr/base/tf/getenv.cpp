//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
