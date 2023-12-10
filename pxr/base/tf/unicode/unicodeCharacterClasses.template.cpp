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

#include "pxr/pxr.h"
#include "pxr/base/tf/unicodeCharacterClasses.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// @brief 
/// Provides static initialization of the character class data
/// contained within the XID_Start set of Unicode character classes.
///
struct Tf_UnicodeXidStartRangeData
{
public:

    Tf_UnicodeXidStartRangeData();

    std::vector<std::pair<uint32_t, uint32_t>> ranges;
};

/// @brief 
/// Provides static initialization of the character class data
/// contained within the XID_Continue set of Unicode character classes.
///
struct Tf_UnicodeXidContinueRangeData
{
public:

    Tf_UnicodeXidContinueRangeData();

    std::vector<std::pair<uint32_t, uint32_t>> ranges;
};

// holds the compacted ranges of XID_Start and XID_Continue
// character classes
static TfStaticData<Tf_UnicodeXidStartRangeData> _xidStartRangeData;
static TfStaticData<Tf_UnicodeXidContinueRangeData> _xidContinueRangeData;

Tf_UnicodeXidStartRangeData::Tf_UnicodeXidStartRangeData()
{
    {xid_start_ranges}
}

Tf_UnicodeXidContinueRangeData::Tf_UnicodeXidContinueRangeData()
{
    {xid_continue_ranges}
}

TfUnicodeXidStartFlagData::TfUnicodeXidStartFlagData()
{
    // set all of the bits corresponding to the code points in the range
    for (const auto& pair : _xidStartRangeData->ranges)
    {
        for (uint32_t i = pair.first; i <= pair.second; i++)
        {
            this->_flags[static_cast<size_t>(i)] = true;
        }
    }
}

bool
TfUnicodeXidStartFlagData::IsXidStartCodePoint(uint32_t codePoint) const
{
    return (codePoint < TF_MAX_CODE_POINT) ? _flags[codePoint] : false;
}

TfUnicodeXidContinueFlagData::TfUnicodeXidContinueFlagData()
{
    // set all of the bits corresponding to the code points in the range
    for (const auto& pair : _xidContinueRangeData->ranges)
    {
        for (uint32_t i = pair.first; i <= pair.second; i++)
        {
            this->_flags[static_cast<size_t>(i)] = true;
        }
    }
}

bool
TfUnicodeXidContinueFlagData::IsXidContinueCodePoint(uint32_t codePoint) const
{
    return (codePoint < TF_MAX_CODE_POINT) ? _flags[codePoint] : false;
}

static TfStaticData<TfUnicodeXidStartFlagData> xidStartFlagData;
static TfStaticData<TfUnicodeXidContinueFlagData> xidContinueFlagData;

const TfUnicodeXidStartFlagData&
TfUnicodeGetXidStartFlagData()
{
    return *xidStartFlagData;
}

const TfUnicodeXidContinueFlagData&
TfUnicodeGetXidContinueFlagData()
{
    return *xidContinueFlagData;
}

PXR_NAMESPACE_CLOSE_SCOPE