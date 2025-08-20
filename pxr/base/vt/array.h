//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_ARRAY_H
#define PXR_BASE_VT_ARRAY_H

/// \file vt/array.h

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/hash.h"
#include "pxr/base/vt/streamOut.h"
#include "pxr/base/vt/traits.h"
#include "pxr/base/vt/types.h"

#include "pxr/base/arch/functionLite.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

// Helper class for clients that create VtArrays referring to foreign-owned
// data.
class Vt_ArrayForeignDataSource
{
public:
    explicit Vt_ArrayForeignDataSource(
        void (*detachedFn)(Vt_ArrayForeignDataSource *self) = nullptr,
        size_t initRefCount = 0)
        : _refCount(initRefCount)
        , _detachedFn(detachedFn) {}
    
private:
    template <class T> friend class VtArray;
    // Invoked when no more arrays share this data source.
    void _ArraysDetached() { if (_detachedFn) { _detachedFn(this); } }
protected:
    std::atomic<size_t> _refCount;
    void (*_detachedFn)(Vt_ArrayForeignDataSource *self);
};

// Private base class helper for VtArray implementation.
class Vt_ArrayBase
{
public:
    Vt_ArrayBase() : _shapeData { 0 }, _foreignSource(nullptr) {}

    Vt_ArrayBase(Vt_ArrayForeignDataSource *foreignSrc)
        : _shapeData { 0 }, _foreignSource(foreignSrc) {}

    Vt_ArrayBase(Vt_ArrayBase const &other) = default;
    Vt_ArrayBase(Vt_ArrayBase &&other) : Vt_ArrayBase(other) {
        other._shapeData.clear();
        other._foreignSource = nullptr;
    }

    Vt_ArrayBase &operator=(Vt_ArrayBase const &other) = default;
    Vt_ArrayBase &operator=(Vt_ArrayBase &&other) {
        if (this == &other)
            return *this;
        *this = other;
        other._shapeData.clear();
        other._foreignSource = nullptr;
        return *this;
    }
    
protected:
    // Control block header for native data representation.  Houses refcount and
    // capacity.  For arrays with native data, this structure always lives
    // immediately preceding the start of the array's _data in memory.  See
    // _GetControlBlock() for details.
    struct _ControlBlock {
        _ControlBlock() : nativeRefCount(0), capacity(0) {}
        _ControlBlock(size_t initCount, size_t initCap)
            : nativeRefCount(initCount), capacity(initCap) {}
        mutable std::atomic<size_t> nativeRefCount;
        size_t capacity;
    };
    
    _ControlBlock &_GetControlBlock(void *nativeData) {
        TF_DEV_AXIOM(!_foreignSource);
        return *(reinterpret_cast<_ControlBlock *>(nativeData) - 1);
    }
    
    _ControlBlock const &_GetControlBlock(void *nativeData) const {
        TF_DEV_AXIOM(!_foreignSource);
        return *(reinterpret_cast<_ControlBlock *>(nativeData) - 1);
    }

    // Mutable ref count, as is standard.
    std::atomic<size_t> &_GetNativeRefCount(void *nativeData) const {
        return _GetControlBlock(nativeData).nativeRefCount;
    }

    size_t &_GetCapacity(void *nativeData) {
        return _GetControlBlock(nativeData).capacity;
    }
    size_t const &_GetCapacity(void *nativeData) const {
        return _GetControlBlock(nativeData).capacity;
    }

    VT_API void _DetachCopyHook(char const *funcName) const;

    Vt_ShapeData _shapeData;
    Vt_ArrayForeignDataSource *_foreignSource;
};

/// \class VtArray 
///
/// Represents an arbitrary dimensional rectangular container class.
///
/// Originally, VtArray was built to mimic the arrays in menv2x's MDL language,
/// but since VtArray has typed elements, the multidimensionality has found
/// little use.  For example, if you have only scalar elements, then to
/// represent a list of vectors you need a two dimensional array.  To represent
/// a list of matrices you need a three dimensional array.  However with
/// VtArray<GfVec3d> and VtArray<GfMatrix4d>, the VtArray is one dimensional and
/// the extra dimensions are encoded in the element types themselves.
///
/// For this reason, VtArray has been moving toward being more like std::vector,
/// and it now has much of std::vector's API, but there are still important
/// differences.
///
/// First, VtArray shares data between instances using a copy-on-write scheme.
/// This means that making copies of VtArray instances is cheap: it only copies
/// the pointer to the data.  But on the other hand, invoking any non-const
/// member function incurs a copy of the underlying data if it is not
/// uniquely owned.  For example, assume 'a' and 'b' are VtArray<int>:
///
/// \code
/// a = b;       // No copy; a and b now share ownership of underlying data.
/// a[0] = 123;  // A copy is incurred, to detach a's data from b.
///              // a and b no longer share data.
/// a[1] = 234;  // No copy: a's data is uniquely owned by a.
/// \endcode
///
/// Note that since all non-const member functions will potentially cause a
/// copy, it's possible to accidentally incur a copy even when unintended, or
/// when no actual data mutation occurs.  For example:
/// 
/// \code
/// int sum = 0;
/// for (VtArray<int>::iterator i = a.begin(), end = a.end(); i != end; ++i) {
///    sum += *i;
/// }
/// \endcode
///
/// Invoking a.begin() here will incur a copy if a's data is shared.  This is
/// required since it's possible to mutate the data through the returned
/// iterator, even though the subsequent code doesn't do any mutation.  This can
/// be avoided by explicitly const-iterating like the following:
///
/// \code
/// int sum = 0;
/// for (VtArray<int>::const_iterator i = a.cbegin(), end = a.cend(); i != end; ++i) {
///    sum += *i;
/// }
/// \endcode
///
/// This can be quite subtle.  In C++, calling a member function that has const
/// and non-const overloads on a non-const object must invoke the non-const
/// version, even if the const version would suffice.  So something as simple
/// this:
///
/// \code
/// float x = array[123];
/// \endcode
///
/// Invokes the non-const operator[] if \p array is non-const.  That means this
/// kind of benign looking code can cause a copy-on-write detachment of the
/// entire array, and thus is not safe to invoke concurrently with any other
/// member function.  If we were building this class today we would make
/// different choices about this API, but changing this now is a gargantuan
/// task, so it remains.
///
/// So, it is best practice to ensure you use const VtArray, or const VtArray &,
/// or VtArray::AsConst(), as well as the `c`-prefixed member functions like
/// cbegin()/cend(), cfront()/cback() to avoid these pitfalls when your intent
/// is not to mutate the array.
///
/// Regarding thread safety, for the same reasons spelled out above, all
/// mutating member functions must be invoked exclusively to all other member
/// functions, even if they are invoked in a way that does not mutate (as in the
/// operator[] example above).  This is the same general rule that the STL
/// abides by.
///
/// Also, and again for the same reasons, all mutating member functions can
/// invalidate iterators, even if the member functions are invoked in a way that
/// does not mutate (as in the operator[] example above).
///
/// The TfEnvSetting 'VT_LOG_STACK_ON_ARRAY_DETACH_COPY' can be set to help
/// determine where unintended copy-on-write detaches come from.  When set,
/// VtArray will log a stack trace for every copy-on-write detach that occurs.
///
ARCH_PRAGMA_PUSH
ARCH_PRAGMA_NON_EXPORTED_BASE_CLASS
template<typename ELEM>
class VtArray : public Vt_ArrayBase {
ARCH_PRAGMA_POP
  public:

