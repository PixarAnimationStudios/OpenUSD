//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_USDC_FILE_FORMAT_H
#define PXR_USD_USD_USDC_FILE_FORMAT_H
 
#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/usdcFileFormat.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_USDC_FILE_FORMAT_TOKENS   \
    ((Id,      "usdc"))

/// Struct containing .usdc file format related tokens.
///
/// \deprecated in favor of SdfUsdcFileFormatTokens
TF_DECLARE_PUBLIC_TOKENS(UsdUsdcFileFormatTokens, USD_API,
    USD_USDC_FILE_FORMAT_TOKENS);

/// \class UsdUsdcFileFormat
///
/// \deprecated and aliased in favor of SdfUsdcFileFormat
using UsdUsdcFileFormat = SdfUsdcFileFormat;
using UsdUsdcFileFormatPtr = SdfUsdcFileFormatPtr;
using UsdUsdcFileFormatConstPtr = SdfUsdcFileFormatConstPtr;
using UsdUsdcFileFormatPtrVector = SdfUsdcFileFormatPtrVector;
using UsdUsdcFileFormatConstPtrVector = SdfUsdcFileFormatConstPtrVector;
using UsdUsdcFileFormatRefPtr = SdfUsdcFileFormatRefPtr;
using UsdUsdcFileFormatConstRefPtr = SdfUsdcFileFormatConstRefPtr;
using UsdUsdcFileFormatRefPtrVector = SdfUsdcFileFormatRefPtrVector;
using UsdUsdcFileFormatConstRefPtrVector = SdfUsdcFileFormatConstRefPtrVector;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_USDC_FILE_FORMAT_H
