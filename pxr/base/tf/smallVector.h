//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SMALL_VECTOR_H
#define PXR_BASE_TF_SMALL_VECTOR_H

///
/// \file

#include "pxr/pxr.h"

#include "pxr/base/arch/defines.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

// Contains parts of the small vector implementation that do not depend on
// *all* of TfSmallVector's template parameters.
class TfSmallVectorBase
{
protected:
    // We present the public size_type and difference_type as std::size_t and
    // std::ptrdiff_t to match std::vector, but internally we store size &
    // capacity as uint32_t.
    using _SizeMemberType = std::uint32_t;

    // Union type containing local storage or a pointer to heap storage.
    template <size_t Size, size_t Align, size_t NumLocal>
    union _DataUnion;

    // Helper alias to produce the right _DataUnion instantiation for a given
    // ValueType and NumLocal elements.
    template <class ValueType, size_t NumLocal>
    using _Data = _DataUnion<sizeof(ValueType), alignof(ValueType), NumLocal>;

public:
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    // Returns the local capacity that may be used without increasing the size
    // of the TfSmallVector.  TfSmallVector<T, N> will never use more local
    // capacity than is specified by N but clients that wish to maximize local
    // occupancy in a generic way can compute N using this function.
    template <typename U>
    static constexpr size_type ComputeSerendipitousLocalCapacity() {
        return (alignof(U) <= alignof(_Data<U, 0>))
            ? sizeof(_Data<U, 0>) / sizeof(U)
            : 0;
    }

protected:

    // Enabler used to disambiguate the range-based constructor (begin, end)
    // from the n-copies constructor (size_t n, value_type const &value)
    // when the value_type is integral.
    template<typename _ForwardIterator>
    using _EnableIfForwardIterator =
        std::enable_if_t<
            std::is_convertible_v<
                typename std::iterator_traits<
                    _ForwardIterator>::iterator_category,
                std::forward_iterator_tag
                >
        >;
    
    // Invoke std::uninitialized_copy that either moves or copies entries,
    // depending on whether the type is move constructible or not.
    template <typename Iterator>
    static Iterator _UninitializedMove(
        Iterator first, Iterator last, Iterator dest) {
        return std::uninitialized_copy(
            std::make_move_iterator(first),
            std::make_move_iterator(last),
            dest);
    }

    // Invokes either the move or copy constructor (via placement new),
    // depending on whether U is move constructible or not.
    template <typename U>
    static void _MoveConstruct(U *p, U *src) {
        new (p) U(std::move(*src));
    }

    // The data storage, which is a union of both the local storage, as well
    // as a pointer, holding the address to the remote storage on the heap, if
    // used.
    template <size_t Size, size_t Align, size_t NumLocal>
    union _DataUnion {
    public:
        // XXX: Could in principle assert in calls to GetLocalStorage() when
        // HasLocal is false. Add dependency on tf/diagnostic.h?
        static constexpr bool HasLocal = NumLocal != 0;

        void *GetLocalStorage() {
            return HasLocal ? _local : nullptr;
        }
        const void *GetLocalStorage() const {
            return HasLocal ? _local : nullptr;
        }

        void *GetRemoteStorage() {
            return _remote;
        }
        const void *GetRemoteStorage() const {
            return _remote;
        }

        void SetRemoteStorage(void *p) {
            _remote = p;
        }
    private:
        // Pointer to heap storage.
        void *_remote;
        // Local storage -- min size is sizeof(_remote).
        alignas(NumLocal == 0 ? std::alignment_of_v<void *> : Align)
        char _local[std::max<size_t>(Size * NumLocal, sizeof(_remote))];
    };
};

////////////////////////////////////////////////////////////////////////////////
///
/// \class TfSmallVector
///
/// This is a small-vector class with local storage optimization, the local
/// storage can be specified via a template parameter, and expresses the
/// number of entries the container can store locally.
///
/// In addition to the local storage optimization, this vector is also
/// optimized for storing a smaller number of entries on the heap: It features
/// a reduced memory footprint (minimum 16 bytes) by limiting max_size() to
/// 2^32, which should still be more than enough for most use cases where a 
/// small-vector is advantageous.
///
/// TfSmallVector mimics the std::vector API, and can thus be easily used as a
/// drop-in replacement where appropriate. Note, however, that not all the
/// methods on std::vector are implemented here, and that TfSmallVector may
/// have methods in addition to those that you would find on std::vector.
///
/// Note that a TfSmallVector that has grown beyond its local storage, will
/// NOT move its entries back into the local storage once it shrinks back to N.
///
template <typename T, uint32_t N>
class TfSmallVector : public TfSmallVectorBase
{
public:

