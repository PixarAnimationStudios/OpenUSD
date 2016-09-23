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
#ifndef TF_TOKEN_H
#define TF_TOKEN_H

/// \file tf/token.h
///
/// \c TfToken class for efficient string referencing and hashing, plus
/// conversions to and from stl string containers.

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pointerAndBits.h"
#include "pxr/base/tf/traits.h"

#include "pxr/base/tf/hashset.h"
#include <atomic>
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <set>

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
    TfToken() {}

    /// Copy constructor.
    TfToken(TfToken const& rhs) : _rep(rhs._rep) { _AddRef(); }
    
    /// Assignment.
    TfToken& operator= (TfToken const& rhs) {
        if (&rhs != this) {
        rhs._AddRef();
        _RemoveRef();
        _rep = rhs._rep;
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
    explicit TfToken(std::string const& s);
    /// \overload
    // Create a token for \p s, and make it immortal.  If \p s exists in the
    // token table already and is not immortal, make it immortal.  Immortal
    // tokens are faster to copy than mortal tokens, but they will never expire
    // and release their memory.
    TfToken(std::string const& s, _ImmortalTag);

    /// Acquire a token for the given string.
    //
    // This constructor involves a string hash and a lookup in the global
    // table, and so should not be done more often than necessary.  When
    // possible, create a token once and reuse it many times.
    explicit TfToken(char const* s);
    /// \overload
    // Create a token for \p s, and make it immortal.  If \p s exists in the
    // token table already and is not immortal, make it immortal.  Immortal
    // tokens are faster to copy than mortal tokens, but they will never expire
    // and release their memory.
    TfToken(char const* s, _ImmortalTag);

    /// Find the token for the given string, if one exists.
    //
    // If a token has previous been created for the given string, this
    // will return it.  Otherwise, the empty token will be returned.
    static TfToken Find(std::string const& s);

    /// Return a size_t hash for this token.
    //
    // The hash is based on the token's storage identity; this is immutable
    // as long as the token is in use anywhere in the process.
    //
    size_t Hash() const { return TfHash()(_rep.Get()); }

    /// Functor to use for hash maps from tokens to other things.
    struct HashFunctor {
        size_t operator()(TfToken const& token) const { return token.Hash(); }
    };

    /// Functor to be used with std::set when lexicographical ordering
    // isn't crucial and you just want uniqueness and fast lookup
    // without the overhead of an TfHashSet.
    struct LTTokenFunctor {
        bool operator()(TfToken const& s1, TfToken const& s2) const {
            return s1._rep.Get() < s2._rep.Get();
        }
    };

    /// Functor for comparing two tokens, for use in TfHashSet.
    struct TokensEqualFunctor {
        bool operator()(TfToken const& t1, TfToken const& t2) const {
            return t1 == t2;
        }
    };

    /// \typedef TfHashSet<TfToken, TfToken::HashFunctor, TfToken::TokensEqualFunctor> HashSet;
    ///
    /// Predefined type for TfHashSet of tokens, since it's so awkward to
    /// manually specify.
    ///
    typedef TfHashSet<TfToken, TfToken::HashFunctor, TfToken::TokensEqualFunctor> HashSet;
    
    /// \typedef std::set<TfToken, TfToken::LTTokenFunctor> Set;
    ///
    /// Predefined type for set of tokens, for when faster lookup is
    /// desired, without paying the memory or initialization cost of a
    /// TfHashSet.
    ///
    typedef std::set<TfToken, TfToken::LTTokenFunctor> Set;
    
    /// Return the text that this token represents.
    ///
    /// \note The returned pointer value is not valid after this TfToken
    /// object has been destroyed.
    ///
    char const* GetText() const {
        return _rep ? _rep->_str.c_str() : _emptyStr;
    }

    /// Return the string that this token represents.
    std::string const& GetString() const {
        return _rep ? _rep->_str : _GetEmptyString();
    }

    /// Swap this token with another.
    inline void Swap(TfToken &other) {
        std::swap(_rep, other._rep);
    }
    
    /// Equality operator
    bool operator==(TfToken const& o) const {
        return _rep.Get() == o._rep.Get();
    }

    /// Equality operator
    bool operator!=(TfToken const& o) const {
        return !(*this == o);
    }

    /// Equality operator for \c char strings.  Not as fast as direct
    /// token to token equality testing
    bool operator==(std::string const& o) const;

    /// Equality operator for \c char strings.  Not as fast as direct
    /// token to token equality testing
    bool operator==(const char *) const;

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
    inline bool operator<(TfToken const& o) const {
        return GetString() < o.GetString();
    }

    /// Greater-than operator that compares tokenized strings lexicographically.
    inline bool operator>(TfToken const& o) const {
        return GetString() > o.GetString();
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
    bool IsEmpty() const { return !_rep; }

    /// Stream insertion.
    friend std::ostream &operator <<(std::ostream &stream, TfToken const&);

private:
    // Add global swap overload.
    friend void swap(TfToken &lhs, TfToken &rhs) {
        lhs.Swap(rhs);
    }

    void _AddRef() const {
        if (_rep.BitsAs<bool>()) {
            // We believe this rep is refCounted.
            if (not _rep->IncrementIfCounted()) {
                // Our belief is wrong, update our cache of countedness.
                _rep.SetBits(false);
            }
        }
    }

    void _RemoveRef() const {
        if (_rep.BitsAs<bool>()) {
            // We believe this rep is refCounted.
            if (_rep->_isCounted) {
                if (_rep->_refCount == 1)
                    _PossiblyDestroyRep();
                else {
                    /*
                     * This is deliberately racy.  It's possible the statement
                     * below drops our count to zero, and we leak the rep
                     * (i.e. we leave it in the table).  That's a low
                     * probability event, in exchange for only grabbing the lock
                     * (in _PossiblyDestroyRep()) when the odds are we really do
                     * need to modify the table.
                     *
                     * Note that even if we leak the rep, if we look it up
                     * again, we'll simply repull it from the table and keep
                     * using it.  So it's not even necessarily a true leak --
                     * it's just a potential leak.
                     */
                    --_rep->_refCount;
                }
            } else {
                // Our belief is wrong, update our cache of countedness.
                _rep.SetBits(false);
            }
        }
    }
    
    void _PossiblyDestroyRep() const;
    
    struct _Rep {
        _Rep() {}
        explicit _Rep(char const *s) : _str(s), _cstr(_str.c_str()) {}
        explicit _Rep(std::string const &s) : _str(s), _cstr(_str.c_str()) {}

        // Make sure we reacquire _cstr from _str on copy and assignment
        // to avoid holding on to a dangling pointer. However, if rhs'
        // _cstr member doesn't come from its _str, just copy it directly
        // over. This is to support lightweight _Rep objects used for 
        // internal lookups.
        _Rep(_Rep const &rhs) : _str(rhs._str), 
                                _cstr(rhs._str.c_str() != rhs._cstr ? 
                                          rhs._cstr : _str.c_str()),
                                _refCount(rhs._refCount.load()),
                                _isCounted(rhs._isCounted), 
                                _setNum(rhs._setNum) {}
        _Rep& operator=(_Rep const &rhs) {
            _str = rhs._str;
            _cstr = (rhs._str.c_str() != rhs._cstr ? rhs._cstr : _str.c_str());
            _refCount = rhs._refCount.load();
            _isCounted = rhs._isCounted;
            _setNum = rhs._setNum;
            return *this;
        }

        bool IncrementIfCounted() const {
            const bool isCounted = _isCounted;
            if (isCounted)
                ++_refCount;
            return isCounted;
        }
        std::string _str;
        char const *_cstr;
        mutable std::atomic_int _refCount;
        mutable bool _isCounted;
        mutable unsigned char _setNum;
    };

    friend struct TfTokenFastArbitraryLessThan;
    friend struct Tf_TokenRegistry;
    
    static std::string const& _GetEmptyString();
    static char const* _emptyStr;

    mutable TfPointerAndBits<const _Rep> _rep;
};

/// Fast but non-lexicographical (in fact, arbitrary) less-than comparison for
/// TfTokens.  Should only be used in performance-critical cases.
struct TfTokenFastArbitraryLessThan {
    inline bool operator()(TfToken const &lhs, TfToken const &rhs) const {
        return lhs._rep.Get() < rhs._rep.Get();
    }
};
            
/// Convert the vector of strings \p sv into a vector of \c TfToken
std::vector<TfToken>
TfToTokenVector(const std::vector<std::string> &sv);

/// Convert the vector of \c TfToken \p tv into a vector of strings
std::vector<std::string>
TfToStringVector(const std::vector<TfToken> &tv);

/// Overload hash_value for TfToken.
inline size_t hash_value(const TfToken& x) { return x.Hash(); }

/// Convenience types.
typedef std::vector<TfToken> TfTokenVector;

#endif
