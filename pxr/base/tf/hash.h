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
#ifndef PXR_BASE_TF_HASH_H
#define PXR_BASE_TF_HASH_H

/// \file tf/hash.h
/// \ingroup group_tf_String

#include "pxr/pxr.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/arch/hash.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

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
///   \li const void*
///   \li size_t
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
///     TfHashMap<const void*, T, TfHash> m5;
///     TfHashMap<size_t, T, TfHash> m6;
/// \endcode
///
class TfHash {
private:
    // This turns into a single bswap/pshufb type instruction on most compilers.
    inline uint64_t _SwapByteOrder(uint64_t val) const {
        val =
            ((val & 0xFF00000000000000u) >> 56u) |
            ((val & 0x00FF000000000000u) >> 40u) |
            ((val & 0x0000FF0000000000u) >> 24u) |
            ((val & 0x000000FF00000000u) >>  8u) |
            ((val & 0x00000000FF000000u) <<  8u) |
            ((val & 0x0000000000FF0000u) << 24u) |
            ((val & 0x000000000000FF00u) << 40u) |
            ((val & 0x00000000000000FFu) << 56u);
        return val;
    }

    inline size_t _Mix(size_t val) const {
        // This is based on Knuth's multiplicative hash for integers.  The
        // constant is the closest prime to the binary expansion of the golden
        // ratio - 1.  The best way to produce a hash table bucket index from
        // the result is to shift the result right, since the higher order bits
        // have the most entropy.  But since we can't know the number of buckets
        // in a table that's using this, we just reverse the byte order instead.
        uint64_t h = static_cast<uint64_t>(val) * 11400714819323198549ULL;
        return static_cast<size_t>(_SwapByteOrder(h));
    }

public:
    size_t operator()(const std::string& s) const {
        return ArchHash(s.c_str(), s.length());
    }

    template <class T>
    size_t operator()(const TfRefPtr<T>& ptr) const {
        return (*this)(ptr._refBase);
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

    TF_API size_t operator()(const TfEnum& e) const;

    TF_API size_t operator()(const TfType& t) const;

    // We refuse to hash const char*.  You're almost certainly trying to
    // hash the pointed-to string and this will not do that (it will hash
    // the pointer itself).  If you really want to hash the pointer then
    // use static_cast<const void*>(ptr) or TfHashCharPtr and use
    // TfHashCString if you want to hash the string.
    template <class T>
    size_t operator()(const T* ptr) const {
        static_assert(!std::is_same<T, char>::value,
                      "Can not hash const char*.");
        return _Mix((size_t) ptr);
    }

    size_t operator()(size_t i) const {
        return _Mix(i);
    }

    // Provide an overload for TfToken to prevent hashing via TfToken's implicit
    // conversion to std::string.
    TF_API size_t operator()(const TfToken& t) const;
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif
