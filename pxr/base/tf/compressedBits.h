//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_COMPRESSED_BITS_H
#define PXR_BASE_TF_COMPRESSED_BITS_H

#include "pxr/base/arch/align.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Forward Declarations
///
class TfBits;

/// \class TfCompressedBits
///
/// \brief Fast, compressed bit array which is capable of performing logical
/// operations without first decompressing the internal data representation.
///
/// The internal data compression is based on a form of RLE, where words are
/// used to indicate the number of bits set to the same value. Each subsequent
/// word denotes that the bit value has changed and a "runningBit" is set
/// internally, in order to denote the bit value for the first word.
///
/// Internally, a bitset like this:
///
/// 111000101000
///
/// Will be represented as:
///
/// 1 331113
///
/// i.e., the running bit is '1', and there are 3 of those, followed by 3
/// zeroes, followed by 1 one, followed by 1 zero, followed by 1 one, followed
/// by three zeroes. Each word is called a "platform".
///
/// Compressed bits are very fast when used for logical operations (conjugate, 
/// and, or, xor, etc.), and when iterated over. Contains and Overlaps are also
/// very fast. The representation is lightweight in memory and hence very cache
/// efficient.
///
/// Whenever indexing, setting and resetting of seemingly random bits is a
/// requirement, however, TfBits will perform better, since finding a specific
/// bit requires a linear search.
///
class TfCompressedBits
{
private:
    // Type of one word stored in the word array
    typedef uint32_t _WordType;

    // Lightweight, re-allocating array type optimized for native, word data.
    //
    // Note, this is not a std::vector, because we really want a container,
    // which is optimized for native types, allowing fast memcpy capabilities,
    // and providing local storage optimizations.
    class _WordArray
    {
    public:
        static const uint32_t LOCAL_SIZE = 6;

        _WordArray() :
            _data(_local),
            _numAllocated(LOCAL_SIZE),
            _num(0) {}

        _WordArray(const _WordArray &rhs) :
            _data(_local),
            _numAllocated(LOCAL_SIZE),
            _num(rhs._num) {
            _Duplicate(rhs);
        }

        ~_WordArray() {
            _Deallocate();
        }

        _WordArray& operator=(const _WordArray &rhs) {
            if (this == &rhs) {
                return *this;
            }

            _Duplicate(rhs);
            return *this;
        }

        // Clear all words
        void Clear() {
            _num = 0;
        }

        // Add a word (may cause re-allocation)
        void PushBack(_WordType value) {
            // Reallocate?
            if (_num >= _numAllocated) {
                _Reallocate();
            }

            // PushBack
            _data[_num] = value;
            ++_num;
        }

        // Remove a word
        void PopBack() {
            --_num;
        }

        // Remove multiple words
        void PopBackNum(uint32_t popNum) {
            _num -= popNum;
        }

        // Move this representation into rhs. This representation will be
        // invalid after this operation.
        void MoveInto(_WordArray &rhs) {
            rhs._numAllocated = _numAllocated;
            rhs._num = _num;

            rhs._Deallocate();

            // If the data is stored locally, copy it
            if (_IsStoredLocally()) {
                // The compiler will unroll this loop, making this faster
                // than memcpy!
                for (size_t i = 0; i < LOCAL_SIZE; ++i) {
                    rhs._data[i] = _data[i];
                }
            }

            // Otherwise, just assign pointers
            else {
                rhs._data = _data;

                // Set our pointer back to local, so we won't deallocate the
                // storage, which is now owned by rhs.
                _data = _local;
                _numAllocated = LOCAL_SIZE;
            }
        }

        // Swap two representations
        void Swap(_WordArray &rhs) {
            if (!_IsStoredLocally() && !rhs._IsStoredLocally()) {
                std::swap(_data, rhs._data);
                std::swap(_numAllocated, rhs._numAllocated);
                std::swap(_num,  rhs._num);
            } else {
                // Fall back to a copy. This could be optimized.
                std::swap(*this, rhs);
            }
        }

        // Index operator
        _WordType &operator[](size_t index) {
            return _data[index];
        }

        const _WordType &operator[](size_t index) const {
            return _data[index];
        }

        // Returns the number of words stored (not allocated)
        uint32_t GetNum() const {
            return _num;
        }

        // Return the number of allocated words
        uint32_t GetNumAllocated() const {
            return _numAllocated;
        }

        // Return a pointer to the first word
        const _WordType *Begin() const {
            return _data;
        }

        // Return a pointer one past the end of the array.
        const _WordType *End() const {
            return _data + _num;
        }

        // Returns the first word
        _WordType &Front() {
            return _data[0];
        }

        const _WordType &Front() const {
            return _data[0];
        }

        // Returns the last word
        _WordType &Back() {
            return _data[_num - 1];
        }

        const _WordType &Back() const {
            return _data[_num - 1];
        }

    private:
        bool _IsStoredLocally() const {
            return _data == _local;
        }

        void _Deallocate() {
            if (!_IsStoredLocally()) {
                delete[] _data;
                _data = _local;
            }
        }

        void _Duplicate(const _WordArray &rhs) {
            if (rhs._num > 0) {
                if (_numAllocated < rhs._num) {
                    _Deallocate();
                    _data = new _WordType[rhs._numAllocated];
                    _numAllocated = rhs._numAllocated;
                }

                if (rhs._IsStoredLocally()) {
                    // The compiler will unroll this loop, making this faster
                    // than memcpy!
                    for (size_t i = 0; i < LOCAL_SIZE; ++i) {
                        _data[i] = rhs._data[i];
                    }
                } else {
                    memcpy(_data, rhs._data, sizeof(_WordType) * rhs._num);
                }
            }

            _num = rhs._num;
        }

        void _Reallocate() {
            _numAllocated <<= 1;
            _WordType *newData = new _WordType[_numAllocated];
            memcpy(newData, _data, sizeof(_WordType) * _num);
            _Deallocate();
            _data = newData;
        }

        // Pointer to the data
        _WordType *_data;

        // Local storage optimization
        _WordType _local[LOCAL_SIZE];

        // Number of words allocated
        uint32_t _numAllocated;

        // Number of words stored
        uint32_t _num;
    };

public:

    // View and iterator modes: All bits, all set bits, all unset bits,
    // platforms (iterator provides platform size and value)
    enum class Mode { All, AllSet, AllUnset, Platforms };

