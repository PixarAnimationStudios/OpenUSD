//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/eternalString.h"

#include "pxr/base/tf/token.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

// Return the canonical, immortal address of the characters naming \p content.
//
// Storage comes from the TfToken table, which deduplicates by content, so this
// address is a content identity in both directions: equal content always yields
// the same address, and distinct content never does.  Equal content shares a
// single copy of the characters, whether it came from another TfEternalString
// or from an ordinary TfToken.
//
static std::string const *
_Intern(std::string_view content)
{
    // Taking the address of a temporary's referent is safe here precisely
    // because the characters are immortal: whether they belong to an immortal
    // rep or to the canonical empty string, they remain valid at a fixed
    // address for the remainder of the process, outliving every TfToken that
    // refers to them.  See TfToken's immortal constructors and GetString().
    return &TfToken(std::string(content), TfToken::Immortal).GetString();
}

TfEternalString::TfEternalString()
    : _str(&TfToken().GetString())
{
}

TfEternalString
TfEternalString::Immortalize(std::string_view content)
{
    return TfEternalString(_Intern(content));
}

std::string
TfEternalString::_Cat(char const *a, size_t aLen, char const *b, size_t bLen)
{
    std::string result;
    result.reserve(aLen + bLen);
    result.append(a, aLen);
    result.append(b, bLen);
    return result;
}

std::ostream &
operator<<(std::ostream &o, TfEternalString s)
{
    return o << s.GetString();
}

PXR_NAMESPACE_CLOSE_SCOPE
