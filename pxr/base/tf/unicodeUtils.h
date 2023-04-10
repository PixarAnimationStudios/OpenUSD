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
#ifndef PXR_BASE_TF_UNICODE_UTILS_H
#define PXR_BASE_TF_UNICODE_UTILS_H

/// \file tf/unicodeUtils.h
/// \ingroup group_tf_String

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include <string>
#include <iterator>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <utility>

constexpr uint32_t INVALID_CODE_POINT = 0xFFFD;

PXR_NAMESPACE_OPEN_SCOPE

/// \namespace TfUnicodeUtils
/// \ingroup group_tf_String
///
/// Contains utility methods for processing classes of Unicode characters
/// according to the Unicode standard.
///
/// Used by \ref page_tf_StringUtils to determine validity of identifiers and names.
///
namespace TfUnicodeUtils {

    // 21-bits are required to store the maximum Unicode code point value
    // so we use an unsigned integer to represent the value
    extern TF_API std::unordered_set<uint32_t> xidStartClass;
    extern TF_API std::unordered_set<uint32_t> xidContinueClass;
    extern TF_API std::vector<std::pair<uint32_t, uint32_t>> xidStartRangeClass;
    extern TF_API std::vector<std::pair<uint32_t, uint32_t>> xidContinueRangeClass;

    void AppendUTF8Char(uint32_t codePoint, std::string& result);

    /// Defines an iterator over a UTF-8 encoded string that extracts unicode code point values.
    ///
    /// UTF-8 is a variable length encoding, meaning that one Unicode character can be encoded in UTF-8 as
    /// 1, 2, 3, or 4 bytes.  This iterator takes care of iterating the necessary characters in a string
    /// and extracing the Unicode code point of each UTF-8 encoded character in the sequence.
    ///
    class utf8_const_iterator {

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = uint32_t;
        using difference_type = ptrdiff_t;
        using pointer = uint32_t*;
        using reference = uint32_t&;

    public:

        /// Constructs an iterator that can read UTF-8 character sequences from the given starting string iterator \a it.
        /// \a end is used as a guard against reading byte sequences past the end of the source string.
        ///
        explicit utf8_const_iterator(const std::string::const_iterator& it, const std::string::const_iterator& end) : _it(it), _end(end)
        {
        }

        /// Retrieves the next UTF-8 character in the sequence as its Unicode code point value.
        ///
        /// If during read of the UTF-8 character sequence the underlying string iterator would go beyond \a end
        /// defined at construction time, a std::out_of_range exception will be thrown.
        ///
        uint32_t operator* () const
        {
            std::string::const_iterator temp = this->_it;
            uint32_t codePoint = 0;
            if (!this->_AdvanceUTF8Character(temp, codePoint))
            {
                // this is an exception case, the next UTF8 character is invalid!
                // but we don't like exceptions in USD, so we'll signal this is bad
                // by setting the code point to 0xFFFD (this mostly happens
                // when a high / low private surrogate is used)
                // TODO: note that this isn't precisely conformant, as we likely
                // consumed an entire sequence when a subset would have been invalid
                // e.g. the byte sequence C2 41 would be detected as a 2-byte 
                // sequence, but it is invalid because of the second byte signature
                // by the standard, this should process the C2 as invalid but consume
                // 41 as a valid 1-byte UTF-8 character
                codePoint = INVALID_CODE_POINT;
            }

            return codePoint;
        }

        ///
        /// Retrieves the wrapped string iterator.
        ///
        std::string::const_iterator Wrapped() const
        {
            return this->_it;
        }

        ///
        /// Determines if the underlying string iterator is pointing to the position of \a rhs.
        ///
        bool operator== (const std::string::const_iterator& rhs) const
        {
            return (this->_it == rhs);
        }

        ///
        /// Determines if the underlying string iterator is not pointing to the position of \a rhs.
        ///
        bool operator!= (const std::string::const_iterator& rhs) const
        {
            return (this->_it != rhs);
        }

        ///
        /// Determines if two iterators are equal.
        ///
        bool operator== (const utf8_const_iterator& rhs) const
        {
            return ((this->_it == rhs._it) && (this->_end == rhs._end));
        }

