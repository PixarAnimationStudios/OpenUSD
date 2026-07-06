//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdf/globPattern.h"

#include <cstdio>
#include <cstring>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

static int numTests = 0;
static int numFailed = 0;

static void
_Test(const char *pattern, const char *str, bool expected,
      const char *file, int line)
{
    ++numTests;
    auto pat = Sdf_GlobPattern::Compile(pattern, strlen(pattern));
    if (!pat) {
        if (expected) {
            printf("FAIL [%s:%d] pattern '%s' failed to compile\n",
                   file, line, pattern);
            ++numFailed;
        }
        return;
    }
    bool got = pat.Match(str, strlen(str));
    if (got != expected) {
        printf("FAIL [%s:%d] Match('%s', '%s') = %s, expected %s\n",
               file, line, pattern, str,
               got ? "true" : "false",
               expected ? "true" : "false");
        ++numFailed;
    }
}

#define TEST(pat, str, expected) _Test(pat, str, expected, __FILE__, __LINE__)

int main()
{
    // Trivial
    TEST("*", "", true);
    TEST("*", "anything", true);
    TEST("*", "Foo", true);
    TEST("", "", true);
    TEST("", "x", false);

    // Prefix only (Foo*)
    TEST("Foo*", "Foo", true);
    TEST("Foo*", "FooBar", true);
    TEST("Foo*", "Fo", false);
    TEST("Foo*", "xFoo", false);
    TEST("Foo*", "", false);

    // Suffix only (*Foo)
    TEST("*Foo", "Foo", true);
    TEST("*Foo", "BarFoo", true);
    TEST("*Foo", "FooBar", false);
    TEST("*Foo", "", false);
    TEST("*Foo", "Fo", false);

    // Prefix + suffix (Foo*Bar)
    TEST("Foo*Bar", "FooBar", true);
    TEST("Foo*Bar", "FooXBar", true);
    TEST("Foo*Bar", "FooXYZBar", true);
    TEST("Foo*Bar", "Foo", false);
    TEST("Foo*Bar", "Bar", false);
    TEST("Foo*Bar", "FooXBa", false);

    // Middle segments
    TEST("*Foo*", "Foo", true);
    TEST("*Foo*", "xFooy", true);
    TEST("*Foo*", "xBarx", false);
    TEST("*Foo*Bar*", "FooBar", true);
    TEST("*Foo*Bar*", "xFooyBarz", true);
    TEST("*Foo*Bar*", "xBaryFooz", false);
    TEST("*X*Y*Z*", "aXbYcZd", true);
    TEST("*X*Y*Z*", "aXbZcYd", false);

    // Exact match (no star)
    TEST("Foo", "Foo", true);
    TEST("Foo", "FooBar", false);
    TEST("Foo", "Fo", false);
    TEST("Foo", "", false);
    TEST("abc", "abc", true);
    TEST("abc", "ab", false);
    TEST("abc", "abcd", false);

    // ? wildcard
    TEST("?", "x", true);
    TEST("?", "ab", false);
    TEST("?", "", false);
    TEST("F?o", "Foo", true);
    TEST("F?o", "Fao", true);
    TEST("F?o", "Fo", false);
    TEST("F?o", "Food", false);
    TEST("???", "abc", true);
    TEST("???", "ab", false);
    TEST("???", "abcd", false);
    TEST("?oo*", "Foo", true);
    TEST("?oo*", "FooBar", true);
    TEST("?oo*", "oo", false);
    TEST("*?oo", "Foo", true);
    TEST("*?oo", "xFoo", true);
    TEST("*?oo", "oo", false);

    // Character classes
    TEST("[Ff]oo", "Foo", true);
    TEST("[Ff]oo", "foo", true);
    TEST("[Ff]oo", "goo", false);
    TEST("[Ff]oo*", "FooBar", true);
    TEST("[Ff]oo*", "fooBar", true);
    TEST("[Ff]oo*", "gooBar", false);
    TEST("[a-z]", "a", true);
    TEST("[a-z]", "m", true);
    TEST("[a-z]", "z", true);
    TEST("[a-z]", "A", false);
    TEST("[a-z]", "0", false);
    TEST("[a-zA-Z0-9_]", "x", true);
    TEST("[a-zA-Z0-9_]", "X", true);
    TEST("[a-zA-Z0-9_]", "5", true);
    TEST("[a-zA-Z0-9_]", "_", true);
    TEST("[a-zA-Z0-9_]", " ", false);

    // Negated classes
    TEST("[!a-z]", "A", true);
    TEST("[!a-z]", "0", true);
    TEST("[!a-z]", "a", false);
    TEST("[^A-Z]x", "ax", true);
    TEST("[^A-Z]x", "Ax", false);

    // Class edge cases
    TEST("[]]", "]", true);
    TEST("[]]", "x", false);
    TEST("[-abc]", "-", true);
    TEST("[-abc]", "a", true);
    TEST("[abc-]", "-", true);
    TEST("[abc-]", "c", true);
    TEST("[abc-]", "d", false);

    // Backslash escapes
    TEST("\\*", "*", true);
    TEST("\\*", "x", false);
    TEST("\\?", "?", true);
    TEST("\\?", "x", false);
    TEST("\\[", "[", true);
    TEST("\\[", "x", false);
    TEST("\\\\", "\\", true);
    TEST("Foo\\*Bar", "Foo*Bar", true);
    TEST("Foo\\*Bar", "FooXBar", false);

    // UTF-8
    // 2-byte: U+00E9 (e-acute) = C3 A9
    TEST("?", "\xC3\xA9", true);
    TEST("??", "\xC3\xA9x", true);
    TEST("??", "\xC3\xA9", false);
    // 3-byte: U+2603 (snowman) = E2 98 83
    TEST("?", "\xE2\x98\x83", true);
    TEST("*?", "\xE2\x98\x83", true);
    TEST("?*", "\xE2\x98\x83", true);
    // 4-byte: U+1F600 (grinning face) = F0 9F 98 80
    TEST("?", "\xF0\x9F\x98\x80", true);
    TEST("??", "\xF0\x9F\x98\x80\xF0\x9F\x98\x80", true);

    // UTF-8 character class with range
    // U+00E0 to U+00FF range
    TEST("[\xC3\xA0-\xC3\xBF]", "\xC3\xA9", true);  // e-acute in range
    TEST("[\xC3\xA0-\xC3\xBF]", "a", false);         // ASCII not in range

    // Multiple consecutive stars
    TEST("**", "anything", true);
    TEST("Foo**Bar", "FooBar", true);
    TEST("Foo**Bar", "FooXBar", true);

    // Long literal runs: literal-run elements cap at 251 bytes per op, so
    // patterns longer than that exercise the chunk-splitting path.
    {
        std::string pat250(250, 'a');
        TEST(pat250.c_str(), pat250.c_str(), true);
        std::string pat251(251, 'a');
        TEST(pat251.c_str(), pat251.c_str(), true);
        std::string pat252(252, 'a');
        TEST(pat252.c_str(), pat252.c_str(), true);
        std::string pat500(500, 'a');
        TEST(pat500.c_str(), pat500.c_str(), true);
        // Boundary-spanning suffix.
        std::string longPlusX = std::string(252, 'a') + "X";
        TEST(longPlusX.c_str(), longPlusX.c_str(), true);
        std::string longOnly(252, 'a');
        TEST(longPlusX.c_str(), longOnly.c_str(), false);
        // Mixed long literal with star.
        std::string starThenLong = "*" + std::string(300, 'b');
        TEST(starThenLong.c_str(),
             ("xy" + std::string(300, 'b')).c_str(), true);
        TEST(starThenLong.c_str(),
             ("xy" + std::string(299, 'b')).c_str(), false);
    }

    // Parse errors: invalid patterns never compile, so Match is false.
    TEST("\\", "", false);          // trailing backslash
    TEST("\\", "x", false);
    TEST("[abc", "abc", false);     // unclosed class
    TEST("[", "x", false);
    TEST("a[b", "ab", false);

    // Many ?'s: exercises codePointsBefore book-keeping for middles.
    {
        std::string manyQ(50, '?');
        std::string str50(50, 'x');
        TEST(manyQ.c_str(), str50.c_str(), true);
        std::string str49(49, 'x');
        TEST(manyQ.c_str(), str49.c_str(), false);
        std::string str51(51, 'x');
        TEST(manyQ.c_str(), str51.c_str(), false);

        // Many ?'s before a literal in a middle segment.
        std::string mid = "*" + std::string(20, '?') + "X*";
        std::string ok = "ab" + std::string(20, 'y') + "Xcd";
        TEST(mid.c_str(), ok.c_str(), true);
        // Need at least 20 codepoints before some X for a match.
        TEST(mid.c_str(), "X", false);
        TEST(mid.c_str(), std::string(19, 'y').c_str(), false);
        TEST(mid.c_str(), (std::string(19, 'y') + "X").c_str(), false);
        TEST(mid.c_str(), (std::string(20, 'y') + "X").c_str(), true);
    }

    // Class with many ranges (numRanges is uint8_t).
    {
        std::string cls = "[";
        for (char c = 'a'; c != 'z'; ++c) {
            cls += c; cls += '-'; cls += c;
        }
        cls += "]*";
        TEST(cls.c_str(), "abc", true);
        TEST(cls.c_str(), "Abc", false);
    }

    // Many segments: stress middle-segment loop.
    TEST("*a*b*c*d*e*f*g*", "1a2b3c4d5e6f7g8", true);
    TEST("*a*b*c*d*e*f*g*", "abcdefg", true);
    TEST("*a*b*c*d*e*f*g*", "1a2b3c4d5e6f7", false); // missing g
    TEST("*a*b*c*d*e*f*g*", "abcedfg", false);       // out of order

    // UTF-8 boundary cases.
    // 4-byte codepoint at end of string consumed by ? in trailing position.
    TEST("*?", "a\xF0\x9F\x98\x80", true);
    TEST("?*", "\xF0\x9F\x98\x80", true);
    // Multi-byte literal as prefix / suffix.
    TEST("\xC3\xA9*", "\xC3\xA9 anything", true);
    TEST("\xC3\xA9*", "x\xC3\xA9", false);
    TEST("*\xC3\xA9", "anything \xC3\xA9", true);
    TEST("*\xC3\xA9", "\xC3\xA9x", false);
    // Multi-byte literal in middle.  Use string concat to terminate the hex
    // escape sequence cleanly.
    TEST("*\xE2\x98\x83" "*", "abc\xE2\x98\x83" "def", true); // snowman
    TEST("*\xE2\x98\x83" "*", "abcdef", false);

    // Empty-pattern / empty-string corners.
    TEST("a", "", false);
    TEST("?", "", false);
    TEST("[a]", "", false);
    TEST("*?*", "", false);
    TEST("**", "", true);

    // Star-only patterns.
    TEST("***", "anything", true);
    TEST("****", "", true);

    // Single-literal vs non-literal middle: exercises both the fast-path
    // (literal IS the whole segment) and the general path (literal mixed with
    // ?/class).
    TEST("*Foo*",     "xFooy",     true);
    TEST("*Foo*",     "Foo",       true);
    TEST("*Foo*",     "xx",        false);
    TEST("*?Foo*",    "xFooy",     true);
    TEST("*?Foo*",    "Foo",       false);
    TEST("*[A-Z]oo*", "xMoox",     true);
    TEST("*[A-Z]oo*", "xmoox",     false);
    TEST("*Foo?*",    "xFood",     true);
    TEST("*Foo?*",    "xFoo",      false);

    // Summary
    printf("\n%d tests, %d failed\n", numTests, numFailed);
    return numFailed ? 1 : 0;
}
