//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/diagnostic.h"

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
Test_TfToken()
{
    TfToken empty1, empty2, nonEmpty3("nonEmpty");
    std::string  sEmpty1;
    TF_AXIOM(empty1 == empty2);
    TF_AXIOM(empty1 != nonEmpty3);
    TF_AXIOM(empty1.Hash() == empty2.Hash());
    TF_AXIOM(empty1 == "" && empty2 == "");
    TF_AXIOM(sEmpty1 == empty1 && sEmpty1 == empty2);
    TF_AXIOM(empty1 == sEmpty1 && empty2 == sEmpty1);
    TF_AXIOM(sEmpty1 != nonEmpty3);
    TF_AXIOM(nonEmpty3 != sEmpty1);
    TF_AXIOM("" == empty1);
    TF_AXIOM("" != nonEmpty3);
    TF_AXIOM(empty1.IsEmpty());
    TF_AXIOM( !nonEmpty3.IsEmpty());

    // Test swapping.
    empty1.Swap(nonEmpty3);
    TF_AXIOM(nonEmpty3.IsEmpty());
    TF_AXIOM(!empty1.IsEmpty());
    TF_AXIOM(empty1 == "nonEmpty");
    TF_AXIOM(nonEmpty3 == "");

    std::swap(nonEmpty3, empty1);
    TF_AXIOM(empty1.IsEmpty());
    TF_AXIOM(!nonEmpty3.IsEmpty());
    TF_AXIOM(empty1 == "");
    TF_AXIOM(nonEmpty3 == "nonEmpty");

    // A token's identity is its NUL-terminated c-string, and the string it
    // stores matches: content past an embedded NUL is trimmed rather than
    // retained.  Without that, a token's observable content would depend on
    // whether a std::string or char * happened to intern it, and size() could
    // disagree with strlen(GetText()).
    //
    // The discriminating case is interning the std::string first.
    {
        TfToken fromStr(std::string("trimA\0junk", 10));
        TfToken fromCStr("trimA");

        TF_AXIOM(fromStr == fromCStr);
        TF_AXIOM(fromStr.Hash() == fromCStr.Hash());
        TF_AXIOM(fromStr.GetText() == fromCStr.GetText());

        TF_AXIOM(fromStr.size() == 5);
        TF_AXIOM(fromStr.GetString() == "trimA");
        TF_AXIOM(fromStr == "trimA");
        TF_AXIOM(fromStr == std::string("trimA"));
        TF_AXIOM(fromStr.size() == std::strlen(fromStr.GetText()));

        TF_AXIOM(fromCStr.size() == 5);
        TF_AXIOM(fromCStr.GetString() == "trimA");
        TF_AXIOM(fromCStr == "trimA");
        TF_AXIOM(fromCStr.size() == std::strlen(fromCStr.GetText()));

        // Find() derives identity the same way, so either spelling locates it.
        TF_AXIOM(TfToken::Find("trimA") == fromStr);
        TF_AXIOM(TfToken::Find(std::string("trimA\0junk", 10)) == fromStr);

        // Comparison against a std::string, though, is a plain string compare
        // and is not trimmed, so the string that created the token does not
        // equal it.
        TF_AXIOM(fromStr != std::string("trimA\0junk", 10));
        TF_AXIOM(!(fromStr == std::string("trimA\0junk", 10)));
    }

    // The other order must agree.
    {
        TfToken fromCStr("trimB");
        TfToken fromStr(std::string("trimB\0junk", 10));

        TF_AXIOM(fromCStr == fromStr);
        TF_AXIOM(fromStr.size() == 5);
        TF_AXIOM(fromStr.GetString() == "trimB");
        TF_AXIOM(fromStr == "trimB");
        TF_AXIOM(fromStr.size() == std::strlen(fromStr.GetText()));
    }

    // Emptiness follows the same rule: a std::string whose first character is
    // NUL names the empty token, not a token that merely prints as empty.
    {
        TfToken justNul(std::string("\0", 1));
        TF_AXIOM(justNul.IsEmpty());
        TF_AXIOM(justNul.size() == 0);
        TF_AXIOM(justNul == TfToken());
        TF_AXIOM(justNul == TfToken(""));
        TF_AXIOM(justNul == "");
        TF_AXIOM(justNul == std::string());
        TF_AXIOM(justNul.Hash() == TfToken().Hash());
        TF_AXIOM(justNul.size() == std::strlen(justNul.GetText()));
        TF_AXIOM(TfToken::Find(std::string("\0", 1)).IsEmpty());
    }

    std::string a1("alphabet");
    const char *a2 = "alphabet";

    std::cout << TfToken(a1);

    std::string b1("barnacle");
    const char *b2 = "barnacle";

    std::string c1("cinnamon");
    const char *c2 = "cinnamon";

    TF_AXIOM(TfToken(a1) < TfToken(b1));


    TF_AXIOM(TfToken(a1) == TfToken(a1));
    TF_AXIOM(TfToken(a1) == TfToken(a2));
    TF_AXIOM(TfToken(a1).Hash() == TfToken(a2).Hash());

    TF_AXIOM(TfToken(b1) == TfToken(b1));
    TF_AXIOM(TfToken(b1) == TfToken(b2));
    TF_AXIOM(TfToken(b1).Hash() == TfToken(b2).Hash());

    TF_AXIOM(TfToken(c1) == TfToken(c1));
    TF_AXIOM(TfToken(c1) == TfToken(c2));
    TF_AXIOM(TfToken(c1).Hash() == TfToken(c2).Hash());

    TF_AXIOM(TfToken(a1).Hash() != TfToken(b1).Hash());

    TF_AXIOM(TfToken(a1) != TfToken(b1));
    TF_AXIOM(TfToken(a1) != TfToken(c1));
    TF_AXIOM(TfToken(b1) != TfToken(c1));

    TF_AXIOM(TfToken(b1) > TfToken(a1));

    // Ordering is lexicographic by unsigned byte value, agreeing with strcmp,
    // for every byte value -- not just for ASCII.  The relational operators
    // compare a code packing the first eight characters into a uint64_t and
    // fall back to comparing the strings; both halves must order bytes the same
    // way.
    //
    // The failure this guards against was sign extension while building that
    // code: with a signed char cast straight to uint64_t, "a\xff" produced
    // 0xffff000000000000 rather than 0x61ff000000000000, because the extended
    // high bits landed on top of the slot holding 'a'.  That sorted it after
    // "b" (0x6200000000000000), and the result varied with whether char is
    // signed on the target.
    {
        char const *words[] = {
            "", "a", "b", "ab", "a\x01", "a\xff", "a\xff\xff",
            "\x7f", "\x80", "\xff", "\xff\x01",
            // Longer than the eight characters the code covers, so the string
            // tie-break decides these -- with a high-bit byte in the prefix.
            "hi\xffthere", "hi\xffthere_one", "hi\xffthere_two",
        };
        size_t const n = sizeof(words) / sizeof(words[0]);

        // The specific regression, spelled out.
        TF_AXIOM(TfToken("a\xff") < TfToken("b"));
        TF_AXIOM(!(TfToken("b") < TfToken("a\xff")));

        // Test exhaustively against strcmp, in both argument orders.
        for (size_t i = 0; i != n; ++i) {
            for (size_t j = 0; j != n; ++j) {
                TfToken const ti(words[i]), tj(words[j]);
                int const c = std::strcmp(words[i], words[j]);
                TF_AXIOM((ti <  tj) == (c <  0));
                TF_AXIOM((ti >  tj) == (c >  0));
                TF_AXIOM((ti <= tj) == (c <= 0));
                TF_AXIOM((ti >= tj) == (c >= 0));
                TF_AXIOM((ti == tj) == (c == 0));
            }
        }
    }

    TfToken t1(a1);
    TfToken t2(t1);  // Copy construct
    
    TF_AXIOM(t1 == t2);
    
    t1 = TfToken(b1);

    TF_AXIOM(t1 != t2);

    t2 = TfToken(b2);

    TF_AXIOM(t1 == t2);
    TF_AXIOM(t1 == TfToken("barnacle"));

    std::vector<std::string> strVec;

    strVec.push_back("string1");
    strVec.push_back("string2");
    strVec.push_back("string3");

    std::vector<TfToken> tokVec = TfToTokenVector(strVec);

    TF_AXIOM(TfToken(strVec[0]) == tokVec[0]);
    TF_AXIOM(TfToken(strVec[1]) == tokVec[1]);
    TF_AXIOM(TfToken(strVec[2]) == tokVec[2]);

    std::vector<std::string> strVec2 = TfToStringVector(tokVec);
    TF_AXIOM(TfToken(strVec2[0]) == tokVec[0]);
    TF_AXIOM(TfToken(strVec2[1]) == tokVec[1]);
    TF_AXIOM(TfToken(strVec2[2]) == tokVec[2]);

    return true;
}

TF_ADD_REGTEST(TfToken);
