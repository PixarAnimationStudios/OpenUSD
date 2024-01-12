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
#include "pxr/base/tf/diagnostic.h"

#include <ostream>
#include <string>
#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

/// Wrapper for a code point value that can be encoded as UTF-8
class TfUtf8CodePoint {
public:
    /// Code points that cannot be decoded or outside of the valid range are
    /// may be replaced with this value.
    static constexpr uint32_t ReplacementValue = 0xFFFD;

    /// Values higher than this will be replaced with the replacement
    /// code point.
    static constexpr uint32_t MaximumValue = 0x10FFFF;

    /// Values in this range (inclusive) cannot be constructed and will be
    /// replaced by the replacement code point.
    static constexpr std::pair<uint32_t, uint32_t>
    SurrogateRange = {0xD800, 0xDFFF};

    /// Construct a code point initialized to the replacement value
    constexpr TfUtf8CodePoint() = default;

    /// Construct a UTF-8 valued code point, constrained by the maximum value
    /// and surrogate range.
    constexpr explicit TfUtf8CodePoint(uint32_t value) :
        _value(((value <= MaximumValue) &&
                ((value < SurrogateRange.first) ||
                 (value > SurrogateRange.second))) ?
               value : ReplacementValue) {}

    constexpr uint32_t AsUInt32() const { return _value; }

    friend constexpr bool operator==(const TfUtf8CodePoint left,
                                     const TfUtf8CodePoint right) {
        return left._value == right._value;
    }
    friend constexpr bool operator!=(const TfUtf8CodePoint left,
                                     const TfUtf8CodePoint right) {
        return left._value != right._value;
    }

private:
    uint32_t _value{ReplacementValue};
};

TF_API std::ostream& operator<<(std::ostream&, const TfUtf8CodePoint);

/// The replacement code point can be used to signal that a code point could
/// not be decoded and needed to be replaced.
constexpr TfUtf8CodePoint TfUtf8InvalidCodePoint{
    TfUtf8CodePoint::ReplacementValue};

class TfUtf8CodePointIterator;

/// Wrapper for a UTF-8 encoded `std::string_view` that can be iterated over
/// as code points instead of bytes.
///
/// Because of the variable length encoding, the `Utf8StringView` iterator is
/// a ForwardIterator and is read only.
///
/// \code{.cpp}
/// std::string value{"∫dx"};
/// TfUtf8CodePointView view{value};
/// for (const uint32_t codePoint : view) {
///     if (codePoint == TfUtf8InvalidCodePoint.AsUInt32()) {
///         TF_WARN("String cannot be decoded.");
///     }
/// }
/// (The TfUtf8CodePointView's sentinel end() will make it compatible with
///  the STL ranges library).
/// \endcode
class TfUtf8CodePointView final {
public:
    using const_iterator = TfUtf8CodePointIterator;

    /// Model iteration ending when the underlying string_view's end iterator
    /// has been exceeded. This guards against strings whose variable length
    /// encoding pushes the iterator past the end of the underlying
    /// string_view.
    class PastTheEndSentinel final {};

    TfUtf8CodePointView() = default;
    explicit TfUtf8CodePointView(const std::string_view& view) : _view(view) {}

    inline const_iterator begin() const;

    /// The sentinel will compare as equal with any iterator at or past the end
    /// of the underlying string_view
    PastTheEndSentinel end() const
    {
        return PastTheEndSentinel{};
    }

    inline const_iterator cbegin() const;

    /// The out of range sentinel will compare as equal with any iterator
    /// at or past the end of the underlying string_view's
    PastTheEndSentinel cend() const
    {
        return end();
    }

    /// Returns true if the underlying view is empty
    bool empty() const
    {
        return _view.empty();
    }

    /// Returns an iterator of the same type as begin that identifies the end
    /// of the string.
    ///
    /// As the end iterator is stored three times, this is slightly heavier
    /// than using the PastTheEndSentinel and should be avoided in performance
    /// critical code paths. It is provided for convenience when an algorithm
    /// restricts the iterators to have the same type.
    ///
    /// As C++20 ranges exposes more sentinel friendly algorithms, this can
    /// likely be deprecated in the future.
    inline const_iterator EndAsIterator() const;

private:
    std::string_view _view;
};

