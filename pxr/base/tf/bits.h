//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_BITS_H
#define PXR_BASE_TF_BITS_H

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfBits
///
/// \brief Fast bit array that keeps track of the number of bits set and 
/// can find the next set in a timely manner. 
///
/// Note about thread safety in this class:
///
/// TfBits supports only the most basic thread safety guarantee: multiple
/// threads may safely call const methods concurrently.  A thread must not
/// invoke any non-const method on a TfBits object while any other thread is
/// accessing it.
///
/// There are certain members in this class that are mutable and modified in
/// const methods.  However, since they are updated without being read and
/// all threads would update them with the same values in the case of a race
/// condition, the worst thing that can happen is redundant computation.
///
/// Successive access to these members will result in read only access
/// patterns.
///
/// All occurances are marked.
///
class TfBits
{
public:

    // View and iterator modes: All bits, all set bits, all unset bits.
    enum Mode { All, AllSet, AllUnset };

    /// Hash for TfBits.
    ///
    /// This hash is linear in time as it considers all the bits between
    /// first set and last set.  If you need a faster hash, see FastHash,
    /// it may be suitable for your needs.
    ///
    struct Hash {
        size_t operator()(TfBits const &bits) const {
            return bits.GetHash();
        }
    };

    /// A hash functor for TfBits that is faster than Hash.
    ///
    /// This hash uses the number of bits in total, the number of bits
    /// set, the first set and last set to compute the hash.
    ///
    struct FastHash {
        size_t operator()(TfBits const &bits) const {
            return TfHash::Combine(
                bits.GetSize(),
                bits.GetFirstSet(),
                bits.GetLastSet(),
                bits.GetNumSet());
        }
    };


    /// Constructs a fixed size bit array, clears all bits.
    ///
    explicit TfBits(size_t num=0)
    {
        _bits     = NULL;
        _numWords = 0;
        Resize(num);
        ClearAll();
    }

    /// Constructs a fixed size bit array, with a range of bits set.
    ///
    TfBits(size_t num, size_t first, size_t last)
    {
        _bits     = NULL;
        _numWords = 0;
        Resize(num);
        
        if (num == 0) {
            ClearAll();
        } else if (first == 0 && last >= (num - 1)) {
            SetAll();
        } else {
            ClearAll();
            for (size_t i = first; i <= last; ++i)
                Set(i);
        }
    }

    /// Copy-constructs a fixed size bit array.
    ///
    TfBits(const TfBits &rhs)
    {
        _num      = rhs._num;
        _numSet   .Store(rhs._numSet.Load());
        _firstSet .Store(rhs._firstSet.Load());
        _lastSet  .Store(rhs._lastSet.Load());
        _numWords = rhs._numWords;
        _bits     = _Alloc(_numWords);

        // This loop turns out to be faster than a memcpy.
        for (size_t i = 0; i < _numWords; ++i)
            _bits[i] = rhs._bits[i];
    }

    /// Move constructor.
    ///
    TfBits(TfBits &&rhs) : TfBits(0)
    {
        Swap(rhs);
    }

    /// Destructor
    ///
    ~TfBits()
    {
        _Free(_bits, _numWords);
    }

    /// Assignment operator
    ///
    TfBits &operator=(const TfBits &rhs)
    {
        // Early bail out.
        if (this == &rhs) {
            return *this;
        }

        // Avoid free-ing and reallocing if we have the same size
        if (_numWords != rhs._numWords) {
            _Free(_bits, _numWords);
            _bits = _Alloc(rhs._numWords);
        }

        _num      = rhs._num;
        _numSet   .Store(rhs._numSet.Load());
        _firstSet .Store(rhs._firstSet.Load());
        _lastSet  .Store(rhs._lastSet.Load());
        _numWords = rhs._numWords;

        // This loop turns out to be faster than a memcpy.
        for (size_t i = 0; i < _numWords; ++i)
            _bits[i] = rhs._bits[i];

        return (*this);
    }

    /// Move assignment operator.
    ///
    TfBits &operator=(TfBits &&rhs)
    {
        if (this == &rhs)
            return *this;

        Swap(rhs);

        return *this;
    }

    /// Resizes the bit array, however, the bits are left uninitialized.
    /// So you most likely want to call ClearAll(); or SetAll();.
    /// 
    void Resize(size_t num)
    {
        if (_bits && _num == num)
            return;
            
        _Free(_bits, _numWords);

        _num      = num;
        _numSet   .Store(-1);
        _firstSet .Store(-1);
        _lastSet  .Store(-1);
        _numWords = (num + 63) >> 6;
        _bits     = _Alloc(_numWords);

        // By definition, the unused, trailing bits always needs to be
        // initialized to 0 and all operations can assume they are 0.

        if (_numWords)
            _bits[_numWords - 1] = 0;
    }

