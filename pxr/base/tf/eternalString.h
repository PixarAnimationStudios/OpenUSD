//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_ETERNAL_STRING_H
#define PXR_BASE_TF_ETERNAL_STRING_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfEternalString
///
/// An immutable, interned string whose character data is guaranteed to remain
/// valid at a fixed address for the remainder of the process.
///
/// The point of this type is not to be a better string; it is to make a promise
/// to the functions you pass it to, in a form the compiler can check.  A callee
/// that receives a \c TfEternalString may:
///
///   \li retain \c c_str() / \c data() indefinitely without copying, and
///   \li use that address as a durable content identity.
///
/// The characters live in the \c TfToken table, which deduplicates by content,
/// so the implication runs both ways and holds for the life of the process:
///
///   \li same address  <==>  same content
///
/// That is, two equal addresses denote the same characters, and two differing
/// addresses denote different characters.  Equality and hashing are therefore
/// defined on the address, and are constant time regardless of length.
///
/// Interning also means there is only ever one copy of a given string.  Two
/// call sites that produce the same content share it, rather than each holding
/// their own copy.  \c TF_FUNC_NAME(), which returns a \c TfEternalString, does
/// this whenever \c ArchGetPrettierFunctionName() truncates two names to the
/// same text:
///
/// \code
///   Outer::Member<int, float>()   -->  "Outer::Member"
///   Outer::Member<char, bool>()   -->  "Outer::Member"
/// \endcode
///
/// Two call sites, one string, one address.  Content that an ordinary \c TfToken
/// elsewhere in the process already names is shared as well.
///
/// \section TfEternalString_cost Cost
///
/// Creating one permanently retains an entry in the \c TfToken table.  Nothing
/// reclaims it, deliberately, to satisfy the immortality guarantee.  Creating
/// one also costs a hash, a \c strcmp, and a lock on one of the token table's
/// sets, which is paid even when the content is already interned and no
/// allocation results.
///
/// This makes the type suitable for a *bounded* set of strings fixed by the
/// program source code -- one per call site, as \c TF_FUNC_NAME() does -- and
/// actively not suitable for anything dynamic or derived from data.  Never call
/// \c Immortalize() in a loop, per prim, per frame, or on user input.  Repeating
/// the same content costs no additional space, since it dedups; the hazard is
/// unbounded *distinct* content.  If you cannot name a static bound on how many
/// distinct strings your code creates, this is the wrong type.
///
/// \section TfEternalString_vs_token Relationship to TfToken
///
/// This type is implemented on the \c TfToken table: \c Immortalize() creates
/// an immortal token and keeps the address of its characters.  That is where
/// the content identity and the sharing described above come from.
///
/// Given that, the reason a separate type exists is *not* that an immortal \c
/// TfToken is less durable.  It is the same storage and exactly as durable.
/// The reasons are these:
///
///   \li The promise would live in a runtime property rather than in the type.
///       The durability is real and documented (see \c TfToken's immortal
///       constructors) but it is a property of how a particular token was made,
///       not of the type.  A callee handed a \c TfToken must consult \c
///       IsImmortal() and handle the case when it is not.  Handed a \c
///       TfEternalString, it knows statically that retaining the pointer is
///       safe.
///   \li \c TfToken is not a drop-in for a string-like result.  It has no
///       \c c_str(), \c length(), or \c empty(), and no \c operator+ at all --
///       and its implicit \c std::string const & conversion does not rescue
///       concatenation, because \c std::operator+ is a template and deduction
///       does not consider user-defined conversions.  Every concatenating or
///       \c c_str() call site would have to change.
///   \li \c tf/diagnostic.h, which defines \c TF_FUNC_NAME(), is deliberately
///       light and does not include \c tf/token.h.  Making it do so would pull
///       the token table's dependencies into nearly every translation unit.
///       Only \c tf/eternalString.cpp includes \c tf/token.h, so depending on
///       the token table for storage costs this header nothing.
///
class TfEternalString
{
public:
    /// Construct the empty eternal string.
    TF_API TfEternalString();

    /// Return the \c TfEternalString naming \p content, interning its
    /// characters if they are not interned already.
    ///
    /// This is spelled as a named factory, not a converting constructor, so
    /// that callers make an explicit acknowledgement of intent at the call
    /// site.  There is deliberately no implicit conversion from \c char const *
    /// or \c std::string for the same reason.  See \ref TfEternalString_cost.
    ///
    /// Embedded NULs are not supported.  \p content is trimmed at the first
    /// NUL, so \c Immortalize("a\\0b") and \c Immortalize("a") name the same
    /// string.  This matches \c TfToken, which derives string identity from a
    /// NUL-terminated c-string and therefore cannot tell such a name from its
    /// prefix either.  Consequently \c size() is always \c strlen(c_str()), and
    /// this type is for names, not for arbitrary binary content.
    TF_API static TfEternalString Immortalize(std::string_view content);

