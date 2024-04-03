//
// Copyright 2017 Pixar
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
#include <iostream>
#include <cassert>
#include <cstdio>

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

void testPaths(char const *paths[], bool expectEmpty) {

    while(*paths) {

        printf("testing: %s\n", *paths);

        SdfPath resultPath(*paths);
        
        // Report parse errors.
        if (resultPath.IsEmpty() != expectEmpty) {
            TF_FATAL_ERROR("Expected <%s> %sto parse",
                           *paths, expectEmpty ? "not " : "");
        }

        // Report mismatches between original string and the string
        // representation of the parsed path.  We allow whitespace to
        // be different.
        if (!resultPath.IsEmpty()) {
            std::string s = resultPath.GetAsString();
            std::string expected = TfStringReplace(*paths, " ", "");
            if (s != expected) {
                TF_FATAL_ERROR("mismatch: %s -> %s", *paths, expected.c_str());
            }
            char const *debugText = Sdf_PathGetDebuggerPathText(resultPath);
            if (std::string(debugText) != expected) {
                TF_FATAL_ERROR("debug mismatch: %s -> %s",
                               debugText, expected.c_str());
            }
        }
        ++paths;
    }
}

int main()
{

    char const *good[] = {
        ".",
        "/",
        "Foo",
        "/Foo",
        ".bar",
        "/Foo/Bar.baz",
        "/Foo/Bar.baz:argle:bargle",
        "/_Foo/_Bar._baz",
        "/_Foo/_Bar._baz:_argle:_bargle",
        "Foo/Bar",
        "Foo.bar",
        "Foo.bar:baz",
        "Foo/Bar.bar",
        "Foo/Bar.bar:baz",
        "/Some/Kinda/Long/Path/Just/To/Make/Sure",
        "Some/Kinda/Long/Path/Just/To/Make/Sure.property",
        "Some/Kinda/Long/Path/Just/To/Make/Sure.property:bar:baz",
        "../Some/Kinda/Long/Path/Just/To/Make/Sure",
        "../../Some/Kinda/Long/Path/Just/To/Make/Sure.property",
        "../../Some/Kinda/Long/Path/Just/To/Make/Sure.property:bar:baz",
        "/Foo/Bar.baz[targ].boom",
        "/Foo/Bar.baz:argle[targ].boom:bargle",
        "Foo.bar[targ].boom",
        "Foo.bar:argle[targ].boom:bargle",
        ".bar[targ].boom",
        ".bar:argle[targ].boom:bargle",
        "Foo.bar[targ.attr].boom",
        "Foo.bar:argle[targ.attr:baz].boom:bargle",
        "/a.rel[/b.rel[/c.rel[/d.rel[/e.a1].a2].a3].a4]",
        "/a.rel[/b.rel[/c.rel[/d.a1].a2].a3]",
        "/a.rel[/b.rel[/c.a2].a3]",
        "/a.rel[/b.rel[/c.rel[/d.rel[/e.a1].a2].a3].a4].a0",
        "/a.rel[/b.rel[/c.rel[/d.a1].a2].a3].a0",
        "/a.rel[/b.rel[/c.a2].a3].a0",
        "../../.radius",
        "../../.radius:bar:baz",
        "../..",
        "foo{a=x}",
        "/foo{a=x}",
        "../foo{a=x}",
        "foo{a=x}.prop",
        "foo{a=x}.prop:bar:baz",
        "foo{a=1}",
        "foo{ a = x }",
        "foo{a=x}{b=y}",
        "foo {a=x} {b=y} ",
        "foo { a = x} { b = y } ",
        "foo{a=x}{b=y}.prop",
        "foo{a=x}{b=y}.prop:bar:baz",
        "foo{ a = x }{b=y}",
        "foo{a=x}{ b = y }",
        "foo{ a = x }{ b = y }",
        "foo{a=x}{b=y}{c=z}",
        "foo{a=x}{b=y}{c=z}.prop",
        "foo{a=x}{b=y}{c=z}.prop:bar:baz",
        "foo{a=x}bar",
        "/foo{a=x}bar",
        "../foo{a=x}bar",
        "foo{a=x}bar.prop",
        "foo{a=x}bar.prop:bar:baz",
        "foo{a=x}bar{b=y}",
        "foo{a=x}bar{b=y}.prop",
        "foo{a=x}bar{b=y}.prop:bar:baz",
        "foo{a=x}{b=y}bar{c=z}{d=w}",
        "foo{a=x}bar{b=y}blah{c=z}",
        "foo{a=x}bar{b=y}blah{c=z}.prop",
        "foo{a=x}bar{b=y}blah{c=z}.prop:bar:baz",
        "foo{a=x}bar/blah",
        "foo{a=x}bar/blah.prop",
        "foo{a=x}bar/blah.prop:bar:baz",
        "foo{a=x}bar/blah{c=z}",
        "foo{a=x}bar/blah{c=z}.prop",
        "foo{a=x}bar/blah{c=z}.prop:bar:baz",
        "foo{a=x}bar/blah/baz{c=z}",
        "foo{a=x}bar/blah{c=z}baz/Burma/Shave",
        "foo{a=x}bar/blah{c=z}baz/Burma.Shave",
        "foo{a=x}bar/blah{c=z}baz/Burma.Shave:argle:bargle",
        "foo{a=.x}",
        "foo{a=1}",
        "foo{a=|}",
        "foo{a=-}",
        "foo{a=_}",
        "foo{a=.1}",
        "foo{a=.|}",
        "foo{a=.-}",
        "foo{a=._}",
        "foo{a=|-_|-_}",
        "foo{a=.|-_|-_}",
        "foo.expression",
        "foo.expression.expression",
        "foo.expression.mapper[/A.b]",
        "foo.mapper",
        "foo.mapper.expression",
        "foo.mapper.mapper[/A.b]",
        "/root_utf8_umlaute_ß_3",
        NULL
    };

    char const *bad[] = {
        "DD/DDD.&ddf$",
        "DD[]/DDD",
        "DD[]/DDD.bar",
        "foo.prop/bar",
        "/foo.prop/bar.blah",
        "/foo.prop/bar.blah",
        "/foo//bar",
        "/foo/.bar",
        "/foo..bar",
        "/foo.bar.baz",
        "/.foo",
        "/Foo.:bar",
        "/Foo.bar:",
        "/Foo.:bar:",
        "/Foo.:bar:baz",
        "/Foo.bar:baz:",
        "/Foo.:bar:baz:",
        "/Foo.bar::baz",
        "/Foo.bar:0",
        "</foo.bar",
        "</Foo/Bar/>",
        "/Foo:Bar",
        "/Foo/Bar/",
        "/Foo.bar[targ]/Bar",
        "/Foo.bar[targ].foo.foo",
        "/Foo.bar[targ].foo[targ].foo",
        "../../",
        ".rel[targ][targ].attr",
        ".attr[1, 2, 3].attr",
        "/TestScene/sphere0.fakepoints[&1 &2 &3]",
        "/  Foo",
        "/	Foo",  // <- tab
        "  Foo",
        "	Foo",  // <- tab
        "/foo.b ar",
        "/foo. bar",
        "Foo.bar[targ].attr[//..]",
        "foo{}",
        "foo{,}",
        "foo{a=x,}",
        "foo{a=x}{}",
        "foo{1=x}",
        "foo{,a=x}",
        "foo{}{a=x}",
        "foo{,a=x,}",
        "foo{}{a=x}{}",
        "foo{a=x}/bar",
        "foo{a=x}.prop/bar",
        "foo{a=x}.prop{b=y}",
        "foo{a=x.}",
        "foo{a=.x.}",
        "foo{a=:}",
        "foo{a=x:}",
        "Foo.attr.mapper[/Bar].arg:baz",
        "/foo😀", // valid UTF-8 string that isn't an identifier
        "/foo/bar/_∂baz", // valid UTF-8 string that isn't an identifier
        NULL
    };

    testPaths(good, 0);

    printf("Testing bad paths: errors expected\n");
    testPaths(bad, 1);

    printf("Done expecting errors\n");

    printf("Test PASSED\n");

    return 0;

}

