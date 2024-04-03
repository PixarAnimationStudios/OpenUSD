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

#include <array>

PXR_NAMESPACE_OPEN_SCOPE

static constexpr
std::array<std::pair<uint32_t, uint32_t>, {xid_start_ranges_size}>
_xidStartRanges = {{
{xid_start_ranges}
}};

static constexpr
std::array<std::pair<uint32_t, uint32_t>, {xid_continue_ranges_size}>
_xidContinueRanges = {{
{xid_continue_ranges}
}};

TfUnicodeXidStartFlagData::TfUnicodeXidStartFlagData()
{
    // set all of the bits corresponding to the code points in the range
    for (const auto& pair : _xidStartRanges)
    {
        for (uint32_t i = pair.first; i <= pair.second; i++)
        {
            this->_flags[static_cast<size_t>(i)] = true;
        }
    }
}

TfUnicodeXidContinueFlagData::TfUnicodeXidContinueFlagData()
{
    // set all of the bits corresponding to the code points in the range
    for (const auto& pair : _xidContinueRanges)
    {
        for (uint32_t i = pair.first; i <= pair.second; i++)
        {
            this->_flags[static_cast<size_t>(i)] = true;
        }
    }
}

static TfStaticData<TfUnicodeXidStartFlagData> _xidStartFlagData;
static TfStaticData<TfUnicodeXidContinueFlagData> _xidContinueFlagData;

const TfUnicodeXidStartFlagData&
TfUnicodeGetXidStartFlagData()
{
    return *_xidStartFlagData;
}

const TfUnicodeXidContinueFlagData&
TfUnicodeGetXidContinueFlagData()
{
    return *_xidContinueFlagData;
}

PXR_NAMESPACE_CLOSE_SCOPE