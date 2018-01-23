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
#ifndef TF_COPY_ON_WRITE_PTR
#define TF_COPY_ON_WRITE_PTR

/// \file tf/copyOnWritePtr.h
/// Provides a simple mechanism for implementing copy-on-write internally
/// shared objects.
///
/// A smart pointer that points to a shared data object.  Const accesses
/// simply dereference like regular pointers.  Non-const accesses will
/// "detach" from a shared data object if more than one client is sharing it.
/// This lets us create implicitly shared copy-on-write classes easily.  Here
/// is a complete comparison example showing how to make a class implicitly
/// shared.
///
/// \code
/// class Unshared {
///   public:
///     Unshared() {}
///     string const &GetString() const { return _str; }
///     void SetString(string str) { _str = str; }
///   private:
///     string _str;
/// };
/// \endcode
///
/// To make this class use implicit sharing, simply make a private helper
/// struct which stores the data members, then store a CopyOnWritePtr to this
/// struct in the class.
///
/// \code
/// class Shared {
///   public:
///     Shared() : _data(new _Data) {}
///     string const &GetString() const { return _data->_str; }
///     void SetString(string str) { _data->_str = str; }
///   private:
///     struct _Data { string _str; };
///     TfCopyOnWritePtr<_Data> _data;
/// };
/// \endcode
///
/// For larger classes this can be a performance win, as well as simplify
/// notation since objects can be passed around by value without incurring
/// massive copying hits.  For instance, Qt's QPixmap class uses this
/// technique so users may pass QPixmaps around as if they are value types but
/// no copies occur until a QPixmap's content is changed.
///
/// Copy-on-write pointers are reference counted so there is no need to
/// explicity delete the memory pointed to in the above example.
///
/// Note that default-constructed copy-on-write pointers are null, and may be
/// checked for null, but do not need to be checked for null.  Copy-on-write
/// pointers will allocate on demand if necessary.

#include "pxr/pxr.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/refPtr.h"

#include <boost/mpl/if.hpp>
#include <boost/operators.hpp>
#include <boost/type_traits/is_base_of.hpp>

PXR_NAMESPACE_OPEN_SCOPE

// General case -- use shared_ptr.
template <typename Pointee>
struct Tf_CowSharedPtrHelper {
    typedef std::shared_ptr<Pointee> PtrType;
    static PtrType New(Pointee const *p = 0) {
        return p ? PtrType(new Pointee(*p)) : PtrType(new Pointee());
    }
    static void Reset(PtrType &ptr) {
        ptr.reset();
    }
    static bool IsUnique(PtrType const &ptr) {
        return ptr.unique();
    }
};

// For things that derive from TfRefBase, use TfRefPtr.
template <typename Pointee>
struct Tf_CowRefPtrHelper {
    typedef TfRefPtr<Pointee> PtrType;
    static PtrType New(Pointee const *p = 0) {
        return p ? TfCreateRefPtr(new Pointee(*p)) :
            TfCreateRefPtr(new Pointee());
    }
    static void Reset(PtrType &ptr) {
        ptr.Reset();
    }
    static bool IsUnique(PtrType const &ptr) {
        return ptr->IsUnique();
    }
};

/// \class TfCopyOnWritePtr
///
template<typename T>
class TfCopyOnWritePtr : boost::equality_comparable<TfCopyOnWritePtr<T> >
{
    // Choose the helper.
    typedef typename boost::mpl::if_<boost::is_base_of<TfRefBase, T>,
        Tf_CowRefPtrHelper<T>, Tf_CowSharedPtrHelper<T> >::type _Helper;
    
  public:

    /// Data type
    typedef T Data;
    /// Pointer to data type
    typedef T *Pointer;
    /// Const pointer to data type
    typedef T const *ConstPointer;
    /// Internally held pointer type.
    typedef typename _Helper::PtrType PtrType;

    typedef TfCopyOnWritePtr<Data> This;

    /// Default constructor leaves pointer NULL.
    TfCopyOnWritePtr() {}

    /// Construct with a copy of \a data.
    explicit TfCopyOnWritePtr(Data const &data) :
        _ptr(_Helper::New(&data)) {}
    
    /// Copy construct a copy on write pointer with ptr.
    explicit TfCopyOnWritePtr(PtrType const &ptr) :
        _ptr(ptr) {}

    /// Destructor
    ~TfCopyOnWritePtr() {}

    operator bool() const {
        return static_cast<bool>(_ptr);
    }
    
    /// Returns true if the pointer is NULL.
    bool operator !() const {
        return !_ptr;
    }

    /// Returns true if \a this and \a other point at the same object.
    bool operator ==(const This &other) const {
        return _ptr == other._ptr;
    }

    /// Const get -- does not copy.
    Pointer get() const {
        _AllocateIfNull();
        return get_pointer(_ptr);
    }

    /// Non-const get -- causes a copy of not \a IsUnique().
    Pointer get() {
        _Detach();
        return get_pointer(_ptr);
    }

    /// Replace what's pointed to by this pointer with a copy of \a
    // data.
    void Reset(Data const &data) {
        _ptr = _Helper::New(&data);
    }

    /// Set this pointer to NULL.
    void Reset() {
        _Helper::Reset(_ptr);
    }

    /// Non-const dereference -- causes a copy if not \a IsUnique().
    Data &operator *() {
        return *get();
    }

    /// Const dereference -- never copies.
    const Data &operator *() const {
        return *get();
    }

    /// Non-const indirection -- causes a copy if not \a IsUnique().
    Pointer operator ->() {
        return get();
    }

    /// Const indirection -- never copies.
    Pointer operator ->() const {
        return get();
    }

    /// Returns true if this pointer is unique, that is, if this is the
    //only pointer pointing to this data.  This means that operations that would
    //ordinarily force a copy will not.
    bool IsUnique() const {
        return _Helper::IsUnique(_ptr);
    }

    void swap(This &other) {
        _ptr.swap(other._ptr);
    }
    
  private:

    void _Detach() {
        if (!_ptr || !IsUnique())
            _ptr = _Helper::New(get_pointer(_ptr));
    }

    void _AllocateIfNull() const {
        if (!_ptr)
            _ptr = _Helper::New();
    }

    mutable PtrType _ptr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_COPY_ON_WRITE_PTR