        ///
        /// Determines if two iterators are unequal.
        ///
        bool operator!= (const utf8_const_iterator& rhs) const
        {
            return ((this->_it != rhs._it) || (this->_end != rhs._end));
        }

        ///
        /// Advances the iterator logically one UTF-8 character sequence in the string.
        /// The underlying string iterator will be advanced according to the variable length encoding
        /// of the next UTF-8 character.  If the encoded sequence is invalid, the iterator will not be advanced.
        ///
        utf8_const_iterator& operator++ ()
        {
            // note that in cases where the encoding is invalid, we move to the next byte
            // this is necessary because otherwise the iterator would never advanced and
            // the end condition of == iterator::end() would never be satisfied
            std::iterator_traits<std::string::const_iterator>::difference_type encodingLength = this->_GetEncodingLength();
            std::advance(this->_it, (encodingLength != 0) ? encodingLength : 1);

            return *this;
        }

        ///
        /// Advances the iterator logically one UTF-8 character sequence in the string.
        /// The underlying string iterator will be advanced according to the variable length encoding
        /// of the next UTF-8 character.  If the encoded sequence is invalid, the iterator will not be advanced.
        ///
        utf8_const_iterator operator++ (int)
        {
            // note that in cases where the encoding is invalid, we move to the next byte
            // this is necessary because otherwise the iterator would never advanced and
            // the end condition of == iterator::end() would never be satisfied
            utf8_const_iterator temp = *this;
            std::iterator_traits<std::string::const_iterator>::difference_type encodingLength = this->_GetEncodingLength();
            std::advance(this->_it, (encodingLength != 0) ? encodingLength : 1);

            return temp;
        }

    private:

        ///
        /// Retrieves the variable encoding length of the UTF-8 character currently pointed to by the iterator.
        /// This can be 1, 2, 3, or 4 depending on the encoding of the UTF-8 character.
        /// If the encoding cannot be determined, this method will return 0.
        ///
        inline std::iterator_traits<std::string::const_iterator>::difference_type _GetEncodingLength() const
        {
            // already at the end, no valid character sequence
            if (this->_it >= this->_end)
            {
                return 0;
            }

            // determine what encoding length the character is
            // 1-byte characters have a leading 0 sequence
            // 2-byte characters have a leading 110 sequence
            // 3-byte characters have a leading 1110 sequence
            // 4-byte characters have a leading 11110 sequence
            unsigned char x = static_cast<unsigned char>(*_it);
            if (x < 0x80)
            {
                return 1;
            }
            else if ((x >> 5) == 0x6)
            {
                return 2;
            }
            else if ((x >> 4) == 0xe)
            {
                return 3;
            }
            else if ((x >> 3) == 0x1e)
            {
                return 4;
            }
            else
            {
                // can't determine encoding, this is an error
                return 0;
            }
        }

