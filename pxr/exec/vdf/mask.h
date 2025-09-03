//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_MASK_H
#define PXR_EXEC_VDF_MASK_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"

#include "pxr/base/tf/compressedBits.h"
#include "pxr/base/tf/delegatedCountPtr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"

#include <atomic>
#include <functional>
#include <iosfwd>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfMask
///
/// \brief A VdfMask is placed on connections to specify the data flowing
/// through them.
///
class VdfMask
{
    class _BitsImpl;

public:
    /// Typedef on the internal bitset implementation used.
    ///
    typedef TfCompressedBits Bits;
    
    /// Constructs an empty mask.
    ///
    VdfMask() {}
    
    /// Constructs a mask of size \p size.
    ///
    explicit VdfMask(size_t size) {
        // If size == 0, we want to leave the optional _bits uninitialized.
        // This is important, because we use the uninitialized optional as a
        // sentinal for a zero size mask, and we need to be consistent about
        // that.
        if (size != 0) {
            _bits = _FindOrInsert(VdfMask::Bits(size));
        }
    }

    /// Constructs a mask from VdfMask::Bits.
    ///
    explicit VdfMask(VdfMask::Bits const &bits) {
        // If the bits denote a 1x1 mask, use the static 1x1 mask for
        // initialization. This prevents a call to _FindOrInsert, which locks
        // on the mask registry.
        if (bits.GetSize() == 1 && bits.AreAllSet()) {
            *this = _GetAllOnes1();
        }
        
        // If size == 0, we want to leave the optional _bits uninitialized.
        // This is important, because we use the uninitialized optional as a
        // sentinal for a zero size mask, and we need to be consistent about
        // that.
        else if (bits.GetSize() != 0) {
            _bits = _FindOrInsert(bits);
        }
    }

    /// Constructs a mask by *moving* the contents of \p bits into
    /// the mask.
    ///
    /// \p *bits may not be used after passing it to this constructor.
    ///
    explicit VdfMask(VdfMask::Bits &&bits) {
        if (bits.GetSize() != 0) {
            _bits = _FindOrEmplace(std::move(bits));
        }
    }

    /// Swap this mask's bits with \p rhs.
    ///
    void Swap(VdfMask &rhs) noexcept {
        _bits.swap(rhs._bits);
    }

    /// Swap \p lhs's bits with \p rhs.
    ///
    friend void swap(VdfMask &lhs, VdfMask &rhs) noexcept {
        lhs.Swap(rhs);
    }

    /// Enables all the bits in the mask.
    ///
    void SetAll() {
        if (!_bits) {
            return;
        }

        VdfMask::Bits bits(_bits->Get().GetSize());
        bits.Complement();
        _bits = _FindOrInsert(bits);
    }
    
    /// Adds the given \p index to the mask.
    ///
    /// The corresponding element will be set after this call.
    ///
    void SetIndex(size_t index) {
        if (!TF_VERIFY(_bits)) {
            return;
        }

        VdfMask::Bits copy(_bits->Get());
        copy.Set(index);
        _bits = _FindOrEmplace(std::move(copy));
    }

    /// Returns true if mask at index is set
    ///
    bool IsSet(size_t index) const {
        if (!TF_VERIFY(_bits)) {
            return false;
        }

        return _bits->Get().IsSet(index);
    }

    /// Removes the given \p index from the mask.
    ///
    /// The corresponding element will be cleared after this call.
    ///
    void ClearIndex(size_t index) {
        if (!TF_VERIFY(_bits)) {
            return;
        }

        VdfMask::Bits copy(_bits->Get());
        copy.Clear(index);
        _bits = _FindOrEmplace(std::move(copy));
    }

    /// Returns the size of the mask.
    ///
    /// This is the number of elements that can be indexed in the mask,
    /// not the number of elements set.
    ///
    size_t GetSize() const {
        if (!_bits) {
            return 0;
        }

        return _bits->Get().GetSize(); 
    }

    /// Returns \c true if this mask is empty, i.e. it is of size zero.
    ///
    bool IsEmpty() const {
        return !_bits;
    }

    /// Returns \c true if this mask and \p mask have any set entries in
    /// common, and \c false otherwise.
    ///
    bool Overlaps(const VdfMask &mask) const {
        if (_bits == mask._bits) {
            return IsAnySet();
        }

        return GetBits().HasNonEmptyIntersection(mask.GetBits());
    }

    /// Returns \c true if \p mask is a subset-of or equal to this mask,
    /// \c false otherwise.
    ///
    bool Contains(const VdfMask &mask) const {
        if (_bits == mask._bits) {
            return true;
        }

        return !mask.GetBits().HasNonEmptyDifference(GetBits());
    }