    /// Type this array holds.
    typedef ELEM ElementType;
    typedef ELEM value_type;

    /// \defgroup STL_API STL-like API
    /// @{

    /// Iterator type.
    using iterator = ElementType *;
    /// Const iterator type.
    using const_iterator = ElementType const *;
    
    /// Reverse iterator type.
    typedef std::reverse_iterator<iterator> reverse_iterator;
    /// Reverse const iterator type.
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    /// Reference type.
    typedef ElementType &reference;
    /// Const reference type.
    typedef ElementType const &const_reference;
    /// Pointer type.
    typedef ElementType *pointer;
    /// Const pointer type.
    typedef ElementType const *const_pointer;

    /// @}

    /// Create an empty array.
    VtArray() : _data(nullptr) {}

    /// Create an array from a pair of iterators
    ///
    /// Equivalent to:
    /// \code
    /// VtArray<T> v;
    /// v.assign(first, last);
    /// \endcode
    ///
    /// Note we use enable_if with a dummy parameter here to avoid clashing
    /// with our other constructor with the following signature:
    ///
    /// VtArray(size_t n, value_type const &value = value_type())
    template <typename LegacyInputIterator>
    VtArray(LegacyInputIterator first, LegacyInputIterator last,
            std::enable_if_t<
                !std::is_integral_v<LegacyInputIterator>> * = nullptr)
        : VtArray() {
        assign(first, last); 
    }

    /// Create an array with foreign source.
    VtArray(Vt_ArrayForeignDataSource *foreignSrc,
            ElementType *data, size_t size, bool addRef = true)
        : Vt_ArrayBase(foreignSrc)
        , _data(data) {
        if (addRef) {
            foreignSrc->_refCount.fetch_add(1, std::memory_order_relaxed);
        }
        _shapeData.totalSize = size;
    }
    
    /// Copy \p other.  The new array shares underlying data with \p other.
    VtArray(VtArray const &other) : Vt_ArrayBase(other)
                                  , _data(other._data) {
        if (!_data)
            return;

        if (ARCH_LIKELY(!_foreignSource)) {
            _GetNativeRefCount(_data).fetch_add(1, std::memory_order_relaxed);
        }
        else {
            _foreignSource->_refCount.fetch_add(1, std::memory_order_relaxed);
        }
    }
    
    /// Move from \p other.  The new array takes ownership of \p other's
    /// underlying data.
    VtArray(VtArray &&other) : Vt_ArrayBase(std::move(other))
                             , _data(other._data) {
        other._data = nullptr;
    }

    /// Initialize array from the contents of a \p initializerList.
    VtArray(std::initializer_list<ELEM> initializerList)
        : VtArray() {
        assign(initializerList);
    }

    /// Create an array filled with \p n value-initialized elements.
    explicit VtArray(size_t n)
        : VtArray() {
        assign(n, value_type());
    }

    /// Create an array filled with \p n copies of \p value.
    explicit VtArray(size_t n, value_type const &value)
        : VtArray() {
        assign(n, value);
    }

    /// Copy assign from \p other.  This array shares underlying data with
    /// \p other.
    VtArray &operator=(VtArray const &other) {
        // This might look recursive but it's really invoking move-assign, since
        // we create a temporary copy (an rvalue).
        if (this != &other)
            *this = VtArray(other);
        return *this;
    }

    /// Move assign from \p other.  This array takes ownership of \p other's
    /// underlying data.
    VtArray &operator=(VtArray &&other) {
        if (this == &other)
            return *this;
        _DecRef();
        static_cast<Vt_ArrayBase &>(*this) = std::move(other);
        _data = other._data;
        other._data = nullptr;
        return *this;
    }

    /// Replace current array contents with those in \p initializerList 
    VtArray &operator=(std::initializer_list<ELEM> initializerList) {
        this->assign(initializerList.begin(), initializerList.end());
        return *this;
    }

