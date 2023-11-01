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

#include <vector>
#include <bitset>

PXR_NAMESPACE_OPEN_SCOPE

namespace TfUnicodeUtils {
namespace Impl {

    /// @brief 
    /// Small wrapper around a vector of pairs denoting valid code point ranges for a
    /// particular Unicode character class.  This class provides a default constructor
    /// that initializes the data such that it can be used as TfStaticData.
    ///
    class UnicodeRangeData
    {
        public:

            UnicodeRangeData();

        public:

            std::vector<std::pair<uint32_t, uint32_t>> ranges;
    };

    /// @brief 
    /// Provides static initialization of the character class data
    /// contained within the XID_Start set of Unicode character classes.
    ///
    class UnicodeXidStartRangeData : public UnicodeRangeData
    {
        public:

            UnicodeXidStartRangeData();
    };

    /// @brief 
    /// Provides static initialization of the character class data
    /// contained within the XID_Continue set of Unicode character classes.
    ///
    class UnicodeXidContinueRangeData : public UnicodeRangeData
    {
        public:

            UnicodeXidContinueRangeData();
    };

    // holds the compacted ranges of XID_Start and XID_Continue
    // character classes
    extern TfStaticData<UnicodeXidStartRangeData> xidStartRangeData;
    extern TfStaticData<UnicodeXidContinueRangeData> xidContinueRangeData;

    /// @brief 
    /// Small wrapper around a bitset denoting whether a given code point (uint32_t index)
    /// is contained within the set (1 = contained, 0 = not) such that it can
    /// be used with TfStaticData.
    ///
    class UnicodeFlagData
    {
        public:

            UnicodeFlagData();

        public:

            // Unicode defines a maximum of 17 * 2^16 code points
            // not all of these code points are valid code points
            // but we need this set to be contiguous
            std::bitset<1114112> flags;
    };

    /// @brief 
    /// Provides static initialization of the whether a Unicode code
    /// point is contained with the XID_Start set of Unicode character
    /// classes.
    ///
    class UnicodeXidStartFlagData : public UnicodeFlagData
    {
        public:

            UnicodeXidStartFlagData();
    };

    /// @brief 
    /// Provides static initialization of the whether a Unicode code
    /// point is contained with the XID_Continue set of Unicode character
    /// classes.
    ///
    class UnicodeXidContinueFlagData : public UnicodeFlagData
    {
        public:

            UnicodeXidContinueFlagData();
    };

    // holds the uncompressed data per Unicode code point
    // determining whether or not the code point is in the given
    // character class
    extern TfStaticData<UnicodeXidStartFlagData> xidStartFlagData;
    extern TfStaticData<UnicodeXidContinueFlagData> xidContinueFlagData;
}
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_UNICODE_CHARACTER_CLASSES_H_