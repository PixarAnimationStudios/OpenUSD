//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/tf.h"

#include "pxr/base/arch/hash.h"

#include <bitset>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

// XXX:optimization Instead of always leaving the trailing bits to zero, we
//                  could let the first trailing bit (we might need to allocate
//                  one more word due to this), be 1.  This "guard bit" can
//                  then be used to make FindNextSet() faster.

// Note that the 64-bit array holding the bits can have unused bits at the end.
// By definition, these bits always needs to be kept at 0.

size_t
TfBits::_CountNumSet() const
{
    // Accumulate number of set bits in a local variable (this makes the
    // compiler generate better code since it does not have to repeatedly
    // read/store the contents of a member variable).
    size_t numSet = 0;

    // Limit the range of words we're looking at.
    size_t firstSet = GetFirstSet();
    size_t lastSet  = GetLastSet();
    size_t offset   = firstSet >> 6;
    size_t numWords = (firstSet < _num) ? (lastSet >> 6) + 1 - offset : 0;

    uint64_t *p = _bits + offset;

    // Loop over all words in the range.
    for(size_t n=numWords; n>0; n--)
    {
        // On both gcc and clang, with the appropriate instruction set
        // enabled, bitset::count compiles down to popcntq.
        numSet += std::bitset<64>(*p++).count();
    }

    return numSet;
}

size_t      
TfBits::_FindNextSet(size_t index, size_t startBit) const
{
    for(size_t w=index>>6; w<_numWords; w++)
    {
        if (uint64_t bits = _bits[w])
            for(size_t i=startBit; i<64; i++)
                if (bits & (UINT64_C(1) << i))
                {
                    // Note: Since we round up the # bits to fit an 
                    //       integer # of words, we need to check if we 
                    //       read too far...

                    size_t bit = i + (w << 6);

                    return (bit >= _num) ? _num : bit;
                }

        startBit = 0;
    }

    return _num;
}

size_t      
TfBits::_FindPrevSet(size_t index, size_t startBit) const
{
    for(int w=index>>6; w>=0; w--)
    {
        if (uint64_t bits = _bits[w])
            for(int i=startBit; i>=0; i--)
                if (bits & (UINT64_C(1) << i))
                {
                    // Note: Since we round up the # bits to fit an 
                    //       integer # of words, we need to check if we 
                    //       read too far...

                    return i + (w << 6);
                }

        startBit = 63;
    }

    return _num;
}

size_t    
TfBits::_FindNextUnset(size_t index, size_t startBit) const
{
    for(size_t w=index>>6; w<_numWords; w++)
    {
        // Note: We're operating on the flipped bits here...
        if (uint64_t bits = ~_bits[w])
            for(size_t i=startBit; i<64; i++)
                if (bits & (UINT64_C(1) << i))
                {
                    // Note: Since we round up the # bits to fit an 
                    //       integer # of words, we need to check if we 
                    //       read too far...

                    size_t bit = i + (w << 6);

                    return (bit >= _num) ? _num : bit;
                }

        startBit = 0;
    }

    return _num;
}

size_t
TfBits::GetHash() const
{
    size_t firstSet = GetFirstSet();
    size_t lastSet  = GetLastSet();
    size_t offset   = firstSet >> 6;
    size_t numWords = (lastSet >> 6) + 1 - offset;

    uint64_t *p0 = _bits + offset;

    if (firstSet == _num) {
        return firstSet;
    }

    // Use firstSet as the seed.
    return ArchHash((const char *)p0, numWords * sizeof(uint64_t), firstSet);
}

