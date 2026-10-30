//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/pyUtils.h"
#include "pxr/base/tf/diagnostic.h"

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

bool
PcpVariantFallbackMapFromPython(
    const nanobind::dict& d,
    PcpVariantFallbackMap *result)
{
    for (auto item : d) {
        std::string k;
        std::vector<std::string> v;

        if (!nanobind::try_cast<std::string>(item.first, k)) {
            TF_CODING_ERROR("unrecognized type for PcpVariantFallbackMap key");
            return false;
        }
        if (!nanobind::try_cast<std::vector<std::string>>(item.second, v)) {
            TF_CODING_ERROR("unrecognized type for PcpVariantFallbackMap val");
            return false;
        }
        if (!k.empty() && !v.empty()) {
            (*result)[k] = v;
        }
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
