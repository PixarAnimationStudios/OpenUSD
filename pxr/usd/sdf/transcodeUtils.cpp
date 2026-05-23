//
// Copyright 2024 Pixar
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
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/unicodeUtils.h"
#include "pxr/usd/sdf/transcodeUtils.h"

#include <array>
#include <algorithm>
#include <tuple>
#include <vector>
#include <unordered_set>
#include <functional>
#include <limits>
#include <random>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// To represent values from 0 to 0x3D
using base62_t = unsigned char;

constexpr int BASE62 = 62;

constexpr char BOOTSTRING_DELIMITER = '_';

constexpr base62_t BOOTSTRING_THRESHOLD = 31;

// A bootstring prefix which is also a valid ASCII/XID start
constexpr std::string_view BOOTSTRING_PREFIX = "tn__";


// Wrapper for a base62 digit.
struct _Base62 {
    base62_t digit;

    _Base62(const base62_t digit)
    : digit(digit)
    {
    }

    _Base62(const TfUtf8CodePoint code)
    {
        const uint32_t value = code.AsUInt32();
        if (value >= '0' && value <= '9') {
            digit = value - '0';
        }
        else if (value >= 'A' && value <= 'Z') {
            digit = value - 'A' + 10;
        }
        else if (value >= 'a' && value <= 'z') {
            digit = value - 'a' + 36;
        }
        else {
            digit = BASE62;
        }
    }

    bool IsValid() const
    {
        return digit < BASE62;
    }
};

std::ostream&
operator<<(std::ostream& stream, const _Base62 code)
{
    TF_DEV_AXIOM(code.IsValid());

    base62_t digit = code.digit;
    if (digit <= 9) {
        digit += '0';
    }
    else if (digit <= 35) {
        digit += 'A' - 10;
    }
    else {
        digit += 'a' - 36;
    }
    stream << digit;
    return stream;
}

// See https://en.wikipedia.org/wiki/Fenwick_tree
struct _BinaryIndexedTree {
    std::vector<size_t> tree;
    size_t mostSignificantBit;

    _BinaryIndexedTree(const size_t n) 
        : tree(n + 1, 0)
        , mostSignificantBit(0)
    {
        size_t v = n + 1;
        while (v >>= 1)
        {
            ++mostSignificantBit;
        }
    }

    // The greatest power of two which can divides `value`. Note, this work only for positive values.
    size_t LeastSignificantBitSet(const size_t value) const
    {
        return (value & (0 - value));
    }

    // Increase index `i` by 1
    void Increase(const size_t i)
    {
        TF_DEV_AXIOM(i < tree.size() - 1);
        for (size_t idx = i + 1; idx < tree.size(); idx += LeastSignificantBitSet(idx))
        {
            ++tree[idx];
        }
    }

    // Decrease index `i` by 1
    void Decrease(const size_t i)
    {
        TF_DEV_AXIOM(i < tree.size() - 1);
        for (size_t idx = i + 1; idx < tree.size(); idx += LeastSignificantBitSet(idx))
        {
            TF_DEV_AXIOM(tree[idx] > 0);
            --tree[idx];
        }
    }

    // Increase all indices by 1. It is faster than calling N times `Increase`.
    // We use 1-based indexing to satisfy `LeastSignificantBitSet` function.
    void IncreaseAll()
    {
        for (size_t i = 0; i < tree.size() - 1; i++)
        {
            ++tree[i + 1];

            size_t idx = i + 1;
            idx += LeastSignificantBitSet(idx);
            if (idx < tree.size())
            {
                tree[idx] += tree[i + 1];
            }
        }
    }

    // Return the sum of all values from 0 to `i` (included).
    size_t Sum(const size_t i) const
    {
        TF_DEV_AXIOM(i < tree.size() - 1);
        size_t sum = 0;
        for (size_t idx = i + 1; idx > 0; idx -= LeastSignificantBitSet(idx))
        {
            sum += tree[idx];
        }
        return sum;
    }

    //! Reverse operation to `sum`, i.e. return the largest index whose sum is `sum`.
    size_t Lower(size_t sum) const
    {
        TF_DEV_AXIOM(sum < tree.size());
        size_t idx = 0;
        size_t bitmask = static_cast<size_t>(1) << mostSignificantBit;
        while (bitmask > 0)
        {
            const size_t current = idx | bitmask;
            bitmask >>= 1;
            // Note: multiple indices meet `tree[current] <= sum`
            if (current < tree.size() && tree[current] < sum + 1)
            {
                idx = current;
                sum -= tree[current];
            }
        }
        TF_DEV_AXIOM(sum == 0);
        return idx;
    }
};

