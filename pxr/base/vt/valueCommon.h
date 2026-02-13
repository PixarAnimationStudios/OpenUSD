//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_VALUE_COMMON_H
#define PXR_BASE_VT_VALUE_COMMON_H

#include "pxr/pxr.h"

#include "pxr/base/tf/anyUniquePtr.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/tf.h"

#include "pxr/base/vt/api.h"
#include "pxr/base/vt/traits.h"
#include "pxr/base/vt/types.h"

#include <typeinfo>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// Make a default value.
/// VtValue uses this to create values to be returned from failed calls to \a
/// Get. Clients may specialize this for their own types.
template <class T>
struct Vt_DefaultValueFactory;

// This is a helper class used by Vt_DefaultValueFactory to return a value with
// its type erased and only known at runtime via a std::type_info.
struct Vt_DefaultValueHolder
{
    // Creates a value-initialized object and stores the type_info for the
    // static type.
    template <typename T>
    static Vt_DefaultValueHolder Create() {
        return Vt_DefaultValueHolder(TfAnyUniquePtr::New<T>(), typeid(T));
    }

    // Creates a copy of the object and stores the type_info for the static
    // type.
    template <typename T>
    static Vt_DefaultValueHolder Create(T const &val) {
        return Vt_DefaultValueHolder(TfAnyUniquePtr::New(val), typeid(T));
    }

    // Return the runtime type of the held object.
    std::type_info const &GetType() const {
        return *_type;
    }

    // Return a pointer to the held object.  This may be safely cast to the
    // static type corresponding to the type_info returned by GetType.
    void const *GetPointer() const {
        return _ptr.Get();
    }

private:
    Vt_DefaultValueHolder(TfAnyUniquePtr &&ptr, std::type_info const &type)
        : _ptr(std::move(ptr)), _type(&type) {}

    TfAnyUniquePtr _ptr;
    std::type_info const *_type;
};

void const *
Vt_FindOrCreateDefaultValue(std::type_info const &type,
                            Vt_DefaultValueHolder (*factory)());

#define VT_VALUE_SET_STORED_TYPE(SRC, DST)                              \
    template <> struct Vt_ValueStoredType<SRC> { typedef DST Type; }

template <class T> struct Vt_ValueStoredType { typedef T Type; };
VT_VALUE_SET_STORED_TYPE(char const *, std::string);
VT_VALUE_SET_STORED_TYPE(char *, std::string);

template <size_t N>
struct Vt_ValueStoredType<char [N]> {
    using Type = std::string;
};

#ifdef PXR_PYTHON_SUPPORT_ENABLED
VT_VALUE_SET_STORED_TYPE(pxr_boost::python::object, TfPyObjWrapper);
#endif // PXR_PYTHON_SUPPORT_ENABLED

#undef VT_VALUE_SET_STORED_TYPE

// A metafunction that gives the type VtValue should store for a given type T.
template <class T>
using Vt_ValueGetStored = Vt_ValueStoredType<std::decay_t<T>>;

#ifndef doxygen

/// Make a default value.  VtValue uses this to create values to be returned
/// from failed calls to \a Get.  Clients may specialize this for their own
/// types.
template <class T>
struct Vt_DefaultValueFactory {
    static Vt_DefaultValueHolder Invoke();
};

template <class T>
inline Vt_DefaultValueHolder
Vt_DefaultValueFactory<T>::Invoke() {
    return Vt_DefaultValueHolder::Create<T>();
}

// For performance reasons, the default constructors for vectors,
// matrices, and quaternions do *not* initialize the data of the
// object.  This greatly improves the performance of creating large
// arrays of objects.  However, for consistency and to avoid
// errors complaining about uninitialized values, we use VtZero
// to construct zeroed out vectors, matrices, and quaternions by
// explicitly instantiating the factory for these types. 
//
#define _VT_DECLARE_ZERO_VALUE_FACTORY(unused, elem)                    \
template <>                                                             \
VT_API Vt_DefaultValueHolder Vt_DefaultValueFactory<VT_TYPE(elem)>::Invoke();

TF_PP_SEQ_FOR_EACH(_VT_DECLARE_ZERO_VALUE_FACTORY, ~,
                   VT_VEC_VALUE_TYPES
                   VT_MATRIX_VALUE_TYPES
                   VT_QUATERNION_VALUE_TYPES
                   VT_DUALQUATERNION_VALUE_TYPES)

#undef _VT_DECLARE_ZERO_VALUE_FACTORY

#endif // !doxygen

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_VALUE_COMMON_H
