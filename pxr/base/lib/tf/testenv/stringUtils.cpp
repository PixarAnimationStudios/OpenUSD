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
#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/defines.h"

#include <stdarg.h>
#include <string>
#include <vector>
#include <sstream>
#include <stdio.h>

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

static bool
TestNumbers()
{
    /* compare as floats */
    TF_AXIOM(float(TfStringToDouble("")) == 0.0f);
    TF_AXIOM(float(TfStringToDouble("blah")) == 0.0f);
    TF_AXIOM(float(TfStringToDouble("-")) == -0.0f);
    TF_AXIOM(float(TfStringToDouble("1.2")) == 1.2f);
    TF_AXIOM(float(TfStringToDouble("1")) == 1.0f);
    TF_AXIOM(float(TfStringToDouble("-5000001")) == -5000001.0f);
    TF_AXIOM(float(TfStringToDouble("0.123")) == 0.123f);
    TF_AXIOM(float(TfStringToDouble("-.123")) == -0.123f);
    TF_AXIOM(float(TfStringToDouble(string("-1e3"))) == -1e3f);
    TF_AXIOM(float(TfStringToDouble(string("1e6"))) == 1e6f);
    TF_AXIOM(float(TfStringToDouble(string("-1E-1"))) == -1E-1f);

    TF_AXIOM(TfIntToString(1) == "1");
    TF_AXIOM(TfIntToString(1024) == "1024");
    TF_AXIOM(TfIntToString(0) == "0");
    TF_AXIOM(TfIntToString(-22) == "-22");

    // Test round-tripping of floating point numbers.
    // This is obviously not an exhaustive test of the 2^64 space of
    // double-precision floats -- these are simply representative values
    // that failed to round-trip correctly under a prior implementation.
    TF_AXIOM(TfStringToDouble(TfStringify(0.1)) == 0.1);
    TF_AXIOM(TfStringToDouble(TfStringify(0.336316384899143))
             == 0.336316384899143);
    TF_AXIOM(float(TfStringToDouble(TfStringify(0.1f))) == 0.1f);
    TF_AXIOM(float(TfStringToDouble(TfStringify(0.84066f))) == 0.84066f);

    // Test similar operations on stream based stringify operations
    std::stringstream sstr;

    sstr << TfStreamDouble(0.1);
    TF_AXIOM(TfStringToDouble(sstr.str()) == 0.1);
    sstr.str(std::string());

    sstr << TfStreamDouble(0.336316384899143);
    TF_AXIOM(TfStringToDouble(sstr.str()) == 0.336316384899143);
    sstr.str(std::string());

    sstr << TfStreamFloat(0.1f);
    TF_AXIOM(float(TfStringToDouble(sstr.str())) == 0.1f);
    sstr.str(std::string());

    sstr << TfStreamFloat(0.84066f);
    TF_AXIOM(float(TfStringToDouble(sstr.str())) == 0.84066f);

    return true;
}

static bool
DictLessThan(const string &a, const string &b)
{
    return TfDictionaryLessThan()(a, b);
}