// Equivalent to TfIsUtf8CodePointXidStart
bool
IsASCIIStart(const uint32_t value)
{
    return (
        (value >= 'A' && value <= 'Z') ||
        (value == '_') ||
        (value >= 'a' && value <= 'z')
    );
}

// Equivalent to TfIsUtf8CodePointXidContinue
bool
IsASCIIContinue(const uint32_t value)
{
    return (
        (value >= '0' && value <= '9') ||
        (value >= 'A' && value <= 'Z') ||
        (value == '_') ||
        (value >= 'a' && value <= 'z')
    );
}

// Encapsulates IsASCIIStart and TfIsUtf8CodePointXidStart
bool
IsStart(const TfUtf8CodePoint value, const SdfTranscodeFormat format)
{
    if (format == SdfTranscodeFormat::ASCII) 
    {
        return IsASCIIStart(value.AsUInt32());
    } 
    else if (format == SdfTranscodeFormat::UNICODE_XID) 
    {
        return value == TfUtf8CodePointFromAscii('_') || TfIsUtf8CodePointXidStart(value);
    }
    return false;    
}

// Encapsulates IsASCIIContinue and TfIsUtf8CodePointXidContinue
bool
IsContinue(const TfUtf8CodePoint value, const SdfTranscodeFormat format)
{
    if (format == SdfTranscodeFormat::ASCII) 
    {
        return IsASCIIContinue(value.AsUInt32());
    } 
    else if (format == SdfTranscodeFormat::UNICODE_XID) 
    {
        return TfIsUtf8CodePointXidContinue(value);
    }
    return false;
}

// Returns true if value is TfUtf8InvalidCodePoint
// TODO: Check in future for TfUtf8CodePointView::IsInvalid
bool
IsInvalid(const TfUtf8CodePoint value)
{
    return value == TfUtf8InvalidCodePoint;
}

// Encodes variable length integers `number` and writes them to ostringstream.
void
EncodeVariableLength(std::ostringstream& oss, uint64_t number)
{
    base62_t threshold = BOOTSTRING_THRESHOLD;
    while (number >= threshold) {
        const base62_t digit = threshold + (number - threshold) % (BASE62 - threshold);
        oss << _Base62(digit);
        number = (number - threshold) / (BASE62 - threshold);
    }
    // number < threshold
    oss << _Base62(static_cast<base62_t>(number));
}

// Decodes variable length integers starting at index.
// Returns false if:
// - while decoding we run out of indices.
// - the characters do not belong to base62.
std::optional<uint64_t>
DecodeVariableLength(TfUtf8CodePointIterator& it)
{
    uint64_t number = 0;
    uint64_t weight = 1;
    base62_t threshold = BOOTSTRING_THRESHOLD;
    while (true) {
        if (it == TfUtf8CodePointIterator::PastTheEndSentinel{}) {
            // Out of index.
            return std::nullopt;
        }
        const _Base62 character = _Base62(*it);
        if (!character.IsValid())
        {
            // Invalid character
            return std::nullopt;
        }
        const base62_t digit = character.digit;
        ++it;
        if (digit > (std::numeric_limits<uint64_t>::max() - number) / weight) {
            // Overflow
            return std::nullopt;
        }
        number += digit * weight;
        if (digit < threshold) {
            break;
        }
        if (weight > std::numeric_limits<uint64_t>::max() / (BASE62 - threshold)) {
            // Overflow
            return std::nullopt;
        }
        weight *= (BASE62 - threshold);
    }
    return number;
}

