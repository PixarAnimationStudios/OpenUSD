//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/eternalString.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/token.h"

#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

// Helpers for the interop checks below.
static std::string
_TakeStringRef(std::string const &s)
{
    return s;
}

// A callee taking only std::string_view, which is the shape that motivated the
// implicit view conversion (e.g. pxr/exec/exec/requestImpl.cpp's debug-message
// helpers).  It returns the view's data pointer so callers can check that the
// characters were not copied on the way in.
static char const *
_TakeStringView(std::string_view s)
{
    return s.data();
}

// Created during this translation unit's dynamic initialization.  Creation now
// goes through the TfToken table, so it requires TfSingleton, malloc tagging and
// the TfType registry to be usable at that point; before the token table backed
// this type, creation needed only the allocator.
static TfEternalString const _gDuringStaticInit =
    TfEternalString::Immortalize("createdDuringStaticInit");

// A local stand-in for TF_FUNC_NAME(), so that this test exercises
// TfEternalString's own guarantees without depending on tf/diagnostic.h's macro
// or on how ArchGetPrettierFunctionName() formats a name.  It keeps the two
// properties that matter here: the string is computed once per call site and
// cached in an immortal static, and it describes the enclosing function.
//
// Note that __func__ is passed as a lambda *parameter*.  Expanding it inside
// the lambda body would name the lambda's own operator() instead; the naming
// checks below are what catch that mistake.
#define TEST_FUNC_NAME()                                                      \
    ([](char const *f) -> TfEternalString {                                   \
        static TfEternalString const n =                                      \
            TfEternalString::Immortalize(std::string("fn:") + f);             \
        return n;                                                             \
    }(__func__))

// The name TEST_FUNC_NAME() must produce in the function that invokes it.
// Evaluated outside any lambda, so it is the enclosing function's name.  This
// is compared for exact equality rather than matching a substring, so the test
// does not depend on what __func__ actually spells.
#define TEST_FUNC_NAME_EXPECTED() (std::string("fn:") + __func__)

// Returns the address of the character data cached at a single call site.  Two
// calls must return the same address.
static char const *
_OneCallSite()
{
    return TEST_FUNC_NAME().data();
}

// Two distinct call sites inside one function.  Each caches independently, but
// both describe the same function, so interning gives them one shared address.
static void
_TwoCallSitesOneFunction(TfEternalString *first, TfEternalString *second)
{
    *first = TEST_FUNC_NAME();
    *second = TEST_FUNC_NAME();
    TF_AXIOM(*first == TEST_FUNC_NAME_EXPECTED());
    TF_AXIOM(*second == TEST_FUNC_NAME_EXPECTED());
}

// A call site inside a template.  Each instantiation gets its own closure type
// and so its own cached handle, but the content is the same, so the address is
// shared.
template <class T>
static TfEternalString
_TemplateCallSite()
{
    TfEternalString result = TEST_FUNC_NAME();
    TF_AXIOM(result == TEST_FUNC_NAME_EXPECTED());
    return result;
}

