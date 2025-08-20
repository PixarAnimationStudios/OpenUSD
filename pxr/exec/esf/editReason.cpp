//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esf/editReason.h"

#include "pxr/base/tf/diagnostic.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

const char *
EsfEditReason::_GetBitDescription(EsfEditReason::_BitIndex bitIndex)
{
    switch (bitIndex) {
    case EsfEditReason::_BitIndex::ResyncedObject:
        return "ResyncedObject";
    case EsfEditReason::_BitIndex::ChangedPropertyList:
        return "ChangedPropertyList";
    case EsfEditReason::_BitIndex::ChangedTargetPaths:
        return "ChangedTargetPaths";
    case EsfEditReason::_BitIndex::Max:
        break; // fallthrough.
    }

    TF_VERIFY(false, "Invalid EsfEditReason value");
    return "InvalidBit";
}

std::string
EsfEditReason::GetDescription() const
{
    std::string result;

    if (_bits == 0) {
        result = "None";
        return result;
    }

    bool useSeparator = false;
    constexpr static uint8_t MAX_BITS = static_cast<uint8_t>(_BitIndex::Max);
    for (uint8_t bitIndex = 0; bitIndex < MAX_BITS; bitIndex++) {
        if (_bits & (1 << bitIndex)) {
            if (useSeparator) {
                result += ", ";
            }
            result += _GetBitDescription(static_cast<_BitIndex>(bitIndex));
            useSeparator = true;
        }
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
