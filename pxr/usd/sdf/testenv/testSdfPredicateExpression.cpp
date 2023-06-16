//
// Copyright 2023 Pixar
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
#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/usd/sdf/predicateExpression.h"
#include "pxr/usd/sdf/predicateLibrary.h"
#include "pxr/usd/sdf/predicateProgram.h"

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestSimple()
{
    auto predLib = SdfPredicateLibrary<std::string const &>()
        .Define("allVowels", [](std::string const &str) {
            return str.find_first_not_of("aeiouAEIOU") == str.npos;
        })
        .Define("noVowels", [](std::string const &str) {
            return str.find_first_of("aeiouAEIOU") == str.npos;
        })
        .Define("startsWith",
                [](std::string const &str, std::string const &prefix) {
                    return TfStringStartsWith(str, prefix);
                }, {{"prefix"}}
            )
        .Define("endsWith",
                [](std::string const &str, std::string const &suffix) {
                    return TfStringEndsWith(str, suffix);
                }, {{"suffix"}}
            )
        .Define("contains",
                [](std::string const &str, std::string const &subString) {
                    return TfStringContains(str, subString);
                }, {{"subString"}}
            )
        ;

    {
        // Ensure copyability.
        auto predLibCopy = predLib;
    }

    // Link some expressions with predLib to produce programs, and run some
    // simple tests.
    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression("allVowels or startsWith:foo"), predLib);
        
        TF_AXIOM(test("eieio"));
        TF_AXIOM(!test("abcdefg"));
        TF_AXIOM(test("fooabcdefg"));
        TF_AXIOM(test("aieeee"));
        TF_AXIOM(test("oooooo"));
        TF_AXIOM(!test("oops"));
        TF_AXIOM(test("foops"));
    }

    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression("not allVowels"), predLib);
        
        TF_AXIOM(!test("eieio"));
        TF_AXIOM(test("abcdefg"));
        TF_AXIOM(test("fooabcdefg"));
        TF_AXIOM(!test("aieeee"));
        TF_AXIOM(!test("oooooo"));
        TF_AXIOM(test("oops"));
        TF_AXIOM(test("foops"));
    }

    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression("not not allVowels"), predLib);
        
        TF_AXIOM(test("eieio"));
        TF_AXIOM(!test("abcdefg"));
        TF_AXIOM(!test("fooabcdefg"));
        TF_AXIOM(test("aieeee"));
        TF_AXIOM(test("oooooo"));
        TF_AXIOM(!test("oops"));
        TF_AXIOM(!test("foops"));
    }

    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression("not not not allVowels"), predLib);
        
        TF_AXIOM(!test("eieio"));
        TF_AXIOM(test("abcdefg"));
        TF_AXIOM(test("fooabcdefg"));
        TF_AXIOM(!test("aieeee"));
        TF_AXIOM(!test("oooooo"));
        TF_AXIOM(test("oops"));
        TF_AXIOM(test("foops"));
    }
    
    // Link some expressions with predLib to produce programs, and run some
    // simple tests.
    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression("allVowels or startsWith:foo"), predLib);
        
        TF_AXIOM(test("eieio"));
        TF_AXIOM(!test("abcdefg"));
        TF_AXIOM(test("fooabcdefg"));
        TF_AXIOM(test("aieeee"));
        TF_AXIOM(test("oooooo"));
        TF_AXIOM(!test("oops"));
        TF_AXIOM(test("foops"));
    }

    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression(
                "noVowels and startsWith(prefix=\"XXX\")"), predLib);
        
        TF_AXIOM(!test("eieio"));
        TF_AXIOM(!test("abcdefg"));
        TF_AXIOM(test("XXXbcdfg"));
        TF_AXIOM(!test("fooabcdefg"));
        TF_AXIOM(test("XXX fbc dfg"));
        TF_AXIOM(test("XXX hll wld"));
        TF_AXIOM(!test("XXX oooooo"));
    }

    {
        SdfPredicateExpression expr(
            "(allVowels or startsWith:VVV) and "
            "(endsWith:EEE or endsWith:\"END\")");

        auto test = SdfLinkPredicateExpression(expr, predLib);

        TF_AXIOM(test("EEE"));
        TF_AXIOM(test("VVVEEE"));
        TF_AXIOM(test("VVVEND"));
        TF_AXIOM(!test("END"));
        TF_AXIOM(test("VVV!!!EEE"));
        TF_AXIOM(test("VVV!!!END"));
        TF_AXIOM(!test("VVV!!!END "));
        TF_AXIOM(test("VVV abcdefg EEE"));
        TF_AXIOM(!test("abcdefgEEE"));
        TF_AXIOM(!test("VVabcdefgEND"));
    }

    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression("contains:\\n"), predLib);

        TF_AXIOM(test(" new\n line\n text"));
        TF_AXIOM(!test("no newline text"));
        TF_AXIOM(!test("double-escaped \\n aren't newlines"));
    }

    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression("contains(\"\\n\")"), predLib);

        TF_AXIOM(test(" new\n line\n text"));
        TF_AXIOM(!test("no newline text"));
        TF_AXIOM(!test("double-escaped \\n aren't newlines"));
    }

    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression("contains:\\\""), predLib);
        
        TF_AXIOM(test("contains \"quotes\""));
        TF_AXIOM(!test("no ''quotes'' here"));
        TF_AXIOM(test("just one quote:\""));
    }
    
    {
        auto test = SdfLinkPredicateExpression(
            SdfPredicateExpression("contains(\"\\\"\")"), predLib);
        
        TF_AXIOM(test("contains \"quotes\""));
        TF_AXIOM(!test("no ''quotes'' here"));
        TF_AXIOM(test("just one quote:\""));
    }

}