    /// Hash for TfCompressedBits.
    ///
    /// This hash is linear in time as it considers all the words between.
    /// If you need a constant-time hash, see FastHash, it may be suitable for
    /// your needs.
    ///
    struct Hash {
        size_t operator()(const TfCompressedBits &bits) const {
            return bits.GetHash();
        }
    };

    /// A hash functor for TfCompressedBits that is faster than Hash.
    ///
    /// This hash can be computed in constant time because it only uses a
    /// fixed subset of data: the number of bits in total, the running bit,
    /// the number of words and the first cache line of words.
    ///
    struct FastHash {
        size_t operator()(const TfCompressedBits &bits) const {
            if (bits.GetSize() == 0) {
                return 0;
            }

            // Hash the running bit and number of platforms.
            size_t hash = TfHash::Combine(
                bits.GetSize(),
                bits._runningBit,
                bits._platforms.GetNum());

            // Hash a single cache line of platform data.
            const uint32_t n = std::min<uint32_t>(
                bits._platforms.GetNum(),
                ARCH_CACHE_LINE_SIZE / sizeof(uint32_t));
            for (uint32_t i = 0; i < n; ++i) {
                hash = TfHash::Combine(hash, bits._platforms[i]);
            }

            return hash;
        }
    };

    /// Constructs a fixed size bit array, clears all bits.
    ///
    explicit TfCompressedBits(size_t num = 0) :
        _num(num),
        _runningBit(0) {
        _platforms.PushBack(num);
    }

    /// Constructs a fixed size bit array, with a range of bits set.
    ///
    explicit TfCompressedBits(size_t num, size_t first, size_t last) :
        _num(num),
        _runningBit(0) {

        // Empty bitset
        if (num == 0) {
            _platforms.PushBack(0);
            return;
        }

        // Range error (clear the whole bitset):
        if (!TF_VERIFY(first < num && last < num && first <= last)) {
            _platforms.PushBack(num);
            return;
        }

        size_t trailingZeroes = 0;
        const size_t range = last - first + 1;
        if (first == 0) {
            _runningBit = 1;
            _platforms.PushBack(range);
            trailingZeroes = num - range;
        } else {
            _platforms.PushBack(first);
            _platforms.PushBack(range);
            trailingZeroes = num - last - 1;
        }

        // Only push trailing zeroes, if there are any. Otherwise the
        // _platforms array will be in an inconsistent state (containing
        // platforms of size 0, when _num != 0).
        if (trailingZeroes != 0) {
            _platforms.PushBack(trailingZeroes);
        }
    }

    /// Copy-constructs a fixed size bit array.
    ///
    TfCompressedBits(const TfCompressedBits &rhs) :
        _platforms(rhs._platforms),
        _num(rhs._num),
        _runningBit(rhs._runningBit) {}

    /// Copy-construct a fixed sized bit array, from the complement of the
    /// \p rhs bitset.
    ///
    enum ComplementTagType { ComplementTag };
    TfCompressedBits(const TfCompressedBits &rhs, ComplementTagType) :
        _platforms(rhs._platforms),
        _num(rhs._num),
        _runningBit(1 - rhs._runningBit) {
        if (_num == 0) {
            _runningBit = 0;
        }
    }

    /// Construct a TfCompressedBits array from a TfBits array.
    ///
    TF_API
    explicit TfCompressedBits(const TfBits &bits);

    /// Move Constructor
    ///
    TfCompressedBits(TfCompressedBits &&rhs) :
        _num(rhs._num),
        _runningBit(rhs._runningBit) {
        rhs._platforms.MoveInto(_platforms);
        rhs._platforms.Clear();
        rhs._num = 0;
        rhs._runningBit = 0;
    }

    /// Destructor
    ///
    ~TfCompressedBits() {}

    /// Assignment operator
    ///
    TfCompressedBits &operator=(const TfCompressedBits &rhs) {
        if (this == &rhs) {
            return *this;
        }

        _platforms  = rhs._platforms;
        _num        = rhs._num;
        _runningBit = rhs._runningBit;

        return *this;
    }

    /// Move assignment operator.
    ///
    TfCompressedBits &operator=(TfCompressedBits &&rhs) {
        if (this == &rhs) {
            return *this;
        }

        _num = rhs._num;
        _runningBit = rhs._runningBit;
        rhs._platforms.MoveInto(_platforms);
        rhs._platforms.Clear();
        rhs._num = 0;
        rhs._runningBit = 0;

        return *this;
    }

    /// Resize the bitset, while keeping the contents, unless trimmed.
    ///
    void ResizeKeepContents(size_t num) {
        if (_num == num) {
            return;
        }

        // Reduce size to 0
        if (num == 0) {
            _platforms.Clear();
            _platforms.PushBack(0);
            _runningBit = 0;
            _num = 0;
            return;
        }

        // Grow
        if (_num < num) {
            if ((UINT32_C(1) - _runningBit) == 
                    (_platforms.GetNum() & UINT32_C(1))) {
                _platforms.Back() += (num - _num);
            } else {
                _platforms.PushBack(num - _num);
            }
        }

        // Shrink
        else if (_num > num) {
            uint32_t diff = _num - num;
            while (_platforms.Back() <= diff) {
                diff -= _platforms.Back();
                _platforms.PopBack();
            }
            _platforms.Back() -= diff;
        }

        _num = num;
    }

    /// Provides a fast swap.
    ///
    void Swap(TfCompressedBits &rhs) {
        std::swap(_num,         rhs._num);
        std::swap(_runningBit,  rhs._runningBit);
        _platforms.Swap(rhs._platforms);
    }

    /// Clears all bits to zero.
    ///
    void ClearAll() {
        if (_num <= 0 || (_runningBit == 0 && _platforms.GetNum() == 1)) {
            return;
        }

        _runningBit = 0;
        _platforms.Clear();
        _platforms.PushBack(_num);
    }

    /// Sets all bits to one.
    ///
    void SetAll() {
        if (_num <= 0 || (_runningBit == 1 && _platforms.GetNum() == 1)) {
            return;
        }

        _runningBit = 1;
        _platforms.Clear();
        _platforms.PushBack(_num);
    }

    /// Clears bit # index to zero.
    ///
    /// Note: This is a slow operation on TfCompressedBits!
    ///
    void Clear(size_t index) {
        if (!TF_VERIFY(index < _num)) {
            return;
        }

        TfCompressedBits tmp(_num, index, index);
        tmp.Complement();
        *this &= tmp;
    }

