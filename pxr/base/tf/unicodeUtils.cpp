//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/unicodeCharacterClasses.h"
#include "pxr/base/tf/unicodeUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

std::ostream&
operator<<(std::ostream& stream, const TfUtf8CodePoint codePoint)
{
    const auto value = codePoint.AsUInt32();
    if (value < 0x80)
    {
        // 1-byte UTF-8 encoding
        stream << static_cast<char>(value);
    }
    else if (value < 0x800)
    {
        // 2-byte UTF-8 encoding
        stream << (static_cast<char>(static_cast<unsigned char>((value >> 6) | 0xc0)));
        stream << (static_cast<char>(static_cast<unsigned char>((value & 0x3f) | 0x80)));
    }
    else if (value < 0x10000)
    {
        // 3-byte UTF-8 encoding
        stream << (static_cast<char>(static_cast<unsigned char>((value >> 12) | 0xe0)));
        stream << (static_cast<char>(static_cast<unsigned char>(((value >> 6) & 0x3f) | 0x80)));
        stream << (static_cast<char>(static_cast<unsigned char>((value & 0x3f) | 0x80)));
    }
    else if (value < 0x110000)
    {
        // 4-byte UTF-8 encoding
        stream << (static_cast<char>(static_cast<unsigned char>((value >> 18) | 0xf0)));
        stream << (static_cast<char>(static_cast<unsigned char>(((value >> 12) & 0x3f) | 0x80)));
        stream << (static_cast<char>(static_cast<unsigned char>(((value >> 6) & 0x3f) | 0x80)));
        stream << (static_cast<char>(static_cast<unsigned char>((value & 0x3f) | 0x80)));
    }
    else
    {
        stream << TfUtf8InvalidCodePoint;
    }
    return stream;
}

