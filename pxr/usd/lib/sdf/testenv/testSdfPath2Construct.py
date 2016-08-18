#!/pxrpythonsubst

import sys, os, traceback
from Mentor.Runtime import *

from pxr.Sdf.Test.testUtils_Sdf import *

from pxr import Sdf

# Assert failures end test
SetAssertMode(MTR_EXIT_TEST)

goodPaths = {
    # Roots
    "." : (False, []),
    "/" : (True, []),

    # Basic prims
    "/Foo" : (True, ["Foo"]),
    "/Foo/Bar" : (True, ["Foo", "Bar"]),
    "Foo" : (False, ["Foo"]),
    "Foo/Bar" : (False, ["Foo", "Bar"]),

    # Dot-dot stuff
    ".." : (False, [".."]),
    "../.." : (False, ["..", ".."]),
    "../../.." : (False, ["..", "..", ".."]),
    "../Foo" : (False, ["..", "Foo"]),
    "../../Foo" : (False, ["..", "..", "Foo"]),
    "../../../Foo" : (False, ["..", "..", "..", "Foo"]),
    "../Foo/Foo" : (False, ["..", "Foo", "Foo"]),
    "../../Foo/Foo" : (False, ["..", "..", "Foo", "Foo"]),
    "../../../Foo/Foo" : (False, ["..", "..", "..", "Foo", "Foo"]),

    # Basic properties
    "/Foo.prop" : (True, ["Foo", ".prop"]),
    "/Foo/Bar.prop" : (True, ["Foo", "Bar", ".prop"]),
    ".prop" : (False, [".prop"]),
    "Foo.prop" : (False, ["Foo", ".prop"]),
    "Foo/Bar.prop" : (False, ["Foo", "Bar", ".prop"]),

    # Namespaced basic properties
    "/Foo.prop:with:a:lot:of:namespaces" : (True, ["Foo", ".prop:with:a:lot:of:namespaces"]),
    "/Foo/Bar.prop:with:a:lot:of:namespaces" : (True, ["Foo", "Bar", ".prop:with:a:lot:of:namespaces"]),
    ".prop:with:a:lot:of:namespaces" : (False, [".prop:with:a:lot:of:namespaces"]),
    "Foo.prop:with:a:lot:of:namespaces" : (False, ["Foo", ".prop:with:a:lot:of:namespaces"]),
    "Foo/Bar.prop:with:a:lot:of:namespaces" : (False, ["Foo", "Bar", ".prop:with:a:lot:of:namespaces"]),

    # Properties with targets
    "/Foo.prop[/Target]" : (True, ["Foo", ".prop", "[/Target]"]),
    "/Foo/Bar.prop[/Target]" : (True, ["Foo", "Bar", ".prop", "[/Target]"]),
    ".prop[/Target]" : (False, [".prop", "[/Target]"]),
    "Foo.prop[/Target]" : (False, ["Foo", ".prop", "[/Target]"]),
    "Foo/Bar.prop[/Target]" : (False, ["Foo", "Bar", ".prop", "[/Target]"]),

    # Namespaced properties with targets
    "/Foo.prop:baz[/Target]" : (True, ["Foo", ".prop:baz", "[/Target]"]),
    "/Foo/Bar.prop:baz[/Target]" : (True, ["Foo", "Bar", ".prop:baz", "[/Target]"]),
    ".prop:baz[/Target]" : (False, [".prop:baz", "[/Target]"]),
    "Foo.prop:baz[/Target]" : (False, ["Foo", ".prop:baz", "[/Target]"]),
    "Foo/Bar.prop:baz[/Target]" : (False, ["Foo", "Bar", ".prop:baz", "[/Target]"]),

    # Attributes with mappers
    "/Foo.attr.mapper[/ConnTarget]" : (True, ["Foo", ".attr", ".mapper[/ConnTarget]"]),
    "/Foo/Bar.attr.mapper[/ConnTarget]" : (True, ["Foo", "Bar", ".attr", ".mapper[/ConnTarget]"]),
    ".attr.mapper[/ConnTarget]" : (False, [".attr", ".mapper[/ConnTarget]"]),
    "Foo.attr.mapper[/ConnTarget]" : (False, ["Foo", ".attr", ".mapper[/ConnTarget]"]),
    "Foo/Bar.attr.mapper[/ConnTarget]" : (False, ["Foo", "Bar", ".attr", ".mapper[/ConnTarget]"]),

    # Attributes with mappers with args
    "/Foo.attr.mapper[/ConnTarget].arg" : (True, ["Foo", ".attr", ".mapper[/ConnTarget]", ".arg"]),
    "/Foo/Bar.attr.mapper[/ConnTarget].arg" : (True, ["Foo", "Bar", ".attr", ".mapper[/ConnTarget]", ".arg"]),
    ".attr.mapper[/ConnTarget].arg" : (False, [".attr", ".mapper[/ConnTarget]", ".arg"]),
    "Foo.attr.mapper[/ConnTarget].arg" : (False, ["Foo", ".attr", ".mapper[/ConnTarget]", ".arg"]),
    "Foo/Bar.attr.mapper[/ConnTarget].arg" : (False, ["Foo", "Bar", ".attr", ".mapper[/ConnTarget]", ".arg"]),

    # Namespaced attributes with mappers with args
    "/Foo.attr:baz.mapper[/ConnTarget].arg" : (True, ["Foo", ".attr:baz", ".mapper[/ConnTarget]", ".arg"]),
    "/Foo/Bar.attr:baz.mapper[/ConnTarget].arg" : (True, ["Foo", "Bar", ".attr:baz", ".mapper[/ConnTarget]", ".arg"]),
    ".attr:baz.mapper[/ConnTarget].arg" : (False, [".attr:baz", ".mapper[/ConnTarget]", ".arg"]),
    "Foo.attr:baz.mapper[/ConnTarget].arg" : (False, ["Foo", ".attr:baz", ".mapper[/ConnTarget]", ".arg"]),
    "Foo/Bar.attr:baz.mapper[/ConnTarget].arg" : (False, ["Foo", "Bar", ".attr:baz", ".mapper[/ConnTarget]", ".arg"]),

    # Attributes with expressions
    "/Foo.attr.expression" : (True, ["Foo", ".attr", ".expression"]),
    "/Foo/Bar.attr.expression" : (True, ["Foo", "Bar", ".attr", ".expression"]),
    ".attr.expression" : (False, [".attr", ".expression"]),
    "Foo.attr.expression" : (False, ["Foo", ".attr", ".expression"]),
    "Foo/Bar.attr.expression" : (False, ["Foo", "Bar", ".attr", ".expression"]),

    # Namespaced attributes with expressions
    "/Foo.attr:baz.expression" : (True, ["Foo", ".attr:baz", ".expression"]),
    "/Foo/Bar.attr:baz.expression" : (True, ["Foo", "Bar", ".attr:baz", ".expression"]),
    ".attr:baz.expression" : (False, [".attr:baz", ".expression"]),
    "Foo.attr:baz.expression" : (False, ["Foo", ".attr:baz", ".expression"]),
    "Foo/Bar.attr:baz.expression" : (False, ["Foo", "Bar", ".attr:baz", ".expression"]),

    # Relational attrs on simple targets
    "/Foo.rel[/Target].attr" : (True, ["Foo", ".rel", "[/Target]", ".attr"]),
    "/Foo/Bar.rel[/Target].attr" : (True, ["Foo", "Bar", ".rel", "[/Target]", ".attr"]),
    ".rel[/Target].attr" : (False, [".rel", "[/Target]", ".attr"]),
    "Foo.rel[/Target].attr" : (False, ["Foo", ".rel", "[/Target]", ".attr"]),
    "Foo/Bar.rel[/Target].attr" : (False, ["Foo", "Bar", ".rel", "[/Target]", ".attr"]),

    # Relational attrs on nested targets
    "/Foo.rel[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2].attr" : (True, ["Foo", ".rel", "[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2]", ".attr"]),
    "/Foo/Bar.rel[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2].attr" : (True, ["Foo", "Bar", ".rel", "[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2]", ".attr"]),
    ".rel[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2].attr" : (False, [".rel", "[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2]", ".attr"]),
    "Foo.rel[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2].attr" : (False, ["Foo", ".rel", "[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2]", ".attr"]),
    "Foo/Bar.rel[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2].attr" : (False, ["Foo", "Bar", ".rel", "[/Target.rel2[/Target2.rel3[/Target3].attr3].attr2]", ".attr"]),

    # Relational attrs on nested targets with namespaces.
    "/Foo.rel:baz[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz].attr:baz" : (True, ["Foo", ".rel:baz", "[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz]", ".attr:baz"]),
    "/Foo/Bar.rel:baz[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz].attr:baz" : (True, ["Foo", "Bar", ".rel:baz", "[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz]", ".attr:baz"]),
    ".rel:baz[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz].attr:baz" : (False, [".rel:baz", "[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz]", ".attr:baz"]),
    "Foo.rel:baz[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz].attr:baz" : (False, ["Foo", ".rel:baz", "[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz]", ".attr:baz"]),
    "Foo/Bar.rel:baz[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz].attr:baz" : (False, ["Foo", "Bar", ".rel:baz", "[/Target.rel2:baz[/Target2.rel3:baz[/Target3].attr3:baz].attr2:baz]", ".attr:baz"]),

    # Relational attrs on simple targets with connection targets
    "/Foo.rel[/Target].attr[/ConnTarget]" : (True, ["Foo", ".rel", "[/Target]", ".attr", "[/ConnTarget]"]),
    "/Foo/Bar.rel[/Target].attr[/ConnTarget]" : (True, ["Foo", "Bar", ".rel", "[/Target]", ".attr", "[/ConnTarget]"]),
    ".rel[/Target].attr[/ConnTarget]" : (False, [".rel", "[/Target]", ".attr", "[/ConnTarget]"]),
    "Foo.rel[/Target].attr[/ConnTarget]" : (False, ["Foo", ".rel", "[/Target]", ".attr", "[/ConnTarget]"]),
    "Foo/Bar.rel[/Target].attr[/ConnTarget]" : (False, ["Foo", "Bar", ".rel", "[/Target]", ".attr", "[/ConnTarget]"]),

    # Namespaced relational attrs on simple targets with connection targets
    "/Foo.rel[/Target].attr:baz[/ConnTarget]" : (True, ["Foo", ".rel", "[/Target]", ".attr:baz", "[/ConnTarget]"]),
    "/Foo/Bar.rel[/Target].attr:baz[/ConnTarget]" : (True, ["Foo", "Bar", ".rel", "[/Target]", ".attr:baz", "[/ConnTarget]"]),
    ".rel[/Target].attr:baz[/ConnTarget]" : (False, [".rel", "[/Target]", ".attr:baz", "[/ConnTarget]"]),
    "Foo.rel[/Target].attr:baz[/ConnTarget]" : (False, ["Foo", ".rel", "[/Target]", ".attr:baz", "[/ConnTarget]"]),
    "Foo/Bar.rel[/Target].attr:baz[/ConnTarget]" : (False, ["Foo", "Bar", ".rel", "[/Target]", ".attr:baz", "[/ConnTarget]"]),

    # Relational attrs on simple targets with mappers
    "/Foo.rel[/Target].attr.mapper[/ConnTarget]" : (True, ["Foo", ".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]"]),
    "/Foo/Bar.rel[/Target].attr.mapper[/ConnTarget]" : (True, ["Foo", "Bar", ".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]"]),
    ".rel[/Target].attr.mapper[/ConnTarget]" : (False, [".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]"]),
    "Foo.rel[/Target].attr.mapper[/ConnTarget]" : (False, ["Foo", ".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]"]),
    "Foo/Bar.rel[/Target].attr.mapper[/ConnTarget]" : (False, ["Foo", "Bar", ".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]"]),

    # Namespaced relational attrs on simple targets with mappers
    "/Foo.rel[/Target].attr:baz.mapper[/ConnTarget]" : (True, ["Foo", ".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]"]),
    "/Foo/Bar.rel[/Target].attr:baz.mapper[/ConnTarget]" : (True, ["Foo", "Bar", ".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]"]),
    ".rel[/Target].attr:baz.mapper[/ConnTarget]" : (False, [".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]"]),
    "Foo.rel[/Target].attr:baz.mapper[/ConnTarget]" : (False, ["Foo", ".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]"]),
    "Foo/Bar.rel[/Target].attr:baz.mapper[/ConnTarget]" : (False, ["Foo", "Bar", ".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]"]),

    # Relational attrs on simple targets with mappers with args
    "/Foo.rel[/Target].attr.mapper[/ConnTarget].arg" : (True, ["Foo", ".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]", ".arg"]),
    "/Foo/Bar.rel[/Target].attr.mapper[/ConnTarget].arg" : (True, ["Foo", "Bar", ".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]", ".arg"]),
    ".rel[/Target].attr.mapper[/ConnTarget].arg" : (False, [".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]", ".arg"]),
    "Foo.rel[/Target].attr.mapper[/ConnTarget].arg" : (False, ["Foo", ".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]", ".arg"]),
    "Foo/Bar.rel[/Target].attr.mapper[/ConnTarget].arg" : (False, ["Foo", "Bar", ".rel", "[/Target]", ".attr", ".mapper[/ConnTarget]", ".arg"]),

    # Namespaced relational attrs on simple targets with mappers with args
    "/Foo.rel[/Target].attr:baz.mapper[/ConnTarget].arg" : (True, ["Foo", ".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]", ".arg"]),
    "/Foo/Bar.rel[/Target].attr:baz.mapper[/ConnTarget].arg" : (True, ["Foo", "Bar", ".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]", ".arg"]),
    ".rel[/Target].attr:baz.mapper[/ConnTarget].arg" : (False, [".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]", ".arg"]),
    "Foo.rel[/Target].attr:baz.mapper[/ConnTarget].arg" : (False, ["Foo", ".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]", ".arg"]),
    "Foo/Bar.rel[/Target].attr:baz.mapper[/ConnTarget].arg" : (False, ["Foo", "Bar", ".rel", "[/Target]", ".attr:baz", ".mapper[/ConnTarget]", ".arg"]),

    # Relational attrs on simple targets with expressions
    "/Foo.rel[/Target].attr.expression" : (True, ["Foo", ".rel", "[/Target]", ".attr", ".expression"]),
    "/Foo/Bar.rel[/Target].attr.expression" : (True, ["Foo", "Bar", ".rel", "[/Target]", ".attr", ".expression"]),
    ".rel[/Target].attr.expression" : (False, [".rel", "[/Target]", ".attr", ".expression"]),
    "Foo.rel[/Target].attr.expression" : (False, ["Foo", ".rel", "[/Target]", ".attr", ".expression"]),
    "Foo/Bar.rel[/Target].attr.expression" : (False, ["Foo", "Bar", ".rel", "[/Target]", ".attr", ".expression"]),

    # Namespaced relational attrs on simple targets with expressions
    "/Foo.rel[/Target].attr:baz.expression" : (True, ["Foo", ".rel", "[/Target]", ".attr:baz", ".expression"]),
    "/Foo/Bar.rel[/Target].attr:baz.expression" : (True, ["Foo", "Bar", ".rel", "[/Target]", ".attr:baz", ".expression"]),
    ".rel[/Target].attr:baz.expression" : (False, [".rel", "[/Target]", ".attr:baz", ".expression"]),
    "Foo.rel[/Target].attr:baz.expression" : (False, ["Foo", ".rel", "[/Target]", ".attr:baz", ".expression"]),
    "Foo/Bar.rel[/Target].attr:baz.expression" : (False, ["Foo", "Bar", ".rel", "[/Target]", ".attr:baz", ".expression"]),

}

