//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_HASH_H
#define PXR_BASE_TF_HASH_H

/// \file tf/hash.h
/// \ingroup group_tf_String

#include "pxr/pxr.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/api.h"

#include <cstring>
#include <string>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Support integers.
template <class HashState, class T>
std::enable_if_t<std::is_integral<T>::value>
TfHashAppend(HashState &h, T integral)
{
    h.Append(integral);
}

// Simple metafunction that returns an unsigned integral type given a size in
// bytes.
template <size_t Size> struct Tf_SizedUnsignedInt;
template <> struct Tf_SizedUnsignedInt<1> { using type = uint8_t; };
template <> struct Tf_SizedUnsignedInt<2> { using type = uint16_t; };
template <> struct Tf_SizedUnsignedInt<4> { using type = uint32_t; };
template <> struct Tf_SizedUnsignedInt<8> { using type = uint64_t; };

// Support enums.
template <class HashState, class Enum>
std::enable_if_t<std::is_enum<Enum>::value>
TfHashAppend(HashState &h, Enum e)
{
    h.Append(static_cast<std::underlying_type_t<Enum>>(e));
}

// Support floating point.
template <class HashState, class T>
std::enable_if_t<std::is_floating_point<T>::value>
TfHashAppend(HashState &h, T fp)
{
    // We want both positive and negative zero to hash the same, so we have to
    // check against zero here.
    typename Tf_SizedUnsignedInt<sizeof(T)>::type intbuf = 0;
    if (fp != static_cast<T>(0)) {
        memcpy(&intbuf, &fp, sizeof(T));
    }
    h.Append(intbuf);
}

// Support std::pair.
template <class HashState, class T, class U>
inline void
TfHashAppend(HashState &h, std::pair<T, U> const &p)
{
    h.Append(p.first);
    h.Append(p.second);
}

// Support std::tuple.
template <class HashState, class... T>
inline void
TfHashAppend(HashState &h, std::tuple<T...> const &t)
{
    // XXX: 
    // This gives the same hash for a std::pair<T, U> and std::tuple<T, U>
    // which may not be ideal in some cases. See USD-10578.
    std::apply([&h](auto const&... v) { h.Append(v...); }, t);
}

// Support std::vector. std::vector<bool> specialized below.
template <class HashState, class T>
inline void
TfHashAppend(HashState &h, std::vector<T> const &vec)
{
    static_assert(!std::is_same_v<std::remove_cv_t<T>, bool>,
                  "Unexpected usage of vector of 'bool'."
                  "Expected explicit overload.");
    h.AppendContiguous(vec.data(), vec.size());
}

// Support std::vector<bool>.
template <class HashState>
inline void
TfHashAppend(HashState &h, std::vector<bool> const &vec)
{
    h.Append(std::hash<std::vector<bool>>{}(vec));
}

// Support std::set.
// NOTE: Supporting std::unordered_set is complicated because the traversal
// order is not guaranteed
template <class HashState, class T, class Compare>
inline void
TfHashAppend(HashState& h, std::set<T, Compare> const &elements)
{
    h.AppendRange(std::begin(elements), std::end(elements));
}

// Support std::map.
// NOTE: Supporting std::unordered_map is complicated because the traversal
// order is not guaranteed
template <class HashState, class Key, class Value, class Compare>
inline void
TfHashAppend(HashState& h, std::map<Key, Value, Compare> const &elements)
{
    h.AppendRange(std::begin(elements), std::end(elements));
}

// Support for hashing std::string.
template <class HashState>
inline void
TfHashAppend(HashState &h, const std::string& s)
{
    return h.AppendContiguous(s.c_str(), s.length());
}

// Support for hashing pointers, but we explicitly delete the version for
// [const] char pointers.  See more below.
template <class HashState, class T>
inline void
TfHashAppend(HashState &h, const T* ptr) {
    return h.Append(reinterpret_cast<uintptr_t>(ptr));
}