    /// \name std::string-compatible accessors
    /// @{
    char const *c_str()  const { return _str->c_str(); }
    char const *data()   const { return _str->data();  }
    size_t      size()   const { return _str->size();  }
    size_t      length() const { return _str->size();  }
    bool        empty()  const { return _str->empty(); }
    /// @}

    /// Return the underlying string.
    std::string const &GetString() const { return *_str; }

    /// Return a view of the characters.
    std::string_view GetStringView() const { return *_str; }

    /// \name Implicit conversions
    ///
    /// Both of these hand out a reference or a view of internal state, which
    /// for most types would be a dangling hazard.  Here they are safe even when
    /// the \c TfEternalString itself is a temporary, because the referent is
    /// immortal:
    /// \code
    ///     std::string const &name = TF_FUNC_NAME();   // safe
    ///     std::string_view   view = TF_FUNC_NAME();   // safe
    /// \endcode
    ///
    /// Note the \c std::string conversion yields a reference, not a value.
    /// That is what makes the durable-address promise survive being passed
    /// through an ordinary `std::string const &` parameter: no temporary is
    /// materialized, so a callee that retains \c c_str() is safe.
    ///
    /// Providing both is a deliberate trade with one known cost.  A callee
    /// overloaded on both `std::string const &` and `std::string_view` will
    /// fail to compile if passed a \c TfEternalString argument: either implicit
    /// conversion would be viable, so neither candidate is better and the call
    /// is ambiguous.
    ///
    /// If you hit that, the local fix is to pass \c GetString() or \c
    /// GetStringView() to choose one explicitly.  Overloading on both
    /// `std::string const &` and `std::string_view` is rare in practice, so
    /// providing both implicit conversions is judged worth the risk.
    /// @{
    operator std::string const &() const { return *_str; }
    operator std::string_view() const { return *_str; }
    /// @}

    /// \name Operators
    ///
    /// These are hidden friends: findable only by argument-dependent lookup on
    /// \c TfEternalString, so they are not candidates for overload resolution
    /// in unrelated expressions.  They must be written out because the
    /// corresponding \c std:: operators are templates, and template argument
    /// deduction never considers user-defined conversions -- so neither
    /// conversion above can rescue `s + " x"` or `os << s`.
    /// @{

    friend std::string operator+(TfEternalString a, std::string_view b) {
        return _Cat(a.data(), a.size(), b.data(), b.size());
    }
    friend std::string operator+(std::string_view a, TfEternalString b) {
        return _Cat(a.data(), a.size(), b.data(), b.size());
    }
    friend std::string operator+(TfEternalString a, TfEternalString b) {
        return _Cat(a.data(), a.size(), b.data(), b.size());
    }

    /// Compare by address, which interning makes a content identity in both
    /// directions -- see the class documentation.  Constant time regardless of
    /// length.
    friend bool operator==(TfEternalString a, TfEternalString b) {
        return a._str == b._str;
    }
    friend bool operator==(TfEternalString a, std::string_view b) {
        return a.GetStringView() == b;
    }
    friend bool operator==(std::string_view a, TfEternalString b) {
        return a == b.GetStringView();
    }
    friend bool operator!=(TfEternalString a, TfEternalString b) {
        return !(a == b);
    }
    friend bool operator!=(TfEternalString a, std::string_view b) {
        return !(a == b);
    }
    friend bool operator!=(std::string_view a, TfEternalString b) {
        return !(a == b);
    }

    /// Hash by address, which interning makes a content identity -- see the
    /// class documentation.  Constant time regardless of length.
    ///
    /// As with \c TfToken, which hashes its rep pointer for the same reason,
    /// this means hash values vary between runs and do not agree with the hash
    /// of an equal \c std::string.  Do not persist them.
    template <class HashState>
    friend void TfHashAppend(HashState &h, TfEternalString s) {
        h.Append(s._str);
    }

    TF_API friend std::ostream &operator<<(std::ostream &o, TfEternalString s);

    /// @}

private:
    explicit TfEternalString(std::string const *str) : _str(str) {}

    TF_API static std::string
    _Cat(char const *a, size_t aLen, char const *b, size_t bLen);

    std::string const *_str;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_ETERNAL_STRING_H
