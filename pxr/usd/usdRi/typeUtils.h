//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_RI_TYPE_UTILS_H
#define PXR_USD_USD_RI_TYPE_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


std::string UsdRi_GetRiType(const SdfValueTypeName &usdType);
SdfValueTypeName UsdRi_GetUsdType(const std::string &riType);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_RI_TYPE_UTILS_H