    /// XXX: Functionality currently missing, and which we would like to add as
    ///  needed:
    ///     - emplace
    ///     - shrink_to_fit
    ///     - shrink_to_local / shrink_to_internal (or similar, free standing
    ///         function)

    /// \name Relevant Typedefs.
    /// @{

    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;

    /// }@

    /// \name Iterator Support.
    /// @{

    using iterator = T*;
    using const_iterator = const T*;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    /// }@

    /// Default constructor.
    ///
    TfSmallVector() : _size(0), _capacity(N) {}

    /// Construct a vector holding \p n value-initialized elements.
    ///
    explicit TfSmallVector(size_type n) :
        _capacity(N) {
        _InitStorage(n);
        value_type *d = data();
        for (size_type i = 0; i < n; ++i) {
            new (d + i) value_type();
        }
    }

    /// Construct a vector holding \p n copies of \p v.
    ///
    TfSmallVector(size_type n, const value_type &v) :
        _capacity(N) {
        _InitStorage(n);
        std::uninitialized_fill_n(data(), n, v);
    }

    /// Construct a vector holding \p n default-initialized elements.
    ///
    enum DefaultInitTag { DefaultInit };
    TfSmallVector(size_type n, DefaultInitTag) :
        _capacity(N) {
        _InitStorage(n);
        value_type *d = data();
        for (size_type i = 0; i < n; ++i) {
            new (d + i) value_type;
        }
    }

    /// Copy constructor.
    ///
    TfSmallVector(const TfSmallVector &rhs) : _capacity(N) {
        _InitStorage(rhs.size());
        std::uninitialized_copy(rhs.begin(), rhs.end(), begin());
    }

    /// Move constructor.
    ///
    TfSmallVector(TfSmallVector &&rhs) : _size(0), _capacity(N) {
        // If rhs can not be stored locally, take rhs's remote storage and
        // reset rhs to empty.
        if (rhs.size() > N) {
            _SetRemoteStorage(rhs._GetRemoteStorage());
            std::swap(_capacity, rhs._capacity);
        }

        // If rhs is stored locally, it's faster to simply move the entries
        // into this vector's storage, destruct the entries at rhs, and swap
        // sizes. Note that capacities will be the same in this case, so no
        // need to swap those.
        else {
            _UninitializedMove(rhs.begin(), rhs.end(), begin());
            rhs._Destruct();
        }
        std::swap(_size, rhs._size);
    }

    /// Construct a new vector from initializer list
    TfSmallVector(std::initializer_list<T> values)
        : TfSmallVector(values.begin(), values.end()) {
    }

    /// Creates a new vector containing copies of the data between 
    /// \p first and \p last. 
    template<typename ForwardIterator,
         typename = _EnableIfForwardIterator<ForwardIterator>>
    TfSmallVector(ForwardIterator first, ForwardIterator last) : _capacity(N)
    {
        _InitStorage(std::distance(first, last));
        std::uninitialized_copy(first, last, begin());
    }

    /// Destructor.
    ///
    ~TfSmallVector() {
        _Destruct();
        _FreeStorage();
    }

    /// Assignment operator.
    ///
    TfSmallVector &operator=(const TfSmallVector &rhs) {
        if (this != &rhs) {
            assign(rhs.begin(), rhs.end());
        }
        return *this;
    }

    /// Move assignment operator.
    ///
    TfSmallVector &operator=(TfSmallVector &&rhs) {
        if (this != &rhs) {
            swap(rhs);
        }
        return *this;
    }

