//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_UNICODE_UTILS_H
#define PXR_BASE_TF_UNICODE_UTILS_H

/// \file tf/unicodeUtils.h
/// \ingroup group_tf_String
/// Definitions of basic UTF-8 utilities in tf.

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnostic.h"

#include <ostream>
#include <string>
#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfUtf8CodePoint
/// \ingroup group_tf_String
///
/// Wrapper for a 32-bit code point value that can be encoded as UTF-8.
///
/// \code{.cpp}
/// // Stream operator overload encodes each code point as UTF-8.
/// std::stringstream s;
/// s << TfUtf8CodePoint(8747) << " " << TfUtf8CodePoint(120);
/// \endcode
/// A single `TfUtf8CodePoint` may be converted to a string using
/// `TfStringify` as well.
class TfUtf8CodePoint {
public:
    /// Code points that cannot be decoded or are outside of the valid range
    /// will be replaced with this value.
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

/// Constructs a TfUtf8CodePoint from an ASCII charcter (0-127).
constexpr TfUtf8CodePoint TfUtf8CodePointFromAscii(const char value)
{
    return static_cast<unsigned char>(value) < 128 ?
           TfUtf8CodePoint(static_cast<unsigned char>(value)) :
           TfUtf8InvalidCodePoint;
}

/// Defines an iterator over a UTF-8 encoded string that extracts unicode
/// code point values.
///
/// UTF-8 is a variable length encoding, meaning that one Unicode
/// code point can be encoded in UTF-8 as 1, 2, 3, or 4 bytes.  This
/// iterator takes care of consuming the valid UTF-8 bytes for a
/// code point while incrementing.
class TfUtf8CodePointIterator final {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = TfUtf8CodePoint;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = TfUtf8CodePoint;

    /// Model iteration ending when the underlying iterator's end condition
    /// has been met.
    class PastTheEndSentinel final {};

    /// Constructs an iterator that can read UTF-8 character sequences from
    /// the given starting string_view iterator \a it. \a end is used as a
    /// guard against reading byte sequences past the end of the source string.
    ///
    /// When working with views of substrings, \a end must not point to a
    /// continuation byte in a valid UTF-8 byte sequence to avoid decoding
    /// errors.
    TfUtf8CodePointIterator(
        const std::string_view::const_iterator& it,
        const std::string_view::const_iterator& end) : _it(it), _end(end) {
            TF_DEV_AXIOM(_it <= _end);
        }

    /// Retrieves the current UTF-8 character in the sequence as its Unicode
    /// code point value. Returns `TfUtf8InvalidCodePoint` when the
    /// byte sequence pointed to by the iterator cannot be decoded.
    ///
    /// A code point might be invalid because it's incorrectly encoded, exceeds
    /// the maximum allowed value, or is in the disallowed surrogate range.
    value_type operator* () const
    {
        return TfUtf8CodePoint{_GetCodePoint()};
    }

    /// Retrieves the wrapped string iterator.
    std::string_view::const_iterator GetBase() const
    {
        return this->_it;
    }

    /// Determines if two iterators are equal.
    /// This intentionally does not consider the end iterator to allow for
    /// comparison of iterators between different substring views of the
    /// same underlying string.
    bool operator== (const TfUtf8CodePointIterator& rhs) const
    {
        return (this->_it == rhs._it);
    }

    /// Determines if two iterators are unequal.
    /// This intentionally does not consider the end iterator to allow for
    /// comparison of iterators between different substring views of the
    /// same underlying string.
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
        // Note that in cases where the encoding is invalid, we move to the
        // next byte. This is necessary because otherwise the iterator would
        // never advance and the end condition of == iterator::end() would
        // never be satisfied. This means that we increment, even if the
        // encoding length is 0.
        ++_it;
        // Only continuation bytes will be consumed after the the first byte.
        // This avoids consumption of ASCII characters or other starting bytes.
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
    /// underlying `string_view`
    friend bool operator==(const TfUtf8CodePointIterator& lhs,
                           PastTheEndSentinel)
    {
        return lhs._IsPastTheEnd();
    }

