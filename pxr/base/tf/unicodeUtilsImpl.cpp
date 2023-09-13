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
#include "pxr/base/tf/unicodeUtilsImpl.h"
#include "pxr/base/tf/unicodeCharacterClasses.h"

#include <algorithm>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

namespace TfUnicodeUtils {
namespace Impl {

    void AppendUTF8Char(uint32_t codePoint, std::string& result)
    {
        if (codePoint < 0x80)
        {
            // 1-byte UTF-8 encoding
            result.push_back(static_cast<char>(static_cast<unsigned char>(codePoint)));
        }
        else if (codePoint < 0x800)
        {
            // 2-byte UTF-8 encoding
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint >> 6) | 0xc0)));
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint & 0x3f) | 0x80)));
        }
        else if (codePoint < 0x10000)
        {
            // 3-byte UTF-8 encoding
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint >> 12) | 0xe0)));
            result.push_back(static_cast<char>(static_cast<unsigned char>(((codePoint >> 6) & 0x3f) | 0x80)));
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint & 0x3f) | 0x80)));
        }
        else
        {
            // 4-byte UTF-8 encoding
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint >> 18) | 0xf0)));
            result.push_back(static_cast<char>(static_cast<unsigned char>(((codePoint >> 12) & 0x3f) | 0x80)));
            result.push_back(static_cast<char>(static_cast<unsigned char>(((codePoint >> 6) & 0x3f) | 0x80)));
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint & 0x3f) | 0x80)));
        }
    }

    bool IsUTF8CharXIDStart(uint32_t codePoint)
    {
        return !(xidStartClass.find(codePoint) == xidStartClass.end() && 
            std::none_of(xidStartRangeClass.cbegin(), xidStartRangeClass.cend(),
            [codePoint](const auto& range) {
                return codePoint >= range.first && codePoint <= range.second;
            }));
    }

    bool IsUTF8CharXIDContinue(uint32_t codePoint)
    {
        bool not_in_start = (xidStartClass.find(codePoint) == xidStartClass.end() && 
            std::none_of(xidStartRangeClass.cbegin(), xidStartRangeClass.cend(),
            [codePoint](const auto& range) {
                return codePoint >= range.first && codePoint <= range.second;
            }));

        bool not_in_continue = (xidContinueClass.find(codePoint) == xidContinueClass.end() && 
            std::none_of(xidContinueRangeClass.cbegin(), xidContinueRangeClass.cend(),
            [codePoint](const auto& range) {
                return codePoint >= range.first && codePoint <= range.second;
            }));

        return !(not_in_start && not_in_continue);
    }

    bool IsValidUTF8Identifier(const std::string::const_iterator& sequenceStart, const std::string::const_iterator& end)
    {
        // at least one character
        if (sequenceStart == end)
        {
            return false;
        }

        // first character in the XID_Start character class
        TfUnicodeUtils::utf8_const_iterator utf8Iterator(sequenceStart, end);
        if (!IsUTF8CharXIDStart(*utf8Iterator))
        {
            // in this case, the character wasn't in either the start code point
            // singular set or the range of code points in the range sets
            return false;
        }

        // remaining characters
        utf8Iterator++;
        for (; utf8Iterator != end; utf8Iterator++)
        {
            if (!IsUTF8CharXIDContinue(*utf8Iterator))
            {
                return false;
            }
        }

        TF_AXIOM(utf8Iterator == end);

        return true;
    }

    std::string MakeValidUTF8Identifier(const std::string& identifier)
    {
        std::string result;

        // empty strings are always associated with the '_' identifier
        if (identifier.empty())
        {
            result.push_back('_');
            return result;
        }

        // maximum size is always the number of bytes in the UTF-8 encoded string
        // but if a character is invalid it will be replaced by a '_' character, which
        // may compress a e.g., 4-byte UTF-8 invalid character into a single valid 1-byte UTF-8 '_' character
        result.reserve(identifier.size());
        utf8_const_iterator iterator(identifier.begin(), identifier.end());

        // first UTF-8 character must be in XID_Start
        if (!IsUTF8CharXIDStart(*iterator))
        {
            result.push_back('_');
        }
        else
        {
            AppendUTF8Char(*iterator, result);
        }

        for (++iterator; iterator != identifier.end(); iterator++)
        {
            if (!IsUTF8CharXIDContinue(*iterator))
            {
                result.push_back('_');
            }
            else
            {
                AppendUTF8Char(*iterator, result);
            }
        }

        return result;
    }

} // end Impl
} // end TfUnicodeUtils

PXR_NAMESPACE_CLOSE_SCOPE