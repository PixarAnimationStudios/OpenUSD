//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/usd/sdf/textFileFormatParser.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace PEGTL_NS = Sdf_TextFileFormatParser::PEGTL_NS;

template <typename T>
bool DoParse(const std::string& expression)
{
    try
    {
        Sdf_TextParserContext context;
        context.magicIdentifierToken = "sdf";
        context.versionString = "1.4.32";

        std::cout << "Parsing " << expression << "\n";
        if(!PEGTL_NS::parse<T, Sdf_TextFileFormatParser::TextParserAction>(
            PEGTL_NS::string_input<> { expression ,""}, context))
        {
            return false;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }

    return true;    
}

bool
TestDigits()
{
    std::vector<std::string> validExpressions = {
        "0",
        "12345",
        "98765",
        "02345",
        "-0",
        "-12345",
        "-98765",
        "-02345",
        "0.425436",
        ".625462",
        "-0.43626",
        ".9097456",
        "0e2359670",
        "12345e2359670",
        "98765e2359670",
        "02345e2359670",
        "-0e2359670",
        "-12345e2359670",
        "-98765e2359670",
        "-02345e2359670",
        "0.425436e2359670",
        ".625462e2359670",
        "-0.43626e2359670",
        ".9097456e2359670",
        "12345E2359670",
        "98765E2359670",
        "02345E2359670",
        "-0E2359670",
        "-12345E2359670",
        "-98765E2359670",
        "-02345E2359670",
        "0.425436E2359670",
        ".625462E2359670",
        "-0.43626E2359670",
        ".9097456E2359670",
        "12345e-2359670",
        "98765e-2359670",
        "02345e-2359670",
        "-0e-2359670",
        "-12345e-2359670",
        "-98765e-2359670",
        "-02345e-2359670",
        "0.425436e-2359670",
        ".625462e-2359670",
        "-0.43626e-2359670",
        ".9097456e-2359670",
        "12345E-2359670",
        "98765E-2359670",
        "02345E-2359670",
        "-0E-2359670",
        "-12345E-2359670",
        "-98765E-2359670",
        "-02345E-2359670",
        "0.425436E-2359670",
        ".625462E-2359670",
        "-0.43626E-2359670",
        ".9097456E-2359670",
        "-0e+2359670",
        "-12345e+2359670",
        "-98765e+2359670",
        "-02345e+2359670",
        "0.425436e+2359670",
        ".625462e+2359670",
        "-0.43626e+2359670",
        ".9097456e+2359670",
        "12345E+2359670",
        "98765E+2359670",
        "02345E+2359670",
        "-0E+2359670",
        "-12345E+2359670",
        "-98765E+2359670",
        "-02345E+2359670",
        "0.425436E+2359670",
        ".625462E+2359670",
        "-0.43626E+2359670",
        ".9097456E+2359670",
        "0.",
        "inf",
        "-inf",
        "nan"
    };

    std::vector<std::string> invalidExpressions = {
        "-",
        "-nan",
        "0e",
        "e",
        "E",
        "E324",
        "-.E324",
        "-.",
        "0345346a",
        "63.42534t",
        "-23452.e",
        "8476.343e",
        "4264einf",
        "45346e-inf",
        "3456-",
        ".inf",
        ".nan",
        "-.inf",
        "-.nan"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(), 
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
            Sdf_TextFileFormatParser::Number, PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
            Sdf_TextFileFormatParser::Number, PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestIdentifiers()
{
    std::vector<std::string> validIdentifiers = {
        "foo",
        "foo_bar",
        "_foo",
        "_12432foo",
        "foo1257__",
        "foo_1235_bar__",
        "foo::bar",
        "foo::bar::baz",
        "FOO",
        "FOO::BAR",
        "foo__::__bar::_4BAZ99_"
    };

    std::vector<std::string> invalidIdentifiers = {
        "1foo",
        "connect",
        "rel",
        "0foo",
        "-inf",
        "None",
        "foo:bar",
        "foo:bar:baz",
        "foo_bar:baz",
        "FOO::bAr84_:baz",
        "foo::relocates",
        "foo/234"
    };

    TF_AXIOM(std::all_of(validIdentifiers.begin(), 
        validIdentifiers.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
            Sdf_TextFileFormatParser::Identifier, PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidIdentifiers.begin(),
        invalidIdentifiers.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
            Sdf_TextFileFormatParser::Identifier, PEGTL_NS::eof>>(expression);
        }));

    std::vector<std::string> validNamespacedNames = {
        "foo:bar",
        "foo:bar:baz",
        "foo_bar:baz",
        "FOO:bAr84_:baz",
        "foo",
        "_foo",
        "foo_12345_bar__",
        "relocates",
        "def",
        "over",
        "rootPrims:specializes:over"
    };

    std::vector<std::string> invalidNamespacedNames = {
        "foo::bar",
        "foo::bar::baz",
        "0foo",
        "f71.3124o7125o",
        "foo/234",
        "/"
    };

    TF_AXIOM(std::all_of(validNamespacedNames.begin(),
        validNamespacedNames.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
            Sdf_TextFileFormatParser::NamespacedName, PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidNamespacedNames.begin(),
        invalidNamespacedNames.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
            Sdf_TextFileFormatParser::NamespacedName, PEGTL_NS::eof>>(expression);
        }));

    // valid dictionary value types include all identifiers
    // plus identifiers with [] possibly padded with spaces / tabs
    // we algorithmically build this test from the existing identifiers
    size_t i = 1;
    std::vector<std::string> validTypeNames = validIdentifiers;
    for (const std::string& str : validIdentifiers)
    {
        std::string typeName = str;
        if ((i%2) == 0)
        {
            // pad tabs on front
            for (size_t numTabs = 0; numTabs < i; numTabs++)
            {
                typeName += "\t";
            }

            typeName += "[]";
        }
        else
        {
            // pad spaces on front
            for (size_t numSpaces = 0; numSpaces < i; numSpaces++)
            {
                typeName += " ";
            }

            typeName += "[]";
        }

        validTypeNames.push_back(typeName);
        i++;
    }

    // all invalid identifiers are invalid type names
    std::vector<std::string> invalidTypeNames = invalidIdentifiers;
    invalidTypeNames.push_back("foo \n []");
    invalidTypeNames.push_back("foo [3]");

    TF_AXIOM(std::all_of(validTypeNames.begin(),
        validTypeNames.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::DictionaryType,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidTypeNames.begin(),
        invalidTypeNames.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::DictionaryType,
                PEGTL_NS::eof>>(expression);
        }));

    return true; 
}