static bool
TestPreds()
{
    TF_AXIOM(TfStringStartsWith("  ", "  "));
    TF_AXIOM(TfStringStartsWith("abc", "ab"));
    TF_AXIOM(TfStringStartsWith("xyz", "xyz"));
    TF_AXIOM(TfStringStartsWith("a little bit longer string", "a little"));
    TF_AXIOM(TfStringStartsWith("anything", ""));
    TF_AXIOM(!TfStringStartsWith("", " "));
    TF_AXIOM(!TfStringStartsWith("abc", "bc"));

    TF_AXIOM(TfStringEndsWith("  ", "  "));
    TF_AXIOM(TfStringEndsWith("abc", "bc"));
    TF_AXIOM(TfStringEndsWith("xyz", "xyz"));
    TF_AXIOM(TfStringEndsWith("a little bit longer string", " string"));
    TF_AXIOM(TfStringEndsWith("anything", ""));
    TF_AXIOM(!TfStringEndsWith("", " "));
    TF_AXIOM(!TfStringEndsWith("abc", "ab"));

    TF_AXIOM(DictLessThan("ring", "robot"));
    TF_AXIOM(!DictLessThan("robot", "ring"));
    TF_AXIOM(!DictLessThan("Alex", "aardvark"));
    TF_AXIOM(DictLessThan("aardvark", "Alex"));
    TF_AXIOM(DictLessThan("Alex", "AMD"));
    TF_AXIOM(!DictLessThan("AMD", "Alex"));
    TF_AXIOM(DictLessThan("1", "15"));
    TF_AXIOM(!DictLessThan("15", "1"));
    TF_AXIOM(DictLessThan("1998", "1999"));
    TF_AXIOM(!DictLessThan("1999", "1998"));
    TF_AXIOM(DictLessThan("Worker8", "Worker11"));
    TF_AXIOM(!DictLessThan("Worker11", "Worker8"));
    TF_AXIOM(DictLessThan("agent007", "agent222"));
    TF_AXIOM(!DictLessThan("agent222", "agent007"));
    TF_AXIOM(DictLessThan("agent007", "agent0007"));
    TF_AXIOM(DictLessThan("agent7", "agent07"));
    TF_AXIOM(!DictLessThan("agent07", "agent07"));
    TF_AXIOM(DictLessThan("0", "00"));
    TF_AXIOM(DictLessThan("1", "01"));
    TF_AXIOM(!DictLessThan("2", "01"));
    TF_AXIOM(DictLessThan("foo001bar001abc", "foo001bar002abc"));
    TF_AXIOM(DictLessThan("foo001bar01abc", "foo001bar001abc"));
    TF_AXIOM(!DictLessThan("foo001bar002abc", "foo001bar001abc"));
    TF_AXIOM(DictLessThan("foo00001bar0002abc", "foo001bar002xyz"));
    TF_AXIOM(!DictLessThan("foo00001bar0002xyz", "foo001bar002abc"));
    TF_AXIOM(DictLessThan("foo1bar02", "foo01bar2"));
    TF_AXIOM(DictLessThan("agent007", "agent8"));
    TF_AXIOM(DictLessThan("agent007", "agent222"));
    TF_AXIOM(DictLessThan("agent007", "agent222"));
    TF_AXIOM(!DictLessThan("GOTO8", "goto7"));
    TF_AXIOM(DictLessThan("goto7", "GOTO8"));
    TF_AXIOM(DictLessThan("!", "$"));
    TF_AXIOM(!DictLessThan("$", "!"));
    TF_AXIOM(DictLessThan("foo", "foo") == 0);
    TF_AXIOM(DictLessThan("aa", "aaa"));
    TF_AXIOM(!DictLessThan("aaa", "aa"));

    TF_AXIOM(TfIsValidIdentifier("f"));
    TF_AXIOM(TfIsValidIdentifier("foo"));
    TF_AXIOM(TfIsValidIdentifier("foo1"));
    TF_AXIOM(TfIsValidIdentifier("_foo"));
    TF_AXIOM(TfIsValidIdentifier("_foo1"));
    TF_AXIOM(TfIsValidIdentifier("__foo__"));
    TF_AXIOM(TfIsValidIdentifier("__foo1__"));
    TF_AXIOM(TfIsValidIdentifier("__foo1__2"));
    TF_AXIOM(TfIsValidIdentifier("_"));
    TF_AXIOM(TfIsValidIdentifier("_2"));

    TF_AXIOM(!TfIsValidIdentifier(""));
    TF_AXIOM(!TfIsValidIdentifier("1"));
    TF_AXIOM(!TfIsValidIdentifier("2foo"));
    TF_AXIOM(!TfIsValidIdentifier("1_foo"));
    TF_AXIOM(!TfIsValidIdentifier("13_foo2"));

    TF_AXIOM(!TfIsValidIdentifier(" "));
    TF_AXIOM(!TfIsValidIdentifier(" foo"));
    TF_AXIOM(!TfIsValidIdentifier(" _foo\n "));
    TF_AXIOM(!TfIsValidIdentifier(" _foo32 \t   "));

    TF_AXIOM(!TfIsValidIdentifier("$"));
    TF_AXIOM(!TfIsValidIdentifier("\a"));
    TF_AXIOM(!TfIsValidIdentifier("foo$"));
    TF_AXIOM(!TfIsValidIdentifier("_foo$"));
    TF_AXIOM(!TfIsValidIdentifier(" _foo$"));
    TF_AXIOM(!TfIsValidIdentifier("foo bar"));
    TF_AXIOM(!TfIsValidIdentifier("\"foo\""));

    return true;
}

