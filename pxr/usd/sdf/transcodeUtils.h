//
// Copyright 2024 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_USD_SDF_TRANSCODE_UTILS_H
#define PXR_USD_SDF_TRANSCODE_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include <string>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

/// \enum SdfTranscodeFormat
/// Encoding algorithm produces different output depending on Format.
///
/// ASCII: The identifier is composed only of alphanumerics characters and underscore.
///
/// UNICODE_XID: The identifier is composed of XidStart and many XidContinue characters.
///
enum class SdfTranscodeFormat
{
    ASCII,
    UNICODE_XID
};

/// \name Sdf Transcode Encode Identifier
/// @{
/// Encodes an identifier using the Bootstring algorithm.
/// - If the input is empty it will return the smallest posible encoding, i.e. `tn__`.
/// - If the input is non-empty, but it is an invalid UTF-8 string, return value will not be present and raises TF_RUNTIME_ERROR.
/// - If the input is a non-empty valid UTF-8 string and does already complies with format, return value will be input string.
/// - If the input is a non-empty valid UTF-8 string and does not complies with format, return value will be a transcoded string.
/// As per above, re encoding a transcoded string with the same format should lead to the input string. 
/// The ouput string will be prefixed by `tn__` to indicate transcoding.
///
/// \code{.cpp}
/// static_assert(SdfEncodeIdentifier("", SdfTranscodeFormat.ASCII) == "tn__");
/// static_assert(SdfEncodeIdentifier("hello_world", SdfTranscodeFormat.ASCII) == "hello_world");
/// static_assert(SdfEncodeIdentifier("カーテンウォール", SdfTranscodeFormat.ASCII) == "tn__sxB76l2Y5o0X16");
/// \endcode
///
/// \code{.py}
/// assert Sdf.EncodeIdentifier("", Sdf.TranscodeFormat.ASCII) == "tn__"
/// assert Sdf.EncodeIdentifier("hello_world", Sdf.TranscodeFormat.ASCII) == "hello_world"
/// assert Sdf.EncodeIdentifier(""カーテンウォール", Sdf.TranscodeFormat.ASCII) == "tn__sxB76l2Y5o0X16"
/// \endcode
SDF_API
std::optional<std::string> 
SdfEncodeIdentifier(
    std::string_view inputString, 
    SdfTranscodeFormat format);
/// @}

/// \name Sdf Transcode Decode Identifier
/// @{
/// Decodes an identifier using the Bootstring algorithm. Notice decoding process is independent of the encoding format used.
/// - If the input is empty, return value will be empty string.
/// - If the input does not start with `tn__`, return value will be input string.
/// - If the input starts with `tn__`, but it is an invalid UTF-8 string, return value will not be present and raises TF_RUNTIME_ERROR.
/// - If the input starts with `tn__`, it is a valid UTF-8 string but cannot be decoded, return value will not be present and raises TF_RUNTIME_ERROR.
/// - If the input starts with `tn__`, it is a valid UTF-8 string and can be decoded, return value will be present.
///
/// \code{.cpp}
/// static_assert(SdfDecodeIdentifier("tn__") == "");
/// static_assert(SdfDecodeIdentifier("tn__sxB76l2Y5o0X16")=="カーテンウォール");
/// \endcode
///
/// \code{.py}
/// assert Sdf.DecodeIdentifier("tn__") == ""
/// assert Sdf.DecodeIdentifier("tn__sxB76l2Y5o0X16") == "カーテンウォール"
/// \endcode
SDF_API
std::optional<std::string> 
SdfDecodeIdentifier(
    std::string_view inputString);
/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_TRANSCODE_UTILS_H
