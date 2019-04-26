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
#ifndef TF_WEAKPTR_H
#define TF_WEAKPTR_H

/// \file tf/weakPtr.h
/// \ingroup group_tf_Memory
/// Pointer storage with deletion detection.

#include "pxr/pxr.h"

#include "pxr/base/tf/nullPtr.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtrFacade.h"

#include <cstddef>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

class TfHash;
template <class U> class TfRefPtr;
template <class T> class TfWeakPtr;

/// \class TfWeakPtr
/// \ingroup group_tf_Memory
///
/// Pointer storage with deletion detection.
///
/// <b>Overview</b>
///
/// A \c TfWeakPtr is used to cache a pointer to an object; before
/// retrieving/using this pointer, one queries the \c TfWeakPtr object to
/// verify that the objected pointed to has not been deleted in the interim.
///
/// \include test/weakPtr.cpp
///
/// In the code above, if \c PossiblyDeleteLemur() deletes the object pointed
/// to by \c lemur, then the test \c if(lPtr) returns false.  Otherwise, it is
/// safe to call a method on \c lPtr.
///
/// To declare a \c TfWeakPtr<T>, the type \c T must publicly derive from \c
/// TfWeakBase.
///
/// <b>Basic Use</b>
///
/// A \c TfWeakPtr<T> can access \c T's public members by the \c -> operator
/// and can be dereferenced by the "\c *" operator.
///
/// A \c TfWeakPtr converts to a \c true bool value (for example, in an \c if
/// statement) only if the pointer points to an unexpired object.  Otherwise,
/// if the pointer was either initialized to NULL, or points to an expired
/// object, the test returns false.
///
/// Occasionally, it is useful to distinguish between a \c TfWeakPtr being
/// explicitly initialized to NULL versus a \c TfWeakPtr whose object has
/// expired: the member function \c IsInvalid() returns \c true only if the
/// pointer points to an expired object.
///
/// <b>Opaqueness</b>
///
/// See the parallel discussion about these concepts in the documentation for
/// \c TfRefPtr; the same concepts apply.
///
/// <b>Comparisons, Const and Non-Const, Inheritance and Casting</b>
///
/// See the parallel discussion about these concepts in the documentation for
/// \c TfRefPtr; the same concepts apply.
///
/// While it is possible to create TfWeakPtrs to const contents, we recommend
/// against it.  TfCreateNonConstWeakPtr will always create a non-const weak
/// pointer even when passed a const argument (it casts away const).
///
/// The recommendation against use of weak pointers to const content is due to
/// the fact that weak pointers cannot be implicitly cast for both inheritance
/// (derived to base) and const-ness (non-const to const) at the same time.
/// Because of this, using weak pointers to const content is most often much
/// more trouble than the benefit it gives.  Therefore our policy is to not
/// use them.
///
/// <b>Pointer Generality</b>
///
/// While \c TfWeakPtr<TfWeakBase> is specifically forbidden (you cannot
/// construct this kind of object), you can assign any \c TfWeakPtr<T> to a \c
/// TfWeakPtr<void> or TfWeakPtr<const void>.  The only thing you can do with
/// the latter is check to see if it points to an object that has expired.
/// You cannot manipulate the object itself (i.e. access its member
/// functions).
///
/// This is useful when you need to watch for object expiration without being
/// bound by the type(s) of the objects you're watching.  Similarly, you can
/// create a TfWeakPtr<void> from a \c TfWeakBase * using \c
/// TfCreateWeakPtr().
///
/// <b>Performance</b>
///
/// Deriving from \c TfWeakBase results in a single \c TfRefPtr variable being
/// added to a class, which is the size of a regular pointer.  The cost of
/// deleting an object derived from \c TfWeakBase is an extra inline boolean
/// comparison, and possible decrement of a reference count if the object's
/// address was ever given out as a \c TfWeakPtr.
///
/// The cost to create a \c TfWeakPtr is as follows: initial creation of the
/// pointer from a \c TfWeakBase object dynamically creates an object called a
/// \e remnant, whose size is that of two pointers. Subsequent transfers of
/// the same object's address to another \c TfWeakPtr merely bump a reference
/// count to the remnant. When all \c TfWeakPtrs to the object (and the object
/// itself) are destroyed, the remnant is deleted.  An object can have a
/// remnant created and destroyed at most once, regardless of how many times
/// its address is given out in the form of a \c TfWeakPtr.
///
/// Summarizing, the cost of guarding an object is a small amount of extra
/// space, and near-zero runtime cost if the guarding is never used.  Even if
/// the guarding is used, the overhead at deletion time is minimal.
///
/// The time to test if a \c TfWeakPtr is NULL, or to call a member function
/// through a \c TfWeakPtr is small, involving only a single inline boolean
/// comparison.
///
template <class T>
class TfWeakPtr : public TfWeakPtrFacade<TfWeakPtr, T>
{
public:
    
