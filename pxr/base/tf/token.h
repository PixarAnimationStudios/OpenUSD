//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_TOKEN_H
#define PXR_BASE_TF_TOKEN_H

/// \file tf/token.h
///
/// \c TfToken class for efficient string referencing and hashing, plus
/// conversions to and from stl string containers.

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/pointerAndBits.h"

#include <atomic>
#include <iosfwd>
#include <string>
#include <vector>
#include <set>

PXR_NAMESPACE_OPEN_SCOPE

struct TfTokenFastArbitraryLessThan;

/// \class TfToken
/// \ingroup group_tf_String
///
/// Token for efficient comparison, assignment, and hashing of known strings.
///
/// A TfToken is a handle for a registered string, and can be compared,
/// assigned, and hashed in constant time.  It is useful when a bounded number
/// of strings are used as fixed symbols (but never modified).
///
/// For example, the set of avar names in a shot is large but bounded, and
/// once an avar name is discovered, it is never manipulated.  If these names
/// were passed around as strings, every comparison and hash would be linear
/// in the number of characters.  (String assignment itself is sometimes a
/// constant time operation, but it is sometimes linear in the length of the
/// string as well as requiring a memory allocation.)
///
/// To use TfToken, simply create an instance from a string or const char*.
/// If the string hasn't been seen before, a copy of it is added to a global
/// table.  The resulting TfToken is simply a wrapper around an string*,
/// pointing that canonical copy of the string.  Thus, operations on the token
/// are very fast.  (The string's hash is simply the address of the canonical
/// copy, so hashing the string is constant time.)
///
/// The free functions \c TfToTokenVector() and \c TfToStringVector() provide
/// conversions to and from vectors of \c string.
/// 
/// Note: Access to the global table is protected by a mutex.  This is a good
/// idea as long as clients do not construct tokens from strings too
/// frequently.  Construct tokens only as often as you must (for example, as
/// you read data files), and <i>never</i> in inner loops.  Of course, once
/// you have a token, feel free to compare, assign, and hash it as often as
/// you like.  (That's what it's for.)  In order to help prevent tokens from
/// being re-created over and over, auto type conversion from \c string and \c
/// char* to \c TfToken is disabled (you must use the explicit \c TfToken
/// constructors).  However, auto conversion from \c TfToken to \c string and
/// \c char* is provided.
///
class TfToken
{
public:
    enum _ImmortalTag { Immortal };

    /// Create the empty token, containing the empty string.
    constexpr TfToken() noexcept = default;

    /// Copy constructor.
    TfToken(TfToken const& rhs) noexcept : _rep(rhs._rep) { _AddRef(); }

    /// Move constructor.
    TfToken(TfToken && rhs) noexcept : _rep(rhs._rep) {
        rhs._rep = TfPointerAndBits<const _Rep>();
    }
    
    /// Copy assignment.
    TfToken& operator= (TfToken const& rhs) noexcept {
        if (&rhs != this) {
            rhs._AddRef();
            _RemoveRef();
            _rep = rhs._rep;
        }
        return *this;
    }

    /// Move assignment.
    TfToken& operator= (TfToken && rhs) noexcept {
        if (&rhs != this) {
            _RemoveRef();
            _rep = rhs._rep;
            rhs._rep = TfPointerAndBits<const _Rep>();
        }
        return *this;
    }

    /// Destructor.
    ~TfToken() { _RemoveRef(); }
    
    /// Acquire a token for the given string.
    //
    // This constructor involves a string hash and a lookup in the global
    // table, and so should not be done more often than necessary.  When
    // possible, create a token once and reuse it many times.
    TF_API explicit TfToken(std::string const& s);
    /// \overload
    // Create a token for \p s, and make it immortal.  If \p s exists in the
    // token table already and is not immortal, make it immortal.  Immortal
    // tokens are faster to copy than mortal tokens, but they will never expire
    // and release their memory.
    TF_API TfToken(std::string const& s, _ImmortalTag);

    /// Acquire a token for the given string.
    //
    // This constructor involves a string hash and a lookup in the global
    // table, and so should not be done more often than necessary.  When
    // possible, create a token once and reuse it many times.
    TF_API explicit TfToken(char const* s);
    /// \overload
    // Create a token for \p s, and make it immortal.  If \p s exists in the
    // token table already and is not immortal, make it immortal.  Immortal
    // tokens are faster to copy than mortal tokens, but they will never expire
    // and release their memory.
    TF_API TfToken(char const* s, _ImmortalTag);

    /// Find the token for the given string, if one exists.
    //
    // If a token has previous been created for the given string, this
    // will return it.  Otherwise, the empty token will be returned.
    TF_API static TfToken Find(std::string const& s);

