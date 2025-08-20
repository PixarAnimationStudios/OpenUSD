//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_USD_FILE_FORMAT_H
#define PXR_USD_USD_USD_FILE_FORMAT_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/usdFileFormat.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_USD_FILE_FORMAT_TOKENS  \
    ((Id,           "usd"))         \
    ((Version,      "1.0"))         \
    ((Target,       "usd"))         \
    ((FormatArg,    "format"))

/// Struct containing .usd file format related tokens.
///
/// \deprecated in favor of SdfUsdFileFormatTokens
TF_DECLARE_PUBLIC_TOKENS(UsdUsdFileFormatTokens, USD_API,
    USD_USD_FILE_FORMAT_TOKENS);

/// \class UsdUsdFileFormat
///
/// \deprecated and aliased in favor of SdfUsdFileFormat
using UsdUsdFileFormat = SdfUsdFileFormat;
using UsdUsdFileFormatPtr = SdfUsdFileFormatPtr;
using UsdUsdFileFormatConstPtr = SdfUsdFileFormatConstPtr;
using UsdUsdFileFormatPtrVector = SdfUsdFileFormatPtrVector;
using UsdUsdFileFormatConstPtrVector = SdfUsdFileFormatConstPtrVector;
using UsdUsdFileFormatRefPtr = SdfUsdFileFormatRefPtr;
using UsdUsdFileFormatConstRefPtr = SdfUsdFileFormatConstRefPtr;
using UsdUsdFileFormatRefPtrVector = SdfUsdFileFormatRefPtrVector;
using UsdUsdFileFormatConstRefPtrVector = SdfUsdFileFormatConstRefPtrVector;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_USD_FILE_FORMAT_H