// We refuse to hash [const] char *.  You're almost certainly trying to hash the
// pointed-to string and this will not do that (it will hash the pointer
// itself).  To hash a c-style null terminated string, you can use
// TfHashAsCStr() to indicate the intent, or use TfHashCString.  If you
// really want to hash the pointer then use static_cast<const void*>(ptr) or
// TfHashCharPtr.
template <class HashState>
inline void TfHashAppend(HashState &h, char const *ptr) = delete;
template <class HashState>
inline void TfHashAppend(HashState &h, char *ptr) = delete;

/// A structure that wraps a char pointer, indicating intent that it should be
/// hashed as a c-style null terminated string.  See TfhashAsCStr().
struct TfCStrHashWrapper
{
    explicit TfCStrHashWrapper(char const *cstr) : cstr(cstr) {}
    char const *cstr;
};

/// Indicate that a char pointer is intended to be hashed as a C-style null
/// terminated string.  Use this to wrap a char pointer in a HashState::Append()
/// call when implementing a TfHashAppend overload.
///
/// This structure provides a lightweight view on the char pointer passed to its
/// constructor.  It does not copy the data or participate in its lifetime.  The
/// passed char pointer must remain valid as long as this struct is used.
inline TfCStrHashWrapper
TfHashAsCStr(char const *cstr)
{
    return TfCStrHashWrapper(cstr);
}

template <class HashState>
inline void TfHashAppend(HashState &h, TfCStrHashWrapper hcstr)
{
    return h.AppendContiguous(hcstr.cstr, std::strlen(hcstr.cstr));
}

// Implementation detail: dispatch based on hash capability: Try TfHashAppend
// first, otherwise try std::hash, followed by hash_value. We rely on a
// combination of expression SFINAE and establishing preferred order by passing
// a 0 constant and having the overloads take int (highest priority), long
// (next priority) and '...' (lowest priority).

// std::hash version, attempted second.
template <class HashState, class T>
inline auto Tf_HashImpl(HashState &h, T &&obj, long)
    -> decltype(std::hash<typename std::decay<T>::type>()(
                    std::forward<T>(obj)), void())
{
    TfHashAppend(
        h, std::hash<typename std::decay<T>::type>()(std::forward<T>(obj)));
}

// hash_value, attempted last.
template <class HashState, class T>
inline auto Tf_HashImpl(HashState &h, T &&obj, ...)
    -> decltype(hash_value(std::forward<T>(obj)), void())
{
    TfHashAppend(h, hash_value(std::forward<T>(obj)));
}

// TfHashAppend, attempted first.
template <class HashState, class T>
inline auto Tf_HashImpl(HashState &h, T &&obj, int)
    -> decltype(TfHashAppend(h, std::forward<T>(obj)), void())
{
    TfHashAppend(h, std::forward<T>(obj));
}

// Implementation detail, CRTP base that provides the public interface for hash
// state implementations.
template <class Derived>
class Tf_HashStateAPI
{
public:
    // Append several objects to the hash state.
    template <class... Args>
    void Append(Args &&... args) {
        _AppendImpl(std::forward<Args>(args)...);
    }

    // Append contiguous objects to the hash state.
    template <class T>
    void AppendContiguous(T const *elems, size_t numElems) {
        this->_AsDerived()._AppendContiguous(elems, numElems);
    }

    // Append a range of objects to the hash state.
    template <class Iter>
    void AppendRange(Iter f, Iter l) {
        this->_AsDerived()._AppendRange(f, l);
    }

    // Return the hash code for the current state.
    size_t GetCode() const {
        return this->_AsDerived()._GetCode();
    }

private:
    template <class T, class... Args>
    void _AppendImpl(T &&obj, Args &&... rest) {
        this->_AsDerived()._Append(std::forward<T>(obj));
        _AppendImpl(std::forward<Args>(rest)...);
    }
    void _AppendImpl() const {
        // base case intentionally empty.
    }

    Derived &_AsDerived() {
        return *static_cast<Derived *>(this);
    }

    Derived const &_AsDerived() const {
        return *static_cast<Derived const *>(this);
    }
};

// Implementation detail, accumulates hashes.
class Tf_HashState : public Tf_HashStateAPI<Tf_HashState>
{
private:
    friend class Tf_HashStateAPI<Tf_HashState>;