    friend class TfWeakPtrFacadeAccess;
    template <class U> friend class TfWeakPtr;

    template <class U> struct Rebind {
        typedef TfWeakPtr<U> Type;
    };
    
    TfWeakPtr() : _rawPtr(0) {}

    /// Construction, implicit conversion from TfNullPtr.
    TfWeakPtr(TfNullPtrType) : _rawPtr(0) {}

    /// Construction, implicit conversion from nullptr.
    TfWeakPtr(std::nullptr_t) : _rawPtr(nullptr) {}

    /// Copy construction
    TfWeakPtr(TfWeakPtr const &p) : _rawPtr(p._rawPtr), _remnant(p._remnant)
    {
    }

    /// Conversion from \a RefPtr where \a U* is convertible to \a T* (this
    /// pointer type).
    template <class U>
    TfWeakPtr(TfRefPtr<U> const &p,
              typename std::enable_if<
                  std::is_convertible<U*, T*>::value
              >::type *dummy = 0) : _rawPtr(get_pointer(p))
    {
        TF_UNUSED(dummy);
        if (ARCH_LIKELY(_rawPtr))
            _remnant = Tf_WeakBaseAccess::
                GetRemnant(_rawPtr->__GetTfWeakBase__());
    }

    /// Explicitly construct from a raw pointer \a p.
    template <class U>
    explicit TfWeakPtr(U *p, typename std::enable_if<
        std::is_convertible<U*, T*>::value>::type *dummy = nullptr) : _rawPtr(p)
    {
        TF_UNUSED(dummy);
        if (ARCH_LIKELY(_rawPtr))
            _remnant = Tf_WeakBaseAccess::
                GetRemnant(_rawPtr->__GetTfWeakBase__());
    }

    template <class U>
    TfWeakPtr(TfWeakPtr<U> const &p,
              typename std::enable_if<
                  std::is_convertible<U*, T*>::value
              >::type *dummy = 0) : _rawPtr(p._rawPtr), _remnant(p._remnant)
    {
    }

    bool IsExpired() const {
        return this->IsInvalid();
    }
    
private:

    T *_FetchPointer() const {
        if (ARCH_LIKELY(_remnant && _remnant->_IsAlive()))
            return _rawPtr;
        return 0;
    }

    bool _IsInvalid() const {
        return _remnant && !_remnant->_IsAlive();
    }

    void const *_GetUniqueIdentifier() const {
        return _remnant ? _remnant->_GetUniqueIdentifier() : 0;
    }

    void _EnableExtraNotification() const {
        _remnant->EnableNotification();
    }
    
    T *_rawPtr;
    mutable TfRefPtr<Tf_Remnant> _remnant;
    
};


template <class U>
TfWeakPtr<U> TfCreateWeakPtr(U *p) {
    return TfWeakPtr<U>(p);
}

template <class U>
TfWeakPtr<U> TfCreateNonConstWeakPtr(U const *p) {
    return TfWeakPtr<U>(const_cast<U *>(p));
}

/// Thread-safe creation of a Tf ref pointer from a Tf weak pointer.
///
/// This is thread-safe in the sense that the result will be either a ref
/// pointer to a live object with non-zero ref-count, or a NULL ref pointer.
/// However, this depends on the client to provide a guarantee to protect the
/// pointed-to object.
///
/// Specifically, the caller must guarantee that the TfRefBase part of the
/// pointed-to object is not destroyed during this call. It is fine if the
/// destruction process for the object begins (due to the ref-count going to
/// zero as another thread drops the last ref) as long as the TfRefBase
/// portion is not destroyed. If object destruction begins because the
/// ref-count goes to zero before this call completes, this function will
/// reliably return a NULL ref pointer.
///
/// Note that this is not a general mechanism for safely converting weak
/// pointers to ref pointers, because it relies on the type T to provide the
/// above guarantee.
///
template <class T>
TfRefPtr<T>
TfCreateRefPtrFromProtectedWeakPtr(TfWeakPtr<T> const &p) {
    typedef typename TfRefPtr<T>::_Counter Counter;
    if (T *rawPtr = get_pointer(p)) {
        // Atomically increment the ref-count iff it's nonzero.
        if (Counter::AddRefIfNonzero(rawPtr)) {
            // There was at least 1 other ref at the time we acquired our ref,
            // so this object is safe from destruction.  Transfer ownership of
            // the ref to a new TfRefPtr.
            return TfCreateRefPtr(rawPtr);
        }
        // There were 0 refs to this object, so we know it is expiring and
        // we cannot use it.
    }
    return TfNullPtr;
}


#if !defined(doxygen)

//
// Allow TfWeakPtr<void> to be used simply for expiration checking.
//
template <>
class TfWeakPtr<void> {
public:
    TfWeakPtr() {
    }