    /// Resizes the size of the bit array while keeping the content.
    ///
    void ResizeKeepContent(size_t num)
    {
        if (num == _num)
            return;

        //XXX: We could try to be fancy and not re-allocate in certain cases.
        TfBits temp(num);

        // Figure out how much to copy.
        size_t numWordsToCopy = TfMin(temp._numWords, _numWords);

        for(size_t i=0; i<numWordsToCopy; ++i)
            temp._bits[i] = _bits[i];

        // Since we copy whole words above, we may need to clear out some
        // trailing bits that have been copied when they shouldn't have.

        if (num < _num) {

            temp._ClearTrailingBits();

            // Since we just have written directly to the _bits[] array, all 
            // cached information is invalid, so we need to mark it as such.
            temp._numSet.Store(-1);
            temp._firstSet.Store(-1);
            temp._lastSet.Store(-1);

        } else {

            // Since in this case the bit array became bigger, we can keep the
            // cached information.  Need to be careful to keep the end markers
            // as end markers.
            temp._numSet.Store(_numSet.Load());
            const size_t firstSet = _firstSet.Load();
            temp._firstSet.Store(firstSet < _num ? firstSet : num);
            const size_t lastSet = _lastSet.Load();
            temp._lastSet.Store(lastSet < _num ? lastSet : num);
        }

        // Swap out our content /w the resized bits and delete the old one
        // when exiting this scope.
        Swap(temp);
    }
    
    /// Combines two differently sized TfBits using an or operator.  This
    /// can be used if GetSize() >= rhs.GetSize().  This is more efficient than
    /// padding \p rhs to the correct size beforehand.
    /// 
    TF_API
    void OrSubset(const TfBits &rhs);

    /// Provides a fast swap.
    ///
    void Swap(TfBits &rhs)
    {
        if (this == &rhs)
            return;

        std::swap(_num,        rhs._num);

        // Because Swap is a mutating operation, we do not require atomic
        // updates to the set-bits members.
        _numSet.NonAtomicSwap(rhs._numSet);
        _firstSet.NonAtomicSwap(rhs._firstSet);
        _lastSet.NonAtomicSwap(rhs._lastSet);

        if (_numWords == 1 && rhs._numWords == 1) {

            // Both sides use inline storage.
            //
            // We can just swap the inline data.  Both _bits & rhs._bits will
            // already point their respective inline storage.

            std::swap(_inlineData, rhs._inlineData);

        } else if (_numWords == 1) {

            // 'this' uses inline storage; 'rhs' uses heap-allocated storage.
            //
            // Transfer rhs's heap-allocated data to ourself and copy our inline
            // data to rhs.  We leave our _inlineData unchanged as it is now
            // essentially garbage.

            _bits = rhs._bits;
            rhs._inlineData = _inlineData;
            rhs._bits = &rhs._inlineData;

        } else if (rhs._numWords == 1) {

            // 'rhs' uses inline storage; 'this' uses heap-allocated storage.
            //
            // Transfer our heap-allocated data to rhs and copy rhs's inline
            // data to our inline storage.  We leave rhs._inlineData unchanged
            // as it is now essentially garbage.

            rhs._bits = _bits;
            _inlineData = rhs._inlineData;
            _bits = &_inlineData;

        } else {

            // Both sides use heap-allocated storage.
            //
            // We can just swap the _bits pointers and ignore _inlineData.

            std::swap(_bits, rhs._bits);

        }

        // Swap _numWords only after swapping data.  Otherwise, reasoning about
        // whose _bits & _inlineData to update gets confusing.
        std::swap(_numWords, rhs._numWords);
    }

    // Swap overload for unqualified calls in generic code.
    //
    friend void swap(TfBits &lhs, TfBits &rhs) {
        lhs.Swap(rhs);
    }

    /// Clears all bits to zero.
    ///
    void ClearAll()
    {
        memset(_bits, 0x00, _numWords << 3);
        _numSet.Store(0);
        _firstSet.Store(_num);
        _lastSet.Store(_num);
    }
    
    /// Sets all bits to one.
    ///
    void SetAll()
    {
        memset(_bits, 0xff, _numWords << 3);
        _numSet.Store(_num);
        _firstSet.Store(0);
        _lastSet.Store(_num > 0 ? _num-1 : 0);

        // Clear out unused bits...
        _ClearTrailingBits();
    }

