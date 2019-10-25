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
#ifndef TF_ANYWEAKPTR_H
#define TF_ANYWEAKPTR_H

/// \file tf/anyWeakPtr.h
/// \ingroup group_tf_Memory
/// Type independent WeakPtr holder class

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/cxxCast.h"
#include "pxr/base/tf/traits.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakPtr.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyUtils.h"
#include <boost/python/object.hpp>
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <boost/operators.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfAnyWeakPtr
///
/// Provides the ability to hold an arbitrary TfWeakPtr in a non-type-specific
/// manner in order to observe whether it has expired or not
class TfAnyWeakPtr : boost::totally_ordered<TfAnyWeakPtr>
{
    struct _Data {
        void* space[4];
    };

public:
    typedef TfAnyWeakPtr This;

    /// Construct an AnyWeakPtr watching \a ptr.
    template <class Ptr, class = typename
              std::enable_if<Tf_SupportsWeakPtr<
                                 typename Ptr::DataType>::value>::type>
    TfAnyWeakPtr(Ptr const &ptr) {
        static_assert(sizeof(_PointerHolder<Ptr>) <= sizeof(_Data),
                      "Ptr is too big to fit in a TfAnyWeakPtr");
        new (&_ptrStorage) _PointerHolder<Ptr>(ptr);
    }

    /// Construct an AnyWeakPtr not watching any \a ptr.
    TfAnyWeakPtr() {
        static_assert(sizeof(_EmptyHolder) <= sizeof(_Data),
                      "Ptr is too big to fit in a TfAnyWeakPtr");
        new (&_ptrStorage) _EmptyHolder;
    }

    /// Construct and implicitly convert from TfNullPtr.
    TfAnyWeakPtr(TfNullPtrType) : TfAnyWeakPtr() {}

    /// Construct and implicitly convert from std::nullptr_t.
    TfAnyWeakPtr(std::nullptr_t) : TfAnyWeakPtr() {}

    TfAnyWeakPtr(TfAnyWeakPtr const &other) {
        other._Get()->Clone(&_ptrStorage);
    }

    TfAnyWeakPtr &operator=(TfAnyWeakPtr const &other) {
        if (this != &other) {
            _Get()->~_PointerHolderBase();
            other._Get()->Clone(&_ptrStorage);
        }
        return *this;
    }

    ~TfAnyWeakPtr() {
        _Get()->~_PointerHolderBase();
    }

    /// Return true *only* if this expiry checker is watching a weak pointer
    /// which has expired.
    TF_API bool IsInvalid() const;

    /// Return the unique identifier of the WeakPtr this AnyWeakPtr contains
    TF_API void const *GetUniqueIdentifier() const;

    /// Return the TfWeakBase object of the WeakPtr we are holding
    TF_API TfWeakBase const *GetWeakBase() const;

    /// bool operator
    TF_API operator bool() const;

    /// operator !
    TF_API bool operator !() const;

    /// equality operator
    TF_API bool operator ==(const TfAnyWeakPtr &rhs) const;

    /// comparison operator
    TF_API bool operator <(const TfAnyWeakPtr &rhs) const;

    /// returns the type_info of the underlying WeakPtr
    TF_API const std::type_info & GetTypeInfo() const;

    /// Returns the TfType of the underlying WeakPtr.
    TF_API TfType const& GetType() const;

    /// Return a hash value for this instance.
    size_t GetHash() const {
        return reinterpret_cast<uintptr_t>(GetUniqueIdentifier()) >> 3;
    }

  private:
#ifdef PXR_PYTHON_SUPPORT_ENABLED
    // This grants friend access to a function in the wrapper file for this
    // class.  This lets the wrapper reach down into an AnyWeakPtr to get a
    // boost::python wrapped object corresponding to the held type.  This
    // facility is necessary to get the python API we want.
    friend boost::python::api::object
    Tf_GetPythonObjectFromAnyWeakPtr(This const &self);

    TF_API
    boost::python::api::object _GetPythonObject() const;
#endif // PXR_PYTHON_SUPPORT_ENABLED

    template <class WeakPtr>
    friend WeakPtr TfAnyWeakPtrDynamicCast(const TfAnyWeakPtr &anyWeak, WeakPtr*);

    // This is using the standard type-erasure pattern.
    struct _PointerHolderBase {
        TF_API virtual ~_PointerHolderBase();
        virtual void Clone(_Data *target) const = 0; 
        virtual bool IsInvalid() const = 0;
        virtual void const * GetUniqueIdentifier() const = 0;
        virtual TfWeakBase const *GetWeakBase() const = 0;
        virtual operator bool() const = 0;
        virtual bool _IsConst() const = 0;
#ifdef PXR_PYTHON_SUPPORT_ENABLED
        virtual boost::python::api::object GetPythonObject() const = 0;
#endif // PXR_PYTHON_SUPPORT_ENABLED
        virtual const std::type_info & GetTypeInfo() const = 0;
        virtual TfType const& GetType() const = 0;
        virtual const void* _GetMostDerivedPtr() const = 0;
        virtual bool _IsPolymorphic() const = 0;
    };