    template <class U>
    TfWeakPtr(TfWeakPtr<U> const& wp)
        : _remnant(wp._remnant) {
    }

    template <template <class> class PtrTemplate, class Type>
    TfWeakPtr(TfWeakPtrFacade<PtrTemplate, Type> const& wpf)
        : _remnant(_GetRemnant(wpf)) {
    }

    template <class U>
    TfWeakPtr<void>&
    operator= (TfWeakPtr<U> const& wp) {
        _remnant = wp._remnant;
        return *this;
    }

    template <template <class> class PtrTemplate, class Type>
    TfWeakPtr<void>&
    operator= (TfWeakPtrFacade<PtrTemplate, Type> const& wpf) {
        _remnant = _GetRemnant(wpf);
        return *this;
    }

    template <class U>
    bool operator== (TfWeakPtr<U> const& wp) const {
        return wp._remnant == _remnant;
    }
    
    template <template <class> class PtrTemplate, class Type>
    bool operator== (TfWeakPtrFacade<PtrTemplate, Type> const& wpf) const {
        return _GetRemnant(wpf) == _remnant;
    }
    
    template <class U>
    bool operator!= (TfWeakPtr<U> const& wp) const {
        return wp._remnant != _remnant;
    }

    template <template <class> class PtrTemplate, class Type>
    bool operator!= (TfWeakPtrFacade<PtrTemplate, Type> const& wpf) const {
        return _GetRemnant(wpf) != _remnant;
    }

    template <class U>
    bool operator< (TfWeakPtr<U> const& wp) {
        return wp._remnant < _remnant;
    }

    template <template <class> class PtrTemplate, class Type>
    bool operator< (TfWeakPtrFacade<PtrTemplate, Type> const& wpf) {
        return _GetRemnant(wpf) < _remnant;
    }

    using UnspecifiedBoolType = TfRefPtr<Tf_Remnant> (TfWeakPtr::*);

    operator UnspecifiedBoolType() const {
        return (_remnant && _remnant->_IsAlive())
            ? &TfWeakPtr::_remnant : nullptr;
    }

    bool operator !() const {
        return !bool(*this);
    }

    bool IsExpired() const {
        return _remnant && !_remnant->_IsAlive();
    }

private:
    template <template <class> class PtrTemplate, class Type>
    static TfRefPtr<Tf_Remnant>
    _GetRemnant(TfWeakPtrFacade<PtrTemplate, Type> const& wpf) {
        TfWeakBase const *weakBase = wpf.GetWeakBase();
        if (ARCH_LIKELY(weakBase)) {
            return Tf_WeakBaseAccess::GetRemnant(*weakBase);
        }
        return TfNullPtr;
    }

private:
    TfRefPtr<Tf_Remnant> _remnant;
};

#endif


//
// A mechanism to determine whether a class type has a method
// __GetTfWeakBase__ with the correct signature.
//
// _HasSig can only be called with a pointer-to-member-function that matches
// the desired signature of __GetTfWeakBase__.
//
// _Deduce has two possible overloads.  The first overload's return value uses
// expression SFINAE to detect if a call to _HasSig(&T::__GetTfWeakBase__) is
// well-formed.  If so, the overload's return type is the return type of
// _HasSig, specifically std::true_type.  The second _Deduce overload returns
// std::false_type and is viable for all types.
//
template <class T>
struct Tf_HasGetWeakBase
{
private:

    // The required method signature of __GetTfWeakBase__ for implementations
    // of the weak pointable interface.
    template <class U>
    using _SignatureOf__GetTfWeakBase__ = TfWeakBase const & (U::*)() const;

    template <class U>
    static std::true_type
    _HasSig(_SignatureOf__GetTfWeakBase__<U>);

    template <class U>
    static decltype(_HasSig(&U::__GetTfWeakBase__))
    _Deduce(U*);

    static std::false_type
    _Deduce(...);

public:
    using type = decltype(_Deduce(static_cast<T*>(nullptr)));
    using value_type = bool;
    static const bool value = type::value;
};

template <class T>
struct Tf_SupportsWeakPtr
{
    static const bool value =
        std::is_base_of<TfWeakBase, T>::value || Tf_HasGetWeakBase<T>::value;
};

#define TF_SUPPORTS_WEAKPTR(T) (Tf_SupportsWeakPtr<T>::value)
#define TF_TRULY_SUPPORTS_WEAKPTR(T)   std::is_base_of<TfWeakBase, T >::value

#define TF_DECLARE_WEAK_POINTABLE_INTERFACE                     \
    virtual TfWeakBase const &__GetTfWeakBase__() const = 0

#define TF_IMPLEMENT_WEAK_POINTABLE_INTERFACE                   \
    virtual TfWeakBase const &__GetTfWeakBase__() const {       \
        return *this;                                           \
    }

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_WEAKPTR_H
