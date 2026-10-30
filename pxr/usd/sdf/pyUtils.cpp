//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/pyUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
SdfFileFormatArgumentsFromPython(
    const nanobind::dict& dict,
    SdfLayer::FileFormatArguments* args,
    std::string* errMsg)
{
    SdfLayer::FileFormatArguments argsMap;
    typedef SdfLayer::FileFormatArguments::key_type ArgKeyType;
    typedef SdfLayer::FileFormatArguments::mapped_type ArgValueType;

    for (auto item : dict) {
        ArgKeyType key;
        if (!nanobind::try_cast<ArgKeyType>(item.first, key)) {
            if (errMsg) {
                *errMsg = "All file format argument keys must be strings";
            }
            return false;
        }

        ArgValueType val;
        if (!nanobind::try_cast<ArgValueType>(item.second, val)) {
            if (errMsg) {
                *errMsg = "All file format argument values must be strings";
            }
            return false;
        }

        argsMap[key] = val;
    }

    args->swap(argsMap);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