    /// Clears bit # index to zero.
    ///
    void Clear(size_t index)
    {
        TF_AXIOM(index < _num);

        uint64_t mask = UINT64_C(1) << (index & 63);

        if (_bits[index >> 6] & mask)
        {
            const size_t numSet = _numSet.Load();
            TF_AXIOM(numSet == size_t(-1) || numSet > 0);

            if (numSet != size_t(-1))
                _numSet.Decrement();
            if (index == _firstSet.Load())
                _firstSet.Store(-1);
            if (index == _lastSet.Load())
                _lastSet.Store(-1);

            _bits[index >> 6] ^= mask;
        }
    }

    /// Sets bit # index to one.
    ///
    void Set(size_t index)
    {
        TF_AXIOM(index < _num);

        uint64_t mask = UINT64_C(1) << (index & 63);

        if (!(_bits[index >> 6] & mask))
        {
            const size_t numSet = _numSet.Load();
            TF_AXIOM(numSet == size_t(-1) || numSet < _num);

            if (numSet != size_t(-1))
                _numSet.Increment();
            if (index < _firstSet.Load())
                _firstSet.Store(index);
            const size_t lastSet = _lastSet.Load();
            if (index > lastSet || lastSet == _num)
                _lastSet.Store(index);

            _bits[index >> 6] |= mask;
        }
    }

    /// Assigns val to bit # index.
    ///
    void Assign(size_t index, bool val)
    {
        if (val)
            Set(index);
        else
            Clear(index);
    }

    /// Returns true, if bit # index is set.
    ///
    bool IsSet(size_t index) const
    {
        TF_AXIOM(index < _num);

        return _bits[index >> 6] & (UINT64_C(1) << (index & 63));
    }

    /// Finds the next set bit that has a higher or equal index than index.
    /// If no more set bits are found, index returns 'GetSize()'.
    ///
    size_t FindNextSet(size_t index) const
    {
        if (ARCH_UNLIKELY(index >= _num)) {
            return _num;
        }

        size_t startBit = index & 63;

        // Early out for bit set...
        if (_bits[index >> 6] & (UINT64_C(1) << startBit))
            return index;

        return _FindNextSet(index, startBit);
    }

    /// Finds the prev set bit that has a lower or equal index than index.
    /// If no more set bits are found, index returns 'GetSize()'.
    ///
    size_t FindPrevSet(size_t index) const
    {
        if (ARCH_UNLIKELY(index >= _num)) {
            return _num;
        }

        size_t startBit = index & 63;

        // Early out for bit set...
        if (_bits[index >> 6] & (UINT64_C(1) << startBit))
            return index;

        return _FindPrevSet(index, startBit);
    }

    /// Finds the next unset bit that has a higher or equal index than index.
    /// If no more set bits are found, index returns 'GetSize()'.
    ///
    size_t FindNextUnset(size_t index) const
    {
        if (ARCH_UNLIKELY(index >= _num)) {
            return _num;
        }

        size_t startBit = index & 63;

        // Early out for bit set...
        if (!(_bits[index >> 6] & (UINT64_C(1) << startBit)))
            return index;

        return _FindNextUnset(index, startBit);
    }

    /// Returns the size of the bit array, ie. the # of bits it can hold.
    ///
    size_t GetSize() const
    {
        return _num;
    }
    
    /// Returns \c true if this bit array is empty, i.e. it is of size zero.
    ///
    bool IsEmpty() const 
    {
        return _num == 0;
    }

    /// Returns the index of the first bit set in the bit array.  If no bits
    /// are set, the return value is 'GetSize()'.
    ///
    size_t GetFirstSet() const
    {
        // See comment at top of this file on why this is thread safe.
        size_t firstSet = _firstSet.Load();
        if (firstSet == size_t(-1)) {
            firstSet = FindNextSet(0);
            _firstSet.Store(firstSet);
        }

        return firstSet;
    }

    /// Returns the index of the last bit set in the bit array.  If no bits
    /// are set, the return value is 'GetSize()'.
    ///
    size_t GetLastSet() const
    {
        // See comment at top of this file on why this is thread safe.
        size_t lastSet = _lastSet.Load();
        if (lastSet == size_t(-1)) {
            // Also works if _num is 0.
            lastSet = FindPrevSet(_num-1);
            _lastSet.Store(lastSet);
        }

        return lastSet;
    }