static void
TestParseErrors() {

    auto testErr = [](char const *exprStr) {
        SdfPredicateExpression e(exprStr);
        if (e) {
            TF_FATAL_ERROR("Expected error in '%s'", exprStr);
        }
        printf("Expected parse error in '%s': %s\n",
               exprStr, e.GetParseError().c_str());
    };

    for (auto testStr: {
            "and",
            "and and",
            "x and",
            "",
            "or",
            "or or",
            "or b",
            "not",
            "not not",
            "a not",
            "(",
            ")",
            "((foo)",
            "bar)",
            "(baz))",
            "badCallArgs(pos1, kw=foo,pos2)",
            "badCallArgs(",
        }) {
        testErr(testStr);
    }
}

static void
TestLinkErrors() {

    struct None {};
    
    // These "do nothing" because this test is only about linking expressions
    // with libraries.
    auto testLib = SdfPredicateLibrary<None>()
        .Define("noArgs", [](None) { return true; })
        .Define("onePosArg", [](None, int) { return true; })
        .Define("twoPosArgs", [](None, int, int) { return true; })
        .Define("oneKwArg", [](None, int) { return true; }, {{"kw1"}})
        .Define("twoKwArgs", [](None, int, int) { return true; },
                {{"kw1"}, {"kw2"}})
        .Define("onePosOneKw", [](None, int, int) { return true; },
                {{"kw1"}})
        .Define("twoKwOneDefault", [](None, int, int) { return true; },
                {{"kw1"}, {"kw2", 123}})
        ;

    auto testErr = [&testLib](char const *exprStr) {
        SdfPredicateExpression e(exprStr);
        TF_AXIOM(e);

        // Check that linking produces an invalid program and emits errors.
        TfErrorMark m;
        auto prog = SdfLinkPredicateExpression(e, testLib);
        TF_AXIOM(!prog);
        TF_AXIOM(!m.IsClean());
        printf("Expected link errors in '%s':\n", exprStr);
        for (TfError const &err: m) {
            printf("  - %s\n", err.GetCommentary().c_str());
        }
    };

    for (auto testStr: {
            "noArgs:1",
            "noArgs:1,2,3",
            "noArgs(1,2,3)",
            "onePosArg",
            "onePosArg:a,b",
            "onePosArg(1,2,3)",
            "twoPosArgs",
            "twoPosArgs:1",
            "twoPosArgs(1,2,3)",
            "oneKwArg",
            "oneKwArg:hello",
            "oneKwArg:1,2",
            "oneKwArg(wrongName=1)",
            "oneKwArg(1, kw1=1)",
            "oneKwArg(kw1=hello)",
            "twoKwArgs",
            "twoKwArgs:hello",
            "twoKwArgs:1,2,3",
            "twoKwArgs(wrongName1=1,wrongName2=2)",
            "twoKwArgs(1, kw1=1)",
            "onePosOneKw",
            "onePosOneKw:1",
            "onePosOneKw:1,2,3",
            "onePosOneKw(1)",
            "onePosOneKw(1,2,3)",
            "onePosOneKw(kw1=1, kw2=2)",
            "twoKwOneDefault",
            "twoKwOneDefault:1,2,3",
            "twoKwOneDefault(1,2,3)",
            "twoKwOneDefault(kw2=2)",
            "twoKwOneDefault(kw1=hello)",
            "twoKwOneDefault(hello)",
        }) {
        testErr(testStr);
    }

}

int
main(int argc, char **argv)
{
    TestSimple();
    TestParseErrors();
    TestLinkErrors();

    printf(">>> Test SUCCEEDED\n");
    return 0;
}