    ~VtArray() { _DecRef(); }
    
    /// Return *this as a const reference.  This ensures that all operations on
    /// the result do not mutate and thus are safe to invoke concurrently with
    /// other non-mutating operations, and will never cause a copy-on-write
    /// detach.
    ///
    /// Note that the return is a const reference to this object, so it is only
    /// valid within the lifetime of this array object.  Take special care
    /// invoking AsConst() on VtArray temporaries/rvalues.
    VtArray const &AsConst() const noexcept {
        return *this;
    }

    /// \addtogroup STL_API
    /// @{
    
    /// Return a non-const iterator to the start of the array.  The underlying
    /// data is copied if it is not uniquely owned.
    iterator begin() { return iterator(data()); }
    /// Returns a non-const iterator to the end of the array.  The underlying
    /// data is copied if it is not uniquely owned.
    iterator end() { return iterator(data() + size()); }

    /// Return a const iterator to the start of the array.
    const_iterator begin() const { return const_iterator(data()); }
    /// Return a const iterator to the end of the array.
    const_iterator end() const { return const_iterator(data() + size()); }

    /// Return a const iterator to the start of the array.
    const_iterator cbegin() const { return begin(); }
    /// Return a const iterator to the end of the array.
    const_iterator cend() const { return end(); }

    /// Return a non-const reverse iterator to the end of the array.  The
    /// underlying data is copied if it is not uniquely owned.
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    /// Return a reverse iterator to the start of the array.  The underlying
    /// data is copied if it is not uniquely owned.
    reverse_iterator rend() { return reverse_iterator(begin()); }