    /// Returns true if this mask has all entries set.
    ///
    bool IsAllOnes() const {
        if (!_bits) {
            return true;
        }

        return _bits->Get().AreAllSet();
    }

    /// Returns true if this mask has all entries unset.
    ///
    bool IsAllZeros() const {
        if (!_bits) {
            return true;
        }

        return _bits->Get().AreAllUnset();
    }

    /// Returns true, if there is at least a single set entry.
    /// 
    bool IsAnySet() const {
        if (!_bits) {
            return false;
        }

        return _bits->Get().IsAnySet();
    }

    /// Returns the first set bit in the mask.
    ///
    size_t GetFirstSet() const {
        if (!_bits) {
            return 0;
        }

        return _bits->Get().GetFirstSet();
    }

    /// Returns the last set bit in the mask.
    ///
    size_t GetLastSet() const {
        if (!_bits) {
            return 0;
        }

        return _bits->Get().GetLastSet();
    }

    /// Returns the number of set bits in the mask.
    ///
    size_t GetNumSet() const {
        if (!_bits) {
            return 0;
        }

        return _bits->Get().GetNumSet();
    }

    /// Returns true if the set bits in the mask are contiguous.
    ///
    /// Note: This returns false if there are no set bits in the mask.
    ///
    bool IsContiguous() const {
        if (!_bits) {
            return false;
        }

        return _bits->Get().AreContiguouslySet();
    }

    /// \name Operators
    /// @{

    /// Returns true if this and \p rhs are equal, false otherwise.
    ///
    bool operator==(const VdfMask &rhs) const {
        return _bits == rhs._bits;
    }

    bool operator!=(const VdfMask &rhs) const {
        return !(*this == rhs);
    }

    /// Arbitrary total ordering of masks.  The order does not depend on the
    /// actual mask values and may change from run to run.
    ///
    struct ArbitraryLessThan
    {
        bool operator()(const VdfMask &lhs, const VdfMask &rhs) const
        {
            return lhs._bits < rhs._bits;
        }
    };

    /// Ands two masks together.
    ///
    /// The result is that an element is set iff it is set in both masks.
    ///
    VdfMask &operator&=(const VdfMask &rhs) {
        if (_bits == rhs._bits) {
            return *this;
        }

        _bits = _FindOrEmplace(GetBits() & rhs.GetBits());
        return *this;
    }

    VdfMask operator&(const VdfMask &rhs) const {
        VdfMask r(*this);
        r &= rhs;
        return r;
    }

    /// Ors two masks together.
    ///
    /// The result is that an element is set iff it is set in either mask.
    ///
    VdfMask &operator|=(const VdfMask &rhs) {
        if (_bits == rhs._bits) {
            return *this;
        }

        _bits = _FindOrEmplace(GetBits() | rhs.GetBits());
        return *this;
    }

    VdfMask operator|(const VdfMask &rhs) const {
        VdfMask r(*this);
        r |= rhs;
        return r;
    }

    /// Xors two masks together.
    ///
    /// The result is that an element is set iff it is set in exactly one
    /// of the two masks.
    ///
    VdfMask &operator^=(const VdfMask &rhs) {
        if (!_bits && TF_VERIFY(!rhs._bits)) {
            return *this;
        }

        _bits = _FindOrEmplace(GetBits() ^ rhs.GetBits());
        return *this;
    }

    VdfMask operator^(const VdfMask &rhs) const {
        VdfMask r(*this);
        r ^= rhs;
        return r;
    }

    /// Performs an asymmetric set difference.
    ///
    /// This method turns off the bits that are set in both \p this and
    /// in \p rhs.
    ///
    VdfMask &operator-=(const VdfMask &rhs) {
        if (!_bits && TF_VERIFY(!rhs._bits)) {
            return *this;
        }

        VdfMask::Bits copy = GetBits();
        copy -= rhs.GetBits();
        _bits = _FindOrEmplace(std::move(copy));
        return *this;
    }

    VdfMask operator-(const VdfMask &rhs) const {
        VdfMask r(*this);
        r -= rhs;
        return r;
    }

    /// Complement. Flips all the bits in the mask.
    ///
    VdfMask &Complement() {
        if (!_bits) {
            return *this;
        }

        _bits = _FindOrEmplace(
            VdfMask::Bits(_bits->Get(), VdfMask::Bits::ComplementTag));
        return *this;
    }

    /// Sets this mask to \p rhs if this mask is of zero size.  Otherwise, will
    /// or \p rhs to this mask.
    ///
    VdfMask &SetOrAppend(const VdfMask &rhs) {
        if (!_bits) {
            *this = rhs;
        } else {
            *this |= rhs;
        }
        return *this;
    }

