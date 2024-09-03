//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/compressedBits.h"
#include "pxr/base/tf/bits.h"

#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/stringUtils.h"

#include <array>

PXR_NAMESPACE_OPEN_SCOPE

TfCompressedBits::TfCompressedBits(const TfBits &bits) :
    _num(bits.GetSize())
{
    if (bits.GetSize() == 0) {
        _runningBit = 0;
        _platforms.PushBack(0);
        return;
    }

    bool set = bits.IsSet(0);
    _runningBit = set ? 1 : 0;

    size_t i = 0;
    while (i < bits.GetSize()) {
        size_t next = set ? 
            bits.FindNextUnset(i + 1) : 
            bits.FindNextSet(i + 1);

        _platforms.PushBack(next - i);

        set = !set;
        i = next;
    }
}

size_t
TfCompressedBits::GetHash() const
{
    if (_num == 0) {
        return 0;
    }

    // Hash the running bit and number of platforms.
    uint64_t seed = TfHash::Combine(
        _runningBit,
        _platforms.GetNum());

    // Hash all the platform data.
    return ArchHash64(
        (const char*)&(_platforms[0]), 
        _platforms.GetNum() * sizeof(_WordType), 
        seed);
}

std::string
TfCompressedBits::GetAsStringLeftToRight() const
{
    std::string res;

    uint8_t bit = _runningBit;
    for (size_t i = 0; i < _platforms.GetNum(); ++i) {
        for (size_t j = 0; j < _platforms[i]; ++j) {
            res.push_back('0' + bit);
        }
        bit = 1 - bit;
    }

    return res;
}

std::string
TfCompressedBits::GetAsStringRightToLeft() const
{
    std::string res;

    uint8_t bit = _runningBit;
    if ((_platforms.GetNum() & 1) == 0) {
        bit = 1 - bit;
    }

    for (int i = _platforms.GetNum() - 1; i >= 0; --i) {
        for (size_t j = 0; j < _platforms[i]; ++j) {
            res.push_back('0' + bit);
        }
        bit = 1 - bit;
    }

    return res;
}

std::string
TfCompressedBits::GetAsRLEString() const
{
    std::string res;

    // If the length of the mask is <= 4 bits we just print them left to
    // right.  This makes a lot of the simple unit tests much easier to 
    // read.
    if (_num == 0) {
        return res;
    } else if (_num <= 4) {
        return GetAsStringLeftToRight();
    }
    
    uint8_t bit = _runningBit;
    res = TfIntToString(bit) + "x" + TfIntToString(_platforms[0]);
    bit = 1 - bit;

    for (size_t i = 1; i < _platforms.GetNum(); ++i) {
        res.push_back('-');
        res += TfIntToString(bit);
        res.push_back('x');
        res += TfIntToString(_platforms[i]);
        bit = 1 - bit;
    }

    return res;
}

static bool
_IsWhiteSpace(const char c)
{
    // These characters are considered whitespace in the string representation
    // of a compressed bitset.
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static std::vector<uint32_t>
_TokenizeRLEString(const std::string &source)
{
    // There are two types of token delimiters, and we toggle between the two.
    // We first expect a 'x' delimiter, followed by a '-' delimiter, followed
    // by another 'x', and so forth.
    const std::array<char, 2> delimiters = { 'x', '-' };
    uint32_t nextDelimiterIdx = 0;

    // The resulting tokens. A platform is comprised of two tokens, a bit value
    // (either zero or one), followed by a platform length.
    std::vector<uint32_t> tokens(1, 0);

    // Iterate over the source string and build a vector of tokens for the
    // platforms representing the bitset.
    for (const char c : source) {

        // Digits encode the integer value of the current token.
        if (c >= '0' && c <= '9') {
            const uint32_t digit = c - '0';
            tokens.back() = tokens.back() * 10 + digit;
        }

        // Whitespace is ignored entirely.
        else if (_IsWhiteSpace(c)) {
            continue;
        }

        // Delimiters constitute the beginning of a new token, but the next
        // expected delimiter is toggled between 'x' and '-'.
        else if (c == delimiters[nextDelimiterIdx]) {
            nextDelimiterIdx = (nextDelimiterIdx + 1) % delimiters.size();
            tokens.push_back(0);
        }

        // Reset the resulting vector of tokens and break the loop, if an
        // unexpected character is encountered. This could be a non-digit, non-
        // whitespace character, or an unexpected delimiter.
        else {
            tokens.clear();
            break;
        }
    }

    return tokens;
}

static TfCompressedBits
_FromRLETokens(const std::vector<uint32_t> &tokens)
{
    // The number of tokens must be even, because each platform is comprised of
    // two tokens: A bit value (zero or one), and a platform length.
    if (tokens.size() & 1) {
        return TfCompressedBits();
    }

    // Iterate over the tokens in pairs, extract the bit value and platform
    // length, and append a new platform to the resulting bitset.
    TfCompressedBits result;
    for (size_t i = 0; i < tokens.size(); i += 2) {
        const uint32_t bit = tokens[i];
        const uint32_t length = tokens[i + 1];

        // In order to be a valid representation, the bit value must be zero or
        // one, and the platform length must be greater than 0.
        if (bit > 1 || length == 0) {
            return TfCompressedBits();
        }

        result.Append(length, bit);
    }

    return result;
}

static TfCompressedBits
_FromBinaryRepresentation(const std::string &source)
{
    TfCompressedBits result;

    // Iterate over the string and treat it as a binary representation, i.e. a
    // string of zeros and ones.
    for (const char c : source) {

        // Zeros and ones will be appended to the bitset.
        if (c == '0' || c == '1') {
            const bool bit = c - '0';
            result.Append(1, bit);
        }

        // White space will be ignored entirely.
        else if (_IsWhiteSpace(c)) {
            continue;
        }

        // Any other character is unexpected and constitutes an invalid
        // representation.
        else {
            return TfCompressedBits();
        }
    }

    return result;
}

TfCompressedBits
TfCompressedBits::FromString(const std::string &source)
{
    TfCompressedBits result;

    // Assume the string is a RLE representation of the bits. Let's tokenize it
    // (i.e. interleaved pairs of platform value bits, and platform lengths.)
    std::vector<uint32_t> tokens = _TokenizeRLEString(source);

    // If no tokens have been found, the source string is considered an empty
    // representation.
    if (tokens.empty()) {
        return result;
    }

    // Build a compressed bitset from the RLE tokens.
    result = _FromRLETokens(tokens);
    
    // If this returns an empty bitset, maybe the string is encoded as a binary
    // representation, i.e. a string of zeros and ones.
    if (result.GetSize() == 0) {
        result = _FromBinaryRepresentation(source);
    }

    // Return the result from the conversion, or an empty representation if
    // the conversion failed, due to an invalid string representation.
    return result;
}

void
TfCompressedBits::Decompress(TfBits *bits) const
{
    bits->Resize(_num);
    bits->ClearAll();

    size_t bitIndex = 0;
    bool bitValue = _runningBit == 1;
    for (size_t i = 0; i < _platforms.GetNum(); ++i) {
        _WordType numBits = _platforms[i];
        for (_WordType j = 0; j < numBits; ++j) {
            bits->Assign(bitIndex, bitValue);
            ++bitIndex;
        }
        bitValue = !bitValue;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