        ///
        /// Retrieves the Unicode code point of the next character in the UTF-8 encoded sequence (defined by
        /// \a begin) and returns the value in \a codePoint.  This method will return \a true if the encoded
        /// sequence is valid.
        /// If the encoding is invalid, this method will return \a false and \a codePoint will be set to 0.
        ///
        inline bool _AdvanceUTF8Character(std::string::const_iterator& begin, uint32_t& codePoint) const
        {
            codePoint = 0;
            if (begin >= this->_end)
            {
                // error condition, we can't advance and the code point is invalid
                return false;
            }

            // determine what encoding length the character is
            std::iterator_traits<std::string::const_iterator>::difference_type encodingLength = this->_GetEncodingLength();
            if (encodingLength == 1)
            {
                codePoint = static_cast<uint32_t>(static_cast<unsigned char>(*begin));

                return true;
            }
            else if (encodingLength == 2)
            {
                if ((this->_end - begin) < encodingLength)
                {
                    return false;
                }

                unsigned char byte1 = static_cast<unsigned char>(*begin);
                unsigned char byte2 = static_cast<unsigned char>(*(++begin));

                // ensure the ranges we expect, or it's not a valid character
                if (byte1 < static_cast<unsigned char>('\xc2') || byte1 > static_cast<unsigned char>('\xdf'))
                {
                    return false;
                }
                if (byte2 < static_cast<unsigned char>('\x80') || byte2 > static_cast<unsigned char>('\xbf'))
                {
                    return false;
                }

                // the code point is constructed from the last 5 bits of byte1
                // and the last 6 bits of byte2
                codePoint = ((byte1 & 0x1f) << 6) + (byte2 & 0x3f);

                return true;
            }
            else if (encodingLength == 3)
            {
                if ((this->_end - begin) < encodingLength)
                {
                    return false;
                }

                unsigned char byte1 = static_cast<unsigned char>(*begin);
                unsigned char byte2 = static_cast<unsigned char>(*(++begin));
                unsigned char byte3 = static_cast<unsigned char>(*(++begin));

                // ensure the ranges we expect, or it's not a valid character
                if (byte1 == static_cast<unsigned char>('\xe0'))
                {
                    // byte2 must be in range A0..BF
                    // byte3 must be in range 80..BF
                    if (byte2 < static_cast<unsigned char>('\xa0') || byte2 > static_cast<unsigned char>('\xbf') ||
                        byte3 < static_cast<unsigned char>('\x80') || byte3 > static_cast<unsigned char>('\xbf'))
                    {
                        return false;
                    }
                }
                else if ((byte1 >= static_cast<unsigned char>('\xe1') && byte1 <= static_cast<unsigned char>('\xec')) ||
                            byte1 == static_cast<unsigned char>('\xee') ||
                            byte1 == static_cast<unsigned char>('\xef'))
                {
                    // byte2 must be in range 80..BF
                    // byte3 must be in range 80..BF
                    if (byte2 < static_cast<unsigned char>('\x80') || byte2 > static_cast<unsigned char>('\xbf') ||
                        byte3 < static_cast<unsigned char>('\x80') || byte3 > static_cast<unsigned char>('\xbf'))
                    {
                        return false;
                    }
                }
                else if (byte1 == static_cast<unsigned char>('\xed'))
                {
                    // byte2 must be in range 80..9F
                    // byte3 must be in range 80..BF
                    if (byte2 < static_cast<unsigned char>('\x80') || byte2 > static_cast<unsigned char>('\x9f') ||
                        byte3 < static_cast<unsigned char>('\x80') || byte3 > static_cast<unsigned char>('\xbf'))
                    {
                        return false;
                    }
                }
                else
                {
                    // byte 1 invalid
                    return false;
                }
                
                // code point is constructed from the last 4 bits of byte1
                // and the last 6 bits of bytes 2 and 3
                codePoint = ((byte1 & 0xf) << 12) + ((byte2 & 0x3f) << 6) + (byte3 & 0x3f);

                return true;
            }
            else if (encodingLength == 4)
            {
                if ((this->_end - begin) < encodingLength)
                {
                    return false;
                }

                unsigned char byte1 = static_cast<unsigned char>(*begin);
                unsigned char byte2 = static_cast<unsigned char>(*(++begin));
                unsigned char byte3 = static_cast<unsigned char>(*(++begin));
                unsigned char byte4 = static_cast<unsigned char>(*(++begin));

                if (byte1 == static_cast<unsigned char>('\xf0'))
                {
                    // byte2 must be in range 90..BF
                    // byte3 must be in range 80..BF
                    // byte4 must be in range 80..BF
                    if (byte2 < static_cast<unsigned char>('\x90') || byte2 > static_cast<unsigned char>('\xbf') ||
                        byte3 < static_cast<unsigned char>('\x80') || byte3 > static_cast<unsigned char>('\xbf') ||
                        byte4 < static_cast<unsigned char>('\x80') || byte4 > static_cast<unsigned char>('\xbf'))
                    {
                        return false;
                    }
                }
                else if (byte1 >= static_cast<unsigned char>('\xf1') && byte1 <= static_cast<unsigned char>('\xf3'))
                {
                    // byte2 must be in range 80..BF
                    // byte3 must be in range 80..BF
                    // byte4 must be in range 80..BF
                    if (byte2 < static_cast<unsigned char>('\x80') || byte2 > static_cast<unsigned char>('\xbf') ||
                        byte3 < static_cast<unsigned char>('\x80') || byte3 > static_cast<unsigned char>('\xbf') ||
                        byte4 < static_cast<unsigned char>('\x80') || byte4 > static_cast<unsigned char>('\xbf'))
                    {
                        return false;
                    }
                }
                else if (byte1 == static_cast<unsigned char>('\xf4'))
                {
                    // byte2 must be in range 80..8F
                    // byte3 must be in range 80..BF
                    // byte4 must be in range 80..BF
                    if (byte2 < static_cast<unsigned char>('\x80') || byte2 > static_cast<unsigned char>('\x8f') ||
                        byte3 < static_cast<unsigned char>('\x80') || byte3 > static_cast<unsigned char>('\xbf') ||
                        byte4 < static_cast<unsigned char>('\x80') || byte4 > static_cast<unsigned char>('\xbf'))
                    {
                        return false;
                    }
                }
                else
                {
                    // byte 1 is invalid
                    return false;
                }

                // code point is constructed from the last 3 bits of byte 1
                // and the last 6 bits of bytes 2, 3, and 4
                codePoint = ((byte1 & 0x7) << 18) + ((byte2 & 0x3f) << 12) + ((byte3 & 0x3f) << 6) + (byte4 & 0x3f);

                return true;
            }

            return false;
        }

