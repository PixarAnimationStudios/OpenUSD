//
// Copyright 2023 Pixar
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
#ifndef PXR_BASE_TF_UNICODE_UTILS_IMPL_H
#define PXR_BASE_TF_UNICODE_UTILS_IMPL_H

/// \file tf/unicodeUtils.h
/// \ingroup group_tf_String

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace TfUnicodeUtils {
namespace Impl {

/// Determines whether the given Unicode \a codePoint is in the XID_Start charaacter class.
///
/// The XID_Start class of characters are derived from the Unicode General_Category of uppercase letters, lowercase letters, titlecase letters,
/// modifier letters, other letters, letters numbers, plus Other_ID_Start, minus Pattern_Syntax and Pattern_White_Space code points.
/// That is, the character must have a category of Lu | Ll | Lt | Lm | Lo | Nl | '_'
///
TF_API
bool IsUTF8CharXIDStart(uint32_t codePoint);

/// Determines whether the given Unicode \a codePoint is in the XID_Continue character class.
///
/// The XID_Continue class of characters include those in XID_Start plus characters having the Unicode General Category of nonspacing marks,
/// spacing combining marks, decimal number, and connector punctuation.
/// That is, the character must have a category of XID_Start | Nd | Mn | Mc | Pc
///
TF_API
bool IsUTF8CharXIDContinue(uint32_t codePoint);

/// 
/// Appends the UTF-8 byte representation of the given \a codePoint to the end of \a result.
///
void AppendUTF8Char(uint32_t codePoint, std::string& result);

/// Determines whether the UTF-8 encoded substring in a string starting at position \a sequenceStart and ending at position \a end is a
/// valid Unicode identifier.  A valid Unicode identifier is a string that starts with something from the XID_Start character class (including the '_'
/// character) followed by one or more characters in the XID_Continue character class (including the '_' character).
///
/// The XID_Start class of characters are derived from the Unicode General_Category of uppercase letters, lowercase letters, titlecase letters,
/// modifier letters, other letters, letters numbers, plus Other_ID_Start, minus Pattern_Syntax and Pattern_White_Space code points.
/// That is, the character must have a category of Lu | Ll | Lt | Lm | Lo | Nl | '_'
///
/// The XID_Continue class of characters include those in XID_Start plus characters having the Unicode General Category of nonspacing marks,
/// spacing combining marks, decimal number, and connector punctuation.
/// That is, the character must have a category of XID_Start | Nd | Mn | Mc | Pc
///
/// UTF-8 characters are variable encoded, so \a sequenceStart defines the first byte in the UTF-8 character sequence. This method can be used for entire
/// strings by passing identifier.begin() and identifier.end(), but also on sub sequences defined by the given iterator ranges to avoid copying
/// the subsequence to a temporary string for evaluation.
///
TF_API
bool IsValidUTF8Identifier(const std::string::const_iterator& sequenceStart, const std::string::const_iterator& end);

///
/// Constructs a valid result identifier from \a identifier.
/// If \a identifier is already valid, the return value of this method should be value stored in \a identifier.
///
/// An identifier is valid according to the rules associated with \a IsValidUTF8Identifier.
///
TF_API
std::string MakeValidUTF8Identifier(const std::string& identifier);

} // end Impl
} // end TfUnicodeUtils

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_UNICODE_UTILS_IMPL_H_