    /// Sets bit # index to zero.
    ///
    /// Note: This is a slow operation on TfCompressedBits!
    ///
    void Set(size_t index) {
        if (!TF_VERIFY(index < _num)) {
            return;
        }

        TfCompressedBits tmp(_num, index, index);
        *this |= tmp;
    }

    /// Sets the bit within the range of first and last.
    ///
    /// Note: This is a slow operation on TfCompressedBits!
    ///
    void SetRange(size_t first, size_t last) {
        // Range constructor does error checking
        TfCompressedBits tmp(_num, first, last);
        *this |= tmp;
    }

    /// Append a number of bits with the given \p value to this bitset. 
    /// This also increases the size of the bitset by the number of bits added.
    ///
    void Append(size_t num, bool value) {
        if (num == 0) {
            return;
        }

        if (_num == 0) {
            _platforms[0] = num;
            _runningBit = value;
            _num = num;
            return;
        }

        const bool lastValue = _runningBit == (_platforms.GetNum() & 1);
        if (value == lastValue) {
            _platforms.Back() += num;
        } else {
            _platforms.PushBack(num);
        }

        _num += num;
    }

    /// Assigns val to bit # index.
    ///
    void Assign(size_t index, bool value) {
        if (value) {
            Set(index);
        } else {
            Clear(index);
        }
    }

    /// Shift this bitset a given number of \p bits to the right, and extend to
    /// the left with zeroes.
    ///
    void ShiftRight(size_t bits) {
        if (_num == 0 || bits == 0) {
            return;
        }

        // If the running bit is 0, just increment the first word (num zeroes)
        if (_runningBit == 0) {
            _platforms.Front() += bits;
        } 

        // If the running bit is 1, shift all the _platforms to the right and
        // flip the running bit. Set the first platform (num zeroes) to the
        // number of bits shifted.
        else {
            _runningBit = 0;
            _platforms.PushBack(0);
            for (size_t i = _platforms.GetNum() - 1; i > 0; --i) {
                _platforms[i] = _platforms[i - 1];
            }
            _platforms[0] = bits;
        }

        // Now trim the _platforms on the right
        while (_platforms.Back() <= bits) {
            bits -= _platforms.Back();
            _platforms.PopBack();
        }
        _platforms.Back() -= bits;
    }

    /// Shift this bitset a given number of \p bits to the left, and extend the
    /// right with zeroes.
    ///
    void ShiftLeft(size_t bits) {
        if (_num == 0 || bits == 0) {
            return;
        }

        // How many platforms to trim on the left?
        size_t trimBits = bits;
        size_t platformIndex = 0;
        while (platformIndex < _platforms.GetNum() && 
            _platforms[platformIndex] <= trimBits) {
            trimBits -= _platforms[platformIndex];
            ++platformIndex;
        }

        // Reduce the size of the first platform or, if the shift clears the
        // whole bitset, remove all platforms.
        if (platformIndex < _platforms.GetNum()) {
            _platforms[platformIndex] -= trimBits;
        } else {
            _platforms.Clear();
            _runningBit = 0;
            platformIndex = 0;
        }

        // Are there any platforms to be trimmed on the left?
        if (platformIndex > 0) {
            // Shift the platforms to the left, by the number of
            // platforms trimmed
            const size_t last = _platforms.GetNum() - platformIndex;
            for (size_t i = 0; i < last; ++i) {
                _platforms[i] = _platforms[i + platformIndex];
            }
            _platforms.PopBackNum(platformIndex);

            // Flip the running bit, if necessary
            if (platformIndex & 1) {
                _runningBit = 1 - _runningBit;
            }
        }

        // Extend on the right, by adding zeros, if the last platform
        // is zeros ...
        if ((UINT32_C(1) - _runningBit) ==
                (_platforms.GetNum() & UINT32_C(1))) {
            _platforms.Back() += bits;
            return;
        }

        // ... or adding a new platform with zeros, if the last platform
        // is ones
        _platforms.PushBack(std::min<_WordType>(_num, bits));
    }

    /// Returns true, if bit # index is set.
    ///
    /// Note: This is a slow operation on TfCompressedBits.
    ///       Please, use an iterator if possible. Iterators are fast!
    ///
    bool IsSet(size_t index) const {
        if (!TF_VERIFY(index < _num)) {
            return false;
        }

        size_t platformIndex, bitCount;
        return _LinearSearch(index, &platformIndex, &bitCount) == 1;
    }

    /// Returns the index of the n-th bit set in this bit set.
    /// 
    /// This function counts the set bits up to the \p nth bit, and returns
    /// the index of that n-th set bit. If there are less than \p nth bits set,
    /// returns GetSize().
    /// 
    /// Note: This operation is slower than using an iterator. For forward or
    ///       reverse iteration, use the iterator instead.
    ///
    size_t FindNthSet(size_t nth) const {
        size_t index = 0;
        size_t count = 0;
        uint8_t bit = _runningBit;

        // Iterate over all platforms to find the nth set bit using a running
        // count of set bits, and an up-to-date index into the bitset.
        for (size_t i = 0; i < _platforms.GetNum(); ++i) {
            const _WordType platform = _platforms[i];

            // Since bit toggles between 1 and 0 for every iteration of the
            // loop, using it in a conditional guarantees a misprediction every
            // time. Doing the multiplication instead is cheap and doesn't
            // change the result of the conditional until we find the right
            // index.
            if (((count + platform) * bit) > nth) {
                return index + (nth - count);
            }

            index += platform;
            count += (platform * bit);
            bit = 1 - bit;
        }

        // Less than nth bits are set, so return the size.
        return _num;
    }

    /// Find the next bit set that is higher or equal to index. 
    /// If no more set bits are found, index returns 'GetSize()'.
    ///
    /// Note: This is a slow operation on TfCompressedBits.
    ///       Please, use an iterator if possible. Iterators are fast!
    ///
    size_t FindNextSet(size_t index) const
    {
        if (index >= _num) {
            return _num;
        }

        size_t platformIndex, bitCount;
        const uint8_t bit = _LinearSearch(index, &platformIndex, &bitCount);

        if (bit == 1) {
            return index;
        }

        return bitCount;
    }