    /// Returns the number of bits currently set in this array.
    ///
    size_t GetNumSet() const
    {
        // See comment at top of this file on why this is thread safe.
        size_t numSet = _numSet.Load();
        if (numSet == size_t(-1)) {
            numSet = _CountNumSet();
            _numSet.Store(numSet);
        }

        return numSet;
    }

    /// Returns true, if all the bits in this bit array are set.
    ///
    bool AreAllSet() const
    {
        // Note that "not IsAnyUnset();" is not cached because FindNextUnset(0);
        // isn't. Therefore we use GetNumSet() which is cached.
        return GetNumSet() == GetSize();
    }

    /// Returns true, if all the bits in this bit array are unset.
    ///
    bool AreAllUnset() const
    {
        return !IsAnySet();
    }

    /// Returns true, if there is at least a single set bit.
    /// 
    bool IsAnySet() const 
    {
        return GetFirstSet() < GetSize();
    }

    /// Returns true, if there is at least a single unset bit.
    /// 
    bool IsAnyUnset() const 
    {
        return !AreAllSet();
    }

    /// Returns true if the set bits in this bit array are contiguous.
    ///
    /// Note: This returns false if there are no set bits.
    ///
    bool AreContiguouslySet() const
    {
        return GetNumSet() == GetLastSet() - GetFirstSet() + 1;
    }

