//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_ACCESSOR_HELPERS_H
#define PXR_USD_SDF_ACCESSOR_HELPERS_H

/// \file sdf/accessorHelpers.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/types.h"

#include <type_traits>

// This file defines macros intended to reduce the amount of boilerplate code
// associated with adding new metadata to SdfSpec subclasses.  There's still a
// lot of files to touch, but these at least reduce the copy/paste/edit load.
//
// Prior to using these macros in the SdfSpec implementation file, define the
// following symbols:
//
// #define SDF_ACCESSOR_CLASS                    SdfSomeSpec
// #define SDF_ACCESSOR_READ_PREDICATE(key_)     _CanRead(key_)
// #define SDF_ACCESSOR_WRITE_PREDICATE(key_)    _CanWrite(key_)
//
// ...where _CanRead and _CanWrite are member functions of the specified class,
// with the signature 'bool _fn_(const TfToken&)'.  If either accessor predicate
// is unnecessary, #define the corresponding symbol to 'SDF_NO_PREDICATE'.
//
// Also, please observe good form and #undef the symbols after instancing the
// accessor macros.

PXR_NAMESPACE_OPEN_SCOPE

// "Helper" macros
#define _GET_KEY_(key_)     key_
#define SDF_NO_PREDICATE    true

#define _GET_WITH_FALLBACK(key_, heldType_)                                    \
    {                                                                          \
        typedef Sdf_AccessorHelpers<SDF_ACCESSOR_CLASS> _Helper;               \
        const VtValue& value = _Helper::GetField(this, key_);                  \
        if (value.IsEmpty() || !value.IsHolding<heldType_>()) {                \
            const SdfSchemaBase& schema = _Helper::GetSchema(this);            \
            return schema.GetFallback(_GET_KEY_(key_)).Get<heldType_>();       \
        }                                                                      \
        else {                                                                 \
            return value.Get<heldType_>();                                     \
        }                                                                      \
    }

// Accessor methods for "simple type" values: Get, Is, Set, Has, Clear
// Usually the client will utilize one of the combination macros (below).

#define SDF_DEFINE_GET(name_, key_, heldType_)                                 \
heldType_                                                                      \
SDF_ACCESSOR_CLASS::Get ## name_() const                                       \
{                                                                              \
    if (SDF_ACCESSOR_READ_PREDICATE(_GET_KEY_(key_))) {                        \
        /* Empty clause needed to prevent compiler complaints */               \
    }                                                                          \
                                                                               \
    _GET_WITH_FALLBACK(key_, heldType_);                                       \
}

#define SDF_DEFINE_IS(name_, key_)                                             \
bool                                                                           \
SDF_ACCESSOR_CLASS::Is ## name_() const                                        \
{                                                                              \
    if (!SDF_ACCESSOR_READ_PREDICATE(_GET_KEY_(key_))) {                       \
        return false;                                                          \
    }                                                                          \
                                                                               \
    _GET_WITH_FALLBACK(key_, bool);                                            \
}

#define SDF_DEFINE_SET(name_, key_, argType_)                                  \
void                                                                           \
SDF_ACCESSOR_CLASS::Set ## name_(argType_ value)                               \
{                                                                              \
    if (SDF_ACCESSOR_WRITE_PREDICATE(_GET_KEY_(key_))) {                       \
        typedef Sdf_AccessorHelpers<SDF_ACCESSOR_CLASS> _Helper;               \
        _Helper::SetField(this, _GET_KEY_(key_), value);                       \
    }                                                                          \
}

#define SDF_DEFINE_HAS(name_, key_)                                            \
bool                                                                           \
SDF_ACCESSOR_CLASS::Has ## name_() const                                       \
{                                                                              \
    typedef Sdf_AccessorHelpers<SDF_ACCESSOR_CLASS> _Helper;                   \
    return SDF_ACCESSOR_READ_PREDICATE(_GET_KEY_(key_)) ?                      \
        _Helper::HasField(this, _GET_KEY_(key_)) : false;                      \
}

#define SDF_DEFINE_CLEAR(name_, key_)                                          \
void                                                                           \
SDF_ACCESSOR_CLASS::Clear ## name_()                                           \
{                                                                              \
    typedef Sdf_AccessorHelpers<SDF_ACCESSOR_CLASS> _Helper;                   \
    if (SDF_ACCESSOR_WRITE_PREDICATE(_GET_KEY_(key_))) {                       \
        _Helper::ClearField(this, _GET_KEY_(key_));                            \
    }                                                                          \
}

// Accessor methods similar to the above, but intended for private use in
// the SdSpec classes.

#define SDF_DEFINE_GET_PRIVATE(name_, key_, heldType_)                         \
heldType_                                                                      \
SDF_ACCESSOR_CLASS::_Get ## name_() const                                      \
{                                                                              \
    if (SDF_ACCESSOR_READ_PREDICATE(_GET_KEY_(key_))) {                        \
        /* Empty clause needed to prevent compiler complaints */               \
    }                                                                          \
                                                                               \
    _GET_WITH_FALLBACK(key_, heldType_);                                       \
}