if verbose:
    print "Testing good paths"

for str in goodPaths:
    path = Sdf.Path(str)
    elemTuple = goodPaths[str]
    AssertEqual(path.IsAbsolutePath(), elemTuple[0], "Failed on path string %s"%str)
    prefixes = PathElemsToPrefixes(elemTuple[0], elemTuple[1])
    AssertEqual(path.GetPrefixes(), prefixes)
    AssertEqual(path.pathString, str)
    repr = path.__repr__()
    exec "newPath = %s"%repr
    AssertEqual(newPath, path)

repr = Sdf.Path.emptyPath.__repr__()
exec "newPath = %s"%repr
AssertEqual(newPath, Sdf.Path.emptyPath)

if verbose:
    print "    ...passed"

if verbose:
    print "Testing bad path strings"

badPathStrings = [
    # Invalid path (empty)
    "",

    # Illegal characters
    "Foo<Bar",
    "Foo>Bar",
    "Foo[Bar",
    "Foo]Bar",
    "Foo\\Bar",
    "Foo:Bar",
    
    # Reflexive relative as first element
    "./Foo",
    
    # Rooted properties
    "/.prop",
    "/.attr[1]",
    "/.rel[/Foo].attr",
    
    # Bad use of dot-dot
    "/..",
    "/../..",
    "Foo/..",
    "/Foo/..",

    # Bad initial stuff
    "[1]",
    "[/Target]",

    # Bad stuff after slash
    "/[1]",
    "/<>",
    "//<>",
    
    # Bad stuff after prim
    "/Foo[1]",
    "/Foo[/Target]",

    # Bad stuff after prop
    "/Foo.prop/Bar",
    "/Foo.prop.baz",

    # Bad target path
    "/Foo.rel[/.Target].attr",

]

ExpectedErrorBegin(len(badPathStrings))
for str in badPathStrings:
    AssertEqual(Sdf.Path(str), Sdf.Path.emptyPath, "Failed on path str %s"%str)
ExpectedErrorEnd()

if verbose:
    print "    ...passed"

# ========================================================================
# Print final status
# ========================================================================

ExitTest()