    friend bool operator==(PastTheEndSentinel lhs,
                           const TfUtf8CodePointIterator& rhs)
    {
        return rhs == lhs;
    }

    friend bool operator!=(const TfUtf8CodePointIterator& lhs,
                           PastTheEndSentinel rhs)
    {
        return !(lhs == rhs);
    }
    friend bool operator!=(PastTheEndSentinel lhs,
                           const TfUtf8CodePointIterator& rhs)
    {
        return !(lhs == rhs);
    }

private:
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
};

/// \class TfUtf8CodePointView
/// \ingroup group_tf_String
///
/// Wrapper for a UTF-8 encoded `std::string_view` that can be iterated over
/// as code points instead of bytes.
///
/// Because of the variable length encoding, the `TfUtf8CodePointView` iterator is
/// a ForwardIterator and is read only.
///
/// \code{.cpp}
/// std::string value{"∫dx"};
/// for (const auto codePoint : TfUtf8CodePointView{value}) {
///     if (codePoint == TfUtf8InvalidCodePoint) {
///         TF_WARN("String cannot be decoded.");
///         break;
///     }
/// }
/// \endcode
///
/// The `TfUtf8CodePointView`'s sentinel `end()` is compatible with range
/// based for loops and the forthcoming STL ranges library; it avoids
/// triplicating the storage for the end iterator. `EndAsIterator()`
/// can be used for algorithms that require the begin and end iterators to be
/// of the same type but necessarily stores redundant copies of the endpoint.
///
/// \code{.cpp}
/// if (std::any_of(std::cbegin(codePointView), codePointView.EndAsIterator(),
///     [](const auto c) { return c == TfUtf8InvalidCodePoint; }))
/// {
///     TF_WARN("String cannot be decoded");
/// }
/// \endcode
class TfUtf8CodePointView final {
public:
    using const_iterator = TfUtf8CodePointIterator;

    TfUtf8CodePointView() = default;
    explicit TfUtf8CodePointView(const std::string_view& view) : _view(view) {}

    inline const_iterator begin() const
    {
        return const_iterator{std::cbegin(_view), std::cend(_view)};
    }

    /// The sentinel will compare as equal to any iterator at the end
    /// of the underlying `string_view`
    TfUtf8CodePointIterator::PastTheEndSentinel end() const
    {
        return TfUtf8CodePointIterator::PastTheEndSentinel{};
    }

    inline const_iterator cbegin() const
    {
        return begin();
    }

    /// The sentinel will compare as equal to any iterator at the end
    /// of the underlying `string_view`
    TfUtf8CodePointIterator::PastTheEndSentinel cend() const
    {
        return end();
    }

    /// Returns true if the underlying view is empty
    bool empty() const
    {
        return _view.empty();
    }

    /// Returns an iterator of the same type as `begin` that identifies the end
    /// of the string.
    ///
    /// As the end iterator is stored three times, this is slightly heavier
    /// than using the `PastTheEndSentinel` and should be avoided in performance
    /// critical code paths. It is provided for convenience when an algorithm
    /// restricts the iterators to have the same type.
    ///
    /// As C++20 ranges exposes more sentinel friendly algorithms, this can
    /// likely be deprecated in the future.
    inline const_iterator EndAsIterator() const
    {
        return const_iterator(std::cend(_view), std::cend(_view));
    }

private:
    std::string_view _view;
};

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

/// Determines whether the given Unicode \a codePoint is in the XID_Start
/// character class.
/// \overload
///
inline bool TfIsUtf8CodePointXidStart(const TfUtf8CodePoint codePoint)
{
    return TfIsUtf8CodePointXidStart(codePoint.AsUInt32());
}

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

/// Determines whether the given Unicode \a codePoint is in the XID_Continue
/// character class.
/// \overload
///
inline bool TfIsUtf8CodePointXidContinue(const TfUtf8CodePoint codePoint)
{
    return TfIsUtf8CodePointXidContinue(codePoint.AsUInt32());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_UNICODE_UTILS_H_