static bool
Test_TfEternalString()
{
    // Construction and basic accessors.
    {
        TfEternalString s = TfEternalString::Immortalize("Foo::Bar");
        TF_AXIOM(s.size() == 8);
        TF_AXIOM(s.length() == 8);
        TF_AXIOM(!s.empty());
        TF_AXIOM(s.GetString() == "Foo::Bar");
        TF_AXIOM(s.GetStringView() == "Foo::Bar");
        TF_AXIOM(std::strcmp(s.c_str(), "Foo::Bar") == 0);
        // c_str() must be NUL terminated; data() and c_str() agree.
        TF_AXIOM(s.c_str()[s.size()] == '\0');
        TF_AXIOM(s.data() == s.c_str());
    }

    // The empty cases.  Empty content has no rep in the token table, so it is
    // TfToken's canonical empty string that supplies the one address for it.
    {
        TfEternalString dflt;
        TF_AXIOM(dflt.empty());
        TF_AXIOM(dflt.size() == 0);
        TF_AXIOM(dflt.c_str() != nullptr);
        TF_AXIOM(dflt.c_str()[0] == '\0');
        TF_AXIOM(dflt == "");
        TF_AXIOM(dflt == std::string());

        TfEternalString empty = TfEternalString::Immortalize("");
        TF_AXIOM(empty.empty());
        TF_AXIOM(empty == dflt);
        TF_AXIOM(empty.data() == dflt.data());

        // Shared with TfToken, like every other value of this type.  Note this
        // must go through GetString(): for the empty token GetText() returns a
        // string literal instead, at a different address.
        TF_AXIOM(dflt.data() == TfToken().GetString().data());
        TF_AXIOM(dflt.data() == TfToken("").GetString().data());
    }

    // Immortalize copies, so the source need not outlive the result.  This is
    // the core immortality claim: the data survives its origin.
    {
        char const *survivor = nullptr;
        {
            std::string temporary = "transient content";
            survivor = TfEternalString::Immortalize(temporary).c_str();
        }
        TF_AXIOM(std::strcmp(survivor, "transient content") == 0);
    }

    // Embedded NULs are not supported: content is trimmed at the first NUL, so
    // that this type agrees with TfToken about what a given argument names.
    {
        TfEternalString s =
            TfEternalString::Immortalize(std::string_view("a\0b", 3));
        TF_AXIOM(s.size() == 1);
        TF_AXIOM(s == "a");
        TF_AXIOM(s == TfEternalString::Immortalize("a"));
        // Naming the same string means sharing the same characters.
        TF_AXIOM(s.data() == TfEternalString::Immortalize("a").data());

        // Content differing only past a NUL names the same string.
        TF_AXIOM(s == TfEternalString::Immortalize(
                     std::string_view("a\0zzz", 5)));

        // Trimming also resolves what would otherwise be an odd case: content
        // that is nothing but a NUL is the empty string, not a size-1 string
        // that prints as empty.
        TfEternalString justNul =
            TfEternalString::Immortalize(std::string_view("\0", 1));
        TF_AXIOM(justNul.empty());
        TF_AXIOM(justNul == TfEternalString());
        TF_AXIOM(justNul.data() == TfEternalString().data());

        // Trimming makes size() and strlen(c_str()) agree unconditionally.
        TF_AXIOM(s.size() == std::strlen(s.c_str()));
        TF_AXIOM(justNul.size() == std::strlen(justNul.c_str()));
    }

    // Copying shares the address; the type is a small, immutable handle.
    {
        TF_AXIOM(sizeof(TfEternalString) == sizeof(char const *));

        TfEternalString a = TfEternalString::Immortalize("shared");
        TfEternalString b = a;
        TfEternalString c;
        c = a;
        TF_AXIOM(b.data() == a.data());
        TF_AXIOM(c.data() == a.data());
        TF_AXIOM(a == b && b == c);
    }

    // The contract: because the characters are interned, the address is a
    // content identity in both directions.  Equality and hashing are defined on
    // it.
    {
        TfEternalString x = TfEternalString::Immortalize("Outer::Method");
        TfEternalString y = TfEternalString::Immortalize("Outer::Method");

        // Equal content, so one interned copy at one address...
        TF_AXIOM(x.data() == y.data());
        // ...which compares and hashes equal.
        TF_AXIOM(x == y);
        TF_AXIOM(!(x != y));
        TF_AXIOM(TfHash()(x) == TfHash()(y));

        // Distinct content compares unequal, and never shares an address.
        TfEternalString z = TfEternalString::Immortalize("Outer::Other");
        TF_AXIOM(x != z);
        TF_AXIOM(!(x == z));
        TF_AXIOM(x.data() != z.data());
    }

    // The characters are the token table's, so equal content shared with an
    // ordinary TfToken is stored once.  This must hold in either interning
    // order.
    {
        // Token first.  It is mortal; Immortalize() immortalizes the same rep
        // rather than creating a second one.
        TfToken tok("sharedWithToken.A");
        TfEternalString s = TfEternalString::Immortalize("sharedWithToken.A");
        TF_AXIOM(s.data() == tok.GetText());
        TF_AXIOM(tok.IsImmortal());

        // Eternal string first; the token finds the existing immortal rep.
        TfEternalString t = TfEternalString::Immortalize("sharedWithToken.B");
        TfToken tok2("sharedWithToken.B");
        TF_AXIOM(t.data() == tok2.GetText());
    }

    // A token's identity is its NUL-terminated c-string, and TfToken stores it
    // trimmed to match (which TfToken's own test covers).  What matters here is
    // that this insulates TfEternalString: an unrelated party interning a name
    // from a std::string carrying bytes past a NUL cannot change what the prefix
    // names, or make size() disagree with strlen(c_str()).  Both interning
    // orders must agree; before TfToken was fixed this depended on which of the
    // two came first.
    {
        // Token first, from a std::string carrying bytes past the NUL.
        TfToken tok(std::string("nulTrim.A\0junk", 14));
        TF_AXIOM(tok.size() == 9);
        TF_AXIOM(tok.GetString() == "nulTrim.A");
        TF_AXIOM(tok == "nulTrim.A");
        TF_AXIOM(tok.size() == std::strlen(tok.GetText()));

        TfEternalString s = TfEternalString::Immortalize("nulTrim.A");
        TF_AXIOM(s.size() == 9);
        TF_AXIOM(s == "nulTrim.A");
        TF_AXIOM(s.size() == std::strlen(s.c_str()));
        TF_AXIOM(s.data() == tok.GetText());
    }
    {
        // Eternal string first; the token must land on the same rep.
        TfEternalString s = TfEternalString::Immortalize("nulTrim.B");
        TfToken tok(std::string("nulTrim.B\0junk", 14));
        TF_AXIOM(tok.size() == 9);
        TF_AXIOM(tok.GetString() == "nulTrim.B");
        TF_AXIOM(s.data() == tok.GetText());
        TF_AXIOM(s.size() == 9);
    }

    // Comparison against std::string and char const *, both orders.  These stay
    // content-based; only TfEternalString-to-TfEternalString compares addresses.
    {
        TfEternalString s = TfEternalString::Immortalize("cmp");
        std::string const same = "cmp";
        std::string const diff = "nope";

        TF_AXIOM(s == same);
        TF_AXIOM(same == s);
        TF_AXIOM(s != diff);
        TF_AXIOM(diff != s);
        TF_AXIOM(s == "cmp");
        TF_AXIOM("cmp" == s);
        TF_AXIOM(s != "nope");
        TF_AXIOM("nope" != s);
        TF_AXIOM(s == std::string_view("cmp"));
        TF_AXIOM(std::string_view("cmp") == s);
    }

    // Concatenation, in both directions and chained.  These exist because the
    // std:: operators are templates and will not see the conversion to
    // std::string const &.
    {
        TfEternalString s = TfEternalString::Immortalize("Foo::Bar");

        TF_AXIOM(s + " suffix" == "Foo::Bar suffix");
        TF_AXIOM("prefix " + s == "prefix Foo::Bar");
        TF_AXIOM(s + std::string(" str") == "Foo::Bar str");
        TF_AXIOM(std::string("str ") + s == "str Foo::Bar");
        TF_AXIOM(s + s == "Foo::BarFoo::Bar");
        TF_AXIOM(s + " x " + s == "Foo::Bar x Foo::Bar");

        // The result is an ordinary std::string.
        std::string built = s + " built";
        built += "!";
        TF_AXIOM(built == "Foo::Bar built!");

        // Concatenation is length-based rather than c_str()-based, which is
        // still observable: a std::string_view operand may itself contain a
        // NUL, and those bytes must survive even though Immortalize() trims.
        TF_AXIOM((s + std::string_view("\0z", 2)).size() == 10);
    }

    // Stream insertion.
    {
        TfEternalString s = TfEternalString::Immortalize("streamed");
        std::ostringstream os;
        os << s << "!";
        TF_AXIOM(os.str() == "streamed!");
    }

    // Interop with std::string via the implicit conversion.
    {
        TfEternalString s = TfEternalString::Immortalize("interop");

        // Binds to a std::string const & parameter, and to a by-value one.
        TF_AXIOM(_TakeStringRef(s) == "interop");

        // Copy-initializes a std::string.
        std::string copy = s;
        TF_AXIOM(copy == "interop");

        // Usable as a std::string-keyed container key.
        std::map<std::string, int> m;
        m[s] = 7;
        TF_AXIOM(m.find(s) != m.end());
        TF_AXIOM(m.find(s)->second == 7);

        // Storable in a vector of string.
        std::vector<std::string> v;
        v.push_back(s);
        TF_AXIOM(v[0] == s);
    }

    // Interop with std::string_view via the implicit conversion.
    {
        TfEternalString s = TfEternalString::Immortalize("viewed");

        // Binds to a std::string_view parameter without copying: the callee
        // sees this object's own durable characters.
        TF_AXIOM(_TakeStringView(s) == s.data());

        // Copy-initializes a std::string_view, which also does not copy.
        std::string_view view = s;
        TF_AXIOM(view == "viewed");
        TF_AXIOM(view.data() == s.data());
        TF_AXIOM(view.size() == s.size());

        // Adding the view conversion must not make the comparison operators
        // ambiguous: the hidden friends are exact matches and so still win over
        // std::string_view's own comparisons.
        std::string_view other = "viewed";
        TF_AXIOM(s == other);
        TF_AXIOM(other == s);
        TF_AXIOM(!(s != other));

        // Nor may it disturb resolution for std::string, which reaches the
        // string_view-like member templates by exact deduction.
        std::string str;
        str = s;
        str.append(s);
        TF_AXIOM(str == "viewedviewed");
    }

    // Both conversions yield a reference or view of immortal storage, so
    // anything obtained from them stays valid after the TfEternalString is
    // gone.  This is what lets a callee retain c_str() or a view safely.
    {
        char const *retained = nullptr;
        std::string const *referenced = nullptr;
        std::string_view survivingView;
        {
            TfEternalString scoped = TfEternalString::Immortalize("retained");
            std::string const &ref = scoped;
            retained = ref.c_str();
            referenced = &ref;
            survivingView = scoped;
        }
        TF_AXIOM(std::strcmp(retained, "retained") == 0);
        TF_AXIOM(*referenced == "retained");
        TF_AXIOM(survivingView == "retained");
        TF_AXIOM(survivingView.data() == retained);
    }

    // Caching an immortal string in a per-call-site static, which is how
    // TF_FUNC_NAME() is expected to use this type.
    {
        // Same call site, called twice: one string, one address.
        char const *first = _OneCallSite();
        char const *second = _OneCallSite();
        TF_AXIOM(first == second);
        TF_AXIOM(std::strlen(first) > 0);
    }

    {
        // Two sites in one function: equal content, and now shared storage.
        // Note the per-call-site static caching is no longer observable from
        // outside -- interning gives one address either way -- so this checks
        // the sharing, not the caching.
        TfEternalString a, b;
        _TwoCallSitesOneFunction(&a, &b);
        TF_AXIOM(a == b);
        TF_AXIOM(a.data() == b.data());
    }

    {
        // Distinct instantiations of this template have the same __func__, so
        // they name the same string and share one address.
        TfEternalString i1 = _TemplateCallSite<int>();
        TfEternalString i2 = _TemplateCallSite<int>();
        TfEternalString c1 = _TemplateCallSite<char>();
        TF_AXIOM(i1.data() == i2.data());
        TF_AXIOM(i1 == c1);
        TF_AXIOM(i1.data() == c1.data());
    }

    // Creation during dynamic initialization works, and yields the same address
    // as an equal string interned later.
    {
        TF_AXIOM(_gDuringStaticInit == "createdDuringStaticInit");
        TF_AXIOM(_gDuringStaticInit.size() == 23);
        TF_AXIOM(_gDuringStaticInit.data() ==
                 TfEternalString::Immortalize("createdDuringStaticInit").data());
    }

    return true;
}

TF_ADD_REGTEST(TfEternalString);
