//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_USDZ_FILE_FORMAT_H
#define PXR_USD_USD_USDZ_FILE_FORMAT_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/usdzFileFormat.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_USDZ_FILE_FORMAT_TOKENS  \
    ((Id,      "usdz"))              \
    ((Version, "1.0"))               \
    ((Target,  "usd"))

/// Struct containing .usdz file format related tokens.
///
/// \deprecated in favor of SdfUsdzFileFormatTokens
TF_DECLARE_PUBLIC_TOKENS(
    UsdUsdzFileFormatTokens, USD_API, USD_USDZ_FILE_FORMAT_TOKENS);

/// \class UsdUsdzFileFormat
///
/// \deprecated in favor of SdfUsdzFileFormat
/// File format for package .usdz files.
using UsdUsdzFileFormat = SdfUsdzFileFormat;
using UsdUsdzFileFormatPtr = SdfUsdzFileFormatPtr;
using UsdUsdzFileFormatConstPtr = SdfUsdzFileFormatConstPtr;
using UsdUsdzFileFormatPtrVector = SdfUsdzFileFormatPtrVector;
using UsdUsdzFileFormatConstPtrVector = SdfUsdzFileFormatConstPtrVector;
using UsdUsdzFileFormatRefPtr = SdfUsdzFileFormatRefPtr;
using UsdUsdzFileFormatConstRefPtr = SdfUsdzFileFormatConstRefPtr;
using UsdUsdzFileFormatRefPtrVector = SdfUsdzFileFormatRefPtrVector;
using UsdUsdzFileFormatConstRefPtrVector = SdfUsdzFileFormatConstRefPtrVector;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_USDZ_FILE_FORMAT_H