/// Defines an iterator over a UTF-8 encoded string that extracts unicode
/// code point values.
///
/// UTF-8 is a variable length encoding, meaning that one Unicode
/// character can be encoded in UTF-8 as 1, 2, 3, or 4 bytes.  This
/// iterator takes care of iterating the necessary characters in a string
/// and extracing the Unicode code point of each UTF-8 encoded character
/// in the sequence.
class TfUtf8CodePointIterator final {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = uint32_t;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = uint32_t;

    /// Retrieves the next UTF-8 character in the sequence as its Unicode
    /// code point value. Returns TfUtf8InvalidCodePoint.AsUInt32() when the
    /// byte sequence pointed to by the iterator cannot be decoded.
    ///
    /// If during read of the UTF-8 character sequence the underlying
    /// string iterator would go beyond \a end defined at construction
    /// time, a std::out_of_range exception will be thrown.
    uint32_t operator* () const
    {
        // If the current UTF-8 character is invalid, instead of
        // throwing an exception, _GetCodePoint signals this is
        // bad by setting the code point to 0xFFFD (this mostly happens
        // when a high / low private surrogate is used)
        return _GetCodePoint();
    }

    /// Retrieves the wrapped string iterator.
    std::string_view::const_iterator GetBase() const
    {
        return this->_it;
    }

    /// Determines if two iterators are equal.
    /// This intentionally does not consider the end iterator to allow for
    /// comparison of iterators between substring views.
    bool operator== (const TfUtf8CodePointIterator& rhs) const
    {
        return (this->_it == rhs._it);
    }

    /// Determines if two iterators are unequal.
    /// This intentionally does not consider the end iterator to allow for
    /// comparison of iterators between substring views.
    bool operator!= (const TfUtf8CodePointIterator& rhs) const
    {
        return (this->_it != rhs._it);
    }

    /// Advances the iterator logically one UTF-8 character sequence in
    /// the string. The underlying string iterator will be advanced
    /// according to the variable length encoding of the next UTF-8
    /// character, but will never consume non-continuation bytes after
    /// the current one.
    TfUtf8CodePointIterator& operator++ ()
    {
        // The increment operator should never be called if it's past
        // the end. The user is expected to have already checked this
        // condition.
        TF_DEV_AXIOM(!_IsPastTheEnd());
        _EncodingLength increment = _GetEncodingLength();
        // note that in cases where the encoding is invalid, we move to the
        // next byte this is necessary because otherwise the iterator would
        // never advanced and the end condition of == iterator::end() would
        // never be satisfied. This means that we increment, even if the
        // encoding length is 0.
        ++_it;
        // Only continuation bytes will be consumed after the the first byte.
        // This avoid consumption of ASCII characters or other starting bytes.
        auto isContinuation = [](const char c) {
            const auto uc = static_cast<unsigned char>(c);
            return (uc >= static_cast<unsigned char>('\x80')) &&
                   (uc < static_cast<unsigned char>('\xc0'));
        };
        while ((increment > 1) && !_IsPastTheEnd() && isContinuation(*_it)) {
            ++_it;
            --increment;
        }
        return *this;
    }

    /// Advances the iterator logically one UTF-8 character sequence in
    /// the string. The underlying string iterator will be advanced
    /// according to the variable length encoding of the next UTF-8
    /// character, but will never consume non-continuation bytes after
    /// the current one.
    TfUtf8CodePointIterator operator++ (int)
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    /// Checks if the `lhs` iterator is at or past the end for the
    /// underlying string_view
    friend bool operator==(const TfUtf8CodePointIterator& lhs,
                           TfUtf8CodePointView::PastTheEndSentinel)
    {
        return lhs._IsPastTheEnd();
    }