    struct _EmptyHolder : _PointerHolderBase {
        TF_API virtual ~_EmptyHolder();
        TF_API virtual void Clone(_Data *target) const; 
        TF_API virtual bool IsInvalid() const;
        TF_API virtual void const * GetUniqueIdentifier() const;
        TF_API virtual TfWeakBase const *GetWeakBase() const;
        TF_API virtual operator bool() const;
        TF_API virtual bool _IsConst() const;
#ifdef PXR_PYTHON_SUPPORT_ENABLED
        TF_API virtual boost::python::api::object GetPythonObject() const;
#endif // PXR_PYTHON_SUPPORT_ENABLED
        TF_API virtual const std::type_info & GetTypeInfo() const;
        TF_API virtual TfType const& GetType() const;
        TF_API virtual const void* _GetMostDerivedPtr() const;
        TF_API virtual bool _IsPolymorphic() const;
    };
    
    template <typename Ptr>
    struct _PointerHolder : _PointerHolderBase {
        _PointerHolder(Ptr const &ptr) : _ptr(ptr) {
        }

        virtual ~_PointerHolder();
        virtual void Clone(_Data *target) const; 
        virtual bool IsInvalid() const;
        virtual void const *GetUniqueIdentifier() const;
        virtual TfWeakBase const *GetWeakBase() const;
        virtual operator bool() const;
        virtual bool _IsConst() const;
#ifdef PXR_PYTHON_SUPPORT_ENABLED
        virtual boost::python::api::object GetPythonObject() const;
#endif // PXR_PYTHON_SUPPORT_ENABLED
        virtual const std::type_info & GetTypeInfo() const;
        virtual TfType const& GetType() const;
        virtual const void* _GetMostDerivedPtr() const;
        virtual bool _IsPolymorphic() const;
      private:
        Ptr _ptr;
    };

    _PointerHolderBase* _Get() const {
        return (_PointerHolderBase*)(&_ptrStorage);
    }
    
    _Data _ptrStorage;
};

template <class Ptr>
TfAnyWeakPtr::_PointerHolder<Ptr>::~_PointerHolder() {}

template <class Ptr>
void
TfAnyWeakPtr::_PointerHolder<Ptr>::Clone(_Data *target) const
{
    new (target) _PointerHolder<Ptr>(_ptr);
}

template <class Ptr>
bool
TfAnyWeakPtr::_PointerHolder<Ptr>::IsInvalid() const
{
    return _ptr.IsInvalid();
}

template <class Ptr>
void const *
TfAnyWeakPtr::_PointerHolder<Ptr>::GetUniqueIdentifier() const
{
    return _ptr.GetUniqueIdentifier();
}

template <class Ptr>
TfWeakBase const *
TfAnyWeakPtr::_PointerHolder<Ptr>::GetWeakBase() const
{
    return &(_ptr->__GetTfWeakBase__());
}

template <class Ptr>
TfAnyWeakPtr::_PointerHolder<Ptr>::operator bool() const
{
    return bool(_ptr);
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
template <class Ptr>
boost::python::api::object
TfAnyWeakPtr::_PointerHolder<Ptr>::GetPythonObject() const
{
    return TfPyObject(_ptr);
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

template <class Ptr>
const std::type_info &
TfAnyWeakPtr::_PointerHolder<Ptr>::GetTypeInfo() const
{
    return TfTypeid(_ptr);
}

template <class Ptr>
TfType const&
TfAnyWeakPtr::_PointerHolder<Ptr>::GetType() const
{
    return TfType::Find(_ptr);
}

template <class Ptr>
const void *
TfAnyWeakPtr::_PointerHolder<Ptr>::_GetMostDerivedPtr() const
{
    if (!_ptr) {
        return 0;
    }

    typename Ptr::DataType const *rawPtr = get_pointer(_ptr);
    return TfCastToMostDerivedType(rawPtr);
}

template <class Ptr>
bool
TfAnyWeakPtr::_PointerHolder<Ptr>::_IsPolymorphic() const
{
    return std::is_polymorphic<typename Ptr::DataType>::value;
}

template <class Ptr>
bool
TfAnyWeakPtr::_PointerHolder<Ptr>::_IsConst() const
{
    return TfTraits::Type<typename Ptr::DataType>::isConst;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