bool
TestStrings()
{
    // analyze will fail with custom actions like those we need for strings
    // so we skip the analyze part
    std::vector<std::string> validSingleLineStrings = {
        "'a simple string'",
        "'a simple string with a couple of utf-8 characters ß篲ü濯'",
        "'a string with an escaped \\''",
        "'a string with an escaped character \\b\\b'",
        "'ß___\\y\\x'",
        "'a string with embedded double quote\"'"
    };

    std::vector<std::string> invalidSingleLineStrings = {
        "'''",
        "'a string with an embedded CR \r'",
        "'a string with windows style line ending \r\n'",
        "'a string with no end quote",
        "'a string with an embedded LF \n'",
        "'a string with an attempt at escaping \''",
        "'a string with a properly escaped \\' but ending wrong''",
    };

    TF_AXIOM(std::all_of(validSingleLineStrings.begin(),
        validSingleLineStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::SinglelineSingleQuoteString,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidSingleLineStrings.begin(),
        invalidSingleLineStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::SinglelineSingleQuoteString,
                PEGTL_NS::eof>>(expression);
        }));

    std::vector<std::string> validMultiLineStrings = {
        "'''a simple multi-line string\n'''",
        "'''a simple windows style multi-line line ending string\r\n'''",
        "'''a \n many \n lined \r\n multi-line \n string\n'''",
        "'''a multiline string \n\n\n\r\n with an escaped \\'\\' set\n'''",        
        "'''\n\n A string \n containing \n utf-8 characters\n ß篲ü濯 \n '''"
    };

    std::vector<std::string> invalidMultiLineStrings = {
        "'''\nan \nunterminated multi-\r\nline string",
        "'''An incorrectly \n terminated multi-line string''",
        "'''\n\n A string \n containing \n utf-8 characters\n ß篲ü濯 \n ''\\''",
        "'A regular single quote string'"
    };

    TF_AXIOM(std::all_of(validMultiLineStrings.begin(),
        validMultiLineStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::MultilineSingleQuoteString,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidMultiLineStrings.begin(),
        invalidMultiLineStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::MultilineSingleQuoteString,
                PEGTL_NS::eof>>(expression);
        }));

    std::vector<std::string> validSingleLineDoubleQuoteStrings = {
        "\"a simple string\"",
        "\"a simple string with a couple of utf-8 characters ß篲ü濯\"",
        "\"a string with an escaped \\\"\"",
        "\"a string with an escaped character \\b\\b\"",
        "\"ß___\\y\\x\"",
        "\"a string with embedded single quote '\""
    };

    std::vector<std::string> invalidSingleLineDoubleQuoteStrings = {
        "\"\"\"",
        "\"a string with an embedded CR \r\"",
        "\"a string with no end quote",
        "\"a string with an embedded LF \n\"",
        "\"a string with an attempt at escaping \\ \"\"",
        "\"a string with a properly escaped \\\" but ending wrong\"\"",
    };

    TF_AXIOM(std::all_of(validSingleLineDoubleQuoteStrings.begin(),
        validSingleLineDoubleQuoteStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::SinglelineDoubleQuoteString,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidSingleLineDoubleQuoteStrings.begin(),
        invalidSingleLineDoubleQuoteStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::SinglelineDoubleQuoteString,
                PEGTL_NS::eof>>(expression);
        }));

    std::vector<std::string> validMultiLineDoubleQuoteStrings = {
        "\"\"\"a simple multi-line string\n\"\"\"",
        "\"\"\"a \n many \n lined \r\n multi-line \n string\n\"\"\"",
        "\"\"\"a multiline string \n\n\n\r\n with"
        " an escaped \\\"\\\" set\n\"\"\"",
        "\"\"\"\n\n A string \n containing \n "
        "utf-8 characters\n ß篲ü濯 \n \"\"\""
    };

    std::vector<std::string> invalidMultiLineDoubleQuoteStrings = {
        "\"\"\"\nan \nunterminated multi-\r\nline string",
        "\"\"\"An incorrectly \n terminated multi-line string\"\"",
        "\"\"\"\n\n A string \n containing \n "
        "utf-8 characters\n ß篲ü濯 \n ''\\'\"\"",
        "\"A regular single quote string\""
    };

    TF_AXIOM(std::all_of(validMultiLineDoubleQuoteStrings.begin(),
        validMultiLineDoubleQuoteStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::MultilineDoubleQuoteString,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidMultiLineDoubleQuoteStrings.begin(),
        invalidMultiLineDoubleQuoteStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::MultilineDoubleQuoteString,
                PEGTL_NS::eof>>(expression);
        }));

    std::vector<std::string> validMixedStrings = {
        "''",
        "\"\"",
        "''''''",
        "\"\"\"\"\"\"",
        "\"a simple string\"",
        "'''a multiline string \n\n\n\r\n with an escaped \\'\\' set\n'''",
        "'''\n\n\n\n\n\\'\\'\n\n\n'''",
        "\"\"\"''\\\"\\\"\'\n\n\n\n\n\n\n\"\"\"",
        "'''\n\n\n\n\n\n''\n\n\n\n\n'''"
    };

    std::vector<std::string> invalidMixedStrings = {
        "'",
        "\"",
        "'''''",
        "\"\"\"\"\"",
        "\"a string with a properly escaped \\\" but ending wrong\"\"",
        "\"\"\"\n\n A string \n containing \n "
        "utf-8 characters\n ß篲ü濯 \n ''\\'\"\"",
        "'''\n\n A string \n containing \n utf-8 characters\n ß篲ü濯 \n ''\\''"
    };

    TF_AXIOM(std::all_of(validMixedStrings.begin(),
        validMixedStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::String,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidMixedStrings.begin(),
        invalidMixedStrings.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::String,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestAssetRefs()
{
    std::vector<std::string> validExpressions = {
        "@@",
        "@c:\\foo\\bar_baz\\foo@",
        "@foo__34-123\\ß篲ü濯@",
        "@@@C:foobar_bazfoo@@@",
        "@@@c:\\foo\\bar_baz\\foo@@@",
        "@@@foo__34-123\\ß篲ü濯@@@",
        "@@@c:\\foo\\@@@\\@@_@_\\@@@@@@",
        "@@@@foo.sdf@@@@"
    };

    std::vector<std::string> invalidExpressions = {
        "@c:\\foo\\@bar_baz\\foo@",
        "@@@c:\\foo@@@\\@@_@_\\@@@@@@"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::AssetRef,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::AssetRef,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestPathRefs()
{
    std::vector<std::string> validExpressions = {
        "</Foo/Bar.baz>",
        "<Foo>",
        "<Foo/Bar>",
        "<Foo.bar>",
        "<Foo/Bar.bar>",
        "<.bar>",
        "</Some/Kinda/Long/Path/Just/To/Make/Sure>",
        "<Some/Kinda/Long/Path/Just/To/Make/Sure.property>",
        "<../Some/Kinda/Long/Path/Just/To/Make/Sure>",
        "<../../Some/Kinda/Long/Path/Just/To/Make/Sure.property>",
        "</Foo/Bar.baz[targ].boom>",
        "<Foo.bar[targ].boom>",
        "<.bar[targ].boom>",
        "<Foo.bar[targ.attr].boom>",
        "</A/B/C.rel3[/Blah].attr3>",
        "<A/B.rel2[/A/B/C.rel3[/Blah].attr3].attr2>",
        "</A.rel1[/A/B.rel2[/A/B/C.rel3[/Blah].attr3].attr2].attr1>",
        "</root_utf8_umlaute_ß_3>"
    };

    std::vector<std::string> invalidExpressions = {
        "</Foo/Bar.baz",
        "<DD/DDD.&ddf$>",
        "<DD[]/DDD>",
        "<DD[]/DDD.bar>",
        "<foo.prop/bar>",
        "</foo.prop/bar.blah>",
        "</foo.prop/bar.blah>",
        "</foo//bar>",
        "</foo/.bar>",
        "</foo..bar>",
        "</foo.bar.baz>",
        "</.foo>",
        "</foo.bar",
        "</Foo/Bar/>",
        "</Foo.bar[targ]/Bar>",
        "</Foo.bar[targ].foo.foo>",
        "<123>",
        "<123test>",
        "</Foo:Bar>",
        "</Foo.bar.mapper[/Targ.attr].arg:name:space>"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PathRef,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PathRef,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestTupleValues()
{
    std::vector<std::string> validExpressions = {
        "(-inf)",
        "(-inf,)",
        "(  -923452345.2,            .234125e+56243   ,)",
        "(foo, bar, baz)",
        "(   foo   , bar, baz)",
        "(\n \"this is a string value\", -67.45e2311, \n\n 'another string',)",
        "(\"\"\"a multiline \n \n string as a tuple value\n\"\"\")",
        "(@this is an asset references in a tuple@, foo, nan)",
        "(45.75, @@@an escaped asset reference @ in \\@@@ a tuple@@@)",
        "(foo,   bar,    baz,)",
        "(-0.56e-456,   foo,            0, 0.56, bar)",
        "(\n-inf)",
        "(\r\n-inf,)",
        "(\r  -923452345.2,            .234125e+56243   ,)",
        "(foo, bar, baz\n)",
        "(\nfoo,  \n bar,  \n  baz,)",
        "(-0.56e-456,   foo,            0, 0.56, bar\r\n)",
        "(\n  foo, \n  bar  ,\n  (\n    baz,\n\n    567.3e-45\n)\n\n\n)",
        "(foo /*HELLO*/, bar)",
        "(foo, #comment\n bar)",
        "(\n1 /*HELLO*//*WORLD*/,\n 2, \n 3)"
    };

    std::vector<std::string> invalidExpressions = {
        "(",
        ")",
        "()",
        "(foo, bar, ())",
        "\n(foo, bar, baz)",
        "(varying)",
        "(foo, uniform)",
        "(foo \n, uniform)", // The newline may not be before the commma
        "(foo #comment\n, uniform)" // Single line comments may not preceed the comma
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::TupleValue,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::TupleValue,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestListValues()
{
    std::vector<std::string> validExpressions = {
        "[-inf]",
        "[-inf,]",
        "[  -923452345.2,            .234125e+56243   ,]",
        "[foo, bar, baz]",
        "[   foo   , bar, baz]",
        "[\n \"this is a string value\", -67.45e2311, \n\n 'another string',]",
        "[\"\"\"a multiline \n \n string as a list value\n\"\"\"]",
        "[@this is an asset references in a list@, foo, nan]",
        "[45.75, @@@an escaped asset reference @ in \\@@@ a list@@@]",
        "[foo,   bar,    baz,]",
        "[-0.56e-456,   foo,            0, 0.56, bar]",
        "[\n-inf]",
        "[\r\n-inf,]",
        "[\r  -923452345.2,            .234125e+56243   ,]",
        "[foo, bar, baz\n]",
        "[\nfoo,  \n bar,  \n  baz,]",
        "[-0.56e-456,   foo,            0, 0.56, bar\r\n]",
        "[\n  foo, \n  bar  ,\n  (\n    baz,\n\n    567.3e-45\n)\n\n\n]",
        "[\n  foo, \n  bar  ,\n  [\n    baz,\n\n    567.3e-45\n]\n\n\n]",
        "[\n  foo, \n  bar  ,\n  (\n    baz,\n\n"
        "    567.3e-45\n)  , [(4.5, -2)]    , \n\n\n]"
    };

    std::vector<std::string> invalidExpressions = {
        "[",
        "]",
        "[]",
        "[foo, bar, []]",
        "[foo, bar, ()]",
        "\n[foo, bar, baz]",
        "[varying]",
        "[foo, uniform]",
        "[foo, bar, \n  (7, config)]"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::ListValue,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::ListValue,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestDictionaryValues()
{
    std::vector<std::string> validExpressions = {
        "{}",
        "{\n\n\n\n}",
        "{\r\n\r\n\n\r}",
        "{  \n   \n  \n  }",
        "{\n\nfoo \"bar_key\"=[foo, bar, baz]\n}",
        "{\n\nfoo \"bar_key\"     =       [foo, bar, baz];\n}",
        "{\n\ndictionary \"bar_key\"     =       "
        "{float foo = bar; int bar=baz; newType baz=foo;};\n}",
        "{\n\ndictionary \"bar_key\"     =       "
        "{float foo = bar\nint bar=baz\nnewType baz=foo;};\n}",
        "{foo_   uniform  =  \"myValue\"}",
        "{foo_   _bar_234  =  \"\"\"my\n\nValue\"\"\"}",
        "{\n    dictionary foo={double key=-23.6e7}    ;"
        "\n\n foo_type baz_key   =  \"bazValue\"\n\n string add = "
        "(\"keyword_test\");\n\n}"
    };

    std::vector<std::string> invalidExpressions = {
        "{",
        "{foo=bar add=baz}",
        "{float foo=bar double add=baz}",
        "{dictionary foo=bar add=baz}",
        "{foo=2;bar=\"string\";baz=;}",
        "{foo=2;bar=\"string\";baz=foo;;;}",
        "no_open_brace = \"foo\";}",
        "{\n\ndictionary foo \"bar_key\"     =       [foo, bar, baz];\n}",
        "{\n\ndictionary \"bar_key\"     =       [foo, bar, baz];\n}",
        "{\n    dictionary foo={double key=-23.6e7}    ;"
        "\n\n foo_type baz_key   =  \"bazValue\"\n\n; add = "
        "(\"keyword_test\");\n\n}"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::DictionaryValue,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::DictionaryValue,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestMetadata()
{
    std::vector<std::string> validExpressions = {
        "()",
        "(    \t   \t\t\t     )",
        "(\r\n\n\n\r)",
        "(  \r    \n\n\n\n\n  \n\n   \n\n )",
        "(\"a comment\" ; \"another comment\"; \n \n \n )",
        "(\n\tfoo =    baz\n\ndoc     =  \"my doc\"   ;  "
        "\n\t\treorder foo   =  None\ndelete bar::baz    ="
        "    [\"myString\", (23.4, -inf, @assetRef\\path@)]\n\n)",
        "(doc=\"\"\"all list\n ops\n\"\"\";delete foo =[\"not an empty list\"]"
        " ;\nadd bar::baz= [@@@asset\\ref\\@@@@@@ , [ 123e45]];prepend "
        "foo_bar=[\"string1\",'string2'];\nappend foo2bar5=[(34, 45, 56)]"
        "\n\nreorder foo2::bar5=[-.9876];\n)",
        "(\n\npermission=foo;)",
        "(permission=foo::bar)",
        "(symmetryFunction=)",
        "(doc=\"test of symmetryFunction\"\nsymmetryFunction=foo::bar;)"
    };

    std::vector<std::string> invalidExpressions = {
        "(",
        ")",
        "(\r\n\n  \n doc=\"several metadata items not properly "
        "separated\"\nfoo=bar baz=foo)",
        "(doc=\"no close parenthesis\";",
    };

    std::vector<std::string> invalidRelationshipOnlyExpressions = {
        "(\t   doc=\"test of displayUnit\"\n    displayUnit = foo)",
        "(displayUnit         =\t\t  foo::bar::baz)"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::RelationshipMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::RelationshipMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidRelationshipOnlyExpressions.begin(),
        invalidRelationshipOnlyExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::RelationshipMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    // all of the above metadata should also be valid prim attribute metadata
    // but prim attributes also have displayUnit, so we check both the original
    // set and a few new ones
    std::vector<std::string> validAdditionalExpressions = {
        "(\t   doc=\"test of displayUnit\"\n    displayUnit = foo)",
        "(displayUnit         =\t\t  foo::bar::baz)"
    };

    std::vector<std::string> invalidAdditionalExpressions = {
        "(displayUnit=)",
        "(doc='An invalid display unit definition'\n    "
        "displayUnit=foo:bar:baz)"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::AttributeMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::AttributeMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::all_of(validAdditionalExpressions.begin(),
        validAdditionalExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::AttributeMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidAdditionalExpressions.begin(),
        invalidAdditionalExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::AttributeMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestPrimRelationship()
{
    std::vector<std::string> validExpressions = {
        "rel F00OO = [  <Foo.bar[targ.attr].boom>   ]",
        "varying \t\t rel foo.\tdefault\t\t = <.bar>",
        "rel myRel = <.bar> (doc=\"\"\"all list\n ops\n\"\"\";delete foo ="
        "[\"not an empty list\"] ;\nadd bar::baz= [@@@asset\\ref\\@@@@@@ , "
        "[ 123e45]];prepend foo_bar=[\"string1\",'string2'];\nappend foo2bar5="
        "[(34, 45, 56)]\n\nreorder foo2::bar5=[-.9876];\n)",
        "custom rel withoutAssignment   (\n\tfoo =    baz\n\ndoc     =  \"my "
        "doc\"   ;  \n\t\treorder foo   =  None\ndelete bar::baz    =    "
        "[\"myString\", (23.4, -inf, @assetRef\\path@)]\n\n)",
        "rel myRel [</World/Sphere>]"
    };

    std::vector<std::string> invalidExpressions = {
        "add",
        "rel",
        "custom varying rel foo.",
        "custom rel varying foo",
        "varying rel badMetadata=<.bar>(displayUnit=mm)",
        "prepend\t\t\tvarying            rel my:relationship=[<Foo.bar"
        "[targ.attr].boom>\n\n<Foo.bar[targ.attr].boom>,\r\n<Foo.bar"
        "[targ.attr].boom>,\n]",
        "custom rel withoutAssignment\n\n(\n\tfoo =    baz\n\ndoc     "
        "=  \"my doc\"   ;  \n\t\treorder foo   =  None\ndelete bar::baz    "
        "=    [\"myString\", (23.4, -inf, @assetRef\\path@)]\n\n)",

        // these are valid relations, but are parsed with the PropertySpec reduction
        "add rel add:delete",
        "delete custom rel foo_bar_23 = None",
        "prepend custom varying rel FOO_=[]",
        "append varying rel F00OO = <Foo.bar[targ.attr].boom>",
        "reorder rel foo =[<Foo.bar[targ.attr].boom>,  \n<.bar>]",
        "add   \t\t   \t  rel  \t\t  add:delete",
        "prepend\t\t\tvarying            rel my:relationship=[<Foo.bar"
        "[targ.attr].boom>,\n\n<Foo.bar[targ.attr].boom>,"
        "\r\n<Foo.bar[targ.attr].boom>,\n]",
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::RelationshipSpec,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::RelationshipSpec,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestPrimAttribute()
{
    std::vector<std::string> validExpressions = {
        "custom float[] add:delete:rootPrims",
        "float3 foo",
        "uniform half bar_\t\t=  \"not a valid value but validated later\"",
        "double3   [] foo.connect      =     None",
        "custom_type my:custom:type:instance:_:add.\t\ttimeSamples = "
        "{\n\n 24.567e23   : @foo\\asset\\ref@  ,\t\n 2: \"string value\","
        "-77    :(\"tuple_value\", -65.8),\n45.65    :\t None\n}",
        "uniform int3 foo          =     (7, 6,\n 2)(displayUnit=mm)",
        "config int3 foo\t= (\n7,2,\n\n5)(doc=\"\"\"all list\n ops\n\"\"\";"
        "delete foo =[\"not an empty list\"] ;\nadd bar::baz= [@@@asset\\ref\\@@@@@@ , "
        "[ 123e45]];prepend foo_bar=[\"string1\",'string2'];\nappend foo2bar5="
        "[(34, 45, 56)]\n\nreorder foo2::bar5=[-.9876];\n)"
    };

    std::vector<std::string> invalidExpressions = {
        "noTypeAttribute",
        "custom\nfloat[] add:delete:rootPrims",
        "double3   [].connect",
        "config int3 foo\t= (\n7,2,\n\n5)\n(doc=\"\"\"all list\n ops\n\"\"\";"
        "delete foo =[\"not an empty list\"] \n;\nadd bar::baz= [@@@asset\\ref"
        "\\@@@@@@ , [ 123e45]];prepend foo_bar=[\"string1\",'string2'];\n"
        "append foo2bar5=[(34, 45, 56)]\n\nreorder foo2::bar5=[-.9876]\n;\n)",

        // these are valid attribute specs, but get parsed with
        // the PropertySpec reduction
        "add int2 foo:bar.connect=[]",
        "delete myType _F00_.connect\t=[\n\n\n\n]",
        "prepend foo[] __:connect:_.connect=<Foo.bar[targ.attr].boom>",
        "append string bar__\t.\t  connect    \t\t =[<Foo.bar[targ.attr].boom>]",
        "reorder foo::bar[] _baz00.connect=[<Foo.bar[targ.attr].boom>,"
        "\n<Foo.bar[targ.attr].boom>,]",
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::AttributeSpec,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::AttributeSpec,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestPrimProperty()
{
    // a prim property is either a relationship or attribute
    // so we take all the valid relationships and attributes
    // we had in the individual tests, combine them and make
    // sure the rule parses (or doesn't for the invalid ones)
    std::vector<std::string> validExpressions = {
        "add rel add:delete",
        "delete custom rel foo_bar_23 = None",
        "prepend custom varying rel FOO_=[]",
        "append varying rel F00OO = <Foo.bar[targ.attr].boom>",
        "reorder rel foo =[<Foo.bar[targ.attr].boom>,\n<.bar>]",
        "add   \t\t   \t  rel  \t\t  add:delete",
        "prepend\t\t\tvarying            rel my:relationship=[<Foo.bar[targ.attr]."
        "boom>,\n\n<Foo.bar[targ.attr].boom>,\r\n<Foo.bar[targ.attr].boom>,\n]",
        "rel F00OO = [  <Foo.bar[targ.attr].boom>   ]",
        "delete myType _F00_.connect\t=[\n\n\n\n]",
        "prepend foo[] __:connect:_.connect=<Foo.bar[targ.attr].boom>",
        "append string bar__\t.\t  connect    \t\t =[<Foo.bar[targ.attr].boom>]",
        "reorder foo::bar[] _baz00.connect=[<Foo.bar[targ.attr].boom>,"
        "\n<Foo.bar[targ.attr].boom>,]",
        "varying \t\t rel foo.\tdefault\t\t = <.bar>",
        "rel myRel = <.bar> (doc=\"\"\"all list\n ops\n\"\"\";delete foo "
        "=[\"not an empty list\"] ;\nadd bar::baz= [@@@asset\\ref\\@@@@@@ , "
        "[ 123e45]];prepend foo_bar=[\"string1\",'string2'];\nappend "
        "foo2bar5=[(34, 45, 56)]\n\nreorder foo2::bar5=[-.9876];\n)",
        "custom rel withoutAssignment(\n\tfoo =    baz\n\ndoc     =  \"my "
        "doc\"   ;  \n\t\treorder foo   =  None\ndelete bar::baz    =    "
        "[\"myString\", (23.4, -inf, @assetRef\\path@)]\n\n)",
        "custom float[] add:delete:rootPrims",
        "float3 foo",
        "uniform half bar_\t\t=  \"not a valid value but validated later\"",
        "double3   [] foo.connect      =     None",
        "add int2 foo:bar.connect=[]",
        "delete myType _F00_.connect\t=[\n\n\n\n]",
        "prepend foo[] __:connect:_.connect=<Foo.bar[targ.attr].boom>",
        "append string bar__\t.\t  connect    \t\t ="
        "[<Foo.bar[targ.attr].boom>]",
        "reorder foo::bar[] _baz00.connect=[<Foo.bar[targ.attr].boom>,"
        "\n<Foo.bar[targ.attr].boom>,]",
        "custom_type my:custom:type:instance:_:add.\t\ttimeSamples = "
        "{\n\n 24.567e23   : @foo\\asset\\ref@  ,\t\n 2: \"string value\","
        "-77    :(\"tuple_value\", -65.8),\n45.65    :\t None\n}",
        "uniform int3 foo          =     (7, 6,\n 2)(displayUnit=mm)",
        "config int3 foo\t= (\n7,2,\n\n5)(doc=\"\"\"all list\n ops\n\"\"\";"
        "delete foo =[\"not an empty list\"] ;\nadd bar::baz= [@@@asset\\ref\\"
        "@@@@@@ , [ 123e45]];prepend foo_bar=[\"string1\",'string2'];\nappend "
        "foo2bar5=[(34, 45, 56)]\n\nreorder foo2::bar5=[-.9876];\n)"
    };

    std::vector<std::string> invalidExpressions = {
        "add",
        "rel",
        "custom varying rel foo.",
        "custom rel varying foo",
        "prepend\t\t\tvarying            rel my:relationship="
        "[<Foo.bar[targ.attr].boom>\n\n<Foo.bar[targ.attr].boom>,"
        "\r\n<Foo.bar[targ.attr].boom>,\n]",
        "noTypeAttribute",
        "custom\nfloat[] add:delete:rootPrims",
        "double3   [].connect",
        "config int3 foo\t= (\n7,2,\n\n5)\n(doc=\"\"\"all list\n "
        "ops\n\"\"\";delete foo =[\"not an empty list\"] \n;\nadd bar::"
        "baz= [@@@asset\\ref\\@@@@@@ , [ 123e45]];prepend foo_bar=[\""
        "string1\",'string2'];\nappend foo2bar5=[(34, 45, 56)]\n\n"
        "reorder foo2::bar5=[-.9876]\n;\n)",
        "custom rel withoutAssignment\n\n(\n\tfoo =    baz\n\ndoc     "
        "=  \"my doc\"   ;  \n\t\treorder foo   =  None\ndelete bar::"
        "baz    =    [\"myString\", (23.4, -inf, @assetRef\\path@)]\n\n)"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PropertySpec,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PropertySpec,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestPrimMetadata()
{
    std::vector<std::string> validExpressions = {
        "(\n\n\n)",
        "()",
        "(\"one piece of prim metadata\"    ;\n\n)",
        "('foo'; foo=\"\"\"bar\n\n\"\"\"   ; foo   =\t\t34.64\n\nadd __  =  "
        "None;delete foo\t=[-89.723   , \"foo_string\"]    ;prepend foo_bar="
        "[bar\t, baz::foo]\nappend _056245 =    \t[@c:\\asset\\ref\\path@  "
        ",];\n\n\n\n\nreorder _my__f00  = None;)",
        "(    doc=\"\"\"my_Documentation \n\n string\n\"\"\")",
        "(kind\t   =  '____'  ;   \n\n)",
        "(kind = \"class\"\npermission=__foo)",
        "(\t\tpayload\t\t = \tNone\n\npayload  =[]; payload=[  \n  \n  "
        "\n];payload=[\n\n@./asset1@  , @@@escaped_asset\\@@@_1@@@   ,\n\n])",
        "(payload=[@my_layer_ref@</my/path/ref>  (offset =  45\n  scale = 2.6)"
        "  ,  <just/a/path/ref>])",
        "(add payload=\t\t[@my_layer_ref@</my/path/ref>  (offset =  45\n  "
        "scale = 2.6)  ,  <just/a/path/ref>] ;\n\n  delete payload=\t\t"
        "[@my_layer_ref@</my/path/ref>  (offset =  45\n  scale = 2.6)  ,  "
        "<just/a/path/ref>])",
        "(  prepend payload=\t\t[@my_layer_ref@</my/path/ref>  (offset =  "
        "45\n  scale = 2.6)  ,  <just/a/path/ref>] ;\n\n  append payload="
        "\t\t[@my_layer_ref@</my/path/ref>  (offset =  45\n  scale = 2.6)  "
        ",  <just/a/path/ref>])",
        "(reorder payload=\t\t[<just/a/path/ref>   \t,\n@my_layer_ref@"
        "</my/path/ref>  (offset =  45\n  scale = 2.6)  ,  ])",
        "(references =  [] ;  \n  references = None)",
        "(references=[@@@my_lay@@er_ref@@@</my/path/ref>  (offset =  "
        "45\n  scale = 2.6)])",
        "(references=[@@@my_lay@@er_ref@@@</my/path/ref>  ()])",
        "(references=[@@@my_lay@@er_ref@@@</my/path/ref>  (  \n\n)])",
        "(references=[@@@my_lay@@er_ref@@@</my/path/ref>  (  customData "
        "= { float \"my_key\" = 457.0e23})])",
        "(kind = \"custom_kind_01\"\nsymmetryFunction=\nsymmetryFunction="
        "my_symmetry_function;prefixSubstitutions = {};suffixSubstitutions = "
        "{'key1':'my_value', 'key2':'68.32'})",
        "(explicit=foo_valu3\n'some string metadata')",
        "(inherits =  []\n\n inherits=[];inherits=[\n\n\n  ]\ninherits=[ "
        "</my/prim_path/path>    ,\n <my/other/prim/path>]  \n)",
        "(add inherits = [<this/new/prim/path> ];delete inherits=[]\n\nappend "
        "inherits=   [</prim/path/p1.property>, </prim/path/p2>   ];  prepend "
        "inherits = [</this/other/prim/path>  ]; reorder inherits = [</prim/"
        "path/p2>, </this/other/prim/path> ])",
        "(specializes = \t[]\nspecializes=[    \n\n\t\t  ]; specializes=None"
        "\n\nspecializes = [</prim/path/p1.property>, </prim/path/p2>   ,])",
        "(add specializes = []; \n\n delete specializes = None\n\nprepend "
        "specializes = [\n];append specializes = [</prim/path/p1.property>, "
        "\n</prim/path/p2>]\n\n\n  reorder specializes = [</another/prim/"
        "path.with_property>])",
        "(  relocates = {}; relocates = {\n\n   }  ;relocates={\n\n</prim/"
        "path/p1.property>  :   </another/prim/path.with_property>, \n\n</"
        "another/prim/path.with_property> : <prim/path/p1.property>})",
        "(  variants = {\n\n  float3[] add = (8.3, 0.5,\n  6.7)\n\nstring "
        "shadingVariant = \"red\";})",
        "(\n\tvariants = {\n\t\tstring shadingVariant = \"green\"\n\t}\n\t"
        "prepend variantSets = \"shadingVariant\"\n)",
        "(variantSets=[\"shadingVariant1\",\"shadingVariant2\",\"shadingVariant3"
        "\",\"shadingVariant4\",\"shadingVariant5\"])",
        "(\tvariantSets = [\"shadingVariant1\"  , \"shadingVariant2\",\n"
        "\"shadingVariant3\", 'shadingVariant4', \"\"\"shadingVariant5\"\"\"\n\n])",
        "(add variantSets=[\n\"shadingVariant\"]\ndelete variantSets="
        "[\"shadingVariant\"]; prepend variantSets=\"___\"\n append variantSets="
        "[\"56\", \"anotherVariant\"]; reorder variantSets    =\t[\n\n"
        "\"anotherVariant\",\n\n\"variant2\",\n])"
    };

    std::vector<std::string> invalidExpressions = {
        "(",
        "(kind=   =  '____'  ;   \n\n)",
        "(kind-class)",
        "(payload=[@my_layer_ref@</my/path/ref>  (offset =  45\n  scale = 2.6)"
        "  ;  <just/a/path/ref>])",
        "(references=[@@@my_lay@@er_ref@@@</my/path/ref>  (  customData = "
        "{ \"my_key\" = 457.0e23})])",
        "(kind = \"custom_kind_01\"\nsymmetryFunction=\nsymmetryFunction="
        "my_symmetry_function;displayUnit=mm\n\n\n;permission=\tfoo)",
        "(kind = \"custom_kind_01\"\nsymmetryFunction=\nsymmetryFunction="
        "my_symmetry_function;prefixSubstitutions = {};suffixSubstitutions "
        "= {'key1'='my_value', 'key2'='68.32'})",
        "(variantSets=[])",
        "(variantSets=[56])",
        "(variantSets=[\"variant1\", 56])",
        "(\tvariantSets = [\"shadingVariant1\"  , \"shadingVariant2\"\n\""
        "shadingVariant3\", 'shadingVariant4', \"\"\"shadingVariant5\"\"\"\n\n])"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PrimMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PrimMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestVariantSetStatement()
{
    std::vector<std::string> validVariantStatements = {
        "\"blue\" {}",
        "\"blue\" {\n\t}"
    };

    std::vector<std::string> invalidVariantStatements = {
        "\"blue {}",
        "\"blue\" {"
    };

    TF_AXIOM(std::all_of(validVariantStatements.begin(),
        validVariantStatements.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::VariantStatement,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidVariantStatements.begin(),
        invalidVariantStatements.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::VariantStatement,
                PEGTL_NS::eof>>(expression);
        }));

    std::vector<std::string> validExpressions = {
        "variantSet \"shadingVariant\" = { \"blue\" {}}",
        "variantSet 'shadingVariant' = {'blue'{}'green'{}}",
        "variantSet \"abc\" = { \"inlineproperties\" { int x = 5; int y = 7; } }",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n}",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n\n\t\"green\" "
        "{\n\t}\n\n\t\"red\" {\n\t}\n}",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n\n\t\"green\" "
        "{\n\t}\n\n\t\"red\" {\n\t\tvariantSet \"subShadingVariant\" =\n\t\t"
        "{\n\t\t\t\"scarlet\" {\n\t\t\t}\n\t\t}\t\n}\n}",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n\n\t\"green\" "
        "{\n\t}\n\n\t\"red\" {\n\t\tover \"world\"\n\t\t{}\n\t\n}\n}",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n\n\t\"green\" "
        "{\n\t}\n\n\t\"red\" {\n\t\tover \"world\"\n\t\t{\n\t\t\tcolor3f[] "
        "primvars:displayColor = [(1, 0, 0)]\n\t\t}\n\t\n}\n}"
    };

    std::vector<std::string> invalidExpressions = {
        "variantSet",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n\n\t\"green\" "
        "{\n\t}\n\n\t\"red\" {\n\t\tvariantSet \"subShadingVariant\" =\n\t\t"
        "{\n\t\t\t\"scarlet\" {\n\t\t\t}\n\t\t}\t\n}\n}\n",
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::VariantSetStatement,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::VariantSetStatement,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestPrim()
{
    std::vector<std::string> validExpressions = {
        "custom float[] add:delete:rootPrims\n",
        "float3 foo;",
        "uniform half bar_\t\t=  \"not a valid value but validated later\"\n",
        "double3   [] foo.connect      =     None;",
        "add int2 foo:bar.connect=[]\n",
        "delete myType _F00_.connect\t=[\n\n\n\n];",
        "prepend foo[] __:connect:_.connect=<Foo.bar[targ.attr].boom>\n",
        "append string bar__\t.\t  connect    \t\t =[<Foo.bar"
        "[targ.attr].boom>];",
        "reorder foo::bar[] _baz00.connect=[<Foo.bar[targ.attr].boom>,\n"
        "<Foo.bar[targ.attr].boom>,]\n",
        "custom_type my:custom:type:instance:_:add.\t\ttimeSamples = {\n\n "
        "24.567e23   : @foo\\asset\\ref@  ,\t\n 2: \"string value\",-77    "
        ":(\"tuple_value\", -65.8),\n45.65    :\t None\n};",
        "uniform int3 foo          =     (7, 6,\n 2)(displayUnit=mm)\n",
        "config int3 foo\t= (\n7,2,\n\n5)(doc=\"\"\"all list\n ops\n\"\"\";"
        "delete foo =[\"not an empty list\"] \t;\nadd bar::baz= [@@@asset\\ref"
        "\\@@@@@@ , [ 123e45]];prepend foo_bar=[\"string1\",'string2'];\n"
        "append foo2bar5=[(34, 45, 56)]\n\nreorder foo2::bar5=[-.9876];\n)\n",
        "add rel add:delete\n",
        "delete custom rel foo_bar_23 = None;",
        "prepend custom varying rel FOO_=[]\n",
        "append varying rel F00OO = <Foo.bar[targ.attr].boom>;",
        "reorder rel foo =[<Foo.bar[targ.attr].boom>,\n<.bar>]\n",
        "add   \t\t   \t  rel  \t\t  add:delete;",
        "prepend\t\t\tvarying            rel my:relationship=[<Foo.bar"
        "[targ.attr].boom>,\n\n<Foo.bar[targ.attr].boom>,\r\n<Foo.bar"
        "[targ.attr].boom>,\n]\n",
        "rel F00OO = [  <Foo.bar[targ.attr].boom>   ];",
        "varying \t\t rel foo.\tdefault\t\t = <.bar>;",
        "rel myRel = <.bar> (doc=\"\"\"all list\n ops\n\"\"\";delete foo ="
        "[\"not an empty list\"] ;\nadd bar::baz= [@@@asset\\ref\\@@@@@@ , "
        "[ 123e45]];prepend foo_bar=[\"string1\",'string2'];\nappend foo2bar5="
        "[(34, 45, 56)]\n\nreorder foo2::bar5=[-.9876];\n)\n",
        "custom rel withoutAssignment(\n\tfoo =    baz\n\ndoc     =  \"my doc\""
        "   ;  \n\t\treorder foo   =  None\ndelete bar::baz    =    [\"myString\""
        ", (23.4, -inf, @assetRef\\path@)]\n\n);",
        "variantSet \"shadingVariant\" = { \"blue\" {}}\n",
        "variantSet \"shadingVariant\" = {}\n",
        "variantSet 'shadingVaraint' = {'blue'{}'green'{}}\n",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n}\n",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n\n\t\"green\" "
        "{\n\t}\n\n\t\"red\" {\n\t}\n}\n",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n\n\t\"green\" "
        "{\n\t}\n\n\t\"red\" {\n\t\tvariantSet \"subShadingVariant\" =\n\t\t"
        "{\n\t\t\t\"scarlet\" {\n\t\t\t}\n\t\t}\t\n}\n}\n",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n\n\t\"green\" "
        "{\n\t}\n\n\t\"red\" {\n\t\tover \"world\"\n\t\t{}\n\t\n}\n}\n",
        "variantSet \"shadingVariant\" = {\n\t\"blue\" {\n\t}\n\n\t\"green\" "
        "{\n\t}\n\n\t\"red\" {\n\t\tover \"world\"\n\t\t{\n\t\t\tcolor3f[] "
        "primvars:displayColor = [(1, 0, 0)]\n\t\t}\n\t\n}\n}\n",
        "reorder nameChildren = [\"foo\", \"bar\"];",
        "reorder nameChildren = ['foo', 'bar',\n 'baz']\n",
        "reorder properties = ['prop1', \"_prop2\"];"
    };

    std::vector<std::string> invalidExpressions = {
        "custom float[] add:delete:rootPrims",
        "float3 foo",
        "variantSet \"shadingVariant\" = { \"blue\" {}};",
        "config int3 foo\t= (\n7,2,\n\n5)\n(doc=\"\"\"all list\n ops\n\"\"\";"
        "delete foo =[\"not an empty list\"] \n;\nadd bar::baz= [@@@asset\\ref"
        "\\@@@@@@ , [ 123e45]];prepend foo_bar=[\"string1\",'string2'];\n"
        "append foo2bar5=[(34, 45, 56)]\n\nreorder foo2::bar5=[-.9876]\n;\n);",
        "custom rel withoutAssignment\n\n(\n\tfoo =    baz\n\ndoc     =  "
        "\"my doc\"   ;  \n\t\treorder foo   =  None\ndelete bar::baz    "
        "=    [\"myString\", (23.4, -inf, @assetRef\\path@)]\n\n);"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PrimItem,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PrimItem,
                PEGTL_NS::eof>>(expression);
        }));

    std::vector<std::string> validPrimStatements = {
        "def \"foo\" {}",
        "def F00 \"foo\" {}",
        "class \"foo\" {}",
        "class _ \"foo\" {}",
        "over \"foo\" {}",
        "over __ \"B_A_5\" {}",
        "def Xform \"hello\"\n{\n\tdef Sphere \"world\"\n\t{\n\t}\n}",
        // The comment counts as a newline
        "def \"abc\"(){ /*hello*/ def \"abc\" {}\n}",
        // The semicolon ends the reorder statement
        "def \"cat\" (){\nreorder nameChildren = [\"dog\"]; /*comment*/ def \"dog\" {}\n}",
        // A single line comment is sufficient to separate statements
        "def \"cat\" (){reorder nameChildren = [\"dog\"] #comment\n reorder nameChildren = [\"whale\"]\n}",
        "def \"cat\" (){reorder nameChildren = [\"dog\"] #comment\n def \"dog\" {}\n}",
        "def \"cat\" (){#comment\n def \"dog\" {} //comment\n def \"whale\" {}\n}"
    };

    std::vector<std::string> invalidPrimStatements = {
        "def",
        "def foo {}",
        "def Xform \"hello\"\n{\n\tdef Sphere \"world\"\n\t{\n\t}}",
        "reorder rootPrims = ['foo', '_', \"\"\"B_A_5\"\"\"]",
        // Inline comments do not end statement
        "def \"abc\"(){ def \"abc\" {} /*comment*/ }",
        // Inline comments do not separate statements
        "def \"cat\" (){\nreorder nameChildren = [\"dog\"] /*comment*/ def \"dog\" {}\n}",
        "def \"cat\" (){\ndef \"dog\" {} /*comment*/ def \"whale\" {}\n}"
    };

    TF_AXIOM(std::all_of(validPrimStatements.begin(),
        validPrimStatements.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PrimSpec,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidPrimStatements.begin(),
        invalidPrimStatements.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::PrimSpec,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestLayerMetadata()
{
    std::vector<std::string> validExpressions = {
        "()",
        "(\n)",
        "(\"layerMetadata\"; foo = (3, 2, 1); doc=\"\"\"some documentation "
        "for\n layer \n metadata\n\"\"\"\n)",
        "(add foo = None\ndelete foo = [3, \"5 in a string\", bar,];prepend "
        "_=[(1, 2, \"3\")]\nappend F00 = None;\n\n\nreorder "
        "foo =\t[bar, baz, foo::bar])",
        "(subLayers = [])",
        "(subLayers = [];subLayers=[\n\n\n])",
        "(subLayers = [];subLayers=[\n\n\n]\nsubLayers\t=\t"
        "[\n\n@an/asset/ref@])",
        "(subLayers = [];subLayers=[\n\n\n]\nsubLayers\t=\t"
        "[\n\n@an/asset/ref@];subLayers=[@another/asset/ref@"
        "(offset = 6;\n scale=4.5e0)])",
        "(subLayers=[]#comment\nappend foo = None)"
    };

    std::vector<std::string> invalidExpressions = {
        "(",
        "\n\n(\n\n",
        "subLayers=[@@@an/asset/path@@@(offset=\"myOffset\")]",
        "(add foo = None\ndelete foo = [3, \"5 in a string\", bar,];"
        "prepend _=[(1, 2, \"3\")\nappend = None;\n\n\nreorder "
        "foo =\t[bar, baz, foo::bar])",
        "(subLayers = [];subLayers=[\n\n\n]\nsubLayers\t=\t[\n\n@an/"
        "asset/ref@];subLayers=[@another/asset/ref@[offset = 6,\n scale=4.5e0]])"
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::LayerMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::LayerMetadata,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestLayer()
{
    std::vector<std::string> validExpressions = {
        "#sdf 1.4.32",
        "#sdf 1.4.32\n\ndef Xform \"hello\"\n{\n\tdef Sphere \"world\"\n"
        "\t{\n\t}\n}",
        "#sdf 1.4.32\n\nover \"refSphere\" (\n\tprepend references = @./"
        "HelloWorld.usda@\n)\n{\n}",
        "#sdf 1.4.32\n(\n    doc = \"\"\"Generated from Composed Stage "
        "of root layer RefExample.usda\n\"\"\"\n)\n\ndef Xform \"refSphere"
        "\"\n{\n    double3 xformOp:translate = (4, 5, 6) \n    uniform "
        "token[] xformOpOrder = []\n\n    def Sphere \"world\"\n    {\n    "
        "    float3[] extent = [(-2, -2, -2), (2, 2, 2)]\n        color3f[] "
        "primvars:displayColor = [(0, 0, 1)] \n        double radius = 2\n   "
        " }\n}\n\ndef Xform \"refSphere2\"\n{\n   double3 xformOp:translate ="
        " (4, 5, 6)\n    uniform token[] xformOpOrder = [\"xformOp:translate\""
        "]\n\n    def Sphere \"world\"\n    {\n       float3[] extent = [(-2, "
        "-2, -2), (2, 2, 2)]\n        color3f[] primvars:displayColor = [(1, "
        "0, 0)]\n        double radius = 2\n    }\n}",
        "#sdf 1.4.32\n(\n    doc = \"\"\"Generated from Composed Stage "
        "of root layer RefExample.usda\n\"\"\"\n)\n\ndef Xform \"refSphere"
        "\"\n{\n    double3 xformOp:translate = (4, 5, 6) \n    uniform "
        "token[] xformOpOrder = [\"xformOp:translate\"]\n}\n\ndef Xform "
        "\"refSphere2\"\n{\n   double3 xformOp:translate = (4, 5, 6)\n    "
        "uniform token[] xformOpOrder = [\"xformOp:translate\"]\n\n}",
        "#sdf 1.4.32\nreorder rootPrims = ['foo', '_', \"\"\"B_A_5\"\"\"]"
    };

    std::vector<std::string> invalidExpressions = {
        "def Xform \"hello\"\n{\n\tdef Sphere \"world\"\n\t{\n\t}\n}",
        "usda 1.0\n\ndef Xform \"hello\"\n{\n\tdef Sphere "
        "\"world\"\n\t{\n\t}\n}",
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::LayerSpec,
                PEGTL_NS::eof>>(expression);
        }));

    TF_AXIOM(std::none_of(invalidExpressions.begin(),
        invalidExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::LayerSpec,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

bool
TestSplines()
{
    std::vector<std::string> validExpressions = {
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        bezier,\n   }\n}\n\n",
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        pre: linear,\n   }\n}\n\n",
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        post: sloped(0.57),\n   }\n}\n\n",
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        1: 5; pre ws(0); post held,\n   }\n}\n\n",
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        1: 5; pre ws(0, 0); post held,\n   }\n}\n\n",
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        loop: (15, 25, 0, 2, 11.7),\n   }\n}\n\n",
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        15: 8.18; post curve ws(2.49, 1.17); { string comment = \"climb!\" },\n   }\n}\n\n",
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        20: 14.72; pre ws(3.77, -1.4); post curve ws(1.1, -1.4),\n   }\n}\n\n",
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        7: 5.5 & 7.21,\n   }\n}\n\n",
        "#sdf 1.4.32\n\ndef Xform \"Prim1\"\n{\n    double xformOp:rotateX.spline = {\n        7: 5.5 & 7.21; post held,\n   }\n}\n\n",
    };

    TF_AXIOM(std::all_of(validExpressions.begin(),
        validExpressions.end(), [](const std::string& expression)
        {
            return DoParse<PEGTL_NS::must<
                Sdf_TextFileFormatParser::LayerSpec,
                PEGTL_NS::eof>>(expression);
        }));

    return true;
}

int
main()
{
    bool valid = TestDigits() &&
        TestIdentifiers() &&
        TestStrings() &&
        TestAssetRefs() &&
        TestPathRefs() &&
        TestTupleValues() &&
        TestListValues() &&
        TestDictionaryValues() &&
        TestMetadata() &&
        TestPrimRelationship() &&
        TestPrimAttribute() &&
        TestPrimProperty() &&
        TestPrimMetadata() &&
        TestVariantSetStatement() &&
        TestPrim() &&
        TestLayerMetadata() &&
        TestLayer() &&
        TestSplines();

    return valid ? 0 : -1;
}