        std::string::const_iterator _it;
        std::string::const_iterator _end;
    };

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
    inline bool IsValidUTF8Identifier(const std::string::const_iterator& sequenceStart, const std::string::const_iterator& end)
    {
        // at least one character
        if (sequenceStart == end)
        {
            return false;
        }

        // first character in the XID_Start character class
        utf8_const_iterator utf8Iterator(sequenceStart, end);
        uint32_t codePoint = *utf8Iterator;
        bool found = xidStartClass.find(codePoint) != xidStartClass.end();
        if (!found)
        {
            // not in the singular set, need to check the range sets
            for (size_t i = 0; i < xidStartRangeClass.size() && !found; i++)
            {
                found = (codePoint >= xidStartRangeClass[i].first && codePoint <= xidStartRangeClass[i].second);
            }
        }

        if (!found)
        {
            // wasn't in the XID start class single or range classes
            return false;
        }

        // remaining characters
        utf8Iterator++;
        for (; utf8Iterator != end; utf8Iterator++)
        {
            codePoint = *utf8Iterator;
            bool found = (xidStartClass.find(codePoint) != xidStartClass.end()) || (xidContinueClass.find(codePoint) != xidContinueClass.end());
            if (!found)
            {
                // not in the singular set, need to check the range sets
                for (size_t i = 0; i < xidStartRangeClass.size() && !found; i++)
                {
                    found = (codePoint >= xidStartRangeClass[i].first && codePoint <= xidStartRangeClass[i].second);
                }

                if (!found)
                {
                    for (size_t i = 0; i < xidContinueRangeClass.size() && !found; i++)
                    {
                        found = (codePoint >= xidContinueRangeClass[i].first && codePoint <= xidContinueRangeClass[i].second);
                    }
                }
            }

            if (!found)
            {
                // it's not in either the XID_Start class or the XID_Continue class so it's invalid
                return false;
            }
        }

        return (utf8Iterator == end);
    }

    /// Determines whether the UTF-8 encoded substring in the string starting at position \a sequenceStart and ending at position \a end is a
    /// valid Unicode name. Names are more permissive than identifier names in that all characters can be part of the XID_Continue
    /// character class, meaning that names could start with digits (or be all digits), etc.
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
    inline bool IsValidUTF8Name(const std::string::const_iterator& sequenceStart, const std::string::const_iterator& end)
    {
        // at least one character
        if (sequenceStart == end)
        {
            return false;
        }

        uint32_t codePoint;
        utf8_const_iterator utf8Iterator(sequenceStart, end);
        for (; utf8Iterator != end; utf8Iterator++)
        {
            codePoint = *utf8Iterator;
            bool found = (xidStartClass.find(codePoint) != xidStartClass.end()) || (xidContinueClass.find(codePoint) != xidContinueClass.end());
            if (!found)
            {
                // not in the singular set, need to check the range sets
                for (size_t i = 0; i < xidStartRangeClass.size() && !found; i++)
                {
                    found = (codePoint >= xidStartRangeClass[i].first && codePoint <= xidStartRangeClass[i].second);
                }

                if (!found)
                {
                    for (size_t i = 0; i < xidContinueRangeClass.size() && !found; i++)
                    {
                        found = (codePoint >= xidContinueRangeClass[i].first && codePoint <= xidContinueRangeClass[i].second);
                    }
                }
            }

            if (!found)
            {
                // it's not in either the XID_Start class or the XID_Continue class so it's invalid
                return false;
            }
        }

        return (utf8Iterator == end);
    }