uint32_t TfUtf8CodePointIterator::_GetCodePoint() const
{
    // determine what encoding length the character is
    _EncodingLength encodingLength = this->_GetEncodingLength();
    if (encodingLength > std::distance(_it, _end)) {
        // error condition, would read bytes past the end of the range
        return TfUtf8InvalidCodePoint.AsUInt32();
    }
    if (encodingLength == 1)
    {
        return static_cast<uint32_t>(static_cast<unsigned char>(*_it));
    }
    auto begin = _it;
    if (encodingLength == 2)
    {
        unsigned char byte1 = static_cast<unsigned char>(*begin);
        unsigned char byte2 = static_cast<unsigned char>(*(++begin));

        // ensure the ranges we expect, or it's not a valid character
        if (byte1 < static_cast<unsigned char>('\xc2') ||
            byte1 > static_cast<unsigned char>('\xdf'))
        {
            return TfUtf8InvalidCodePoint.AsUInt32();
        }
        if (byte2 < static_cast<unsigned char>('\x80') ||
            byte2 > static_cast<unsigned char>('\xbf'))
        {
            return TfUtf8InvalidCodePoint.AsUInt32();
        }

        // the code point is constructed from the last 5 bits of byte1
        // and the last 6 bits of byte2
        return ((byte1 & 0x1f) << 6) + (byte2 & 0x3f);
    }
    else if (encodingLength == 3)
    {
        unsigned char byte1 = static_cast<unsigned char>(*begin);
        unsigned char byte2 = static_cast<unsigned char>(*(++begin));
        unsigned char byte3 = static_cast<unsigned char>(*(++begin));

        // ensure the ranges we expect, or it's not a valid character
        if (byte1 == static_cast<unsigned char>('\xe0'))
        {
            // byte2 must be in range A0..BF
            // byte3 must be in range 80..BF
            if (byte2 < static_cast<unsigned char>('\xa0') ||
                byte2 > static_cast<unsigned char>('\xbf') ||
                byte3 < static_cast<unsigned char>('\x80') ||
                byte3 > static_cast<unsigned char>('\xbf'))
            {
                return TfUtf8InvalidCodePoint.AsUInt32();
            }
        }
        else if ((byte1 >= static_cast<unsigned char>('\xe1') &&
                  byte1 <= static_cast<unsigned char>('\xec')) ||
                  byte1 == static_cast<unsigned char>('\xee') ||
                  byte1 == static_cast<unsigned char>('\xef'))
        {
            // byte2 must be in range 80..BF
            // byte3 must be in range 80..BF
            if (byte2 < static_cast<unsigned char>('\x80') ||
                byte2 > static_cast<unsigned char>('\xbf') ||
                byte3 < static_cast<unsigned char>('\x80') ||
                byte3 > static_cast<unsigned char>('\xbf'))
            {
                return TfUtf8InvalidCodePoint.AsUInt32();
            }
        }
        else if (byte1 == static_cast<unsigned char>('\xed'))
        {
            // byte2 must be in range 80..9F
            // byte3 must be in range 80..BF
            if (byte2 < static_cast<unsigned char>('\x80') ||
                byte2 > static_cast<unsigned char>('\x9f') ||
                byte3 < static_cast<unsigned char>('\x80') ||
                byte3 > static_cast<unsigned char>('\xbf'))
            {
                return TfUtf8InvalidCodePoint.AsUInt32();
            }
        }
        else
        {
            // byte 1 invalid
            return TfUtf8InvalidCodePoint.AsUInt32();
        }

        // code point is constructed from the last 4 bits of byte1
        // and the last 6 bits of bytes 2 and 3
        return ((byte1 & 0xf) << 12) + ((byte2 & 0x3f) << 6) +
                (byte3 & 0x3f);
    }
    else if (encodingLength == 4)
    {
        unsigned char byte1 = static_cast<unsigned char>(*begin);
        unsigned char byte2 = static_cast<unsigned char>(*(++begin));
        unsigned char byte3 = static_cast<unsigned char>(*(++begin));
        unsigned char byte4 = static_cast<unsigned char>(*(++begin));

        if (byte1 == static_cast<unsigned char>('\xf0'))
        {
            // byte2 must be in range 90..BF
            // byte3 must be in range 80..BF
            // byte4 must be in range 80..BF
            if (byte2 < static_cast<unsigned char>('\x90') ||
                byte2 > static_cast<unsigned char>('\xbf') ||
                byte3 < static_cast<unsigned char>('\x80') ||
                byte3 > static_cast<unsigned char>('\xbf') ||
                byte4 < static_cast<unsigned char>('\x80') ||
                byte4 > static_cast<unsigned char>('\xbf'))
            {
                return TfUtf8InvalidCodePoint.AsUInt32();
            }
        }
        else if (byte1 >= static_cast<unsigned char>('\xf1') &&
                 byte1 <= static_cast<unsigned char>('\xf3'))
        {
            // byte2 must be in range 80..BF
            // byte3 must be in range 80..BF
            // byte4 must be in range 80..BF
            if (byte2 < static_cast<unsigned char>('\x80') ||
                byte2 > static_cast<unsigned char>('\xbf') ||
                byte3 < static_cast<unsigned char>('\x80') ||
                byte3 > static_cast<unsigned char>('\xbf') ||
                byte4 < static_cast<unsigned char>('\x80') ||
                byte4 > static_cast<unsigned char>('\xbf'))
            {
                return TfUtf8InvalidCodePoint.AsUInt32();
            }
        }
        else if (byte1 == static_cast<unsigned char>('\xf4'))
        {
            // byte2 must be in range 80..8F
            // byte3 must be in range 80..BF
            // byte4 must be in range 80..BF
            if (byte2 < static_cast<unsigned char>('\x80') ||
                byte2 > static_cast<unsigned char>('\x8f') ||
                byte3 < static_cast<unsigned char>('\x80') ||
                byte3 > static_cast<unsigned char>('\xbf') ||
                byte4 < static_cast<unsigned char>('\x80') ||
                byte4 > static_cast<unsigned char>('\xbf'))
            {
                return TfUtf8InvalidCodePoint.AsUInt32();
            }
        }
        else
        {
            // byte 1 is invalid
            return TfUtf8InvalidCodePoint.AsUInt32();
        }

        // code point is constructed from the last 3 bits of byte 1
        // and the last 6 bits of bytes 2, 3, and 4
        return ((byte1 & 0x7) << 18) + ((byte2 & 0x3f) << 12) +
               ((byte3 & 0x3f) << 6) + (byte4 & 0x3f);
    }
    return TfUtf8InvalidCodePoint.AsUInt32();
}

bool TfIsUtf8CodePointXidStart(uint32_t codePoint)
{
    return TfUnicodeGetXidStartFlagData().IsXidStartCodePoint(codePoint);
}

bool TfIsUtf8CodePointXidContinue(uint32_t codePoint)
{
    return TfUnicodeGetXidContinueFlagData().IsXidContinueCodePoint(codePoint);
}

PXR_NAMESPACE_CLOSE_SCOPE