    /// Finds the next unset bit that has a higher or equal index than index.
    /// If no more set bits are found, index returns 'GetSize()'.
    ///
    /// Note: This is a slow operation on TfCompressedBits.
    ///       Please, use an iterator if possible. Iterators are fast!
    ///
    size_t FindPrevSet(size_t index) const
    {
        if (index >= _num) {
            return _num;
        }

        size_t platformIndex, bitCount;
        const uint8_t bit = _LinearSearch(index, &platformIndex, &bitCount);

        if (bit == 1) {
            return index;
        }

        const size_t first = bitCount - _platforms[platformIndex];
        if (first > 0) {
            return first - 1;
        }

        return _num;
    }

    /// Finds the next unset bit that has a higher or equal index than index.
    /// If no more set bits are found, index returns 'GetSize()'.
    ///
    /// Note: This is a slow operation on TfCompressedBits.
    ///       Please, use an iterator if possible. Iterators are fast!
    ///
    size_t FindNextUnset(size_t index) const
    {
        if (index >= _num) {
            return _num;
        }

        size_t platformIndex, bitCount;
        const uint8_t bit = _LinearSearch(index, &platformIndex, &bitCount);

        if (bit == 0) {
            return index;
        }
        
        return bitCount;
    }

    /// Count the bits set, and also return the largest gap between bits.
    ///
    void Count(size_t *numSet, size_t *maxGap) const {
        const uint32_t lastIndex = _platforms.GetNum() - 1;
        uint32_t num = 0;
        uint32_t max = 0;
        uint8_t bit = _runningBit;
        for (size_t i = 0; i < _platforms.GetNum(); ++i) {
            // Accumulate set bits.
            if (bit == 1) {
                num += _platforms[i];
            }
            // Don't account the leading and trailing zeros as gaps.
            else if (i > 0 && i < lastIndex) {
                max = std::max(max, _platforms[i]);
            }
            bit = 1 - bit;
        }
        *numSet = num;
        *maxGap = max;
    }

    /// Returns the size of the bit array, ie. the # of bits it can hold.
    ///
    size_t GetSize() const {
        return _num;
    }

    /// Returns \c true if this bit array is empty, i.e. it is of size zero.
    ///
    bool IsEmpty() const {
        return _num == 0;
    }

    /// Returns the index of the first bit set in the bit array.  If no bits
    /// are set, the return value is 'GetSize()'.
    ///
    size_t GetFirstSet() const {
        if (_num == 0 || _runningBit == 1) {
            return 0;
        }

        return _platforms.Front();
    }

    /// Returns the index of the last bit set in the bit array.  If no bits
    /// are set, the return value is 'GetSize()'.
    ///
    size_t GetLastSet() const {
        // Zero size or all zeros case
        if (_num == 0 || (_runningBit == 0 && _platforms.GetNum() == 1)) {
            return _num;
        }

        // If _runningBit == 1 and number of words is odd or
        //    _runningBit == 0 and number of words is even
        if (_runningBit == (_platforms.GetNum() & 1)) {
            return _num - 1;
        }

        return _num - 1 - _platforms.Back();
    }

    /// Returns the number of bits currently set in this array.
    ///
    size_t GetNumSet() const {
        size_t numSet = 0;
        for (size_t i = 1 - _runningBit; i < _platforms.GetNum(); i += 2) {
            numSet += _platforms[i];
        }
        return numSet;
    }

    /// Returns the number of platforms (zeros or ones) in this bitset.
    ///
    size_t GetNumPlatforms() const {
        if (_num == 0) {
            return 0;
        }

        return _platforms.GetNum();
    }

    /// Returns the number of set (ones) platforms in this bitset.
    ///
    size_t GetNumSetPlatforms() const {
        if (_num == 0) {
            return 0;
        }

        const uint32_t numP = _platforms.GetNum();
        return (numP / 2) + (numP & _runningBit);
    }

    /// Returns the number of unset (zeros) platforms in this bitset.
    ///
    size_t GetNumUnsetPlatforms() const {
        if (_num == 0) {
            return 0;
        }

        const uint32_t numP = _platforms.GetNum();
        return (numP / 2) + (numP & (1 - _runningBit));
    }

    /// Returns true, if all the bits in this bit array are set.
    ///
    bool AreAllSet() const {
        return _num == 0 || (_runningBit == 1 && _platforms.GetNum() == 1);
    }

    /// Returns true, if all the bits in this bit array are unset.
    ///    
    bool AreAllUnset() const {
        return !IsAnySet();
    }

    /// Returns true, if there is at least a single set bit.
    /// 
    bool IsAnySet() const {
        return _num > 0 && (_runningBit == 1 || _platforms.GetNum() > 1);
    }

    /// Returns true, if there is at least a single unset bit.
    ///
    bool IsAnyUnset() const {
        return _num > 0 && (_runningBit == 0 || _platforms.GetNum() > 1);
    }

    /// Returns true if the set bits in this bit array are contiguous.
    ///
    /// Note: This returns false if there are no set bits.
    ///
    bool AreContiguouslySet() const {
        const uint32_t numP = _platforms.GetNum();
        return
            _num > 0 && numP <= 3 &&
                (numP == 2 ||
                (_runningBit == 1 && numP == 1) || 
                (_runningBit == 0 && numP == 3));
    }

    /// Returns the amount of memory this object holds on to.
    ///
    size_t GetAllocatedSize() const
    {
        size_t size = sizeof(TfCompressedBits);
        if (_platforms.GetNumAllocated() > _WordArray::LOCAL_SIZE) {
            size += sizeof(_WordType) * _platforms.GetNumAllocated();
        } 
        return size;
    }

    /// Returns a hash for this instance.
    ///
    TF_API
    size_t GetHash() const;

    /// Returns a string representing the bits for debugging with bits
    /// ordered from left to right with increasing indices.
    ///
    TF_API
    std::string GetAsStringLeftToRight() const;

    /// Returns a string representing the bits for debugging with bits
    /// ordered from right to left with increasing indices.
    ///
    TF_API
    std::string GetAsStringRightToLeft() const;

    /// Returns a string representing the bits for debugging with bits
    /// represented in run-length encoding form.
    ///
    TF_API
    std::string GetAsRLEString() const;