    /// Determines whether a UTF-8 character in \a identifier starting at position \a sequenceStart is part of the XID_Start character class.
    ///
    /// UTF-8 characters are variable encoded, so \a sequenceStart defines the first byte in the UTF8 character sequence.
    /// This method will advance \a sequenceStart iterator to the first byte of the next UTF8 character and evaluate
    /// if the UTF8 character it read is part of the XID_Start character class.
    ///
    /// The XID_Start class of characters are derived from the Unicode General_Category of uppercase letters, lowercase letters, titlecase letters,
    /// modifier letters, other letters, letters numbers, plus Other_ID_Start, minus Pattern_Syntax and Pattern_White_Space code points.
    /// That is, the character must have a category of Lu | Ll | Lt | Lm | Lo | Nl | '_'
    ///
    inline bool IsUTF8CharXIDStart(const std::string& identifier, const std::string::const_iterator& sequenceStart)
    {
        // extract the unicode code point value from the UTF-8 encoding
        utf8_const_iterator utf8Iterator(sequenceStart, identifier.end());

        // check to see whether the code point is in any of the valid singular sets
        uint32_t codePoint = *utf8Iterator;
        bool found = xidStartClass.find(codePoint) != xidStartClass.end();
        if (!found)
        {
            // not in the singular set, need to check the range sets
            for (size_t i = 0; i < xidStartRangeClass.size() && !found; i++)
            {
                found = (codePoint >= xidStartRangeClass[i].first && codePoint <= xidStartRangeClass[i].second);
            }
        }

        return found;
    }

    /// Determines whether a UTF-8 character in \a identifier starting at position \a sequenceStart is part of the XID_Continue character class.
    ///
    /// UTF8 characters are variable encoded, so \a sequenceStart defines the first byte in the UTF8 character sequence.
    /// This method will advance \a sequenceStart iterator to the first byte of the next UTF8 character and evaluate
    /// if the UTF8 character it read is part of the XID_Continue character class.
    ///
    /// The XID_Continue class of characters include those in XID_Start plus characters having the Unicode General Category of nonspacing marks,
    /// spacing combining marks, decimal number, and connector punctuation.
    /// That is, the character must have a category of XID_Start | Nd | Mn | Mc | Pc
    ///
    inline bool IsUTF8CharXIDContinue(const std::string& identifier, const std::string::const_iterator& sequenceStart)
    {
        // extract the unicode code point value from the UTF-8 encoding
        utf8_const_iterator utf8Iterator(sequenceStart, identifier.end());

        // check to see whether the code point is in any of the valid sets
        uint32_t codePoint = *utf8Iterator;
        bool found = (xidStartClass.find(codePoint) != xidStartClass.end()) || (xidContinueClass.find(codePoint) != xidContinueClass.end());
        if (!found)
        {
            // not in the singular set, need to check the range sets
            for (size_t i = 0; i < xidStartRangeClass.size() && !found; i++)
            {
                found = (codePoint >= xidStartRangeClass[i].first && codePoint <= xidStartRangeClass[i].second);
            }

            if (!found)
            {
                for (size_t i = 0; i < xidContinueRangeClass.size() && !found; i++)
                {
                    found = (codePoint >= xidContinueRangeClass[i].first && codePoint <= xidContinueRangeClass[i].second);
                }
            }
        }
        
        return found;
    }

    ///
    /// Constructs a valid result identifier from \a identifier.
    /// If \a identifier is already valid, the return value of this method should be value stored in \a identifier.
    ///
    /// An identifier is valid according to the rules associated with \a IsValidUTF8Identifier.
    ///
    std::string TF_API MakeValidUTF8Identifier(const std::string& identifier);

    ///
    /// Constructs a valid result name from \a name.
    /// If \a name is already valid, then return value of this method should be value stored in \a name.
    ///
    /// A name is valid according to the rules associated with \a IsValidUTF8Name.
    ///
    std::string TF_API MakeValidUTF8Name(const std::string& name);

} // end namespace TfUnicodeUtils

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_UNICODE_UTILS_H