    /// Return a size_t hash for this token.
    //
    // The hash is based on the token's storage identity; this is immutable
    // as long as the token is in use anywhere in the process.
    //
    inline size_t Hash() const;

    /// Functor to use for hash maps from tokens to other things.
    struct HashFunctor {
        size_t operator()(TfToken const& token) const { return token.Hash(); }
    };

    /// \typedef TfHashSet<TfToken, TfToken::HashFunctor> HashSet;
    ///
    /// Predefined type for TfHashSet of tokens, since it's so awkward to
    /// manually specify.
    ///
    typedef TfHashSet<TfToken, TfToken::HashFunctor> HashSet;
    
    /// \typedef std::set<TfToken, TfTokenFastArbitraryLessThan> Set;
    ///
    /// Predefined type for set of tokens, for when faster lookup is
    /// desired, without paying the memory or initialization cost of a
    /// TfHashSet.
    ///
    typedef std::set<TfToken, TfTokenFastArbitraryLessThan> Set;
    
    /// Return the size of the string that this token represents.
    size_t size() const {
        _Rep const *rep = _rep.Get();
        return rep ? rep->_str.size() : 0;
    }

    /// Return the text that this token represents.
    ///
    /// \note The returned pointer value is not valid after this TfToken
    /// object has been destroyed.
    ///
    char const* GetText() const {
        _Rep const *rep = _rep.Get();
        return rep ? rep->_str.c_str() : "";
    }

    /// Synonym for GetText().
    char const *data() const {
        return GetText();
    }

    /// Return the string that this token represents.
    std::string const& GetString() const {
        _Rep const *rep = _rep.Get();
        return rep ? rep->_str : _GetEmptyString();
    }

    /// Swap this token with another.
    inline void Swap(TfToken &other) {
        std::swap(_rep, other._rep);
    }
    
    /// Equality operator
    bool operator==(TfToken const& o) const {
        // Equal if pointers & bits are equal, or if just pointers are.
        return _rep.Get() == o._rep.Get();
    }

    /// Equality operator
    bool operator!=(TfToken const& o) const {
        return !(*this == o);
    }

    /// Equality operator for \c char strings.  Not as fast as direct
    /// token to token equality testing
    TF_API bool operator==(std::string const& o) const;

    /// Equality operator for \c char strings.  Not as fast as direct
    /// token to token equality testing
    TF_API bool operator==(const char *) const;

    /// \overload
    friend bool operator==(std::string const& o, TfToken const& t) {
        return t == o;
    }

    /// \overload
    friend bool operator==(const char *o, TfToken const& t) {
        return t == o;
    }

    /// Inequality operator for \c string's.  Not as fast as direct
    /// token to token equality testing
    bool operator!=(std::string const& o) const {
        return !(*this == o);
    }

    /// \overload
    friend bool operator!=(std::string const& o, TfToken const& t)  {
        return !(t == o);
    }

    /// Inequality operator for \c char strings.  Not as fast as direct
    /// token to token equality testing
    bool operator!=(char const* o) const {
        return !(*this == o);
    }

    /// \overload
    friend bool operator!=(char const* o, TfToken const& t) {
        return !(t == o);
    }

    /// Less-than operator that compares tokenized strings lexicographically.
    /// Allows \c TfToken to be used in \c std::set
    inline bool operator<(TfToken const& r) const {
        auto ll = _rep.GetLiteral(), rl = r._rep.GetLiteral();
        if (!ll || !rl) {
            // One or both are zero -- return true if ll is zero and rl is not.
            return !ll && rl;
        }
        if (ll == rl) {
            return false;
        }
        auto lrep = _rep.Get(), rrep = r._rep.Get();
        uint64_t lcc = lrep->_compareCode, rcc = rrep->_compareCode;
        return lcc < rcc || (lcc == rcc && lrep->_str < rrep->_str);
    }

    /// Greater-than operator that compares tokenized strings lexicographically.
    inline bool operator>(TfToken const& o) const {
        return o < *this;
    }

    /// Greater-than-or-equal operator that compares tokenized strings
    /// lexicographically.
    inline bool operator>=(TfToken const& o) const {
        return !(*this < o);
    }

    /// Less-than-or-equal operator that compares tokenized strings
    /// lexicographically.
    inline bool operator<=(TfToken const& o) const {
        return !(*this > o);
    }

    /// Allow \c TfToken to be auto-converted to \c string
    operator std::string const& () const { return GetString(); }
    
    /// Returns \c true iff this token contains the empty string \c ""
    bool IsEmpty() const { return _rep.GetLiteral() == 0; }