    /// @}


    /// Iterator class used to iterate through the elements of the mask.
    ///
    class iterator {
        using _BaseIterator = VdfMask::Bits::AllSetView::const_iterator;

    public:
        using value_type = _BaseIterator::value_type;

        /// Constructs an null iterator that is already at end.
        ///
        iterator() {}

        /// Returns \c true if this iterator and rhs compare equal.
        ///
        bool operator==(const iterator &rhs) const {
            return _it == rhs._it;
        }

        /// Returns \c true if this iterator and rhs do not compare equal.
        ///
        bool operator!=(const iterator &rhs) const {
            return !operator==(rhs);
        }

        /// Returns the index of the current element.
        ///
        value_type operator*() const {
            return *_it;
        }

        /// Increment the iterator to the next element.
        ///
        iterator &operator++() {
            ++_it;
            return *this;
        }

        /// Returns true if the iteration is finished.
        ///
        bool IsAtEnd() const {
            return _it.IsAtEnd();
        }

        /// Advance the iterator to the end.
        ///
        void AdvanceToEnd() {
            _it = _BaseIterator();
        }

        /// Advance the iterator to the first index that is set in the mask
        /// located at or after \p index.
        ///
        int AdvanceTo(value_type index) {
            if (_it.IsAtEnd()) {
                return 0;
            }

            // The index must be ahead of the current iterator position.
            TF_DEV_AXIOM(index >= *_it);

            // We can simply increment the underlying VdfMask::Bits iterator
            // until we reach (past) index.
            while (!_it.IsAtEnd() && *_it < index) {
                ++_it;
            }
            return *_it;
        }

    private:

        // Only a mask is a allowed to create an iterator.
        iterator(const VdfMask::Bits *bits) :
            _it(bits->GetAllSetView().begin())
        {}

        friend class VdfMask;

        // The wrapped VdfMask::Bits iterator
        _BaseIterator _it;
    };


    /// Returns an iterator that can be used to iterate through the elements
    /// of the mask.
    ///
    iterator begin() const {
        if (!_bits) {
            return iterator(&VdfMask::Bits::GetEmpty());
        }

        return iterator(&_bits->Get());
    }


    /// Returns a mask of the requested size that will iterate over all
    /// elements.
    ///
    static VdfMask AllOnes(size_t size) {
        // special-case all-ones of size 1 and 0
        if (size == 0) {
            return VdfMask();
        } else if (size == 1) {
            return _GetAllOnes1();
        }

        VdfMask::Bits bits(size);
        bits.SetAll();
        return VdfMask(bits);
    }

    /// Returns a mask of the requested size where no element is set.
    ///
    static VdfMask AllZeros(size_t size) {
        // special-case all-zeros of size 0
        if (size == 0) {
            return VdfMask();
        }

        return VdfMask(size);
    }

    /// \name Debugging API
    /// @{
    ///

    /// Returns the mask in an RLE format.
    ///
    /// This is useful for debugging large masks.
    /// For example, the output of a mask that is 110001111 would be:
    ///        1x2-0x3-1x4
    ///
    std::string GetRLEString() const {
        if (!_bits) {
            return std::string();
        }

        return _bits->Get().GetAsRLEString();
    }

    /// Returns the amount of memory in bytes used by this mask.  Note that
    /// masks are now shared, so this method is of dubious value.
    ///
    size_t GetMemoryUsage() const {
        if (!_bits) {
            return 0;
        }

        return _bits->Get().GetAllocatedSize();
    }

    /// @}


    /// \name Performance Considerations
    /// @{
    ///

    /// Get this mask's content as CtCompressedfBits.  This should not be used
    /// except where performance is critical.
    VdfMask::Bits const &GetBits() const {
        if (!_bits) {
            return VdfMask::Bits::GetEmpty();
        }

        return _bits->Get();
    }

    /// Returns a hash for the mask.
    size_t GetHash() const {
        return std::hash<_BitsImpl*>()(_bits.get());
    }

    /// Hash Functor.
    ///
    struct HashFunctor {
        size_t operator()(const VdfMask &mask) const {
            return mask.GetHash();
        }
    };
    
    /// @}
    
private:

    struct _AllOnes1Factory
    {
        static VdfMask * New()
        {
            return new VdfMask(_AllOnes1Factory());
        }
    };

    explicit VdfMask(const _AllOnes1Factory &) {
        Bits allOnes1Bits(1);
        allOnes1Bits.SetAll();
        _bits = _FindOrInsert(allOnes1Bits);
    }

    static VdfMask _GetAllOnes1() {
        return *_allOnes1;
    }

    // Befriend the stream operator so that VdfMasks support output streaming.
    friend VDF_API std::ostream & operator<<(
        std::ostream &os, const VdfMask &mask);