string
DoPrintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    string ret = TfVStringPrintf(fmt, ap);
    va_end(ap);
    return ret;
}

string
DoPrintfStr(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    string ret = TfVStringPrintf(string(fmt), ap);
    va_end(ap);
    return ret;
}

static bool
TestStrings()
{
    TF_AXIOM(TfStringToLower("  ") == "  ");
    TF_AXIOM(TfStringToLower("lower") == "lower");
    TF_AXIOM(TfStringToLower("LOWER") == "lower");
    TF_AXIOM(TfStringToLower("LOWer") == "lower");
    TF_AXIOM(TfStringToLower("LOWer@123") == "lower@123");

    TF_AXIOM(TfStringToUpper("upper") == "UPPER");
    TF_AXIOM(TfStringToUpper("UPPER") == "UPPER");
    TF_AXIOM(TfStringToUpper("UPPer") == "UPPER");
    TF_AXIOM(TfStringToUpper("UPPer@123") == "UPPER@123");

    TF_AXIOM(TfStringCapitalize("Already") == "Already");
    TF_AXIOM(TfStringCapitalize("notyet") == "Notyet");
    TF_AXIOM(TfStringCapitalize("@@@@") == "@@@@");
    TF_AXIOM(TfStringCapitalize("") == "");

    TF_AXIOM(TfStringGetSuffix("file.ext") == "ext");
    TF_AXIOM(TfStringGetSuffix("here are some words", ' ') == "words");
    TF_AXIOM(TfStringGetSuffix("0words", '0') == "words");
    TF_AXIOM(TfStringGetSuffix("A@B@C", '@') == "C");
    TF_AXIOM(TfStringGetSuffix("nothing", ' ') == "");
    TF_AXIOM(TfStringGetSuffix("nothing", '\0') == "");

    TF_AXIOM(TfStringGetBeforeSuffix("file.ext") == "file");
    TF_AXIOM(TfStringGetBeforeSuffix("here are some words", ' ') == "here are some");
    TF_AXIOM(TfStringGetBeforeSuffix("0words", '0') == "");
    TF_AXIOM(TfStringGetBeforeSuffix("A@B@C", '@') == "A@B");
    TF_AXIOM(TfStringGetBeforeSuffix("nothing", ' ') == "nothing");
    TF_AXIOM(TfStringGetBeforeSuffix("nothing", '\0') == "nothing");

    TF_AXIOM(TfGetBaseName("") == "");
    TF_AXIOM(TfGetBaseName("/foo/bar") == "bar");
    TF_AXIOM(TfGetBaseName("/foo/bar/") == "bar");
    TF_AXIOM(TfGetBaseName("../some-dir/bar") == "bar");
    TF_AXIOM(TfGetBaseName("bar") == "bar");
#if defined(ARCH_OS_WINDOWS)
    // Same on Windows but with backslashes.
    TF_AXIOM(TfGetBaseName("\\foo\\bar") == "bar");
    TF_AXIOM(TfGetBaseName("\\foo\\bar\\") == "bar");
    TF_AXIOM(TfGetBaseName("..\\some-dir\\bar") == "bar");
#endif

    TF_AXIOM(TfGetPathName("") == "");
    TF_AXIOM(TfGetPathName("/") == "/");
    TF_AXIOM(TfGetPathName("/foo/bar") == "/foo/");
    TF_AXIOM(TfGetPathName("../some-dir/bar") == "../some-dir/");
    TF_AXIOM(TfGetPathName("bar") == "");
#if defined(ARCH_OS_WINDOWS)
    // Same on Windows but with backslashes.
    TF_AXIOM(TfGetPathName("\\") == "\\");
    TF_AXIOM(TfGetPathName("\\foo\\bar") == "\\foo\\");
    TF_AXIOM(TfGetPathName("..\\some-dir\\bar") == "..\\some-dir\\");
#endif

    TF_AXIOM(TfStringTrimRight("", " ") == "");
    TF_AXIOM(TfStringTrimRight("to be trimmed") == "to be trimmed");
    TF_AXIOM(TfStringTrimRight("to be trimmed", "x") == "to be trimmed");
    TF_AXIOM(TfStringTrimRight(" to be trimmed ") == " to be trimmed");
    TF_AXIOM(TfStringTrimRight("  to be trimmed  ", " ") == "  to be trimmed");
    TF_AXIOM(TfStringTrimRight(" to be trimmed ", "x ") == " to be trimmed");

    TF_AXIOM(TfStringTrimLeft("", " ") == "");
    TF_AXIOM(TfStringTrimLeft("to be trimmed") == "to be trimmed");
    TF_AXIOM(TfStringTrimLeft("to be trimmed", "x") == "to be trimmed");
    TF_AXIOM(TfStringTrimLeft(" to be trimmed ") == "to be trimmed ");
    TF_AXIOM(TfStringTrimLeft("  to be trimmed  ", " ") == "to be trimmed  ");
    TF_AXIOM(TfStringTrimLeft(" to be trimmed ", "x ") == "to be trimmed ");

    TF_AXIOM(TfStringTrim("", " ") == "");
    TF_AXIOM(TfStringTrim("to be trimmed") == "to be trimmed");
    TF_AXIOM(TfStringTrim("to be trimmed", "x") == "to be trimmed");
    TF_AXIOM(TfStringTrim(" to be trimmed ") == "to be trimmed");
    TF_AXIOM(TfStringTrim("  to be trimmed  ", " ") == "to be trimmed");
    TF_AXIOM(TfStringTrim(" to be trimmed ", "x ") == "to be trimmed");
    TF_AXIOM(TfStringTrim("_to be trimmed ", "_ ") == "to be trimmed");

    TF_AXIOM(TfStringReplace("an old string", "n old", " new") == "a new string");
    TF_AXIOM(TfStringReplace("remove", "remove", "") == "");
    TF_AXIOM(TfStringReplace("12121", "21", "31") == "13131");
    TF_AXIOM(TfStringReplace("aaaa", "aa", "b") == "bb");
    TF_AXIOM(TfStringReplace("no more spaces", " ", "_") == "no_more_spaces");
    TF_AXIOM(TfStringReplace("Capital", "cap", "zap") == "Capital");
    TF_AXIOM(TfStringReplace("string", "", "number") == "string");
    TF_AXIOM(TfStringReplace("string", "str", "str") == "string");

    TF_AXIOM(TfStringGetCommonPrefix("", "") == "");
    TF_AXIOM(TfStringGetCommonPrefix("a", "") == "");
    TF_AXIOM(TfStringGetCommonPrefix("", "b") == "");
    TF_AXIOM(TfStringGetCommonPrefix("a", "b") == "");
    TF_AXIOM(TfStringGetCommonPrefix("a", "a") == "a");
    TF_AXIOM(TfStringGetCommonPrefix("abracadabra", "abracababra") == "abraca");
    TF_AXIOM(TfStringGetCommonPrefix("aabcd", "aaabcd") == "aa");
    TF_AXIOM(TfStringGetCommonPrefix("aabcdefg", "aabcd") == "aabcd");

    std::string const s = "foo";
    TF_AXIOM(TfStringify(s) == "foo");
    TF_AXIOM(TfStringify(true) == "true");
    TF_AXIOM(TfStringify(false) == "false");
    TF_AXIOM(TfUnstringify<bool>("true") == true);
    TF_AXIOM(TfUnstringify<bool>("false") == false);
    TF_AXIOM(TfStringify(1) == "1");
    TF_AXIOM(TfUnstringify<int>("1") == 1);
    TF_AXIOM(TfStringify(1.1) == "1.1");
    TF_AXIOM(TfUnstringify<float>("1.1") == 1.1f);
    TF_AXIOM(TfStringify('a') == "a");
    TF_AXIOM(TfUnstringify<char>("a") == 'a');
    TF_AXIOM(TfStringify("string") == "string");
    TF_AXIOM(TfUnstringify<string>("string") == "string");

    bool unstringRet = true;
    TfUnstringify<int>("this ain't no int", &unstringRet);
    TF_AXIOM(unstringRet == false);

    TF_AXIOM(TfStringPrintf("%s", "hello") == "hello");
    TF_AXIOM(TfStringPrintf("%d%d", 1, 2) == "12");
    TF_AXIOM(DoPrintf("%s", "hello") == "hello");
    TF_AXIOM(DoPrintf("%d%d", 1, 2) == "12");
    TF_AXIOM(DoPrintfStr("%s", "hello") == "hello");
    TF_AXIOM(DoPrintfStr("%d%d", 1, 2) == "12");

    TF_AXIOM(TfEscapeString("\\\\") == "\\");
    TF_AXIOM(TfEscapeString("new\\nline") == "new\nline");
    TF_AXIOM(TfEscapeString("two\\nnew\\nlines") == "two\nnew\nlines");
    TF_AXIOM(TfEscapeString("a\\ttab") == "a\ttab");
    TF_AXIOM(TfEscapeString("\\a\\b") == "\a\b");
    TF_AXIOM(TfEscapeString("\\f\\n") == "\f\n");
    TF_AXIOM(TfEscapeString("\\r\\v") == "\r\v");
    TF_AXIOM(TfEscapeString("\\c \\d") == "c d");
    TF_AXIOM(TfEscapeString("\\xB") == "\xB");
    TF_AXIOM(TfEscapeString("\\xab") == "\xab");
    TF_AXIOM(TfEscapeString("\\x01f") == "\x01f");
    TF_AXIOM(TfEscapeString("\\x008d") == "\x008d");
    TF_AXIOM(TfEscapeString("x\\x0x") == string() + 'x' + '\0' + 'x');
    TF_AXIOM(TfEscapeString("\\5") == "\5");
    TF_AXIOM(TfEscapeString("\\70") == "\70");
    TF_AXIOM(TfEscapeString("\\11z") == "\11z");
    TF_AXIOM(TfEscapeString("\\007") == "\007");
    TF_AXIOM(TfEscapeString("\\008") == string() + '\0' + '8');
    TF_AXIOM(TfEscapeString("\\010") == "\010");
    TF_AXIOM(TfEscapeString("\\0077") == "\0077");
    TF_AXIOM(TfEscapeString("\\00107") == "\00107");
    TF_AXIOM(TfEscapeString("\\005107") == "\005107");

    TF_AXIOM(TfStringCatPaths("foo", "bar") == "foo/bar");
    TF_AXIOM(TfStringCatPaths("foo/crud", "../bar") == "foo/bar");
    TF_AXIOM(TfStringCatPaths("foo", "../bar") == "bar");
    TF_AXIOM(TfStringCatPaths("/foo", "../bar") == "/bar");
    TF_AXIOM(TfStringCatPaths("foo/crud/crap", "../bar") == "foo/crud/bar");
#if defined(ARCH_OS_WINDOWS)
    // Same on Windows but with backslashes.
    TF_AXIOM(TfStringCatPaths("foo", "bar") == "foo/bar");
    TF_AXIOM(TfStringCatPaths("foo\\crud", "../bar") == "foo/bar");
    TF_AXIOM(TfStringCatPaths("foo", "..\\bar") == "bar");
    TF_AXIOM(TfStringCatPaths("\\foo", "..\\bar") == "/bar");
    TF_AXIOM(TfStringCatPaths("foo\\crud\\crap", "..\\bar") == "foo/crud/bar");
#endif

    return true;
}