std::optional<std::string>
EncodeBootstring(
    const std::string_view inputString,
    const SdfTranscodeFormat format
)
{
    TfUtf8CodePointView inputView{inputString};
    if (std::any_of(inputView.begin(), inputView.EndAsIterator(), IsInvalid))
    {
        TF_RUNTIME_ERROR("Invalid UTF-8 input string found");
        return std::nullopt;
    }

    std::ostringstream oss;
    for (const TfUtf8CodePoint value : inputView)
    {
        if (IsContinue(value, format))
        {
            oss << value;
        }
    }
    if (oss.tellp() > 0)
    {
        oss << BOOTSTRING_DELIMITER;
    }

    _BinaryIndexedTree tree(std::distance(inputView.begin(), inputView.EndAsIterator()));
    std::vector< std::tuple<uint32_t, size_t> > extendedCodes;
    size_t encodedPoints = 0;
    size_t codePosition = 0;
    for (const TfUtf8CodePoint value : inputView)
    {
        if (IsContinue(value, format))
        {
            tree.Increase(codePosition);
            ++encodedPoints;
        }
        else
        {
            extendedCodes.emplace_back(value.AsUInt32(), codePosition);
        }
        ++codePosition;
    }
    std::sort(extendedCodes.begin(), extendedCodes.end());

    uint32_t prevCodePoint = 0;
    for (const auto& it: extendedCodes)
    {
        const uint32_t codePoint = std::get<0>(it);
        const size_t codePosition = std::get<1>(it);

        uint64_t delta = tree.Sum(codePosition);
        if (codePoint - prevCodePoint > (std::numeric_limits<uint64_t>::max() - delta) / (encodedPoints + 1))
        {
            TF_RUNTIME_ERROR("Numeric overflow while encoding");
            return std::nullopt;
        }
        delta += (codePoint - prevCodePoint) * (encodedPoints + 1);
        EncodeVariableLength(
            oss,
            delta
        );
        prevCodePoint = codePoint;

        tree.Increase(codePosition);
        ++encodedPoints;
    }

    return oss.str();
}

std::optional<std::string>
DecodeBootstring(const std::string_view inputString)
{
    TfUtf8CodePointView inputView{inputString};
    if (std::any_of(inputView.begin(), inputView.EndAsIterator(), IsInvalid))
    {
        TF_RUNTIME_ERROR("Invalid UTF-8 input string found");
        return std::nullopt;
    }

    std::vector<std::tuple<uint32_t, size_t>> values;

    const size_t delimiterPosition = inputString.rfind(BOOTSTRING_DELIMITER);
    if (delimiterPosition != std::string::npos)
    {
        size_t position = 0;
        TfUtf8CodePointView unencodedView{ inputString.substr(0, delimiterPosition) };
        for (const auto value : unencodedView)
        {
            values.emplace_back(value.AsUInt32(), position++);
        }
    }

    size_t decodedPoints = values.size();
    uint32_t codePoint = 0;
    TfUtf8CodePointView encodedView{ (delimiterPosition != std::string::npos)
        ? inputString.substr(delimiterPosition + 1)
        : inputString };
    for (auto it = encodedView.begin(); it != encodedView.end(); ) 
    {
        const std::optional<uint64_t> ret = DecodeVariableLength(it);
        if (!ret)
        {
            TF_RUNTIME_ERROR("Numeric overflow while decoding");
            return std::nullopt;
        }
        const uint64_t value = *ret;
        codePoint += (uint32_t)(value / (decodedPoints + 1));
        const size_t position = value % (decodedPoints + 1);

        ++decodedPoints;
        values.emplace_back(codePoint, position);
    }

    _BinaryIndexedTree tree(values.size());
    tree.IncreaseAll();
    std::vector<uint32_t> codePoints(values.size());
    for (auto it = values.rbegin(); it != values.rend(); ++it)
    {
        const uint64_t value = std::get<0>(*it);
        const size_t position = std::get<1>(*it);
        const size_t index = tree.Lower(position);
        codePoints[index] = value;
        tree.Decrease(index);
    }
    std::ostringstream oss;
    for (const auto codePoint : codePoints)
    {
        oss << TfUtf8CodePoint(codePoint);
    }
    return oss.str();
}

} // Anonymous

std::optional<std::string> 
SdfEncodeIdentifier(
    const std::string_view inputString,
    const SdfTranscodeFormat format
)
{
    std::optional<std::string> ret = EncodeBootstring(inputString, format);
    if (!ret)
    {
        return std::nullopt;
    }
    const std::string& output = *ret;
    if (output.substr(0, output.size() - 1) == inputString)
    {
        const auto it = TfUtf8CodePointView{inputString}.begin();
        if (IsStart(*it, format)) {
            return std::string(inputString);
        }
    }
    std::string result;
    result.append(BOOTSTRING_PREFIX);
    result.append(output);
    return result;
}

std::optional<std::string> 
SdfDecodeIdentifier(const std::string_view inputString)
{
    if (inputString.size() < BOOTSTRING_PREFIX.size())
    {
        return std::string(inputString);
    }
    if (inputString.substr(0, 4) == BOOTSTRING_PREFIX)
    {
        const std::string_view substr = inputString.substr(4);
        const std::optional<std::string> ret = DecodeBootstring(substr);
        if (!ret)
        {
            return std::nullopt;
        }
        return *ret;
    }
    return std::string(inputString);
}

PXR_NAMESPACE_CLOSE_SCOPE