    /// Replace existing contents with the contents of \p ilist.
    ///
    TfSmallVector &operator=(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    /// Swap two vector instances.
    ///
    void swap(TfSmallVector &rhs) {
        // Both this vector and rhs are stored locally.
        if (_IsLocal() && rhs._IsLocal()) {
            TfSmallVector *smaller = size() < rhs.size() ? this : &rhs;
            TfSmallVector *larger = size() < rhs.size() ? &rhs : this;

            // Swap all the entries up to the size of the smaller vector.
            std::swap_ranges(smaller->begin(), smaller->end(), larger->begin());

            // Move the tail end of the entries, and destruct them at the
            // source vector.
            for (size_type i = smaller->size(); i < larger->size(); ++i) {
                _MoveConstruct(smaller->data() + i, &(*larger)[i]);
                (*larger)[i].~value_type();
            }

            // Swap sizes. Capacities are already equal in this case.
            std::swap(smaller->_size, larger->_size);
        }

        // Both this vector and rhs are stored remotely. Simply swap the
        // pointers, as well as size and capacity.
        else if (!_IsLocal() && !rhs._IsLocal()) {
            value_type *tmp = _GetRemoteStorage();
            _SetRemoteStorage(rhs._GetRemoteStorage());
            rhs._SetRemoteStorage(tmp);

            std::swap(_size, rhs._size);
            std::swap(_capacity, rhs._capacity);
        }

        // Either this vector or rhs is stored remotely, whereas the other
        // one is stored locally.
        else {
            TfSmallVector *remote = _IsLocal() ? &rhs : this;
            TfSmallVector *local =  _IsLocal() ? this : &rhs;

            // Get a pointer to the remote storage. We'll be overwriting the
            // pointer value below, so gotta retain it first.
            value_type *remoteStorage = remote->_GetStorage();

            // Move all the entries from the vector with the local storage, to
            // the other vector's local storage. This will overwrite the pointer
            // to the other vectors remote storage. Note that we will have to
            // also destruct the elements at the source's local storage. The
            // source will become the one with the remote storage, so those
            // entries will be essentially freed.
            for (size_type i = 0; i < local->size(); ++i) {
                _MoveConstruct(remote->_GetLocalStorage() + i, &(*local)[i]);
                (*local)[i].~value_type();
            }

            // Swap the remote storage into the vector which previously had the
            // local storage. It's been properly cleaned up now.
            local->_SetRemoteStorage(remoteStorage);

            // Swap sizes and capacities. Easy peasy. 
            std::swap(remote->_size, local->_size);
            std::swap(remote->_capacity, local->_capacity);
        }

    }

    /// Insert an rvalue-reference entry at the given iterator position.
    ///
    iterator insert(const_iterator it, value_type &&v) {
        return _Insert(it, std::move(v));
    }

    /// Insert an entry at the given iterator.
    ///
    iterator insert(const_iterator it, const value_type &v) {
        return _Insert(it, v);
    }

    /// Erase an entry at the given iterator.
    ///
    iterator erase(const_iterator it) {
        return erase(it, it + 1);
    }

    /// Erase entries between [ \p first, \p last ) from the vector.
    ///
    iterator erase(const_iterator it, const_iterator last) {
        value_type *p = const_cast<value_type *>(&*it);
        value_type *q = const_cast<value_type *>(&*last);

         // If we're not removing anything, bail out.
        if (p == q) {
            return iterator(p);
        }
   
        const size_type num = std::distance(p, q);
       
        // Move entries starting at last, down a few slots to starting a it.
        value_type *e = data() + size();
        std::move(q, e, p);

        // Destruct all the freed up slots at the end of the vector.
        for (value_type *i = (e - num); i != e; ++i) {
            i->~value_type();
        }

        // Bump down the size.
        _size -= num;

        // Return an iterator to the next entry.
        return iterator(p);
    }

    /// Reserve storage for \p newCapacity entries.
    ///
    void reserve(size_type newCapacity) {
        // Only reserve storage if the new capacity would grow past the local
        // storage, or the currently allocated storage. We'll grow to
        // accommodate exactly newCapacity entries.
        if (newCapacity > capacity()) {
            _GrowStorage(newCapacity);
        }
    } 

    /// Resize the vector to \p newSize and insert copies of \v.
    ///
    void resize(size_type newSize, const value_type &v = value_type()) {
        // If the new size is smaller than the current size, let go of some
        // entries at the tail.
        if (newSize < size()) {
            erase(const_iterator(data() + newSize), 
                  const_iterator(data() + size())); 
        }

        // Otherwise, lets grow and fill: Reserve some storage, fill the tail
        // end with copies of v, and update the new size.
        else if (newSize > size()) {
            reserve(newSize);
            std::uninitialized_fill(data() + size(), data() + newSize, v);
            _size = newSize;
        }
    }

    /// Clear the entries in the vector. Does not let go of the underpinning
    /// storage.
    ///
    void clear() {
        _Destruct();
        _size = 0;
    }

    /// Clears any previously held entries, and copies entries between
    /// [ \p first, \p last ) to this vector. 
    ///
    template<typename ForwardIterator,
         typename = _EnableIfForwardIterator<ForwardIterator>>
    void assign(ForwardIterator first, ForwardIterator last) {
        clear();
        const size_type newSize = std::distance(first, last);
        reserve(newSize);
        std::uninitialized_copy(first, last, begin());
        _size = newSize;
    }

    /// Replace existing contents with the contents of \p ilist.
    ///
    void assign(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    /// Emplace an entry at the back of the vector.
    ///
    template < typename... Args >
    void emplace_back(Args&&... args) {
        if (size() == capacity()) {
            _GrowStorage(_NextCapacity());
        }
        new (data() + size()) value_type(std::forward<Args>(args)...);
        _size += 1;
    }

    /// Copy an entry to the back of the vector,
    ///
    void push_back(const value_type &v) {
        emplace_back(v);
    }

    /// Move an entry to the back of the vector.
    ///
    void push_back(value_type &&v) {
        emplace_back(std::move(v));
    }

    /// Copy the range denoted by [\p first, \p last) into this vector
    /// before \p pos.
    /// 
    template <typename ForwardIterator>
    void insert(iterator pos, ForwardIterator first, ForwardIterator last)
    {
        static_assert(
            std::is_convertible<
            typename std::iterator_traits<ForwardIterator>::iterator_category,
            std::forward_iterator_tag>::value,
            "Input Iterators not supported.");

        // Check for the insert-at-end special case as the very first thing so
        // that we give the compiler the best possible opportunity to
        // eliminate the general case code.
        const bool insertAtEnd = pos == end();

        const long numNewElems = std::distance(first, last);
        const size_type neededCapacity = size() + numNewElems;
        const size_type nextCapacity =
            std::max(_NextCapacity(), neededCapacity);

        // Insertions at the end would be handled correctly by the code below
        // without this special case.  However, insert(end(), f, l) is an
        // extremely common operation so we provide this fast path both to
        // avoid unneeded work and to make it easier for the compiler to
        // eliminate dead code when pos == end().
        if (insertAtEnd) {
            // The reallocation here is not a simple reserve.  We want to grow
            // the storage only when there are too many new elements but the
            // desired size is based on the growth factor.
            if (neededCapacity > capacity()) {
                _GrowStorage(nextCapacity);
            }
            std::uninitialized_copy(first, last, end());
            _size += numNewElems;
            return;
        }

        if (neededCapacity > capacity()) {
            // Because we need to realloc, we can do the insertion by copying
            // each range, [begin(), pos), [first, last), [pos, end()), into
            // the new storage.

            const size_type posI = std::distance(begin(), pos);
            value_type *newStorage = _Allocate(nextCapacity);

            iterator newPrefixBegin = iterator(newStorage);
            iterator newPos = newPrefixBegin + posI;
            iterator newSuffixBegin = newPos + numNewElems;
            _UninitializedMove(begin(), pos, newPrefixBegin);
            std::uninitialized_copy(first, last, newPos);
            _UninitializedMove(pos, end(), newSuffixBegin);

            // Destroy old data and set up this new buffer.
            _Destruct();
            _FreeStorage();
            _SetRemoteStorage(newStorage);
            _capacity = nextCapacity;
        }
        else {
            // Insert in-place requires handling four ranges.
            //
            // For both the range-to-move [pos, end()) and the range-to-insert
            // [first, last), there are two subranges: the subrange to copy
            // and the subrange to uinitialized_copy.  Note that only three of
            // these ranges may be non-empty: either there is a non-empty
            // prefix of [pos, end()) that needs to be copied over existing
            // elements or there is a non-empty suffix of [first, last) that
            // needs to be placed in uninitialized storage.

            const long numMoveElems = std::distance(pos, end());
            const long numUninitMoves = std::min(numNewElems, numMoveElems);
            const long numInitMoves = numMoveElems - numUninitMoves;
            const long numUninitNews = numNewElems - numUninitMoves;
            const long numInitNews = numNewElems - numUninitNews;

            // Move our existing elements out of the way of new elements.
            iterator umSrc = pos + numInitMoves;
            iterator umDst = end() + numUninitNews;
            _UninitializedMove(umSrc, end(), umDst);
            std::copy_backward(pos, umSrc, umDst);

            // Copy new elements into place.
            for (long i=0; i<numInitNews; ++i, ++first, ++pos) {
                *pos = *first;
            }
            std::uninitialized_copy(first, last, end());
        }

        _size += numNewElems;
    }

    /// Insert elements from \p ilist starting at position \p pos.
    ///
    void insert(iterator pos, std::initializer_list<T> ilist) {
        insert(pos, ilist.begin(), ilist.end());
    }

    /// Remove the entry at the back of the vector.
    ///
    void pop_back() {
        back().~value_type();
        _size -= 1;
    }

    /// Returns the current size of the vector.
    ///
    size_type size() const {
        return _size;
    }

    /// Returns the maximum size of this vector.
    ///
    static constexpr size_type max_size() {
        return std::numeric_limits<_SizeMemberType>::max();
    }

    /// Returns \c true if this vector is empty.
    ///
    bool empty() const {
        return size() == 0;
    }

    /// Returns the current capacity of this vector. Note that if the returned
    /// value is <= N, it does NOT mean the storage is local. A vector that has
    /// previously grown beyond its local storage, will not move entries back to
    /// the local storage once it shrinks to N.
    ///
    size_type capacity() const {
        return _capacity;
    }

    /// Returns the local storage capacity. The vector uses its local storage
    /// if capacity() <= internal_capacity().
    /// This method mimics the boost::container::small_vector interface.
    ///
    static constexpr size_type internal_capacity() {
        return N;
    }

    /// \name Returns an iterator to the beginning of the vector.
    /// @{

    iterator begin() {
        return iterator(_GetStorage());
    }

    const_iterator begin() const {
        return const_iterator(_GetStorage());
    }

    const_iterator cbegin() const {
        return begin();
    }

    /// @}

    /// \name Returns an iterator to the end of the vector.
    /// @{

    iterator end() {
        return iterator(_GetStorage() + size());
    }

    const_iterator end() const {
        return const_iterator(_GetStorage() + size());
    }

    const_iterator cend() const {
        return end();
    }

    /// @}

    /// \name Returns a reverse iterator to the beginning of the vector.
    /// @{

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const {
        return rbegin();
    }

    /// @}

    /// \name Returns a reverse iterator to the end of the vector.
    /// @{

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crend() const {
        return rend();
    }

    /// @}

    /// Returns the first element in the vector.
    ///
    reference front() {
        return *begin();
    }

    /// Returns the first element in the vector.
    ///
    const_reference front() const {
        return *begin();
    }

    /// Returns the last element in the vector.
    ///
    reference back() {
        return data()[size() - 1];
    }

    /// Returns the last elements in the vector.
    ///
    const_reference back() const {
        return data()[size() - 1];
    }

    /// Access the specified element.
    ///
    reference operator[](size_type i) {
        return data()[i];
    }

    /// Access the specified element.
    ///
    const_reference operator[](size_type i) const {
        return data()[i];
    }

    /// Direct access to the underlying array.
    /// 
    value_type *data() {
        return _GetStorage();
    }

    /// Direct access to the underlying array.
    ///
    const value_type *data() const {
        return _GetStorage();
    }

    /// Lexicographically compares the elements in the vectors for equality.
    ///
    bool operator==(const TfSmallVector &rhs) const {
        return size() == rhs.size() && std::equal(begin(), end(), rhs.begin());
    }

    /// Lexicographically compares the elements in the vectors for inequality.
    ///
    bool operator!=(const TfSmallVector &rhs) const {
        return !operator==(rhs);
    }
    
private:

    // Raw data access.
    value_type *_GetLocalStorage() {
        return static_cast<value_type *>(_data.GetLocalStorage());
    }
    const value_type *_GetLocalStorage() const {
        return static_cast<const value_type *>(_data.GetLocalStorage());
    }

    value_type *_GetRemoteStorage() {
        return static_cast<value_type *>(_data.GetRemoteStorage());
    }
    const value_type *_GetRemoteStorage() const {
        return static_cast<const value_type *>(_data.GetRemoteStorage());
    }

    void _SetRemoteStorage(value_type *p) {
        _data.SetRemoteStorage(static_cast<void *>(p));
    }
    
    // Returns true if the local storage is used.
    bool _IsLocal() const {
        return _capacity <= N;
    }

    // Return a pointer to the storage, which is either local or remote
    // depending on the current capacity.
    value_type *_GetStorage() {
        return _IsLocal() ? _GetLocalStorage() : _GetRemoteStorage();
    }

    // Return a const pointer to the storage, which is either local or remote
    // depending on the current capacity.
    const value_type *_GetStorage() const {
        return _IsLocal() ? _GetLocalStorage() : _GetRemoteStorage();
    }

    // Free the remotely allocated storage.
    void _FreeStorage() {
        if (!_IsLocal()) {
            free(_GetRemoteStorage());
        }
    }

    // Destructs all the elements stored in this vector.
    void _Destruct() {
        value_type *b = data();
        value_type *e = b + size();
        for (value_type *p = b; p != e; ++p) {
            p->~value_type();
        }
    }

    // Allocate a buffer on the heap.
    static value_type *_Allocate(size_type size) {
        return static_cast<value_type *>(malloc(sizeof(value_type) * size));
    }

    // Initialize the vector with new storage, updating the capacity and size.
    void _InitStorage(size_type size) {
        if (size > capacity()) {
            _SetRemoteStorage(_Allocate(size));
            _capacity = size;
        }
#if defined(ARCH_COMPILER_GCC) && ARCH_COMPILER_GCC_MAJOR < 11
        else if constexpr (!_data.HasLocal) {
            // When there's no local storage and we're not allocating remote
            // storage, initialize the remote storage pointer to avoid 
            // spurious compiler warnings about maybe-uninitialized values
            // being used. 

            // This clause can be removed upon upgrade to gcc 11 as 
            // the new compiler no longer generates this warning in this case.
            _data.SetRemoteStorage(nullptr);
        }
#endif
        _size = size;
    }

    // Grow the storage to be able to accommodate newCapacity entries. This
    // always allocates remote storage.
    void _GrowStorage(const size_type newCapacity) {
        value_type *newStorage = _Allocate(newCapacity);
        _UninitializedMove(begin(), end(), iterator(newStorage));
        _Destruct();
        _FreeStorage();
        _SetRemoteStorage(newStorage);
        _capacity = newCapacity;
    }

    // Returns the next capacity to use for vector growth. The growth factor
    // here is 1.5. A constant 1 is added so that we do not have to special
    // case initial capacities of 0 and 1.
    size_type _NextCapacity() const {
        const size_type cap = capacity();
        return cap + (cap / 2) + 1;
    }

    // Insert the value v at iterator it. We use this method that takes a 
    // universal reference to de-duplicate the logic required for the insert
    // overloads, one taking an rvalue reference, and the other one taking a
    // const reference. This way, we can take the most optimal code path (
    // move, or copy without making redundant copies) based on whether v is
    // a rvalue reference or const reference.
    template < typename U >
    iterator _Insert(const_iterator it, U &&v) {
        value_type *newEntry;

        // If the iterator points to the end, simply push back.
        if (it == end()) {
            push_back(std::forward<U>(v));
            return end() - 1;
        }

        // Grow the remote storage, if we need to. This invalidates iterators,
        // so special care must be taken in order to return a new, valid
        // iterator.
        else if (size() == capacity()) {
            const size_type newCapacity = _NextCapacity();
            value_type *newStorage = _Allocate(newCapacity);
            
            value_type *i = const_cast<value_type *>(&*it);
            value_type *curData = data();
            newEntry = _UninitializedMove(curData, i, newStorage);

            new (newEntry) value_type(std::forward<U>(v));

            _UninitializedMove(i, curData + size(), newEntry + 1);

            _Destruct();
            _FreeStorage();

            _SetRemoteStorage(newStorage);
            _capacity = newCapacity;
        }

        // Our current capacity is big enough to allow us to simply shift
        // elements up one slot and insert v at it.
        else {
            // Move all the elements after it up by one slot.
            newEntry = const_cast<value_type *>(&*it);
            value_type *last = const_cast<value_type *>(&back());
            new (data() + size()) value_type(std::move(*last));
            std::move_backward(newEntry, last, last + 1);

            // Move v into the slot at the supplied iterator position.
            newEntry->~value_type();
            new (newEntry) value_type(std::forward<U>(v));
        }

        // Bump size and return an iterator to the newly inserted entry.
        ++_size;
        return iterator(newEntry);
    }

    // The vector storage, which is a union of the local storage and a pointer
    // to the heap memory, if allocated.
    _Data<value_type, N> _data;

    // The current size of the vector, i.e. how many entries it contains.
    _SizeMemberType _size;

    // The current capacity of the vector, i.e. how big the currently allocated
    // storage space is.
    _SizeMemberType _capacity;
};

////////////////////////////////////////////////////////////////////////////////

template < typename T, uint32_t N >
void swap(TfSmallVector<T, N> &a, TfSmallVector<T, N> &b)
{
    a.swap(b);
}

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
