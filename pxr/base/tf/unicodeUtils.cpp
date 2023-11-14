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

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/unicodeUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

uint32_t TfUtf8CodePointIterator::_GetCodePoint() const
{
    // determine what encoding length the character is
    _EncodingLength encodingLength = this->_GetEncodingLength();
    if (encodingLength > std::distance(_it, _end)) {
        // error condition, would read bytes past the end of the range
        return INVALID_CODE_POINT;
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
            return INVALID_CODE_POINT;
        }
        if (byte2 < static_cast<unsigned char>('\x80') ||
            byte2 > static_cast<unsigned char>('\xbf'))
        {
            return INVALID_CODE_POINT;
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
                return INVALID_CODE_POINT;
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
                return INVALID_CODE_POINT;
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
                return INVALID_CODE_POINT;
            }
        }
        else
        {
            // byte 1 invalid
            return INVALID_CODE_POINT;
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
                return INVALID_CODE_POINT;
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
                return INVALID_CODE_POINT;
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
                return INVALID_CODE_POINT;
            }
        }
        else
        {
            // byte 1 is invalid
            return INVALID_CODE_POINT;
        }

        // code point is constructed from the last 3 bits of byte 1
        // and the last 6 bits of bytes 2, 3, and 4
        return ((byte1 & 0x7) << 18) + ((byte2 & 0x3f) << 12) +
               ((byte3 & 0x3f) << 6) + (byte4 & 0x3f);
    }
    return INVALID_CODE_POINT;
}

PXR_NAMESPACE_CLOSE_SCOPE