    /// Returns a bitset constructed from the supplied string representation.
    ///
    /// The string representation can be either a RLE encoded bitset, such as
    /// '1x5-0x5-1x100', or a string of zeros and ones, such as '1111100000'.
    /// Note that whitespace anywhere in the string representation is ignored.
    ///
    /// Any character other than whitespace, a digit, 'x' or '-' in the string
    /// representation is considered invalid. Invalid string representations
    /// will return an empty bitset.
    /// An empty string representation (or a string purely comprised of
    /// whitespace), however, is considered a valid representation describing
    /// an empty bitset.
    ///
    TF_API
    static TfCompressedBits FromString(const std::string &source);

    /// \name Operators
    /// @{

    /// Returns true if this == \p rhs.
    ///
    bool operator==(const TfCompressedBits &rhs) const {
        if (this == &rhs || (_num == 0 && rhs._num == 0)) {
            return true;
        }
        
        // Fast comparisons, first
        if (_num == rhs._num &&
            _runningBit == rhs._runningBit &&
            _platforms.GetNum() == rhs._platforms.GetNum()) {

            // Worst case, scenario: Must compare every word
            for (size_t i = 0; i < _platforms.GetNum(); ++i) {
                // Early bailout, if two words don't match
                if (_platforms[i] != rhs._platforms[i]) {
                    return false;
                }
            }

            // All words match
            return true;
        }

        // Early comparison failed
        return false;
    }

    /// Returns true if this != \p rhs.
    ///
    bool operator!=(const TfCompressedBits &rhs) const {
        return !(*this == rhs);
    }

    /// Ands these bits with the \p rhs bits.
    ///
    /// The resulting bit set is the intersection of the two bit sets.
    ///
    TfCompressedBits &operator&=(const TfCompressedBits &rhs) {
        if (!TF_VERIFY(_num == rhs._num) ||
            _num == 0 || rhs._num == 0) {
            return *this;
        }

        const uint32_t numA = _platforms.GetNum();
        const uint32_t numB = rhs._platforms.GetNum();
        const uint8_t bitA = _runningBit;
        const uint8_t bitB = rhs._runningBit;

        // Early bailout: This is all zeroes or all ones
        if (numA == 1) {
            if (bitA == 0) {
                return *this;
            }

            _runningBit = bitB;
            _platforms = rhs._platforms;
            return *this;
        }

        // Early bailout: Rhs is all zeroes or all ones
        if (numB == 1) {
            if (bitB == 1) {
                return *this;
            }

            ClearAll();
            return *this;
        }

        // Early bailout: No bits will overlap, if sets are disjoint
        if (_AreBoundsDisjoint(rhs)) {
            ClearAll();
            return *this;
        }

        return _Logical<_And>(bitB, rhs._platforms);
    }

    /// Returns these bits and'ed with \p rhs.
    ///
    TfCompressedBits operator&(const TfCompressedBits &rhs) const {
        TfCompressedBits r(*this);
        r &= rhs;
        return r;
    }

    /// Ors these bits with the \p rhs bits.
    ///
    /// The resulting bit set is the union of the two bit sets.
    ///
    TfCompressedBits &operator|=(const TfCompressedBits &rhs) {
        if (!TF_VERIFY(_num == rhs._num) ||
            _num == 0 || rhs._num == 0) {
            return *this;
        }

        const uint32_t numA = _platforms.GetNum();
        const uint32_t numB = rhs._platforms.GetNum();
        const uint8_t bitA = _runningBit;
        const uint8_t bitB = rhs._runningBit;

        // Early bailout: This is all zeroes or all ones
        if (numA == 1) {
            if (bitA == 1) {
                return *this;
            }

            _runningBit = bitB;
            _platforms = rhs._platforms;
            return *this;
        }

        // Early bailout: Rhs is all zeroes or all ones
        if (numB == 1) {
            if (bitB == 0) {
                return *this;
            }

            SetAll();
            return *this;
        }

        // If this set already contains all the bits in rhs, there is no
        // point in proceeding with the full logical OR. Note, that although
        // this operation needs to look at all the platforms, it only performs
        // reads from memory, which makes it faster than the logical OR. If
        // this check fails, the data is already prefetched and ready to be
        // consumed by the logical OR.
        if (Contains(rhs)) {
            return *this;
        }

        return _Logical<_Or>(bitB, rhs._platforms);
    }

    /// Returns these bits or'ed with the \p rhs.
    ///
    TfCompressedBits operator|(const TfCompressedBits &rhs) const {
        TfCompressedBits r(*this);
        r |= rhs;
        return r;
    }

    /// Xors these bits with the \p rhs bits.
    ///
    /// The resulting bit set is the union of the two bit sets minus the
    /// intersection of the two bit sets.
    ///
    TfCompressedBits &operator^=(const TfCompressedBits &rhs) {
        if (!TF_VERIFY(_num == rhs._num) ||
            _num == 0 || rhs._num == 0) {
            return *this;
        }

        // Early bailout: This is all zeroes
        if (AreAllUnset()) {
            *this = rhs;
            return *this;
        }

        // Early bailout: Rhs is all zeroes
        if (rhs.AreAllUnset()) {
            return *this;
        }

        return _Logical<_Xor>(rhs._runningBit, rhs._platforms);
    }

    /// Returns these bits xor'ed with \p rhs.
    ///
    TfCompressedBits operator^(const TfCompressedBits &rhs) const {
        TfCompressedBits r(*this);
        r ^= rhs;
        return r;
    }

    /// Removes all bits in the \p rhs bits from these bits.
    ///
    /// The resulting bit set is the asymmetric set difference of
    /// the two bit sets.
    ///
    TfCompressedBits &operator-=(const TfCompressedBits &rhs) {
        if (!TF_VERIFY(_num == rhs._num) ||
            _num == 0 || rhs._num == 0) {
            return *this;
        }

        const uint32_t numA = _platforms.GetNum();
        const uint32_t numB = rhs._platforms.GetNum();
        const uint8_t bitA = _runningBit;
        const uint8_t bitB = rhs._runningBit;

        // Early bailout: This is all zeroes or all ones
        if (numA == 1) {
            if (bitA == 0) {
                return *this;
            }

            _runningBit = 1 - bitB;
            _platforms = rhs._platforms;
            return *this;
        }

        // Early bailout: Rhs is all zeroes or all ones
        if (numB == 1) {
            if (bitB == 0) {
                return *this;
            }

            ClearAll();
            return *this;
        }

        // Early bailout: No bits will be subtracted, if sets are disjoint.
        // Note, that although this operation needs to look at all the
        // platforms, it only performs reads from memory, which makes it faster
        // than the logical AND. If this check fails, the data is already
        // prefetched and ready to be consumed by the logical AND.
        if (_AreBoundsDisjoint(rhs) || !HasNonEmptyIntersection(rhs)) {
            return *this;
        }

        return _Logical<_And>(1 - bitB, rhs._platforms);
    }

