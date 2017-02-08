//
// Copyright 2016 Pixar
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
#ifndef VT_ARRAY_H
#define VT_ARRAY_H

/// \file vt/array.h

#include "pxr/pxr.h"
#include "pxr/base/vt/hash.h"
#include "pxr/base/vt/operators.h"
#include "pxr/base/vt/streamOut.h"
#include "pxr/base/vt/traits.h"
#include "pxr/base/vt/types.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/container/vector.hpp>
#include <boost/operators.hpp>
#include <boost/preprocessor.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <iostream>
#include <vector>

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class VtArray 
///
/// Represents an arbitrary dimensional rectangular container class.
///
/// Originally, VtArray was built to mimic the arrays in menv2x's MDL language,
/// but since VtArray has typed elements, the multidimensionality has found
/// little use.  For example, if you have only scalar elements, then to
/// represent a list of vectors you need an two dimensional array.  To represent
/// a list of matrices you need a three dimensional array.  However with
/// VtArray<GfVec3d> and VtArray<GfMatrix4d>, the VtArray is one dimensional,
/// the extra dimensions are encoded in the element types themselves.
///
/// For this reason, VtArray has been moving toward being more like std::vector,
/// and it now has much of std::vector's API, but there are still important
/// differences.
///
/// First, VtArray shares data between instances using a copy-on-write scheme.
/// This means that making copies of VtArray instances is cheap: it only copies
/// the pointer to the data.  But on the other hand, invoking any non-const
/// member function will incur a copy of the underlying data if it is not
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
template<typename ELEM>
class VtArray {

    typedef boost::container::vector<ELEM> _VecType;

  public:

    /// Type this array holds.
    typedef ELEM ElementType;
    typedef ELEM value_type;

    template <typename Value>
    class PointerIterator
        : public boost::iterator_adaptor<PointerIterator<Value>, Value *> {
    public:
        PointerIterator() :
            PointerIterator::iterator_adaptor_(0) {}
        explicit PointerIterator(Value *p) :
            PointerIterator::iterator_adaptor_(p) {}
        template <typename OtherValue>
        PointerIterator(PointerIterator<OtherValue> const &other,
                        typename boost::enable_if_convertible
                        <OtherValue*, Value*>::type* = 0) :
            PointerIterator::iterator_adaptor_(other.base()) {}
      private:
        friend class boost::iterator_core_access;
    };

    /// \defgroup STL_API STL-like API
    /// @{

    /// Iterator type.
    typedef PointerIterator<ElementType> iterator;
    /// Const iterator type.
    typedef PointerIterator<const ElementType> const_iterator;
    /// Reverse iterator type.
    typedef boost::reverse_iterator<iterator> reverse_iterator;
    /// Reverse const iterator type.
    typedef boost::reverse_iterator<const_iterator> const_reverse_iterator;

    /// Reference type.
    typedef typename PointerIterator<ElementType>::reference
        reference;
    /// Const reference type.
    typedef typename PointerIterator<const ElementType>::reference
        const_reference;
    /// Pointer type.
    typedef typename PointerIterator<ElementType>::pointer pointer;
    /// Const pointer type.
    typedef typename PointerIterator<const ElementType>::pointer const_pointer;

    /// @}

    /// Create a size=0 array.
    VtArray() {}

    /// Create a size=n array.
    explicit VtArray(unsigned int n)
    {
        resize(n);
    }

    /// \addtogroup STL_API
    /// @{
    
    /// Return an iterator to the start of the array.  Iterators are
    /// currently linear regardless of dimension.
    iterator begin() { return iterator(data()); }
    /// Returns an iterator to the end of the array.  Iterators are
    /// currently linear regardless of dimension.
    iterator end() { return iterator(data() + size()); }

    /// Return a const iterator to the start of the array.  Iterators
    /// are currently linear regardless of dimension.
    const_iterator begin() const { return const_iterator(data()); }
    /// Return a const iterator to the end of the array.  Iterators are
    /// currently linear regardless of dimension.
    const_iterator end() const { return const_iterator(data() + size()); }

    /// Return a const iterator to the start of the array.  Iterators
    /// are currently linear regardless of dimension.
    const_iterator cbegin() const { return begin(); }
    /// Return a const iterator to the end of the array.  Iterators are
    /// currently linear regardless of dimension.
    const_iterator cend() const { return end(); }