    /// Return a const reverse iterator to the end of the array.
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }
    /// Return a const reverse iterator to the start of the array.
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    /// Return a const reverse iterator to the end of the array.
    const_reverse_iterator crbegin() const { return rbegin(); }
    /// Return a const reverse iterator to the start of the array.
    const_reverse_iterator crend() const { return rend(); }

    /// Return a non-const pointer to this array's data.  The underlying data is
    /// copied if it is not uniquely owned.
    pointer data() { _DetachIfNotUnique(); return _data; }
    /// Return a const pointer to this array's data.
    const_pointer data() const { return _data; }
    /// Return a const pointer to the data held by this array.
    const_pointer cdata() const { return _data; }

    /// Initializes a new element at the end of the array. The underlying data
    /// is first copied if it is not uniquely owned.
    ///
    /// \sa push_back(ElementType const&)
    /// \sa push_back(ElementType&&)
    template <typename... Args>
    void emplace_back(Args&&... args) {
        // If this is a non-pxr array with rank > 1, disallow push_back.
        if (ARCH_UNLIKELY(_shapeData.otherDims[0])) {
            TF_CODING_ERROR("Array rank %u != 1", _shapeData.GetRank());
            return;
        }
        // If we don't own the data, or if we need more space, realloc.
        size_t curSize = size();
        if (ARCH_UNLIKELY(
                _foreignSource || !_IsUnique() || curSize == capacity())) {
            value_type *newData = _AllocateCopy(
                _data, _CapacityForSize(curSize + 1), curSize);
            ::new (static_cast<void*>(newData + curSize)) value_type(
                std::forward<Args>(args)...);
            _DecRef();
            _data = newData;
        }
        else {
            ::new (static_cast<void*>(_data + curSize)) value_type(
                std::forward<Args>(args)...);
        }
        // Adjust size.
        ++_shapeData.totalSize;
    }

    /// Appends an element at the end of the array. The underlying data
    /// is first copied if it is not uniquely owned.
    ///
    /// \sa emplace_back
    /// \sa push_back(ElementType&&)
    void push_back(ElementType const& element) {
        emplace_back(element);
    }

    /// Appends an element at the end of the array. The underlying data
    /// is first copied if it is not uniquely owned.
    ///
    /// \sa emplace_back
    /// \sa push_back(ElementType const&)
    void push_back(ElementType&& element) {
        emplace_back(std::move(element));
    }

    /// Remove the last element of an array.  The underlying data is first
    /// copied if it is not uniquely owned.
    void pop_back() {
        // If this is a presto array with rank > 1, disallow push_back.
        if (ARCH_UNLIKELY(_shapeData.otherDims[0])) {
            TF_CODING_ERROR("Array rank %u != 1", _shapeData.GetRank());
            return;
        }
        _DetachIfNotUnique();
        // Invoke the destructor.
        (_data + size() - 1)->~value_type();
        // Adjust size.
        --_shapeData.totalSize;
    }

    /// Return the total number of elements in this array.
    size_t size() const { return _shapeData.totalSize; }

    /// Return the number of items this container can grow to hold without
    /// triggering a (re)allocation.  Note that if the underlying data is not
    /// uniquely owned, a reallocation can occur upon object insertion even if
    /// there is remaining capacity.
    size_t capacity() const {
        if (!_data) {
            return 0;
        }
        // We do not allow mutation to foreign source data, so always report
        // foreign sourced arrays as at capacity.
        return ARCH_UNLIKELY(_foreignSource) ? size() : _GetCapacity(_data);
    }

    /// Return a theoretical maximum size limit for the container.  In practice
    /// this size is unachievable due to the amount of available memory or other
    /// system limitations.
    constexpr size_t max_size() const {
        // Popular compilers limit object sizes to half the address space, so we
        // follow suit, taking max bytes as one less than the maximum value of a
        // difference between pointers.  This is unobtainable in practice: on a
        // 64-bit machine this is more than 8 million terabytes.
        return (std::numeric_limits<ptrdiff_t>::max() - 1 -
                sizeof(_ControlBlock)) / sizeof(value_type);
    }

    /// Return true if this array contains no elements, false otherwise.
    bool empty() const { return size() == 0; }
    
    /// Ensure enough memory is allocated to hold \p num elements.  Note that
    /// this currently does not ensure that the underlying data is uniquely
    /// owned.  If that is desired, invoke a method like data() first.
    void reserve(size_t num) {
        if (num <= capacity())
            return;
        
        value_type *newData =
            _data ? _AllocateCopy(_data, num, size()) : _AllocateNew(num);

        _DecRef();
        _data = newData;
    }

    /// Return a non-const reference to the first element in this array.  The
    /// underlying data is copied if it is not uniquely owned.  Invokes
    /// undefined behavior if the array is empty.
    reference front() { return *begin(); }
    /// Return a const reference to the first element in this array.  Invokes
    /// undefined behavior if the array is empty.
    const_reference front() const { return *begin(); }
    /// Return a const reference to the first element in this array.  Invokes
    /// undefined behavior if the array is empty.
    const_reference cfront() const { return *begin(); }

    /// Return a reference to the last element in this array.  The underlying
    /// data is copied if it is not uniquely owned.  Invokes undefined behavior
    /// if the array is empty.
    reference back() { return *rbegin(); }
    /// Return a const reference to the last element in this array.  Invokes
    /// undefined behavior if the array is empty.
    const_reference back() const { return *rbegin(); }
    /// Return a const reference to the last element in this array.  Invokes
    /// undefined behavior if the array is empty.
    const_reference cback() const { return *rbegin(); }

    /// Resize this array.  Preserve existing elements that remain,
    /// value-initialize any newly added elements.  For example, calling
    /// resize(10) on an array of size 5 would change the size to 10, the first
    /// 5 elements would be left unchanged and the last 5 elements would be
    /// value-initialized.
    void resize(size_t newSize) {
        return resize(newSize, value_type());
    }

    /// Resize this array.  Preserve existing elements that remain, initialize
    /// any newly added elements by copying \p value.
    void resize(size_t newSize, value_type const &value) {
        return resize(newSize,
                      [&value](pointer b, pointer e) {
                          std::uninitialized_fill(b, e, value);
                      });
    }

    /// Resize this array.  Preserve existing elements that remain, initialize
    /// any newly added elements by copying \p value.
    void resize(size_t newSize, value_type &value) {
        return resize(newSize, const_cast<value_type const &>(value));
    }

    /// Resize this array.  Preserve existing elements that remain, initialize
    /// any newly added elements by copying \p value.
    void resize(size_t newSize, value_type &&value) {
        return resize(newSize, const_cast<value_type const &>(value));
    }

    /// Resize this array.  Preserve existing elements that remain, initialize
    /// any newly added elements by calling \p fillElems(first, last).  Note
    /// that this function is passed pointers to uninitialized memory, so the
    /// elements must be filled with something like placement-new.
    template <class FillElemsFn>
    void resize(size_t newSize, FillElemsFn &&fillElems) {
        const size_t oldSize = size();
        if (oldSize == newSize) {
            return;
        }
        if (newSize == 0) {
            clear();
            return;
        }

        const bool growing = newSize > oldSize;
        value_type *newData = _data;

        if (!_data) {
            // Allocate newSize elements and initialize.
            newData = _AllocateNew(newSize);
            std::forward<FillElemsFn>(fillElems)(newData, newData + newSize);
        }
        else if (_IsUnique()) {
            if (growing) {
                if (newSize > _GetCapacity(_data)) {
                    newData = _AllocateCopy(_data, newSize, oldSize);
                }
                // fill with newly added elements from oldSize to newSize.
                std::forward<FillElemsFn>(fillElems)(newData + oldSize,
                                                     newData + newSize);
            }
            else {
                // destroy removed elements
                for (auto *cur = newData + newSize,
                         *end = newData + oldSize; cur != end; ++cur) {
                    cur->~value_type();
                }
            }
        }
        else {
            newData =
                _AllocateCopy(_data, newSize, growing ? oldSize : newSize);
            if (growing) {
                // fill with newly added elements from oldSize to newSize.
                std::forward<FillElemsFn>(fillElems)(newData + oldSize,
                                                     newData + newSize);
            }
        }

        // If we created new data, clean up the old and move over to the new.
        if (newData != _data) {
            _DecRef();
            _data = newData;
        }
        // Adjust size.
        _shapeData.totalSize = newSize;
    }

    /// Equivalent to resize(0).
    void clear() {
        if (!_data)
            return;
        if (_IsUnique()) {
            // Clear out elements, run dtors, keep capacity.
            for (value_type *p = _data, *e = _data + size(); p != e; ++p) {
                p->~value_type();
            }
        }
        else {
            // Detach to empty.
            _DecRef();
        }
        _shapeData.totalSize = 0;
    }

    /// Insert a copy of a single element at \p pos into the array.  Return an
    /// iterator pointing to the inserted element.
    iterator insert(const_iterator pos, value_type const &value) {
        // If value is an element of the array, we make a copy and move-insert
        // it.  std::less (and friends) and the standard's guarantee of a strict
        // pointer total order ensures this works.
        const const_pointer valuePtr = std::addressof(value);
        if (std::less_equal<const_pointer>{}(cdata(), valuePtr) &&
            std::less<const_pointer>{}(valuePtr, cdata() + size())) {
            value_type tmp { value };
            return insert(pos,
                          std::make_move_iterator(std::addressof(tmp)),
                          std::make_move_iterator(std::addressof(tmp) + 1));
        }
        return insert(pos, valuePtr, valuePtr + 1);
    }

    /// Insert by moving a single element at \p pos into the array.  Return an
    /// iterator pointing to the move-inserted element.
    iterator insert(const_iterator pos, value_type &&value) {
        // If value is an element of the array, we move it to a temp spot and
        // move-insert that.  std::less (and friends) and the standard's
        // guarantee of a strict pointer total order ensures this works.
        const const_pointer valuePtr = std::addressof(value);
        if (std::less_equal<const_pointer>{}(cdata(), valuePtr) &&
            std::less<const_pointer>{}(valuePtr, cdata() + size())) {
            value_type tmp { std::move(value) };
            return insert(pos,
                          std::make_move_iterator(std::addressof(tmp)),
                          std::make_move_iterator(std::addressof(tmp) + 1));
        }
        return insert(pos,
                      std::make_move_iterator(valuePtr),
                      std::make_move_iterator(valuePtr + 1));
    }

    /// Insert \p count copies of \p fill starting at \p pos in the array.
    /// Return an iterator pointing to the first inserted element, or \p pos if
    /// no elements are inserted.
    iterator insert(const_iterator pos, size_t count, value_type const &fill) {
        // If fill is an element of the array, we copy it to a temporary and
        // insert that.  std::less (and friends) and the standard's guarantee of
        // a strict pointer total order ensures this works.
        const const_pointer fillPtr = std::addressof(fill);
        if (std::less_equal<const_pointer>{}(cdata(), fillPtr) &&
            std::less<const_pointer>{}(fillPtr, cdata() + size())) {
            value_type tmp { fill };
            return insert(pos, count, [&tmp](pointer b, pointer e) {
                std::uninitialized_fill(b, e, tmp);
            });
        }
        return insert(pos, count, [&fill](pointer b, pointer e) {
            std::uninitialized_fill(b, e, fill);
        });
    }

    /// Insert the contents of \p ilist into the array at \p pos.  Return an
    /// iterator pointing to the first inserted element, or \p pos if no
    /// elements are inserted.
    iterator insert(const_iterator pos, std::initializer_list<ELEM> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    /// Insert the elements from the range [first, last) into the array at \p
    /// pos.  Return an iterator pointing to the first inserted element, or \p
    /// pos if no elements are inserted.  The behavior is undefined if [first,
    /// last) intersects [cbegin(), cend()).
    template <class LegacyInputIterator>
    std::enable_if_t<
        !std::is_integral_v<LegacyInputIterator>, iterator>
    insert(
        const_iterator pos,
        LegacyInputIterator first, LegacyInputIterator last) {
        return insert(pos, std::distance(first, last),
                      [&first, &last](pointer b, pointer e) {
                          std::uninitialized_copy(first, last, b);
                      });
    }

    /// Insert \p count elements into the array starting at \p pos, calling \p
    /// fillElems(first, last) to construct the inserted elements in
    /// uninitialized memory.  Note that since \p fillElems is passed pointers
    /// to uninitialized memory, the elements must be constructed using
    /// something like placement-new.  Return an iterator pointing to the first
    /// inserted element, or \p pos if no elements are inserted.
    template <class FillElemsFn>
    std::enable_if_t<
        !std::is_integral_v<std::decay_t<FillElemsFn>>,
        iterator>
    insert(const_iterator pos, size_t count, FillElemsFn &&fillElems) {
        if (count == 0) {
            return begin() + std::distance(cbegin(), pos);
        }
        if (empty()) {
            resize(count, std::forward<FillElemsFn>(fillElems));
            return begin();
        }
        // This array is not empty, and we will insert at least one element.
        const size_t newSize = size() + count;
        if (_IsUnique()) {
            if (newSize <= capacity()) {
                // We can avoid reallocation.
                iterator ncpos = begin() + std::distance(cbegin(), pos);
                iterator iend = end();
                if (pos != cend()) {
                    // The 'count' tail elements must be uninitialized_move'd
                    // over.  The remainder must be move_backward'd.
                    std::uninitialized_copy(
                        std::make_move_iterator(iend-count),
                        std::make_move_iterator(iend), iend);
                    std::move_backward(ncpos, iend-count, iend);
                }
                // Fill new elements.
                value_type *p = std::addressof(*ncpos);
                std::forward<FillElemsFn>(fillElems)(p, p + count);
                // Set new size.
                _shapeData.totalSize = newSize;
                return ncpos;
            }
            else {
                // We need to allocate, but can move items.
                value_type* newData = _AllocateNew(newSize);
                size_t posOffset = std::distance(cbegin(), pos);
                std::uninitialized_copy(
                    std::make_move_iterator(begin()),
                    std::make_move_iterator(begin() + posOffset), newData);
                std::uninitialized_copy(
                    std::make_move_iterator(begin() + posOffset),
                    std::make_move_iterator(end()),
                    newData + posOffset + count);
                // Drop the old data and fill the new elements.
                _DecRef();
                _data = newData;
                _shapeData.totalSize = newSize;
                std::forward<FillElemsFn>(fillElems)(
                    newData + posOffset, newData + posOffset + count);
                return begin() + posOffset;
            }
        }
        else {
            // Allocate new space and copy.
            value_type* newData = _AllocateNew(newSize);
            size_t posOffset = std::distance(cbegin(), pos);
            std::uninitialized_copy(cbegin(), pos, newData);
            std::uninitialized_copy(pos, cend(), newData + posOffset + count);
            // Drop the old data and fill the new elements.
            _DecRef();
            _data = newData;
            _shapeData.totalSize = newSize;
            std::forward<FillElemsFn>(fillElems)(
                newData + posOffset, newData + posOffset + count);
            return begin() + posOffset;
        }
        // unreachable.
    }
    
    /// Removes a single element at \p pos from the array
    /// 
    /// To match the behavior of std::vector, returns an iterator
    /// pointing to the position following the removed element.
    /// 
    /// Since the returned iterator is mutable, when the array is
    /// not uniquely owned, a copy will be required.
    ///
    /// Erase invalidates all iterators (unlike std::vector
    /// where iterators prior to \p pos remain valid).
    ///
    /// \sa erase(const_iterator, const_iterator)
    iterator erase(const_iterator pos) {
        TF_DEV_AXIOM(pos != cend());
        return erase(pos, pos+1);
    }

    /// Remove a range of elements [\p first, \p last) from the array.
    /// 
    /// To match the behavior of std::vector, returns an iterator
    /// at the position following the removed element.
    /// If no elements are removed, a non-const iterator pointing
    /// to last will be returned.
    /// 
    /// Since the returned iterator is mutable, when the array is
    /// not uniquely owned, a copy will be required even when
    /// the contents are unchanged.
    ///
    /// Erase invalidates all iterators (unlike std::vector
    /// where iterators prior to \p first remain valid).
    ///
    /// \sa erase(const_iterator)
    iterator erase(const_iterator first, const_iterator last) {
        if (first == last) {
            return std::next(begin(), std::distance(cbegin(), last));
        }
        if ((first == cbegin()) && (last == cend())) {
            clear();
            return end();
        }
        // Given the previous two conditions, we know that we are removing
        // at least one element and the result array will contain at least one
        // element.
        value_type* removeStart =
            std::next(_data, std::distance(cbegin(), first));
        value_type* removeEnd = std::next(_data, std::distance(cbegin(), last));
        value_type* endIt = std::next(_data, size());
        size_t newSize = size() - std::distance(first, last);
        if (_IsUnique()) {
            // If the array is unique, we can simply move the tail elements
            // and free to the end of the array.
            value_type* deleteIt = std::move(removeEnd, endIt, removeStart);
            for (; deleteIt != endIt; ++deleteIt) {
                deleteIt->~value_type();
            }
            _shapeData.totalSize = newSize;
            return iterator(removeStart);
        } else {
            // If the array is not unique, we want to avoid copying the
            // elements in the range we are erasing. We allocate a
            // new buffer and copy the head and tail ranges, omitting
            // [first, last)
            value_type* newData = _AllocateNew(newSize);
            value_type* newMiddle = std::uninitialized_copy(
                _data, removeStart, newData);
            value_type* newEnd = std::uninitialized_copy(
                removeEnd, endIt, newMiddle);
            TF_DEV_AXIOM(newEnd == std::next(newData, newSize));
            TF_DEV_AXIOM(std::distance(newData, newMiddle) == 
                         std::distance(_data, removeStart));
            _DecRef();
            _data = newData;
            _shapeData.totalSize = newSize;
            return iterator(newMiddle);
        }
    }

    /// Assign array contents.
    /// Equivalent to:
    /// \code
    /// array.resize(std::distance(first, last));
    /// std::copy(first, last, array.begin());
    /// \endcode
    template <class ForwardIter>
    typename std::enable_if<!std::is_integral<ForwardIter>::value>::type
    assign(ForwardIter first, ForwardIter last) {
        struct _Copier {
            void operator()(pointer b, pointer e) const {
                std::uninitialized_copy(first, last, b);
            }
            ForwardIter const &first, &last;
        };
        clear();
        resize(std::distance(first, last), _Copier { first, last });
    }

    /// Assign array contents.
    /// Equivalent to:
    /// \code
    /// array.resize(n);
    /// std::fill(array.begin(), array.end(), fill);
    /// \endcode
    void assign(size_t n, const value_type &fill) {
        struct _Filler {
            void operator()(pointer b, pointer e) const {
                std::uninitialized_fill(b, e, fill);
            }
            const value_type &fill;
        };
        clear();
        resize(n, _Filler { fill });
    }

    /// Assign array contents via intializer list
    /// Equivalent to:
    /// \code
    /// array.assign(list.begin(), list.end());
    /// \endcode
    void assign(std::initializer_list<ELEM> initializerList) {
	assign(initializerList.begin(), initializerList.end());
    }

    /// Swap the contents of this array with \p other.
    void swap(VtArray &other) { 
        std::swap(_data, other._data);
        std::swap(_shapeData, other._shapeData);
        std::swap(_foreignSource, other._foreignSource);
    }

    /// @}

    /// Allows usage of [i].
    ElementType &operator[](size_t index) {
        return data()[index];
    }

    /// Allows usage of [i].
    ElementType const &operator[](size_t index) const {
        return data()[index];
    }

    /// Tests if two arrays are identical, i.e. that they share
    /// the same underlying copy-on-write data.  See also operator==().
    bool IsIdentical(VtArray const & other) const {
        return
            _data == other._data &&
            _shapeData == other._shapeData &&
            _foreignSource == other._foreignSource;
    }

    /// Tests two arrays for equality.  See also IsIdentical().
    bool operator == (VtArray const & other) const {
        return IsIdentical(other) ||
            (*_GetShapeData() == *other._GetShapeData() &&
             std::equal(cbegin(), cend(), other.cbegin()));
    }

    /// Tests two arrays for inequality.
    bool operator != (VtArray const &other) const {
        return !(*this == other);
    }

  public:
    // XXX -- Public so VtValue::_ArrayHelper<T,U>::GetShapeData() has access.
    Vt_ShapeData const *_GetShapeData() const {
        return &_shapeData;
    }
    Vt_ShapeData *_GetShapeData() {
        return &_shapeData;
    }

  private:
    class _Streamer {
    public:
        explicit _Streamer(const_pointer data) : _p(data) { }
        void operator()(std::ostream &out) const {
            VtStreamOut(*_p++, out);
        }

    private:
        mutable const_pointer _p;
    };

    /// Outputs a comma-separated list of the values in the array.
    friend std::ostream &operator <<(std::ostream &out, const VtArray &self) {
        VtArray::_Streamer streamer(self.cdata());
        VtStreamOutArray(out, self._GetShapeData(), streamer);
        return out;
    }

    /// Swap array contents.
    friend void swap(VtArray &lhs, VtArray &rhs) {
        lhs.swap(rhs);
    }

    void _DetachIfNotUnique() {
        if (_IsUnique())
            return;
        // Copy to local.
        _DetachCopyHook(__ARCH_PRETTY_FUNCTION__);
        auto *newData = _AllocateCopy(_data, size(), size());
        _DecRef();
        _data = newData;
    }

    inline bool _IsUnique() const {
        return !_data ||
            (ARCH_LIKELY(!_foreignSource) && _GetNativeRefCount(_data) == 1);
    }

    inline size_t _CapacityForSize(size_t sz) const {
        // Currently just successive powers of two.
        size_t cap = 1;
        while (cap < sz) {
            cap += cap;
        }
        return cap;
    }

    value_type *_AllocateNew(size_t capacity) {
        TfAutoMallocTag2 tag("VtArray::_AllocateNew", __ARCH_PRETTY_FUNCTION__);
        // Need space for the control block and capacity elements.
        // Exceptionally large capacity requests can overflow the arithmetic
        // here.  If that happens we'll just attempt to allocate the max size_t
        // value and let new() throw.
        size_t numBytes = (capacity <= max_size())
            ? sizeof(_ControlBlock) + capacity * sizeof(value_type)
            : std::numeric_limits<size_t>::max();
        void *data = ::operator new(numBytes);
        // Placement-new a control block.
        ::new (data) _ControlBlock(/*count=*/1, capacity);
        // Data starts after the block.
        return reinterpret_cast<value_type *>(
            static_cast<_ControlBlock *>(data) + 1);
    }

    value_type *_AllocateCopy(value_type *src, size_t newCapacity,
                              size_t numToCopy) {
        // Allocate and copy elements.
        value_type *newData = _AllocateNew(newCapacity);
        std::uninitialized_copy(src, src + numToCopy, newData);
        return newData;
    }

    void _DecRef() {
        if (!_data)
            return;
        if (ARCH_LIKELY(!_foreignSource)) {
            // Drop the refcount.  If we take it to zero, destroy the data.
            if (_GetNativeRefCount(_data).fetch_sub(
                    1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                for (value_type *p = _data, *e = _data + _shapeData.totalSize;
                     p != e; ++p) {
                    p->~value_type();
                }
                ::operator delete(static_cast<void *>(
                                      std::addressof(_GetControlBlock(_data))));
            }
        }
        else {
            // Drop the refcount in the foreign source.  If we take it to zero,
            // invoke the function pointer to alert the foreign source.
            if (_foreignSource->_refCount.fetch_sub(
                    1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                _foreignSource->_ArraysDetached();
            }
        }
        _foreignSource = nullptr;
        _data = nullptr;
    }
    
    value_type *_data;
};

// Declare basic array instantiations as extern templates.  They are explicitly
// instantiated in array.cpp.
#define VT_ARRAY_EXTERN_TMPL(unused, elem) \
    VT_API_TEMPLATE_CLASS(VtArray< VT_TYPE(elem) >);
TF_PP_SEQ_FOR_EACH(VT_ARRAY_EXTERN_TMPL, ~, VT_SCALAR_VALUE_TYPES)

template <class HashState, class ELEM>
inline std::enable_if_t<VtIsHashable<ELEM>()>
TfHashAppend(HashState &h, VtArray<ELEM> const &array)
{
    h.Append(array.size());
    h.AppendContiguous(array.cdata(), array.size());
}

template <class ELEM>
typename std::enable_if<VtIsHashable<ELEM>(), size_t>::type
hash_value(VtArray<ELEM> const &array) {
    return TfHash()(array);
}

// Specialize traits so others can figure out that VtArray is an array.
template <typename T>
struct VtIsArray< VtArray <T> > : public std::true_type {};

template <class T>
struct Vt_ArrayOpHelp {
    static T Add(T l, T r) { return l + r; }
    static T Sub(T l, T r) { return l - r; }
    static T Mul(T l, T r) { return l * r; }
    static T Div(T l, T r) { return l / r; }
    static T Mod(T l, T r) { return l % r; }
};

template <class T>
struct Vt_ArrayOpHelpScalar {
    static T Mul(T l, double r) { return l * r; }
    static T Mul(double l, T r) { return l * r; }
    static T Div(T l, double r) { return l / r; }
    static T Div(double l, T r) { return l / r; }
};

// These operations on bool-arrays are highly questionable, but this preserves
// existing behavior in the name of Hyrum's Law.
template <>
struct Vt_ArrayOpHelp<bool> {
    static bool Add(bool l, bool r) { return l | r; }
    static bool Sub(bool l, bool r) { return l ^ r; }
    static bool Mul(bool l, bool r) { return l & r; }
    static bool Div(bool l, bool r) { return l; }
    static bool Mod(bool l, bool r) { return false; }
};

template <>
struct Vt_ArrayOpHelpScalar<bool> {
    static bool Mul(bool l, double r) { return l && (r != 0.0); }
    static bool Mul(double l, bool r) { return (l != 0.0) && r; }
    static bool Div(bool l, double r) { return (r == 0.0) || l; }
    static bool Div(double l, bool r) { return !r || (l != 0.0); }
};

#define VTOPERATOR_CPPARRAY(op, opName)                                        \
    template <class T>                                                         \
    VtArray<T>                                                                 \
    operator op (VtArray<T> const &lhs, VtArray<T> const &rhs)                 \
    {                                                                          \
        using Op = Vt_ArrayOpHelp<T>;                                          \
        /* accept empty vecs */                                                \
        if (!lhs.empty() && !rhs.empty() && lhs.size() != rhs.size()) {        \
            TF_CODING_ERROR("Non-conforming inputs for operator %s", #op);     \
            return VtArray<T>();                                               \
        }                                                                      \
        /* promote empty vecs to vecs of zeros */                              \
        const bool leftEmpty = lhs.size() == 0, rightEmpty = rhs.size() == 0;  \
        VtArray<T> ret(leftEmpty ? rhs.size() : lhs.size());                   \
        T zero = VtZero<T>();                                                  \
        if (leftEmpty) {                                                       \
            std::transform(                                                    \
                rhs.begin(), rhs.end(), ret.begin(),                           \
                [zero](T const &r) { return Op:: opName (zero, r); });         \
        }                                                                      \
        else if (rightEmpty) {                                                 \
            std::transform(                                                    \
                lhs.begin(), lhs.end(), ret.begin(),                           \
                [zero](T const &l) { return Op:: opName (l, zero); });         \
        }                                                                      \
        else {                                                                 \
            std::transform(                                                    \
                lhs.begin(), lhs.end(), rhs.begin(), ret.begin(),              \
                [](T const &l, T const &r) { return Op:: opName (l, r); });    \
        }                                                                      \
        return ret;                                                            \
    }

ARCH_PRAGMA_PUSH
ARCH_PRAGMA_FORCING_TO_BOOL
ARCH_PRAGMA_UNSAFE_USE_OF_BOOL
ARCH_PRAGMA_UNARY_MINUS_ON_UNSIGNED

VTOPERATOR_CPPARRAY(+, Add);
VTOPERATOR_CPPARRAY(-, Sub);
VTOPERATOR_CPPARRAY(*, Mul);
VTOPERATOR_CPPARRAY(/, Div);
VTOPERATOR_CPPARRAY(%, Mod);
    
template <class T>
VtArray<T>
operator-(VtArray<T> const &a) {
    VtArray<T> ret(a.size());
    std::transform(a.begin(), a.end(), ret.begin(),
                   [](T const &x) { return -x; });
    return ret;
}

ARCH_PRAGMA_POP

// Operations on scalars and arrays
// These are free functions defined in Array.h
#define VTOPERATOR_CPPSCALAR(op,opName)                                 \
    template<typename T>                                                \
    VtArray<T> operator op (T const &scalar, VtArray<T> const &arr) {   \
        using Op = Vt_ArrayOpHelp<T>;                                   \
        VtArray<T> ret(arr.size());                                     \
        std::transform(arr.begin(), arr.end(), ret.begin(),             \
                       [&scalar](T const &aObj) {                       \
                           return Op:: opName (scalar, aObj);           \
                       });                                              \
        return ret;                                                     \
    }                                                                   \
    template<typename T>                                                \
    VtArray<T> operator op (VtArray<T> const &arr, T const &scalar) {   \
        using Op = Vt_ArrayOpHelp<T>;                                   \
        VtArray<T> ret(arr.size());                                     \
        std::transform(arr.begin(), arr.end(), ret.begin(),             \
                       [&scalar](T const &aObj) {                       \
                           return Op:: opName (aObj, scalar);           \
                       });                                              \
        return ret;                                                     \
    } 

// define special-case operators on arrays and doubles - except if the array
// holds doubles, in which case we already defined the operator (with
// VTOPERATOR_CPPSCALAR above) so we can't do it again!
#define VTOPERATOR_CPPSCALAR_DOUBLE(op,opName)                          \
    template<typename T>                                                \
    std::enable_if_t<!std::is_same<T, double>::value, VtArray<T>>       \
    operator op (double const &scalar, VtArray<T> const &arr) {         \
        using Op = Vt_ArrayOpHelpScalar<T>;                             \
        VtArray<T> ret(arr.size());                                     \
        std::transform(arr.begin(), arr.end(), ret.begin(),             \
                       [&scalar](T const &aObj) {                       \
                           return Op:: opName (scalar, aObj);           \
                       });                                              \
        return ret;                                                     \
    }                                                                   \
    template<typename T>                                                \
    std::enable_if_t<!std::is_same<T, double>::value, VtArray<T>>       \
    operator op (VtArray<T> const &arr, double const &scalar) {         \
        using Op = Vt_ArrayOpHelpScalar<T>;                             \
        VtArray<T> ret(arr.size());                                     \
        std::transform(arr.begin(), arr.end(), ret.begin(),             \
                       [&scalar](T const &aObj) {                       \
                           return Op:: opName (aObj, scalar);           \
                       });                                              \
        return ret;                                                     \
    } 

// free functions for operators combining scalar and array types
ARCH_PRAGMA_PUSH
ARCH_PRAGMA_FORCING_TO_BOOL
ARCH_PRAGMA_UNSAFE_USE_OF_BOOL
ARCH_PRAGMA_UNARY_MINUS_ON_UNSIGNED
VTOPERATOR_CPPSCALAR(+, Add)
VTOPERATOR_CPPSCALAR(-, Sub)
VTOPERATOR_CPPSCALAR(*, Mul)
VTOPERATOR_CPPSCALAR_DOUBLE(*, Mul)
VTOPERATOR_CPPSCALAR(/, Div)
VTOPERATOR_CPPSCALAR_DOUBLE(/, Div)
VTOPERATOR_CPPSCALAR(%, Mod)
ARCH_PRAGMA_POP

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_ARRAY_H