    // Go thru Tf_HashImpl for non-integers.
    template <class T>
    std::enable_if_t<!std::is_integral<std::decay_t<T>>::value>
    _Append(T &&obj) {
        Tf_HashImpl(*this, std::forward<T>(obj), 0);
    }

    // Integers bottom out here.
    template <class T>
    std::enable_if_t<std::is_integral<T>::value>
    _Append(T i) {
        if (!_didOne) {
            _state = i;
            _didOne = true;
        }
        else {
            _state = _Combine(_state, i);
        }
    }

    // Append contiguous objects.
    template <class T>
    std::enable_if_t<std::is_integral<T>::value>
    _AppendContiguous(T const *elems, size_t numElems) {
        _AppendBytes(reinterpret_cast<char const *>(elems),
                     numElems * sizeof(T));
    }

    // Append contiguous objects.
    template <class T>
    std::enable_if_t<!std::is_integral<T>::value>
    _AppendContiguous(T const *elems, size_t numElems) {
        while (numElems--) {
            Append(*elems++);
        }
    }

    // Append a range.
    template <class Iter>
    void _AppendRange(Iter f, Iter l) {
        while (f != l) {
            _Append(*f++);
        }
    }

    /// Append a number of bytes to the hash state.
    TF_API void _AppendBytes(char const *bytes, size_t numBytes);

    // Return the hash code for the accumulated hash state.
    size_t _GetCode() const {
        // This is based on Knuth's multiplicative hash for integers.  The
        // constant is the closest prime to the binary expansion of the inverse
        // golden ratio.  The best way to produce a hash table bucket index from
        // the result is to shift the result right, since the higher order bits
        // have the most entropy.  But since we can't know the number of buckets
        // in a table that's using this, we just reverse the byte order instead,
        // to get the highest entropy bits into the low-order bytes.
        return _SwapByteOrder(_state * 11400714819323198549ULL);
    }

