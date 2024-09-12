//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_PY_UTILS_H
#define PXR_USD_PCP_PY_UTILS_H

#include "pxr/external/boost/python/dict.hpp"

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/types.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Convert the Python dictionary \p dict to a PcpVariantFallbackMap
/// object and return it via \p result, returning true if successful.
PCP_API
bool
PcpVariantFallbackMapFromPython(
    const pxr_boost::python::dict& dict,
    PcpVariantFallbackMap *result);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_PY_UTILS_H