static bool
TestTokens()
{
    vector<string> tokens;
    vector<string> empty;
    set<string> tokenSet;

    TF_AXIOM(TfStringJoin(empty, " ") == "");

    tokenSet = TfStringTokenizeToSet(" to   be   tokens ", " ");
    TF_AXIOM(tokenSet.size() == 3);

    tokenSet = TfStringTokenizeToSet(" to   be   tokens", " ");
    TF_AXIOM(tokenSet.size() == 3);

    tokens = TfStringTokenize(" to   be   tokens ", " ");
    TF_AXIOM(tokens.size() == 3);
    TF_AXIOM(TfStringJoin(tokens, " ") == "to be tokens");

    tokens = TfStringTokenize("A1B2C3", "123");
    TF_AXIOM(tokens.size() == 3);
    TF_AXIOM(TfStringJoin(tokens, "") == "ABC");

    tokens = TfStringTokenize("no tokens", "");
    TF_AXIOM(tokens.size() == 1);
    TF_AXIOM(TfStringJoin(tokens, "") == "no tokens");

    tokens = TfStringTokenize("no tokens", "xyz");
    TF_AXIOM(tokens.size() == 1);
    TF_AXIOM(TfStringJoin(tokens, " ") == "no tokens");

    tokens = TfQuotedStringTokenize("\"no tokens\"", " ");
    TF_AXIOM(tokens.size() == 1);
    TF_AXIOM(TfStringJoin(tokens, " ") == "no tokens");

    tokens = TfQuotedStringTokenize("  foo\"no tokens\"", " ");
    TF_AXIOM(tokens.size() == 1);
    TF_AXIOM(TfStringJoin(tokens, " ") == "foono tokens");

    // test error conditions
    string errorString;
    tokens = TfQuotedStringTokenize("\"no tokens\"", "\"", &errorString);
    TF_AXIOM(errorString != "");
    errorString = "";
    tokens = TfQuotedStringTokenize("\"no tokens", " ", &errorString);
    TF_AXIOM(errorString != "");
    TF_AXIOM(tokens.empty());
    
    tokens = TfQuotedStringTokenize("A1B2C3", "123");
    TF_AXIOM(tokens.size() == 3);
    TF_AXIOM(TfStringJoin(tokens, "") == "ABC");
    
    tokens = TfQuotedStringTokenize("\"a \\\"b\\\" c\" d");
    TF_AXIOM(tokens.size() == 2);
    TF_AXIOM(TfStringJoin(tokens, " ") == "a \"b\" c d");
    
    tokens = TfQuotedStringTokenize(" \"there are\" \"two tokens\" ");
    TF_AXIOM(tokens.size() == 2);
    TF_AXIOM(TfStringJoin(tokens, " ") == "there are two tokens");
    
    tokens = TfQuotedStringTokenize("\"there is\"\" one token\"", " ");
    TF_AXIOM(tokens.size() == 1);
    TF_AXIOM(TfStringJoin(tokens, " ") == "there is one token");
    
    tokens = TfQuotedStringTokenize("\\\"this_gets_split\\\"", "_");
    TF_AXIOM(tokens.size() == 3);
    TF_AXIOM(TfStringJoin(tokens, " ") == "\"this gets split\"");
    
    tokens = TfQuotedStringTokenize("\"\\\"this_doesn't\\\"\"", "_");
    TF_AXIOM(tokens.size() == 1);
    TF_AXIOM(TfStringJoin(tokens, " ") == "\"this_doesn't\"");
    
    tokens = TfQuotedStringTokenize("\"\'nothing\' `to` \\\"split\\\"\"", " ");
    TF_AXIOM(tokens.size() == 1);
    TF_AXIOM(TfStringJoin(tokens, " ") == "\'nothing\' `to` \"split\"");

    tokens = TfQuotedStringTokenize("\'esc\\\"\' \\\"aped", " ");
    TF_AXIOM(tokens.size() == 2);
    TF_AXIOM(TfStringJoin(tokens, " ") == "esc\" \"aped");

    // test error conditions

    // same delimiter
    errorString = "";
    tokens = TfMatchedStringTokenize("{", '{', '{', '\0', &errorString);
    TF_AXIOM(tokens.empty());
    TF_AXIOM(errorString != "");
    // delimiter order
    errorString = "";
    tokens = TfMatchedStringTokenize("}garble{", '{', '}', '\0', &errorString);
    TF_AXIOM(tokens.empty());
    TF_AXIOM(errorString != "");
    // unmatched open
    errorString = "";
    tokens = TfMatchedStringTokenize("{garble} {", '{', '}', '\0', &errorString);
    TF_AXIOM(tokens.empty());
    TF_AXIOM(errorString != "");
    // unmatched close
    errorString = "";
    tokens = TfMatchedStringTokenize("{garble} }", '{', '}', '\0', &errorString);
    TF_AXIOM(tokens.empty());
    TF_AXIOM(errorString != "");

    tokens = TfMatchedStringTokenize("{", '{', '}');
    TF_AXIOM(tokens.empty());

    tokens = TfMatchedStringTokenize("}", '{', '}');
    TF_AXIOM(tokens.empty());

    tokens = TfMatchedStringTokenize("}{}", '{', '}');
    TF_AXIOM(tokens.empty());

    tokens = TfMatchedStringTokenize("{}{", '{', '}');
    TF_AXIOM(tokens.empty());

    tokens = TfMatchedStringTokenize("{}}", '{', '}');
    TF_AXIOM(tokens.empty());

    tokens = TfMatchedStringTokenize("{{}", '{', '}');
    TF_AXIOM(tokens.empty());

    tokens = TfMatchedStringTokenize("{whoops", '{', '}');
    TF_AXIOM(tokens.empty());

    tokens = TfMatchedStringTokenize("none!", '{', '}');
    TF_AXIOM(tokens.empty());

    tokens = TfMatchedStringTokenize("{test {test} test}", '{', '}');
    TF_AXIOM(tokens.size() == 1);
    TF_AXIOM(TfStringJoin(tokens, " ") == "test {test} test");

    tokens = TfMatchedStringTokenize("{foo} {bar}", '{', '}');
    TF_AXIOM(tokens.size() == 2);
    TF_AXIOM(TfStringJoin(tokens, " ") == "foo bar");

    tokens = TfMatchedStringTokenize("out{in}out", '{', '}');
    TF_AXIOM(tokens.size() == 1);
    TF_AXIOM(TfStringJoin(tokens, " ") == "in");

    tokens = TfMatchedStringTokenize("{} {} {stuff_{foo}_{bar}}", '{', '}');
    TF_AXIOM(tokens.size() == 3);
    TF_AXIOM(TfStringJoin(tokens, " ") == "  stuff_{foo}_{bar}");

    tokens = TfMatchedStringTokenize("{and} {more{nested{braces}}}", '{', '}');
    TF_AXIOM(tokens.size() == 2);
    TF_AXIOM(TfStringJoin(tokens, " ") == "and more{nested{braces}}");

    return true;
}

static bool
TestGetXmlEscapedString()
{
    TF_AXIOM(TfGetXmlEscapedString("Amiga")         == "Amiga");
    TF_AXIOM(TfGetXmlEscapedString("Amiga & Atari") == "Amiga &amp; Atari");
    TF_AXIOM(TfGetXmlEscapedString("Amiga < Atari") == "Amiga &lt; Atari");
    TF_AXIOM(TfGetXmlEscapedString("Amiga > Atari") == "Amiga &gt; Atari");
    TF_AXIOM(TfGetXmlEscapedString("\"Atari\"")     == "&quot;Atari&quot;");
    TF_AXIOM(TfGetXmlEscapedString("'Atari'")       == "&apos;Atari&apos;");

    return true;
}

static bool
Test_TfStringUtils()
{
    return TestNumbers() && TestPreds() && TestStrings() && TestTokens() &&
           TestGetXmlEscapedString();
}

TF_ADD_REGTEST(TfStringUtils);