bool
TfBits::operator==(const TfBits &rhs) const
{
    // Early bail out.
    if (this == &rhs) 
        return true;

    // If the number of bits is different, they can't be equal.
    if (_num != rhs._num)
        return false;

    // XXX:optimization: Faster to always cache _numSet?

    // Check if we can do an early out based on the number of set bits.
    const size_t numSet = _numSet.Load();
    if (numSet != size_t(-1)) {
        const size_t rhsNumSet = rhs._numSet.Load();
        if (rhsNumSet != size_t(-1)) {

            // If the number of set bits is different, they can't be equal.
            if (numSet != rhsNumSet)
                return false;

            // Order doesn't matter if all bits are set or cleared and the # of
            // set bits is the same...
            if (numSet == 0 || numSet == _num)
                return true;
        }
    }

    // Limit the comparison to where we have bits set.
    size_t firstSet    = GetFirstSet();
    size_t lastSet     = GetLastSet();
    size_t rhsFirstSet = rhs.GetFirstSet();
    size_t rhsLastSet  = rhs.GetLastSet();

    if (firstSet != rhsFirstSet || lastSet != rhsLastSet)
        return false;

    // Are any of the sets empty?
    if (firstSet == _num || rhsFirstSet == _num)
        return firstSet == rhsFirstSet;

    size_t offset   = firstSet >> 6;
    size_t numWords = (lastSet >> 6) + 1 - offset;

    // Have to compare the bits.

    // This only works because we are careful to keep the trailing bits unset.
    uint64_t *p0 = _bits + offset;
    uint64_t *p1 = rhs._bits + offset;

    for(size_t n=numWords; n>0; n--)
    {
        if (*p0 != *p1)
            return false;

        p0++;
        p1++;
    }

    return true;
}

TfBits &
TfBits::operator&=(const TfBits &rhs)
{
    // Early bail out.
    if (this == &rhs) 
        return *this;

    TF_AXIOM(_num == rhs._num);

    // Limit the bit operations to where we have bits set in the first of
    // the two sets (we only remove bits).
    size_t firstSet = GetFirstSet();
    size_t lastSet  = GetLastSet();

    // Nothing to do if we already have no bits set.
    if (firstSet < _num)
    {
        size_t offset   = firstSet >> 6;
        size_t numWords = (lastSet >> 6) + 1 - offset;

        uint64_t *p0 = _bits + offset;
        uint64_t *p1 = rhs._bits + offset;

        for(size_t n=numWords; n>0; n--)
        {
            *p0 &= *p1;
            p0++;
            p1++;
        }

        // Note: All lhs & rhs trailing bits are 0, so no trailing bits get set.
        _numSet  .Store(-1);
        _firstSet.Store(FindNextSet(firstSet));
        _lastSet .Store(FindPrevSet(lastSet));
    }

    return *this;
}

TfBits &
TfBits::operator|=(const TfBits &rhs)
{
    if (TF_VERIFY(_num == rhs._num))
        _Or(rhs);
    
    return *this;
}

void
TfBits::_Or(const TfBits &rhs)
{
    // Early bail out.
    if (this == &rhs) 
        return;

    // Limit the bit operations to where we have bits set in the second of
    // the two sets (we only add bits that are set in the second set).
    size_t rhsFirstSet = rhs.GetFirstSet();

    // Nothing to do if rhs has no bits set.
    if (rhsFirstSet < rhs._num)
    {
        size_t rhsLastSet  = rhs.GetLastSet();
        size_t lhsFirstSet = GetFirstSet();
        size_t lhsLastSet  = GetLastSet();

        // Compute the first and last set bits for the result (note that we
        // only do the or operation on the range where we have rhs bits set).
        size_t firstSet = TfMin(lhsFirstSet, rhsFirstSet);
        size_t lastSet  = (lhsLastSet < _num) ?
            TfMax(lhsLastSet, rhsLastSet) : rhsLastSet;

        // Early out for the case that rhs is contained in lhs (note that
        // we do not bother to cache _numSet, if it is not cached we simply
        // skip this early check).
        if (_numSet.Load() == lastSet - firstSet + 1 &&
            firstSet == lhsFirstSet && lastSet == lhsLastSet)
            return;

        size_t offset   = rhsFirstSet >> 6;
        size_t numWords = (rhsLastSet >> 6) + 1 - offset;

        uint64_t *p0 = _bits + offset;
        uint64_t *p1 = rhs._bits + offset;

        for(size_t n=numWords; n>0; n--)
        {
            *p0 |= *p1;
            p0++;
            p1++;
        }

        // Note: All lhs & rhs trailing bits are 0, so no trailing bits get set.
        _numSet  .Store(-1);
        _firstSet.Store(firstSet);
        _lastSet .Store(lastSet);
    }
}

