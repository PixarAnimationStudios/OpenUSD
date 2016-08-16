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
#ifndef TF_HASH_H
#define TF_HASH_H

/// \file tf/hash.h
/// \ingroup group_tf_String

#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/timeStamp.h"
#include "pxr/base/arch/hash.h"
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <string>

class TfAnyWeakPtr;
class TfEnum;
class TfToken;
class TfType;

template <class T> class TfWeakPtr;
template <class T> class TfRefPtr;
class TfRefBase;

template <template <class> class X, class T>
class TfWeakPtrFacade;

/// \class TfHash
/// \ingroup group_tf_String
///
/// Provides hash function on STL string types and other types.
///
/// The \c TfHash class is a functor as defined by the STL standard:
/// currently, it is defined for:
///   \li std::string
///   \li TfRefPtr
///   \li TfWeakPtr
///   \li TfEnum
///   \li TfTimeStamp
///   \li const void*
///   \li int
///
/// The \c TfHash class can be used to implement a
/// \c TfHashMap with \c string keys as follows:
/// \code
///     TfHashMap<string, int, TfHash> m;
///     m["abc"] = 1;
/// \endcode
///
/// \c TfHash()(const char*) is disallowed to avoid confusion of whether
/// the pointer or the string is being hashed.  If you want to hash a
/// C-string use \c TfHashCString and if you want to hash a \c char* use
/// \c TfHashCharPtr.
///
/// One can also declare, for any types \c S and  \c T,
/// \code
///     TfHashMap<TfRefPtr<S>, T, TfHash> m1;
///     TfHashMap<TfWeakPtr<S>, T, TfHash> m2;
///     TfHashMap<TfEnum, T, TfHash> m3;
///     TfHashMap<TfTimeStamp, T, TfHash> m4;
///     TfHashMap<const void*, T, TfHash> m5;
///     TfHashMap<int, T, TfHash> m6;
/// \endcode
///
class TfHash {
private:
    inline size_t _Mix(size_t val) const {
        return val + (val >> 3);
    }

public:
    size_t operator()(const std::string& s) const {
        return ArchHash(s.c_str(), s.length());
    }

    template <class T>
    size_t operator()(const TfRefPtr<T>& ptr) const {
        return (*this)(get_pointer(ptr));
    }

    template <template <class> class X, class T>
    size_t operator()(TfWeakPtrFacade<X, T> const &ptr) const {
        return (*this)(ptr.GetUniqueIdentifier());
    }

    // We don't want to choose the TfAnyWeakPtr overload unless the passed
    // argument is exactly TfAnyWeakPtr.  By making this a function template
    // that's only enabled for TfAnyWeakPtr, C++ will not perform implicit
    // conversions (since T is deduced).
    template <class T, class = typename std::enable_if<
                           std::is_same<T, TfAnyWeakPtr>::value>::type>
    size_t operator()(const T& ptr) const {
        return ptr.GetHash();
    }

    size_t operator()(const TfEnum& e) const;

    size_t operator()(const TfType& t) const;

    size_t operator()(TfTimeStamp stamp) const {
        return _Mix(size_t(stamp.Get()));
    }

    // We refuse to hash const char*.  You're almost certainly trying to
    // hash the pointed-to string and this will not do that (it will hash
    // the pointer itself).  If you really want to hash the pointer then
    // use static_cast<const void*>(ptr) or TfHashCharPtr and use
    // TfHashCString if you want to hash the string.
    template <class T>
    size_t operator()(const T* ptr) const {
        BOOST_STATIC_ASSERT((not boost::is_same<T, char>::value));
        return _Mix((size_t) ptr);
    }

    size_t operator()(int i) const {
        return _Mix(i);
    }

    // Provide an overload for TfToken to prevent hashing via TfToken's implicit
    // conversion to std::string.
    size_t operator()(const TfToken& t) const;
};

struct TfHashCharPtr {
    size_t operator()(const char* ptr) const;
};
struct TfHashCString {
    size_t operator()(const char* ptr) const;
};
struct TfEqualCString {
    bool operator()(const char* lhs, const char* rhs) const;
};

#endif
