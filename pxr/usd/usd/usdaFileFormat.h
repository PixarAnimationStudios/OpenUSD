//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_USDA_FILE_FORMAT_H
#define PXR_USD_USD_USDA_FILE_FORMAT_H
 
#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/textFileFormat.h"
#include "pxr/usd/sdf/usdaFileFormat.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_USDA_FILE_FORMAT_TOKENS \
    ((Id,      "usda"))             \
    ((Version, "1.0"))

/// Struct containing .usda file format related tokens.
///
/// \deprecated in favor of SdfUsdaFileFormatTokens
TF_DECLARE_PUBLIC_TOKENS(UsdUsdaFileFormatTokens, USD_API,
    USD_USDA_FILE_FORMAT_TOKENS);

/// \class UsdUsdaFileFormat
///
/// \deprecated and aliased in favor of SdfUsdaFileFormat
///
using UsdUsdaFileFormat = SdfUsdaFileFormat;
using UsdUsdaFileFormatPtr = SdfUsdaFileFormatPtr;
using UsdUsdaFileFormatConstPtr = SdfUsdaFileFormatConstPtr;
using UsdUsdaFileFormatPtrVector = SdfUsdaFileFormatPtrVector;
using UsdUsdaFileFormatConstPtrVector = SdfUsdaFileFormatConstPtrVector;
using UsdUsdaFileFormatRefPtr = SdfUsdaFileFormatRefPtr;
using UsdUsdaFileFormatConstRefPtr = SdfUsdaFileFormatConstRefPtr;
using UsdUsdaFileFormatRefPtrVector = SdfUsdaFileFormatRefPtrVector;
using UsdUsdaFileFormatConstRefPtrVector = SdfUsdaFileFormatConstRefPtrVector;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_USDA_FILE_FORMAT_H