    // Refcounted hash table nodes for VdfMask::Bits.
    //
    class _BitsImpl
    {
    public:
        // Non-copyable
        _BitsImpl(const _BitsImpl &) = delete;
        _BitsImpl& operator=(const _BitsImpl &) = delete;

        // Non-movable
        _BitsImpl(_BitsImpl &&) = delete;
        _BitsImpl& operator=(_BitsImpl &&) = delete;

        // Provide const access to the bits.  Flyweighting requires that
        // the value is never mutated.
        //
        const VdfMask::Bits &Get() const { return _bits; }

        // Return the pre-computed hash value for _bits.
        //
        // Implemented in MaskRegistry.h
        //
        size_t GetHash() const;

    private:
        // next points to the next entry in the hash bucket (if any) for
        // \p bits.
        //
        _BitsImpl(_BitsImpl *next, size_t hash, VdfMask::Bits &&bits);

        friend inline void TfDelegatedCountIncrement(_BitsImpl *p) noexcept;
        friend inline void TfDelegatedCountDecrement(_BitsImpl *p) noexcept;

        friend class Vdf_MaskRegistry;

        _BitsImpl *_next;
        size_t _hash;
        VdfMask::Bits _bits;
        std::atomic<int> _refCount;
        // Note that the resurrection count is bounded by the number of threads
        // concurrently accessing masks, thus a 16-bit integer is sufficient.
        std::atomic<uint16_t> _resurrectionCount;
        bool _isImmortal;
    };

    friend inline void TfDelegatedCountIncrement(_BitsImpl *p) noexcept;
    friend inline void TfDelegatedCountDecrement(_BitsImpl *p) noexcept;

    friend class Vdf_MaskRegistry;

    typedef TfDelegatedCountPtr<_BitsImpl> _BitsImplRefPtr;

    // Return a ref ptr to the \c _BitsImpl corresponding to \p *bits.  If an
    // existing _BitsImpl for \p *bits is not found, create a new one by
    // *moving* the contents out of \p *bits and into the new \c _BitsImpl.
    //
    // Do not use \p *bits after passing it to this function.
    //
    VDF_API
    static _BitsImplRefPtr _FindOrEmplace(VdfMask::Bits &&bits);

    // Return a ref ptr to the \c _BitsImpl corresponding to \p bits.  If an
    // existing _BitsImpl for \p bits is not found, create a new one by
    // copying \p bits.
    //
    VDF_API
    static _BitsImplRefPtr _FindOrInsert(const VdfMask::Bits &bits);

    // Erase the _BitsImpl pointed to by \p bits.
    //
    VDF_API
    static void _EraseBits(_BitsImpl *bits);

private:
    // Default constructed / empty masks are represented as null
    // _BitsImplRefPtr.
    _BitsImplRefPtr _bits;

    // AllOnes mask of size 1 optimization.
    VDF_API static TfStaticData<VdfMask, _AllOnes1Factory> _allOnes1;
};

// Specialize TfDelegatedCountPtr operations for VdfMask::_BitsImpl.
inline void TfDelegatedCountIncrement(VdfMask::_BitsImpl *p) noexcept
{
    // For immortal masks there is no need to maintain the reference count.
    if (p->_isImmortal) {
        return;
    }

    // There's no need for a stronger memory ordering here because we can only
    // increase the ref count by way of an existing reference and sharing an
    // existing VdfMask between threads requires external synchronization,
    // just like any other non-atomic type.
    //
    // Note that Vdf_MaskRegistry manages reference counting and serialization
    // for threads that are looking up the same bits concurrently rather than
    // simply making copies of an existing VdfMask.
    p->_refCount.fetch_add(1, std::memory_order_relaxed);
}
inline void TfDelegatedCountDecrement(VdfMask::_BitsImpl *p) noexcept
{
    // For immortal masks there is no need to maintain the reference count.
    if (p->_isImmortal) {
        return;
    }

    // Many threads may decrement the ref count but only one thread will be
    // responsible for deleting it.  However, we must ensure that all of the
    // memory operations in all of the threads happen before the final thread
    // performs the deletion.  To establish this happens-before relationship,
    // we need a release-acquire pair of atomic operations.
    const int prevRC = p->_refCount.fetch_sub(1, std::memory_order_release);
    if (prevRC == 1) {
        // Use an acquire fence here because we only need to synchronize with
        // the decrement accesses when we're about to perform the deletion.
        std::atomic_thread_fence(std::memory_order_acquire);
        VdfMask::_EraseBits(p);
    }
}

// Output stream operator
VDF_API
std::ostream &
operator<<(std::ostream &os, const VdfMask &mask);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