    friend bool operator==(TfUtf8CodePointView::PastTheEndSentinel lhs,
                           const TfUtf8CodePointIterator& rhs)
    {
        return rhs == lhs;
    }

    friend bool operator!=(const TfUtf8CodePointIterator& lhs,
                           TfUtf8CodePointView::PastTheEndSentinel rhs)
    {
        return !(lhs == rhs);
    }
    friend bool operator!=(TfUtf8CodePointView::PastTheEndSentinel lhs,
                           TfUtf8CodePointIterator rhs)
    {
        return !(lhs == rhs);
    }

private:
    // Constructs an iterator that can read UTF-8 character sequences from
    // the given starting string_view iterator \a it. \a end is used as a
    // guard against reading byte sequences past the end of the source string.
    // \a end must not be in the middle of a UTF-8 character sequence.
    TfUtf8CodePointIterator(
        const std::string_view::const_iterator& it,
        const std::string_view::const_iterator& end) : _it(it), _end(end) {
            TF_DEV_AXIOM(_it <= _end);
        }

    using _EncodingLength = unsigned char;

    // Retrieves the variable encoding length of the UTF-8 character
    // currently pointed to by the iterator. This can be 1, 2, 3, or 4
    // depending on the encoding of the UTF-8 character. If the encoding
    // cannot be determined, this method will return 0.
    _EncodingLength _GetEncodingLength() const
    {
        // already at the end, no valid character sequence
        if (_IsPastTheEnd())
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
        else if ((x >= 0xc0) && (x < 0xe0))
        {
            return 2;
        }
        else if ((x >= 0xe0) && (x < 0xf0))
        {
            return 3;
        }
        else if ((x >= 0xf0) && (x < 0xf8))
        {
            return 4;
        }
        else
        {
            // can't determine encoding, this is an error
            return 0;
        }
    }

    // Retrieves the Unicode code point of the next character in the UTF-8
    // encoded sequence (defined by \a begin) and returns the value in
    // \a codePoint. This method will return \a true if the encoded
    // sequence is valid. If the encoding is invalid, this method will
    // return \a false and \a codePoint will be set to 0.
    TF_API uint32_t _GetCodePoint() const;

    // Returns true if the iterator at or past the end and can no longer be
    // dereferenced.
    bool _IsPastTheEnd() const
    {
        return _it >= _end;
    }

    std::string_view::const_iterator _it;
    std::string_view::const_iterator _end;

    friend class TfUtf8CodePointView;
};

inline TfUtf8CodePointView::const_iterator TfUtf8CodePointView::begin() const
{
    return TfUtf8CodePointView::const_iterator{
        std::cbegin(_view), std::cend(_view)};
}

inline TfUtf8CodePointView::const_iterator TfUtf8CodePointView::cbegin() const
{
    return begin();
}

inline TfUtf8CodePointView::const_iterator
TfUtf8CodePointView::EndAsIterator() const
{
    return const_iterator(std::cend(_view), std::cend(_view));
}

/// Determines whether the given Unicode \a codePoint is in the XID_Start
/// character class.
///
/// The XID_Start class of characters are derived from the Unicode 
/// General_Category of uppercase letters, lowercase letters, titlecase
/// letters, modifier letters, other letters, letters numbers, plus
/// Other_ID_Start, minus Pattern_Syntax and Pattern_White_Space code points.
/// That is, the character must have a category of Lu | Ll | Lt | Lm | Lo | Nl
///
TF_API
bool TfIsUtf8CodePointXidStart(uint32_t codePoint);

/// Determines whether the given Unicode \a codePoint is in the XID_Continue
/// character class.
///
/// The XID_Continue class of characters include those in XID_Start plus
/// characters having the Unicode General Category of nonspacing marks,
/// spacing combining marks, decimal number, and connector punctuation.
/// That is, the character must have a category of 
/// XID_Start | Nd | Mn | Mc | Pc
///
TF_API
bool TfIsUtf8CodePointXidContinue(uint32_t codePoint);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_UNICODE_UTILS_H_
