//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDF_TEXT_PARSER_UTILS_H
#define PXR_USD_SDF_TEXT_PARSER_UTILS_H

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfValueTypeName;

/// Attempt to parse a VtValue from a string representing a value
/// given the expected sdf type. The parse follows the expectations of
/// the .usda file format. On success, returns a corresponding VtValue.
/// On failure, returns an empty VtValue and populates TfError(s).
SDF_API
VtValue Sdf_ParseValueFromString(const std::string& input,
                                 const SdfValueTypeName& expectedSdfType);

/// Add quotes around given string, escaping inner quotes and
/// unprintable characters as necessary. If the string contains newlines
/// it's quoted with triple quotes and the newlines are not escaped.
SDF_API
std::string Sdf_QuoteString(const std::string& input);

/// Add @'s around a given path to produce a string representation of
/// an asset path. If the path contains @, @@@ will be added around the
/// path. If the path contains @@@, the contained @@@ will be escaped.
SDF_API
std::string Sdf_QuoteAssetPath(const std::string& path);

PXR_NAMESPACE_CLOSE_SCOPE

#endif