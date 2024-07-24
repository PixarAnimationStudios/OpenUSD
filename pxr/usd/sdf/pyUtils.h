//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PY_UTILS_H
#define PXR_USD_SDF_PY_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/layer.h"

#include <boost/python/dict.hpp>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Convert the Python dictionary \p dict to an SdfLayer::FileFormatArguments
/// object and return it via \p args. 
///
/// If a non-string key or value is encountered, \p errMsg will be filled in
/// (if given) and this function will return false. Otherwise, this function
/// will return true.
SDF_API bool
SdfFileFormatArgumentsFromPython(
    const boost::python::dict& dict,
    SdfLayer::FileFormatArguments* args,
    std::string* errMsg = NULL);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PY_UTILS_H
