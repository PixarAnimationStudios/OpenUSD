//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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