    /// Returns \c true iff this is an immortal token.  Note that a return of \c
    /// false could be instantly stale if another thread races to immortalize
    /// this token.  A return of \c true is always valid since tokens cannot
    /// lose immortality.
    bool IsImmortal() const {
        if (!_rep.BitsAs<bool>()) {
            return true;
        }
        // There is no synchronization or ordering constraints between this read
        // and other reads/writes, so relaxed memory order suffices.
        bool immortal = !(_rep->_refCount.load(std::memory_order_relaxed) & 1);
        if (immortal) {
            // Our belief is wrong, update our cache of countedness.
            _rep.SetBits(false);
        }
        return immortal;
    }

    /// Stream insertion.
    friend TF_API std::ostream &operator <<(std::ostream &stream, TfToken const&);

    /// TfHash support.
    template <class HashState>
    friend void
    TfHashAppend(HashState &h, TfToken const &token) {
        h.Append(token._rep.Get());
    }

private:
    // Add global swap overload.
    friend void swap(TfToken &lhs, TfToken &rhs) {
        lhs.Swap(rhs);
    }

    void _AddRef() const {
        if (!_rep.BitsAs<bool>()) {
            // Not counted, do nothing.
            return;
        }
        // We believe this rep is refCounted.
        if (!_rep->IncrementAndCheckCounted()) {
            // Our belief is wrong, update our cache of countedness.
            _rep.SetBits(false);
        }
    }

    void _RemoveRef() const {
        if (!_rep.BitsAs<bool>()) {
            // Not counted, do nothing.
            return;
        }
        // Decrement the refcount.
        _rep->Decrement();
    }

    struct _Rep {
        _Rep() = default;

        explicit _Rep(std::string &&str,
                      unsigned setNum,
                      uint64_t compareCode)
            : _setNum(setNum)
            , _compareCode(compareCode)
            , _str(std::move(str))
            , _cstr(_str.c_str()) {}

        explicit _Rep(std::string const &str,
                      unsigned setNum,
                      uint64_t compareCode)
            : _Rep(std::string(str), setNum, compareCode) {}

        explicit _Rep(char const *str,
                      unsigned setNum,
                      uint64_t compareCode)
            : _Rep(std::string(str), setNum, compareCode) {}

        // Make sure we reacquire _cstr from _str on copy and assignment
        // to avoid holding on to a dangling pointer. However, if rhs'
        // _cstr member doesn't come from its _str, just copy it directly
        // over. This is to support lightweight _Rep objects used for 
        // internal lookups.
        _Rep(_Rep const &rhs)
            : _refCount(rhs._refCount.load(std::memory_order_relaxed))
            , _setNum(rhs._setNum)
            , _compareCode(rhs._compareCode)
            , _str(rhs._str)
            , _cstr(rhs._str.c_str() == rhs._cstr ? _str.c_str() : rhs._cstr) {}

        _Rep &operator=(_Rep const &rhs) {
            _refCount = rhs._refCount.load(std::memory_order_relaxed);
            _setNum = rhs._setNum;
            _compareCode = rhs._compareCode;
            _str = rhs._str;
            _cstr = (rhs._str.c_str() == rhs._cstr ? _str.c_str() : rhs._cstr);
            return *this;
        }

        inline bool IncrementAndCheckCounted() const {
            // Refcounts are manipulated by add/sub 2, since the lowest-order
            // bit indicates whether or not the rep is counted.
            return _refCount.fetch_add(2, std::memory_order_relaxed) & 1;
        }

        inline void Decrement() const {
            // Refcounts are manipulated by add/sub 2, since the lowest-order
            // bit indicates whether or not the rep is counted.
            _refCount.fetch_sub(2, std::memory_order_release);
        }

        mutable std::atomic_uint _refCount;
        unsigned _setNum;
        uint64_t _compareCode;
        std::string _str;
        char const *_cstr;
    };

    friend struct TfTokenFastArbitraryLessThan;
    friend struct Tf_TokenRegistry;
    
    TF_API static std::string const& _GetEmptyString();

    mutable TfPointerAndBits<const _Rep> _rep;
};

inline size_t
TfToken::Hash() const
{
    return TfHash()(*this);
}

/// Fast but non-lexicographical (in fact, arbitrary) less-than comparison for
/// TfTokens.  Should only be used in performance-critical cases.
struct TfTokenFastArbitraryLessThan {
    inline bool operator()(TfToken const &lhs, TfToken const &rhs) const {
        return lhs._rep.Get() < rhs._rep.Get();
    }
};
            
/// Convert the vector of strings \p sv into a vector of \c TfToken
TF_API std::vector<TfToken>
TfToTokenVector(const std::vector<std::string> &sv);

/// Convert the vector of \c TfToken \p tv into a vector of strings
TF_API std::vector<std::string>
TfToStringVector(const std::vector<TfToken> &tv);

/// Overload hash_value for TfToken.
inline size_t hash_value(const TfToken& x) { return x.Hash(); }

/// Convenience types.
typedef std::vector<TfToken> TfTokenVector;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_TOKEN_H