    /// Returns the amount of memory this object holds on to.
    ///
    size_t GetAllocatedSize() const
    {
        size_t memUsed = sizeof(TfBits);

        // Note that up to 64 bits are inlined, cf. _Alloc();
        if (_numWords > 1)
            memUsed += _numWords << 3;

        return memUsed;
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

    /// \name Operators
    /// @{

    /// Returns true if this == \p rhs.
    ///
    TF_API
    bool operator==(const TfBits &rhs) const;

    /// Returns true if this != \p rhs.
    ///
    bool operator!=(const TfBits &rhs) const
    {
        return !(*this == rhs);
    }

    /// Ands these bits with the \p rhs bits.
    ///
    /// The resulting bit set is the intersection of the two bit sets.
    ///
    TF_API
    TfBits &operator&=(const TfBits &rhs);

    /// Returns these bits and'ed with \p rhs.
    ///
    TfBits operator&(const TfBits &rhs) const
    {
        TfBits r(*this);
        r &= rhs;
        return r;
    }

    /// Ors these bits with the \p rhs bits.
    ///
    /// The resulting bit set is the union of the two bit sets.
    ///
    TF_API
    TfBits &operator|=(const TfBits &rhs);

    /// Returns these bits or'ed with \p rhs.
    ///
    TfBits operator|(const TfBits &rhs) const
    {
        TfBits r(*this);
        r |= rhs;
        return r;
    }

    /// Xors these bits with the \p rhs bits.
    ///
    /// The resulting bit set is the union of the two bit sets minus the
    /// intersection of the two bit sets.
    ///
    TF_API
    TfBits &operator^=(const TfBits &rhs);

    /// Returns these bits xor'ed with \p rhs. 
    ///
    TfBits operator^(const TfBits &rhs) const
    {
        TfBits r(*this);
        r ^= rhs;
        return r;
    }

    /// Removes all bits in the \p rhs bits from these bits.
    ///
    /// The resulting bit set is the asymmetric set difference of
    /// the two bit sets.
    ///
    TF_API
    TfBits &operator-=(const TfBits &rhs);

    /// Flips all bits.
    ///
    /// The resulting bit set is the complement of this bit set.
    ///
    TF_API
    TfBits &Complement();

    /// Returns bit at \p index.
    ///
    bool operator[](size_t index) const
    {
        return IsSet(index);
    }

    /// @}


    /// Returns true if the result of the intersection with \p rhs would be
    /// non-zero.
    ///
    /// This method can be used for efficiency because it doesn't perform
    /// the full AND operation on a copy, and it can return early.
    ///
    bool HasNonEmptyIntersection(const TfBits &rhs) const
    {
        TF_AXIOM(_num == rhs._num);

        // Limit the bit operations to where we have bits set in both of
        // the two sets.
        size_t firstSet    = GetFirstSet();
        size_t rhsFirstSet = rhs.GetFirstSet();

        // Nothing to compare if either set is empty.
        if (firstSet < _num && rhsFirstSet < _num)
        {
            firstSet       = TfMax(firstSet, rhsFirstSet);
            size_t lastSet = TfMin(GetLastSet(), rhs.GetLastSet());

            if (firstSet <= lastSet)
            {
                size_t offset   = firstSet >> 6;
                size_t numWords = (lastSet >> 6) + 1 - offset;

                // Have to compare the bits.
                uint64_t *p0 = _bits + offset;
                uint64_t *p1 = rhs._bits + offset;

                for(size_t n=numWords; n>0; n--)
                {
                    // Note: This assumes trailing bits in last word to be zero.
                    if (uint64_t word = *p0)
                        if (word & *p1)
                            return true;
                    p0++;
                    p1++;
                }
            }
        }

        return false;
    }

    /// Returns true if the result of an asymmetric set different is non-zero.
    /// This is the equivalent to computing:
    ///    return (this - rhs).GetNumSet() != 0
    /// but avoids creating temporary copies.
    ///
    bool HasNonEmptyDifference(const TfBits &rhs) const 
    {
        TF_AXIOM(_num == rhs._num);

        // Limit the bit operations to where we have bits set in the first set.
        size_t firstSet = GetFirstSet();

        // The difference is empty if the first set is empty.
        if (firstSet < _num)
        {
            size_t lastSet     = GetLastSet();
            size_t rhsFirstSet = rhs.GetFirstSet();
            size_t rhsLastSet  = rhs.GetLastSet();

            // Check for trivial non-empty difference (we know that the first
            // set is not empty).
            if (firstSet < rhsFirstSet || lastSet > rhsLastSet ||
                firstSet > rhsLastSet || lastSet < rhsFirstSet ||
                GetNumSet() > rhs.GetNumSet())
                return true;

            size_t offset   = firstSet >> 6;
            size_t numWords = (lastSet >> 6) + 1 - offset;

            // Have to compare the bits.
            uint64_t *p0 = _bits + offset;
            uint64_t *p1 = rhs._bits + offset;

            for(size_t n=numWords; n>0; n--)
            {
                // Note: This assumes trailing bits in last word to be the same.
                if (uint64_t word = *p0)
                    if (word & ~*p1)
                        return true;
                p0++;
                p1++;
            }
        }

        return false;
    }

    /// Returns true if this bit array contains \p rhs by computing:
    /// (rhs - this).GetNumSet() == 0.
    /// 
    /// Ie. it will return true if all bits of \p rhs are also set in this.
    ///
    bool Contains(const TfBits &rhs) const
    {
        return !rhs.HasNonEmptyDifference(*this);
    }

    /// Iterator support.
    ///
    template <Mode mode>
    class View
    {
    public:
        class const_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = const size_t;
            using reference = const size_t &;
            using pointer = const size_t *;
            using difference_type = const size_t;

            const_iterator()
            :   _bits(NULL), _index(0) {}

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
    
        private:

            friend class View;
    
            // Ctor.
            const_iterator(
                const TfBits *bits, size_t index)
            :   _bits(bits), _index(index) {}

            bool equal(const const_iterator &rhs) const {
                return _bits == rhs._bits && _index == rhs._index;
            }
    
            void increment() {
                ++_index;

                if (mode == AllSet)
                    _index = _bits->FindNextSet(_index);
                else if (mode == AllUnset)
                    _index = _bits->FindNextUnset(_index);
            }
    
            const size_t &dereference() const {
                return _index;
            }
    
        private:
            
            // The bits being iterated over.
            const TfBits *_bits;
    
            // The index.
            size_t _index;
        };
    
        // Support for TF_FOR_ALL.
        typedef const_iterator iterator;
    
        const_iterator begin() const {
            size_t start = 0;
            if (mode == AllSet)
                start = _bits->GetFirstSet();
            else if (mode == AllUnset)
                start = _bits->FindNextUnset(0);

            return const_iterator(_bits, start);
        }
    
        const_iterator end() const {
            return const_iterator(_bits, _bits->GetSize());
        }

        /// Return true, if the view is empty.
        ///
        bool IsEmpty() const {
            return begin() == end();
        }

    private:

        // The TfBits can create new views.
        friend class TfBits;

        // Ctor.
        View(const TfBits *bits)
        :   _bits(bits) {}
            
        const TfBits *_bits;
    };

    using AllView = View<All>;
    using AllSetView = View<AllSet>;
    using AllUnsetView = View<AllUnset>;

    /// Returns an iteratable view for the bits that steps over all bits.
    ///
    AllView GetAllView() const {
        return AllView(this);
    }

    /// Returns an iteratable view for the bits that steps over all set bits.
    ///
    AllSetView GetAllSetView() const {
        return AllSetView(this);
    }

