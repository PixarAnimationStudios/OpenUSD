
#include <iostream>
#include <cassert>
#include <cstdio>
#include "pxr/base/tf/diagnostic.h"
#include "pxr/usd/sdf/pathParser.h"

const char* path;

void testPaths(char const *paths[], int expect) {

    Sdf_PathParserContext context;

    // Initialize the scanner, allowing it to be reentrant.
    pathYylex_init(&context.scanner);

    while(*paths) {

        printf("testing: %s\n", *paths);
        pathYy_scan_string(*paths, context.scanner);
        int result = pathYyparse(&context);

        // Report parse errors.
        if (result != expect) {
            fprintf(stderr, "parse error: %s in %s\n",
                    context.errStr.c_str(), *paths);
            TF_AXIOM(result == expect);
        }

        // Report mismatches between original string and the string
        // representation of the parsed path.  We allow whitespace to
        // be different.
        if (result == 0) {
            std::string s = context.node->GetPathToken().GetString();
            if (s != TfStringReplace(*paths, " ", "")) {
                fprintf(stderr, "mismatch: %s -> %s\n", *paths, s.c_str());
                TF_AXIOM(s == *paths);
            }
        }

        ++paths;

    }
    // Clean up.
    pathYylex_destroy(context.scanner);
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
        NULL
    };

    testPaths(good, 0);

    printf("Testing bad paths: errors expected\n");
    testPaths(bad, 1);

    printf("Done expecting errors\n");

    printf("Test PASSED\n");

    return 0;

}