    /// Returns bits with all the bits in \p rhs removed from these bits.
    ///
    TfCompressedBits operator-(const TfCompressedBits &rhs) const {
        TfCompressedBits r(*this);
        r -= rhs;
        return r;
    }

    /// Flips all bits.
    ///
    /// The resulting bit set is the complement of this bit set.
    ///
    TfCompressedBits &Complement() {
        if (_num != 0) {        
            _runningBit = 1 - _runningBit;
        }
        return *this;
    }

    /// Returns bit at \p index.
    ///
    /// Note: This is a slow operation on TfCompressedBits!
    ///
    bool operator[](size_t index) const {
        return IsSet(index);
    }

    /// Shifts to the right (see ShiftRight)
    ///
    TfCompressedBits &operator>>=(size_t bits) {
        ShiftRight(bits);
        return *this;
    }

    /// Returns bits shifted to the right.
    ///
    TfCompressedBits operator>>(size_t bits) const {
        TfCompressedBits r(*this);
        r >>= bits;
        return r;
    }

    /// Shifts to the left (see ShiftLeft)
    ///
    TfCompressedBits &operator<<=(size_t bits) {
        ShiftLeft(bits);
        return *this;
    }

    /// Returns bits shifted to the left.
    ///
    TfCompressedBits operator<<(size_t bits) const {
        TfCompressedBits r(*this);
        r <<= bits;
        return r;
    }

    /// @}


    /// Returns true if the result of the intersection with \p rhs would be
    /// non-zero.
    ///
    /// This method can be used for efficiency because it doesn't perform
    /// the full AND operation on a copy, and it can return early.
    ///
    bool HasNonEmptyIntersection(const TfCompressedBits &rhs) const {
        if (!TF_VERIFY(_num == rhs._num) ||
            _num == 0 || rhs._num == 0) {
            return false;
        }

        uint8_t bitA = _runningBit;
        uint8_t bitB = rhs._runningBit;
        if (bitA & bitB) {
            return true;
        }

        const uint32_t numA = _platforms.GetNum();
        const uint32_t numB = rhs._platforms.GetNum();
        if (numA == 1) {
            if (bitA == 0) {
                return false;
            }

            return rhs.IsAnySet();
        }

        if (numB == 1) {
            if (bitB == 0) {
                return false;
            }

            return IsAnySet();
        }

        // We can bail out early if the ranges of set bits do not overlap
        if (_AreBoundsDisjoint(rhs)) {
            return false;
        }

        return _HasLogical<_And>(bitB, rhs._platforms);
    }

    /// Returns true if the result of an asymmetric set different is non-zero.
    /// This is the equivalent to computing:
    ///    return (this - rhs).GetNumSet() != 0
    /// but avoids creating temporary copies.
    ///
    bool HasNonEmptyDifference(const TfCompressedBits &rhs) const {
        if (!TF_VERIFY(_num == rhs._num) ||
            _num == 0 || rhs._num == 0) {
            return false;
        }

        uint8_t bitA = _runningBit;
        uint8_t bitB = rhs._runningBit;
        if (bitA && !bitB) {
            return true;
        }

        const uint32_t numA = _platforms.GetNum();
        const uint32_t numB = rhs._platforms.GetNum();
        if (numA == 1) {
            if (bitA == 0) {
                return false;
            }

            return rhs.IsAnyUnset();
        }

        if (numB == 1) {
            if (bitB == 0) {
                return IsAnySet();
            }

            return false;
        }

        // We can bail out early, if the ranges of set bits do not overlap.
        // Check the first set bits first, because checking for the last set
        // bit is more expensive.
        const size_t firstSet = GetFirstSet();
        const size_t rhsFirstSet = rhs.GetFirstSet();
        if (firstSet < rhsFirstSet) {
            return true;
        }

        // If we still haven't bailed out yet, check the last set bit.
        const size_t lastSet = GetLastSet();
        const size_t rhsLastSet = rhs.GetLastSet();
        if (lastSet > rhsLastSet ||
            firstSet > rhsLastSet ||
            lastSet < rhsFirstSet) {
            return true;
        }

        return _HasLogical<_And>(1 - bitB, rhs._platforms);
    }

    /// Returns true if this bit array contains \p rhs by computing:
    /// (rhs - this).GetNumSet() == 0.
    /// 
    /// Ie. it will return true if all bits of \p rhs are also set in this.
    ///
    bool Contains(const TfCompressedBits &rhs) const {
        return !rhs.HasNonEmptyDifference(*this);
    }

    /// Returns an empty TfBits.
    ///
    static const TfCompressedBits &GetEmpty() {
        static TfStaticData<TfCompressedBits> emptyBits;
        return *emptyBits;
    }

    /// Decompress the bits into a TfBits array.
    ///
    TF_API
    void Decompress(TfBits *bits) const;

    /// Iterator support.
    ///
    template <Mode mode>
    class View;

    /// Returns an iteratable view for the bits that steps over all bits.
    ///
    typedef View<Mode::All> AllView;
    inline AllView GetAllView() const;

    /// Returns an iteratable view for the bits that steps over all set bits.
    ///
    typedef View<Mode::AllSet> AllSetView;
    inline AllSetView GetAllSetView() const;

    /// Returns an iteratable view for the bits that steps over all unset bits.
    ///
    typedef View<Mode::AllUnset> AllUnsetView;
    inline AllUnsetView GetAllUnsetView() const;

    /// Returns an iteratable view for the bits that steps over all platforms.
    ///
    typedef View<Mode::Platforms> PlatformsView;
    inline PlatformsView GetPlatformsView() const;

private:
    // Functor for logical operation: AND
    struct _And {
        inline uint8_t operator() (uint8_t a, uint8_t b) {
            return a & b;
        }
    };

    // Functor for logical operation: OR
    struct _Or {
        inline uint8_t operator() (uint8_t a, uint8_t b) {
            return a | b;
        }
    };

    // Functor for logical operation: XOR
    struct _Xor {
        inline uint8_t operator() (uint8_t a, uint8_t b) {
            return a ^ b;
        }
    };