// Accessor methods for VtDictionary types, utilizing SdDictionaryProxy for the
// 'Get' accessors.  Due to unusual naming in the original SdSpec API, these
// macros accept/require explicit accessor method names.  Dammit.
#define SDF_DEFINE_DICTIONARY_GET(name_, key_)                                 \
SdfDictionaryProxy                                                             \
SDF_ACCESSOR_CLASS::name_() const                                              \
{                                                                              \
    typedef Sdf_AccessorHelpers<SDF_ACCESSOR_CLASS> _Helper;                   \
    return SDF_ACCESSOR_READ_PREDICATE(_GET_KEY_(key_)) ?                      \
        SdfDictionaryProxy(_Helper::GetSpecHandle(this), _GET_KEY_(key_)) :    \
        SdfDictionaryProxy();                                                  \
}

#define SDF_DEFINE_DICTIONARY_SET(name_, key_)                                 \
void                                                                           \
SDF_ACCESSOR_CLASS::name_(                                                     \
    const std::string& name,                                                   \
    const VtValue& value)                                                      \
{                                                                              \
    typedef Sdf_AccessorHelpers<SDF_ACCESSOR_CLASS> _Helper;                   \
    if (SDF_ACCESSOR_WRITE_PREDICATE(_GET_KEY_(key_))) {                       \
        SdfDictionaryProxy proxy(                                              \
            _Helper::GetSpecHandle(this), _GET_KEY_(key_));                    \
        if (value.IsEmpty()) {                                                 \
            proxy.erase(name);                                                 \
        }                                                                      \
        else {                                                                 \
            proxy[name] = value;                                               \
        }                                                                      \
    }                                                                          \
}

// Convenience macros to provide common combinations of value accessors

// Convert non-trivial types like `std::string` to `const std::string&` while
// preserving the type for `int`, `bool`, `char`, etc.
template <typename T>
using Sdf_SetParameter = std::conditional<
    std::is_arithmetic<T>::value, std::add_const_t<T>,
    std::add_lvalue_reference_t<std::add_const_t<T>>>;

#define SDF_DEFINE_TYPED_GET_SET(name_, key_, getType_, setType_)              \
SDF_DEFINE_GET(name_, key_, getType_)                                          \
SDF_DEFINE_SET(name_, key_, setType_)

#define SDF_DEFINE_TYPED_GET_SET_HAS_CLEAR(name_, key_, getType_, setType_)    \
SDF_DEFINE_TYPED_GET_SET(name_, key_, getType_, setType_)                      \
SDF_DEFINE_HAS(name_, key_)                                                    \
SDF_DEFINE_CLEAR(name_, key_)

#define SDF_DEFINE_GET_SET(name_, key_, type_)                                 \
SDF_DEFINE_TYPED_GET_SET(name_, key_, type_,                                   \
                         Sdf_SetParameter<type_>::type)

#define SDF_DEFINE_GET_SET_HAS_CLEAR(name_, key_, type_)                       \
SDF_DEFINE_TYPED_GET_SET_HAS_CLEAR(name_, key_, type_,                         \
                                   Sdf_SetParameter<type_>::type)

#define SDF_DEFINE_IS_SET(name_, key_)                                         \
SDF_DEFINE_IS(name_, key_)                                                     \
SDF_DEFINE_SET(name_, key_, bool)

#define SDF_DEFINE_DICTIONARY_GET_SET(getName_, setName_, key_)                \
SDF_DEFINE_DICTIONARY_GET(getName_, key_)                                      \
SDF_DEFINE_DICTIONARY_SET(setName_, key_)

// Implementation details
// The helper macros above can be used in the implementation of a spec
// class or a spec API class (see declareSpec.h for details). Both cases
// access data in a different way -- spec classes can query their data
// members directly, while spec API classes need to query their associated
// spec. These templates capture those differences.

template <class T,
          bool IsForSpec = std::is_base_of<SdfSpec, T>::value>
struct Sdf_AccessorHelpers;

template <class T>
struct Sdf_AccessorHelpers<T, true>
{
    static const SdfSchemaBase& GetSchema(const T* spec)
    { return spec->GetSchema(); }

    static VtValue GetField(const T* spec, const TfToken& key) 
    { return spec->GetField(key); }

    template <class V> 
    static bool SetField(T* spec, const TfToken& key, const V& value)
    { return spec->SetField(key, value); }

    static bool HasField(const T* spec, const TfToken& key)
    { return spec->HasField(key); }

    static void ClearField(T* spec, const TfToken& key)
    { spec->ClearField(key); }

    static SdfSpecHandle GetSpecHandle(const T* spec)
    { return SdfCreateNonConstHandle(spec); }
};

template <class T>
struct Sdf_AccessorHelpers<T, false>
{
    static const SdfSchemaBase& GetSchema(const T* spec)
    { return spec->_GetSpec().GetSchema(); }

    static VtValue GetField(const T* spec, const TfToken& key) 
    { return spec->_GetSpec().GetField(key); }

    template <class V> 
    static bool SetField(T* spec, const TfToken& key, const V& value)
    { return spec->_GetSpec().SetField(key, value); }

    static bool HasField(const T* spec, const TfToken& key)
    { return spec->_GetSpec().HasField(key); }

    static void ClearField(T* spec, const TfToken& key)
    { spec->_GetSpec().ClearField(key); }

    static SdfSpecHandle GetSpecHandle(const T* spec)
    { return SdfCreateNonConstHandle(&(spec->_GetSpec())); }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // #ifndef PXR_USD_SDF_ACCESSOR_HELPERS_H