    // This turns into a single bswap/pshufb type instruction on most compilers.
    inline uint64_t
    _SwapByteOrder(uint64_t val) const {
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

    size_t _Combine(size_t x, size_t y) const {
        // This is our hash combiner.  The task is, given two hash codes x and
        // y, compute a single hash code.  One way to do this is to exclusive-or
        // the two codes together, but this can produce bad results if they
        // differ by some fixed amount, For example if the input codes are
        // multiples of 32, and the two codes we're given are N and N + 32 (this
        // happens commonly when the hashed values are memory addresses) then
        // the resulting hash codes for successive pairs of these produces many
        // repeating values (32, 96, 32, XXX, 32, 96, 32, YYY...).  That's a lot
        // of collisions.
        //
        // Instead we combine hash values by assigning numbers to the lattice
        // points in the plane, and then treating the inputs x and y as
        // coordinates identifying a lattice point.  Then the combined hash
        // value is just the number assigned to the lattice point.  This way
        // each unique input pair (x, y) gets a unique output hash code.
        //
        // We number lattice points by triangular numbers like this:
        //
        //  X  0  1  2  3  4  5
        // Y
        // 0   0  2  5  9 14 20
        // 1   1  4  8 13 19 26
        // 2   3  7 12 18 25 33
        // 3   6 11 17 24 32 41
        // 4  10 16 23 31 40 50
        // 5  15 22 30 39 49 60
        //
        // This takes a couple of additions and a multiplication, which is a bit
        // more expensive than something like an exclusive or, but the quality
        // improvement outweighs the added expense.
        x += y;
        return y + x * (x + 1) / 2;
    }

    size_t _state = 0;
    bool _didOne = false;
};

/// \class TfHash
/// \ingroup group_tf_String
///
/// A user-extensible hashing mechanism for use with runtime hash tables.
///
/// The hash functions here are appropriate for storing objects in runtime hash
/// tables.  They are not appropriate for document signatures / fingerprinting
/// or for storage and offline use.  No specific guarantee is made about hash
/// function quality, and the resulting hash codes are only 64-bits wide.
/// Callers must assume that collisions will occur and be prepared to deal with
/// them.
///
/// Additionally, no guarantee is made about repeatability from run-to-run.
/// That is, within a process lifetime an object's hash code will not change
/// (provided its internal state does not change).  But an object with
/// equivalent state in a future run of the same process may hash differently.
///
/// At the time of this writing we observe good performance combined with the
/// "avalanche" quality (~50% output bit flip probability for a single input bit
/// flip) in the low-order 40 output bits.  Higher order bits do not achieve
/// avalanche, and the highest order 8 bits are particularly poor.  But for our
/// purposes we deem this performance/quality tradeoff acceptable.
///
/// This mechanism has builtin support for integral and floating point types,
/// some STL types and types in Tf.  TfHash uses three methods to attempt to
/// hash a passed object.  First, TfHash tries to call TfHashAppend() on its
/// argument.  This is the primary customization point for TfHash.  If that is
/// not viable, TfHash tries to call std::hash<T>{}(). Lastly, TfHash makes an
/// unqualified call to hash_value.
///
/// The best way to add TfHash support for user-defined types is to provide a
/// function overload like the following.
///
/// \code
/// template <class HashState>
/// void TfHashAppend(HashState &h, MyType const &myObj)
///     h.Append(myObject._member1,
///              myObject._member2,
///              myObject._member3);
///     h.AppendContiguous(myObject._memberArray, myObject._numArrayElems);
/// }
/// \endcode
///
/// The HashState object is left deliberately unspecified, so that different
/// hash state objects may be used in different circumstances without modifying
/// this support code, and without excess abstraction penalty.  The methods
/// available for use in TfHashAppend overloads are:
///
/// \code
/// // Append several objects to the hash state.  This invokes the TfHash
/// // mechanism so it works for any types supported by TfHash.
/// template <class... Args>
/// void HashState::Append(Args &&... args);
///
/// // Append contiguous objects to the hash state.  Note that this is
/// // explicitly *not* guaranteed to produce the same result as calling
/// // Append() with each object in order.
/// template <class T>
/// void HashState::AppendContiguous(T const *objects, size_t numObjects);
///
/// // Append a general range of objects to the hash state.  Note that this is
/// // explicitly *not* guaranteed to produce the same result as calling
/// // Append() with each object in order.
/// template <class Iter>
/// void AppendRange(Iter f, Iter l);
/// \endcode
///
/// The \c TfHash class function object supports:
///   \li integral types (including bool)
///   \li floating point types
///   \li std::string
///   \li TfRefPtr
///   \li TfWeakPtr
///   \li TfEnum
///   \li const void*
///   \li types that provide overloads for TfHashAppend
///   \li types that provide overloads for std::hash
///   \li types that provide overloads for hash_value
///
/// The \c TfHash class can be used to instantiate a \c TfHashMap with \c string
/// keys as follows:
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
class TfHash {
public:
    /// Produce a hash code for \p obj.  See the class documentation for
    /// details.
    template <class T>
    auto operator()(T &&obj) const ->
        decltype(Tf_HashImpl(std::declval<Tf_HashState &>(),
                             std::forward<T>(obj), 0), size_t()) {
        Tf_HashState h;
        Tf_HashImpl(h, std::forward<T>(obj), 0);
        return h.GetCode();
    }

    /// Produce a hash code by combining the hash codes of several objects.
    template <class... Args>
    static size_t Combine(Args &&... args) {
        Tf_HashState h;
        _CombineImpl(h, std::forward<Args>(args)...);
        return h.GetCode();
    }

private:
    template <class HashState, class T, class... Args>
    static void _CombineImpl(HashState &h, T &&obj, Args &&... rest) {
        Tf_HashImpl(h, std::forward<T>(obj), 0);
        _CombineImpl(h, std::forward<Args>(rest)...);
    }
    
    template <class HashState>
    static void _CombineImpl(HashState &h) {
        // base case does nothing
    }        
};

/// A hash function object that hashes the address of a char pointer.
struct TfHashCharPtr {
    size_t operator()(const char* ptr) const;
};

/// A hash function object that hashes null-terminated c-string content.
struct TfHashCString {
    size_t operator()(const char* ptr) const;
};

/// A function object that compares two c-strings for equality.
struct TfEqualCString {
    bool operator()(const char* lhs, const char* rhs) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
