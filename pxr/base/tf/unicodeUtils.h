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

#include <optional>
#include <string>
#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

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
///     if (codePoint == TfTfUtf8CodePointIterator::INVALID_CODE_POINT) {
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

    static constexpr uint32_t INVALID_CODE_POINT = 0xFFFD;

    /// Retrieves the next UTF-8 character in the sequence as its Unicode
    /// code point value. Returns INVALID_CODE_POINT when the byte sequence
    /// pointed to by the iterator cannot be decoded.
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
        // TODO: note that this isn't precisely conformant, as we
        // likely consumed an entire sequence when a subset would have
        // been invalid e.g. the byte sequence C2 41 would be detected
        // as a 2-byte  sequence, but it is invalid because of the
        // second byte signature by the standard, this should process
        // the C2 as invalid but consume 41 as a valid 1-byte UTF-8
        // character
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
    /// character. Invalid leading bytes will increment until the end
    /// of the range is reached.
    TfUtf8CodePointIterator& operator++ ()
    {
        // note that in cases where the encoding is invalid, we move to the
        // next byte this is necessary because otherwise the iterator would
        // never advanced and the end condition of == iterator::end() would
        // never be satisfied
        _EncodingLength encodingLength = _GetEncodingLength();
        std::advance(_it, (encodingLength != 0) ? encodingLength : 1);
        if (_IsPastTheEnd()) {
            _it = _end;
        }
        return *this;
    }

    /// Advances the iterator logically one UTF-8 character sequence in the
    /// string. The underlying string iterator will be advanced according
    /// to the variable length encoding of the next UTF-8 character.
    TfUtf8CodePointIterator operator++ (int)
    {
        // note that in cases where the encoding is invalid, we move to the
        // next byte this is necessary because otherwise the iterator would
        // never advanced and the end condition of == iterator::end() would
        // never be satisfied
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_UNICODE_UTILS_H_