    // This method performs a logical operation on the passed in running bit
    // and word array. OP denotes a functor implementing the logical operation.
    // The idea is that the compiler will be smart enough to inline the
    // operation, without actually having to call the function.
    template < class OP > TfCompressedBits &
    _Logical(uint8_t rhsRunningBit, const _WordArray &rhsPlatforms) {
        OP op;

        const uint32_t numA = _platforms.GetNum();
        const uint32_t numB = rhsPlatforms.GetNum();
        uint8_t bitA = _runningBit;
        uint8_t bitB = rhsRunningBit;

        uint8_t b = op(bitA, bitB);
        _WordArray result;
        _runningBit = b;

        size_t indexA = 0;
        size_t indexB = 0;
        _WordType platformA = _platforms[indexA];
        _WordType platformB = rhsPlatforms[indexB];

        uint32_t newTotal = 0;
        _WordType newPlatform = 0;

        while (true) {
            if (platformA < platformB) {
                newTotal += platformA;
                newPlatform += platformA;
                bitA = 1 - bitA;

                // Commit the new platform
                const uint8_t newBit = op(bitA, bitB);
                if (newBit != b) {
                    result.PushBack(newPlatform);
                    newPlatform = 0;
                    b = newBit;
                }

                // Move on to the next platform
                ++indexA;
                platformB = platformB - platformA;
                if (indexA == numA) {
                    platformA = _num - newTotal;
                } else if (indexA < numA) {
                    platformA = _platforms[indexA];
                }

            } else if (platformA > platformB) {
                newTotal += platformB;
                newPlatform += platformB;
                bitB = 1 - bitB;

                // Commit the new platform
                const uint8_t newBit = op(bitA, bitB);
                if (newBit != b) {
                    result.PushBack(newPlatform);
                    newPlatform = 0;
                    b = newBit;
                }

                // Move on to the next platform
                ++indexB;
                platformA = platformA - platformB;
                if (indexB == numB) {
                    platformB = _num - newTotal;
                } else if(indexB < numB) {
                    platformB = rhsPlatforms[indexB];
                }

            } else {
                newTotal += platformA;
                newPlatform += platformA;
                bitA = 1 - bitA;
                bitB = 1 - bitB;

                // Commit the new platform
                const uint8_t newBit = op(bitA, bitB);
                if (newBit != b || newTotal >= _num) {
                    result.PushBack(newPlatform);
                    newPlatform = 0;
                    b = newBit;
                }

                if (newTotal >= _num)
                    break;

                // Move on to the next platforms
                ++indexA;
                if (indexA == numA) {
                    platformA = _num - newTotal;
                } else if (indexA < numA) {
                    platformA = _platforms[indexA];
                }

                ++indexB;
                if (indexB == numB) {
                    platformB = _num - newTotal;
                } else if (indexB < numB) {
                    platformB = rhsPlatforms[indexB];
                }
            }
        }

        result.MoveInto(_platforms);
        return *this;
    }

    // Performs a logical operation, but breaks out and returns true, as soon
    // as the logical operation returns true. If the logical operation never
    // returns true, false is returned.
    template < class OP > bool
    _HasLogical(uint8_t rhsRunningBit, const _WordArray &rhsPlatforms) const {
        OP op;

        uint8_t bitA = _runningBit;
        uint8_t bitB = rhsRunningBit;
        const uint32_t numA = _platforms.GetNum();
        const uint32_t numB = rhsPlatforms.GetNum();

        size_t indexA = 0;
        size_t indexB = 0;
        _WordType sumPlatformA = _platforms[indexA];
        _WordType sumPlatformB = rhsPlatforms[indexB];
        while (indexA < numA && indexB < numB) {
            if (op(bitA, bitB)) {
                return true;
            }

            if (sumPlatformA < sumPlatformB) {
                bitA = 1 - bitA;
                ++indexA;
                sumPlatformA += _platforms[indexA];

            } else if (sumPlatformA > sumPlatformB) {
                bitB = 1 - bitB;
                ++indexB;
                sumPlatformB += rhsPlatforms[indexB];

            } else {
                bitA = 1 - bitA;
                bitB = 1 - bitB;
                ++indexA;
                ++indexB;

                if (indexA >= numA || indexB >= numB) {
                    return false;
                }

                sumPlatformA += _platforms[indexA];
                sumPlatformB += rhsPlatforms[indexB];
            }
        }

        return false;
    }

    // Do a liner search for the bit index, returning its bit value.
    // Also returns the index of that bit in the word array, as well as the
    // bitCount denoting the number of bits counted up until the range that
    // terminates the current word, the index is found in.
    uint8_t _LinearSearch(
        size_t index, size_t *platformIndex, size_t *bitCount) const { 
        uint8_t bit = _runningBit;
        size_t count = 0;
        size_t i;

        for (i = 0; i < _platforms.GetNum(); ++i) {
            count += _platforms[i];
            if (count > index) {
                break;
            }
            bit = 1 - bit;
        }

        *platformIndex = i;
        *bitCount = count;
        return bit;
    }

    // Returns true if this bit array's bounds are disjoint from the bounds
    // of the rhs bit array. The two are considered disjoint if the last bit
    // set of array A is at a lower index than the first bit set on array B 
    // (or vice versa).
    // Note, that the bit arrays may still be disjoint, even if this method
    // returns false, but if this method returns true, the bit arrays are
    // guaranteed to be disjoint. This is basically a very cheap early out for
    // the Overlaps() method. 
    bool _AreBoundsDisjoint(const TfCompressedBits &rhs) const {
        return 
            GetLastSet() < rhs.GetFirstSet() || 
            GetFirstSet() > rhs.GetLastSet();
    }

    // The word array, storing the bit platforms.
    _WordArray _platforms;

    // The size of this bit array in number of bits.
    uint32_t _num;

    // The value of the running bit, indicating what the bit value of the first
    // word is.
    uint8_t _runningBit;

};

template <TfCompressedBits::Mode mode>
class TfCompressedBits::View
{
public:
    class const_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const uint32_t;
        using reference = const uint32_t &;
        using pointer = const uint32_t *;
        using difference_type = const uint32_t;

        const_iterator() :
            _bits(nullptr),
            _platformIndex(0),
            _bitIndex(0),
            _bitCounter(0),
            _value(0)
        {}

        reference operator*() const { return dereference(); }
        pointer operator->() const { return &(dereference()); }

        const_iterator& operator++() {
            increment();
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator r(*this);
            increment();
            return r;
        }