    /// Returns an iteratable view for the bits that steps over all unset bits.
    ///
    AllUnsetView GetAllUnsetView() const {
        return AllUnsetView(this);
    }

// -----------------------------------------------------------------------------

private:

    // This counts the number of set bits.
    TF_API
    size_t _CountNumSet() const;

    // This is a helper method for FindNextSet so that we don't have to inline
    // the whole method.  This gives us the best compromise for speed and code
    // size.
    TF_API
    size_t _FindNextSet(size_t index, size_t startBit) const;

    // This is a helper method for FindPrevSet so that we don't have to inline
    // the whole method.  This gives us the best compromise for speed and code
    // size.
    TF_API
    size_t _FindPrevSet(size_t index, size_t startBit) const;

    // This is a helper method for FindNextUnset so that we don't have to inline
    // the whole method.  This gives us the best compromise for speed and code
    // size.
    TF_API
    size_t _FindNextUnset(size_t index, size_t startBit) const;

    // This is a helper method that clear out unused bits in the last word of
    // the bit array.
    TF_API
    void _ClearTrailingBits();

    // Helper that performs the or operation on these bits where rhs must have
    // same or less # of bits.
    TF_API
    void _Or(const TfBits &rhs);

    // Allocates the bits array with \p numWords words.
    // If \p numWords is 0, NULL is returned. If \p numWords is 1, inline 
    // data will be used (to avoid an extra malloc).
    // Returned memory must be freed with _Free().

    uint64_t *_Alloc(size_t numWords)
    {
        if (!numWords)
            return NULL;

        if (numWords == 1)
            return &_inlineData;

        return new uint64_t[numWords];
    }

    // Frees data allocated with _Alloc().
    static void _Free(uint64_t *data, size_t numWords)
    {
        if (numWords > 1)
            delete [] data;
    }

private:

    // # of bits in this array.
    size_t _num;

    // Wrapper class for lazily-initialized size_t members.
    //
    // These members only require relaxed ordering and we want to avoid
    // unintentionally scribbling mfence all over the place with the
    // sequentially consistent std::atomic operator=(size_t).
    class _RelaxedAtomicSize_t
    {
    public:
        _RelaxedAtomicSize_t()
            : _n{}
        {}

        explicit _RelaxedAtomicSize_t(size_t n)
            : _n{n}
        {}

        void Increment() {
            _n.fetch_add(1, std::memory_order_relaxed);
        }

        void Decrement() {
            _n.fetch_sub(1, std::memory_order_relaxed);
        }

        size_t Load() const {
            return _n.load(std::memory_order_relaxed);
        }

        void Store(size_t n) {
            _n.store(n, std::memory_order_relaxed);
        }

        // Note, it's not possible to do an atomic swap of two memory
        // locations.  Provide a non-atomic swap operation to be used when
        // no concurrent operations may be taking place.  See TfBits::Swap.
        void NonAtomicSwap(_RelaxedAtomicSize_t &other) {
            const size_t n = _n.load(std::memory_order_relaxed);
            const size_t o = other._n.load(std::memory_order_relaxed);
            _n.store(o, std::memory_order_relaxed);
            other._n.store(n, std::memory_order_relaxed);
        }

    private:
        std::atomic<size_t> _n;
    };

    // See comment at top of this file on why the usage of _numSet, _firstSet
    // and _lastSet is thread safe.

    // # of bits set in this array (set to size_t(-1) when invalid).
    mutable _RelaxedAtomicSize_t _numSet;

    // Cached first and last set bits (set to size_t(-1) when invalid).
    mutable _RelaxedAtomicSize_t _firstSet;
    mutable _RelaxedAtomicSize_t _lastSet;

    // Size in uint64_t of the bits array.
    size_t _numWords;

    // Pointer to the actual data.
    uint64_t *_bits;

    // Data used if _num <= 64.
    uint64_t _inlineData;
};

// Specialize this template so TfIterator knows to retain a copy when iterating.
template<>
struct Tf_ShouldIterateOverCopy< TfBits::AllView > :
    std::true_type
{
};

template<>
struct Tf_ShouldIterateOverCopy< TfBits::AllSetView > :
    std::true_type
{
};

template<>
struct Tf_ShouldIterateOverCopy< TfBits::AllUnsetView > :
    std::true_type
{
};

//! \brief Output a TfBits, as a stream of 0s and 1s.
// \ingroup group_tf_DebuggingOutput
TF_API std::ostream & operator<<(std::ostream &out, const TfBits & bits);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