    /// Return a reverse iterator to the end of the array.  Iterators are
    /// currently linear regardless of dimension.
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    /// Return a reverse iterator to the start of the array.  Iterators
    /// are currently linear regardless of dimension.
    reverse_iterator rend() { return reverse_iterator(begin()); }

    /// Return a const reverse iterator to the end of the array.
    /// Iterators are currently linear regardless of dimension.
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }
    /// Return a const reverse iterator to the start of the array.
    /// Iterators are currently linear regardless of dimension.
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    /// Return a const reverse iterator to the end of the array.
    /// Iterators are currently linear regardless of dimension.
    const_reverse_iterator crbegin() const { return rbegin(); }
    /// Return a const reverse iterator to the start of the array.
    /// Iterators are currently linear regardless of dimension.
    const_reverse_iterator crend() const { return rend(); }

    /// Return a pointer to this array's data.
    pointer data() { _Detach(); return _data ? _data->vec.data() : NULL; }
    /// Return a const pointer to this array's data.
    const_pointer data() const { return _data ? _data->vec.data() : NULL; }
    /// Return a const pointer to the data held by this array.
    const_pointer cdata() const { return data(); }

    /// Append an element to array.
    void push_back(ElementType const &elem) {
        if (Vt_ArrayStackCheck(size(), _GetReserved())) {
            if (!_data)
                _data.reset(new _Data());
            else
                _Detach();

            _Data *d = _data.get();
            d->vec.push_back(elem);
        }
    }

    /// Remove the last element of an array.
    void pop_back() {
        if (Vt_ArrayStackCheck(size(), _GetReserved())) {
            _Detach();

            _Data *d = _data.get();
            d->vec.pop_back();
        }
    }

    /// Return the total number of elements in this array.
    size_t size() const { return _data ? _data->vec.size() : 0; }

    /// Equivalent to size() == 0.
    bool empty() const { return size() == 0; }
    
    /// Ensure enough memory is allocated to hold \p num elements.
    void reserve(size_t num) {
        if (num >= size()) {
            if (!_data)
                _data.reset(new _Data);
            else
                _Detach();
            _data->vec.reserve(num);
        }
    }

    /// Return a reference to the first element in this array.  Invokes
    /// undefined behavior if the array is empty.
    reference front() { return *begin(); }
    /// Return a const reference to the first element in this array.
    /// Invokes undefined behavior if the array is empty.
    const_reference front() const { return *begin(); }

    /// Return a reference to the last element in this array.  Invokes
    /// undefined behavior if the array is empty.
    reference back() { return *rbegin(); }
    /// Return a const reference to the last element in this array.
    /// Invokes undefined behavior if the array is empty.
    const_reference back() const { return *rbegin(); }

    /// Resize this array.
    /// Preserves existing data that remains, value-initializes any newly added
    /// data.  For example, resize(10) on an array of size 5 would change the
    /// size to 10, the first 5 elements would be left unchanged and the last
    /// 5 elements would be value-initialized.
    void resize(size_t num) {
        if (size() == num) {
            return;
        }
        if (num == 0) {
            clear();
            return;
        }

        TfAutoMallocTag tag("VtArray::reshape");

        if (!_data)
            _data.reset(new _Data);

        if (_data->IsUnique()) {
            _data->vec.resize(num);
        } else {
            // Detach from existing vec and copy contents.
            _DataPtr newData(new _Data(*_data, _NoValues()));
            if (num != 0) {
                size_t numToCopy = std::min(num, _data->vec.size());
                newData->vec.resize(num);
                std::copy(_data->vec.begin(), _data->vec.begin() + numToCopy,
                          newData->vec.begin());
            }
            _data = newData;
        }
    }        

    /// Equivalent to resize(0).
    void clear()
    {
        _data.reset();
    }

    /// Assign array contents.
    /// Equivalent to:
    /// \code
    /// array.resize(std::distance(first, last));
    /// std::copy(first, last, array.begin());
    /// \endcode
    template <class ForwardIter>
    void assign(ForwardIter first, ForwardIter last) {
        resize(std::distance(first, last));
        std::copy(first, last, begin());
    }

    /// Assign array contents.
    /// Equivalent to:
    /// \code
    /// array.resize(n);
    /// std::fill(array.begin(), array.end(), fill);
    /// \endcode
    void assign(size_t n, const value_type &fill) {
        resize(n);
        std::fill(begin(), end(), fill);
    }

    /// Swap the contents of this array with \p other.
    void swap(VtArray &other) { 
        _data.swap(other._data);
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
        return _data == other._data;
    }

    /// Tests two arrays for equality.  See also IsIdentical().
    bool operator == (VtArray const & other) const {
        return IsIdentical(other) || 
            (Vt_ArrayCompareSize(size(), _GetReserved(),
                                 other.size(), other._GetReserved()) &&
             std::equal(begin(), end(), other.begin()));
    }

    /// Tests two arrays for inequality.
    bool operator != (VtArray const &other) const {
        return !(*this == other);
    }

    VTOPERATOR_CPPARRAY(+)
    VTOPERATOR_CPPARRAY(-)
    VTOPERATOR_CPPARRAY(*)
    VTOPERATOR_CPPARRAY(/)
    VTOPERATOR_CPPARRAY(%)
    VTOPERATOR_CPPARRAY_UNARY(-)

  public:
    // XXX -- Public so VtValue::_ArrayHelper<T,U>::GetReserved() has access.
    Vt_Reserved* _GetReserved() {
        if (!_data) {
            _data.reset(new _Data);
        }
        return &_data->reserved;
    }
    const Vt_Reserved* _GetReserved() const {
        return _data ? &_data->reserved : 0;
    }

  private:
    class _Streamer : public VtStreamOutIterator {
    public:
        _Streamer(const_pointer data) : _p(data) { }
        virtual ~_Streamer() { }
        virtual void Next(std::ostream &out)
        {
            VtStreamOut(*_p++, out);
        }

    private:
        const_pointer _p;
    };

    /// Outputs a comma-separated list of the values in the array.
    friend std::ostream &operator <<(std::ostream &out, const VtArray &self) {
        VtArray::_Streamer streamer(self.cdata());
        VtStreamOutArray(&streamer, self.size(), self._GetReserved(), out);
        return out;
    }

    /// Swap array contents.
    friend void swap(VtArray &lhs, VtArray &rhs) {
        lhs.swap(rhs);
    }

    void _Detach() {
        if (_data && !_data->IsUnique())
            _data.reset(new _Data(*_data));
    }

    struct _NoValues {};
    struct _Data {

        _Data() : _refCount(0) {}

        _Data(_Data const &other) 
            : vec(other.vec), reserved(other.reserved), _refCount(0) {}

        _Data(_Data const &other, _NoValues) 
            : reserved(other.reserved), _refCount(0) {}

        _Data &operator=(_Data const &other) {
            vec = other.vec;
            reserved = other.reserved;
            return *this;
        }

        bool IsUnique() const { return _refCount == 1; }

        _VecType vec;
        Vt_Reserved reserved;

    private:
        mutable std::atomic<size_t> _refCount;

        friend inline void intrusive_ptr_add_ref(_Data const *d) {
            d->_refCount++;
        }
        friend inline void intrusive_ptr_release(_Data const *d) {
            if (--d->_refCount == 0)
                delete d;
        }
    };

    typedef boost::intrusive_ptr<_Data> _DataPtr;

    _DataPtr _data;
};

template <class ELEM>
typename std::enable_if<VtIsHashable<ELEM>(), size_t>::type
hash_value(VtArray<ELEM> const &array) {
    size_t h = array.size();
    for (auto const &x: array) {
        boost::hash_combine(h, x);
    }
    return h;
}

// Specialize traits so others can figure out that VtArray is an array.
template <typename T>
struct VtIsArray< VtArray <T> > : public VtTrueType {};

// free functions for operators combining scalar and array types
VTOPERATOR_CPPSCALAR(+)
VTOPERATOR_CPPSCALAR(-)
VTOPERATOR_CPPSCALAR(*)
VTOPERATOR_CPPSCALAR_DOUBLE(*)
VTOPERATOR_CPPSCALAR(/)
VTOPERATOR_CPPSCALAR_DOUBLE(/)
VTOPERATOR_CPPSCALAR(%)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // VT_ARRAY_H