        bool operator==(const const_iterator& rhs) const {
            return equal(rhs);
        }

        bool operator!=(const const_iterator& rhs) const {
            return !equal(rhs);
        }

        bool IsSet() const {
            return _value == 1;
        }

        bool IsAtEnd() const {
            if (!_bits) {
                return true;
            }
            return _bitIndex >= _bits->GetSize();
        }

    private:
        friend class View;

        const_iterator(
            const TfCompressedBits *bits,
            uint32_t platformIndex,
            uint32_t bitIndex,
            uint8_t value) :
            _bits(bits),
            _platformIndex(platformIndex),
            _bitIndex(bitIndex),
            _bitCounter(0),
            _value(value)
        {}

        bool equal(const const_iterator &rhs) const {
            return _bits == rhs._bits && _bitIndex == rhs._bitIndex;
        }

        void increment() {
            // Note, this looks like quite a bit of logic, but mode is a
            // compile time constant. The compiler to the rescue!
            if (!_bits) {
                return;
            }

            // Increment bit index
            ++_bitIndex;

            // Increment bit counter (counting the current word)
            ++_bitCounter;

            // If the bit counter surpasses the current word,
            // skip ahead to the next word
            if (_bitCounter >= _bits->_platforms[_platformIndex]) {

                // If the iterator mode is not All, look at
                // every other word
                const uint32_t numP =
                    _bits->_platforms.GetNum();
                if ((mode == Mode::AllSet || mode == Mode::AllUnset) &&
                    (_platformIndex + 1) < numP) {
                    _bitIndex += _bits->_platforms[_platformIndex + 1];
                    _platformIndex += 2;
                } 

                // Otherwise, look at every word and toggle
                // the value bit
                else {
                    ++_platformIndex;
                    _value = 1 - _value;
                }

                // Reset the bit counter
                _bitCounter = 0;
            }
        }

        const uint32_t &dereference() const {
            return _bitIndex;
        }

        const TfCompressedBits *_bits;
        uint32_t _platformIndex;
        uint32_t _bitIndex;
        uint32_t _bitCounter;
        uint8_t _value;
    };

    // Support for TF_FOR_ALL.
    typedef const_iterator iterator;

    const_iterator begin() const {
        const uint8_t bit = _bits->_runningBit;

        // Skip ahead one word, if looking at AllSet/AllUnset and the
        // first word describes an unset/set platform of bits
        if ((mode == Mode::AllSet && bit == 0) ||
            (mode == Mode::AllUnset && bit == 1)) {
            return const_iterator(_bits, 1, _bits->_platforms[0], 1 - bit);
        }

        return const_iterator(_bits, 0, 0, bit);
    }

    const_iterator end() const {
        return const_iterator(_bits, 0, _bits->GetSize(), 0);
    }

    /// Return true, if the view is empty.
    ///
    bool IsEmpty() const {
        return begin() == end();
    }

private:

    // The TfCompressedBits can create new views.
    friend class TfCompressedBits;

    // Ctor.
    View(const TfCompressedBits *bits) :
        _bits(bits)
    {}

    const TfCompressedBits *_bits;
};

// Specialize the platform view because its iterators are much simpler than
// the per-bit views.
template <>
class TfCompressedBits::View<TfCompressedBits::Mode::Platforms>
{
public:
    class const_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const uint32_t;
        using reference = const uint32_t &;
        using pointer = const uint32_t *;
        using difference_type = const uint32_t;

        const_iterator() :
            _platform(nullptr),
            _bitIndex(0),
            _value(0)
        {}

        reference operator*() const { return dereference(); }
        pointer operator->() const { return &(dereference()); }

        const_iterator& operator++() {
            increment();
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator r(*this);
            increment();
            return r;
        }

        bool operator==(const const_iterator& rhs) const {
            return equal(rhs);
        }

        bool operator!=(const const_iterator& rhs) const {
            return !equal(rhs);
        }

        bool IsSet() const {
            return _value == 1;
        }

        uint32_t GetPlatformSize() const {
            return *_platform;
        }

    private:
        friend class View;

        const_iterator(
            const uint32_t *platform,
            uint8_t value)
            : _platform(platform)
            , _bitIndex(0)
            , _value(value)
        {}

        bool equal(const const_iterator &rhs) const {
            return _platform == rhs._platform;
        }

        void increment() {
            _bitIndex += *_platform;
            ++_platform;
            _value = 1 - _value;
        }

        const uint32_t &dereference() const {
            return _bitIndex;
        }

        const uint32_t *_platform;
        uint32_t _bitIndex;
        uint8_t _value;
    };

    const_iterator begin() const {
        return const_iterator(_bits->_platforms.Begin(), _bits->_runningBit);
    }

    const_iterator end() const {
        return const_iterator(_bits->_platforms.End(), 0);
    }

    /// Return true, if the view is empty.
    ///
    bool IsEmpty() const {
        return begin() == end();
    }

private:

    // The TfCompressedBits can create new views.
    friend class TfCompressedBits;

    // Ctor.
    View(const TfCompressedBits *bits) :
        _bits(bits)
    {}

    const TfCompressedBits *_bits;
};

TfCompressedBits::AllView
TfCompressedBits::GetAllView() const
{
    return View<Mode::All>(this);
}

TfCompressedBits::AllSetView
TfCompressedBits::GetAllSetView() const
{
    return View<Mode::AllSet>(this);
}

TfCompressedBits::AllUnsetView
TfCompressedBits::GetAllUnsetView() const
{
    return View<Mode::AllUnset>(this);
}

TfCompressedBits::PlatformsView
TfCompressedBits::GetPlatformsView() const
{
    return View<Mode::Platforms>(this);
}

// Specializing, so TfIterator knows to retain a copy when iterating.
template<>
struct Tf_ShouldIterateOverCopy<TfCompressedBits::AllView> :
    std::true_type
{};

template<>
struct Tf_ShouldIterateOverCopy<TfCompressedBits::AllSetView> :
    std::true_type
{};

template<>
struct Tf_ShouldIterateOverCopy<TfCompressedBits::AllUnsetView> :
    std::true_type
{};

//! \brief Output a TfBits, as a stream of 0s and 1s.
// \ingroup group_tf_DebuggingOutput
inline std::ostream&
operator<<(std::ostream &out, const TfCompressedBits &bits) {
    out << bits.GetAsStringLeftToRight();
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