void
TfBits::OrSubset(const TfBits &rhs)
{
    if (TF_VERIFY(_num >= rhs._num))
        _Or(rhs);
}

TfBits &
TfBits::operator^=(const TfBits &rhs)
{
    TF_AXIOM(_num == rhs._num);

    // Limit the bit operations to where we have bits set in either of
    // the two sets.
    size_t i0 = GetFirstSet();
    size_t i1 = rhs.GetFirstSet();

    // Nothing to do if rhs has no bits set.
    if (i1 < _num)
    {
        size_t firstSet = TfMin(i0, i1);
        size_t lastSet  = (i0 < _num) ?
            TfMax(GetLastSet(), rhs.GetLastSet()) : rhs.GetLastSet();

        size_t offset   = firstSet >> 6;
        size_t numWords = (lastSet >> 6) + 1 - offset;

        uint64_t *p0 = _bits + offset;
        uint64_t *p1 = rhs._bits + offset;

        for(size_t n=numWords; n>0; n--)
        {
            *p0 ^= *p1;
            p0++;
            p1++;
        }

        // Note: All lhs & rhs trailing bits are 0, so no trailing bits get set.
        _numSet  .Store(-1);
        _firstSet.Store(FindNextSet(firstSet));
        _lastSet .Store(FindPrevSet(lastSet));
    }

    return *this;
}

TfBits &
TfBits::operator-=(const TfBits &rhs)
{
    TF_AXIOM(_num == rhs._num);

    // Limit the bit operations to where we have bits set in both of
    // the two sets.
    size_t lhsFirstSet = GetFirstSet();
    size_t lhsLastSet  = GetLastSet();

    size_t i0 = lhsFirstSet;
    size_t i1 = rhs.GetFirstSet();
    size_t firstSet = TfMax(i0, i1);
    size_t lastSet  = TfMin(lhsLastSet, rhs.GetLastSet());

    // Nothing to do if either set has not bits set, or they do not overlap.
    if (i0 < _num && i1 < _num && firstSet <= lastSet)
    {
        size_t offset   = firstSet >> 6;
        size_t numWords = (lastSet >> 6) + 1 - offset;

        uint64_t *p0 = _bits + offset;
        uint64_t *p1 = rhs._bits + offset;

        for(size_t n=numWords; n>0; n--)
        {
            *p0 &= ~*p1;
            p0++;
            p1++;
        }

        // Note: All lhs & rhs trailing bits are 0, so no trailing bits get set.
        _numSet  .Store(-1);
        _firstSet.Store(FindNextSet(lhsFirstSet));
        _lastSet .Store(FindPrevSet(lhsLastSet));
    }

    return *this;
}

TfBits &
TfBits::Complement()
{
    uint64_t *p = _bits;

    for(size_t i=_numWords; i>0; i--)
    {
        *p = ~*p;
        p++;
    }

    // Note: Need to keep unused bits at 0.
    _ClearTrailingBits();

    const size_t numSet = _numSet.Load();
    if (numSet != size_t(-1))
        _numSet.Store(_num - numSet);
    _firstSet.Store(-1);
    _lastSet .Store(-1);

    return *this;
}

void
TfBits::_ClearTrailingBits()
{
    // Find the total number of words.
    if (_numWords && (_num & 63))
    {
        // Mask out the bits we're not using so that they don't end up in the
        // count.
        size_t numUsedBitsInLastWord = 64 - ((_numWords << 6) - _num);
        TF_AXIOM(numUsedBitsInLastWord > 0 && numUsedBitsInLastWord <= 63);
    
        // Zero out the unused bits so that they don't show up in the counts.
        _bits[_numWords - 1] &= (UINT64_C(1) << numUsedBitsInLastWord) - 1;
    }
}

std::string
TfBits::GetAsStringLeftToRight() const
{
    std::string res;

    for(size_t i=0; i<_num; ++i)
        res.push_back('0' + IsSet(i));

    return res;
}

std::string
TfBits::GetAsStringRightToLeft() const
{
    std::string res;

    for(int64_t i=_num-1; i>=0; --i)
        res.push_back('0' + IsSet(i));

    return res;
}


std::ostream &
operator<<(std::ostream &out, const TfBits & bits)
{
    out << bits.GetAsStringLeftToRight();
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
