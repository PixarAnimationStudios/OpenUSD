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
#ifndef PXR_BASE_TF_UNICODE_CHARACTER_CLASSES_H
#define PXR_BASE_TF_UNICODE_CHARACTER_CLASSES_H

#include "pxr/pxr.h"
#include "pxr/base/tf/staticData.h"

#include <bitset>

PXR_NAMESPACE_OPEN_SCOPE

// Unicode defines a maximum of 17 * 2^16 code points
// not all of these code points are valid code points
// but we need the flags to be contiguous
constexpr uint32_t TF_MAX_CODE_POINT = 1114112;

/// \brief 
/// Provides static initialization of the whether a Unicode code
/// point is contained with the XID_Start set of Unicode character
/// classes.
///
class TfUnicodeXidStartFlagData
{
public:

    TfUnicodeXidStartFlagData();

    /// \brief Determines whether the given code point is contained within
    /// the XID_Start character class.
    /// \param codePoint The Unicode code point to determine inclusion for.
    /// \return true if the given codePoint is in the XID_Start character
    /// class, false otherwise.
    inline bool IsXidStartCodePoint(uint32_t codePoint) const {
        return (codePoint < TF_MAX_CODE_POINT) ? _flags[codePoint] : false;
    }

private:

    std::bitset<TF_MAX_CODE_POINT> _flags;
};

/// \brief 
/// Provides static initialization of the whether a Unicode code
/// point is contained with the XID_Continue set of Unicode character
/// classes.
///
class TfUnicodeXidContinueFlagData
{
public:

    TfUnicodeXidContinueFlagData();

    /// \brief Determines whether the given code point is contained within
    /// the XID_Continue character class.
    /// \param codePoint The Unicode code point to determine inclusion for.
    /// \return true if the given codePoint is in the XID_Continue 
    /// character class false otherwise.
    inline bool IsXidContinueCodePoint(uint32_t codePoint) const {
        return (codePoint < TF_MAX_CODE_POINT) ? _flags[codePoint] : false;
    }

private:
    
    std::bitset<TF_MAX_CODE_POINT> _flags;
};

/// \brief Retrieves character class data for XID_Start.
/// \return An object which can be used to interrogate whether a code point
/// is contained within the XID_Start character class.
const TfUnicodeXidStartFlagData& TfUnicodeGetXidStartFlagData();

/// \brief Retrieves character class data for XID_Continue.
/// \return An object which can be used to interrogate whether a code point
/// is contained within the XID_Continue character class.
const TfUnicodeXidContinueFlagData& TfUnicodeGetXidContinueFlagData();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_UNICODE_CHARACTER_CLASSES_H_