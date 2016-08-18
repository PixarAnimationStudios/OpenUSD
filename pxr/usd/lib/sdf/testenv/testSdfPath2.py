#!/pxrpythonsubst

import sys, os, traceback
from Mentor.Runtime import *
from pxr.Sdf.Test.testUtils_Sdf import *
from pxr import Sdf

# Assert failures end test
SetAssertMode(MTR_EXIT_TEST)

EmptyType = 1 
RootType = 2 
PrimType = 4
PropertyType = 8
PrimPropertyType = 16
RelationalAttributeType = 32
TargetType = 64
MapperType = 128
MapperArgType = 256
ExpressionType = 512
NamespacedPropertyType = 1024

def _CheckPath(path, path2, parentPath, pathStr, pathElems, pathFlags, name, targetPath):
    # Check equality with another one constructed the same way
    AssertEqual(path, path2)

    # Check properties
    AssertEqual(path.pathString, pathStr)
    AssertEqual(path.name, name)
    AssertEqual(path.pathElementCount, len(pathElems))
    AssertEqual(path.targetPath, targetPath)

    # Check queries
    AssertEqual(path.IsAbsolutePath(), pathStr.startswith("/"))
    AssertEqual(path.IsPrimPath(), ((pathFlags & PrimType) or (path == Sdf.Path.reflexiveRelativePath)) != 0 )
    AssertEqual(path.IsRootPrimPath(), ((pathFlags & PrimType) != 0 and (len(pathElems) is 1)))
    AssertEqual(path.IsPropertyPath(), (pathFlags & PropertyType) != 0)
    if (pathFlags & NamespacedPropertyType) != 0:
        AssertEqual(path.IsNamespacedPropertyPath(), (pathFlags & NamespacedPropertyType) != 0)
    if (pathFlags & PrimPropertyType) != 0:
        AssertEqual(path.IsPrimPropertyPath(), (pathFlags & PrimPropertyType) != 0)
    if (pathFlags & RelationalAttributeType) != 0:
        AssertEqual(path.IsRelationalAttributePath(), (pathFlags & RelationalAttributeType) != 0)
    AssertEqual(path.IsTargetPath(), (pathFlags & TargetType) != 0)
    AssertEqual(path.IsMapperPath(), (pathFlags & MapperType) != 0)
    AssertEqual(path.IsMapperArgPath(), (pathFlags & MapperArgType) != 0)
    AssertEqual(path.IsExpressionPath(), (pathFlags & ExpressionType) != 0)

    # Check path elements
    prefixes = PathElemsToPrefixes(path.IsAbsolutePath(), pathElems)
    AssertEqual(path.GetPrefixes(), prefixes)

    # Check parent
    AssertEqual(path.GetParentPath(), parentPath)
    
    # Make sure all parent prefixes are recognized as prefixes, and that
    # each prefix (including terminal) is reconstructible from its string
    # element representation
    currPath = path
    while (parentPath != Sdf.Path.emptyPath) and (parentPath.name != Sdf.Path.parentPathElement):
        currElement = currPath.elementString
        reconPath = parentPath.AppendElementString(currElement)
        Assert(path.HasPrefix(parentPath))
        AssertEqual(reconPath, currPath)
        currPath = parentPath
        parentPath = parentPath.GetParentPath()

def CheckEmptyPath(path):
    _CheckPath(path, Sdf.Path.emptyPath, Sdf.Path.emptyPath, "", [], EmptyType, "", Sdf.Path.emptyPath)
    AssertEqual(path, Sdf.Path())
    
def CheckRootPath(path, path2, pathStr):
    if pathStr == "/":
        parent = Sdf.Path.emptyPath
    else:
        parent = path.AppendChild(Sdf.Path.parentPathElement)
    _CheckPath(path, path2, parent, pathStr, [], RootType, pathStr, Sdf.Path.emptyPath) 
    
def CheckPrimPath(path, path2, parentPath, pathStr, pathElems, name):
    _CheckPath(path, path2, parentPath, pathStr, pathElems, PrimType, name, Sdf.Path.emptyPath) 

def CheckPrimPropertyPath(path, path2, parentPath, pathStr, pathElems, name):
    _CheckPath(path, path2, parentPath, pathStr, pathElems, PropertyType | PrimPropertyType, name, Sdf.Path.emptyPath) 

def CheckPrimNamespacedPropertyPath(path, path2, parentPath, pathStr, pathElems, name):
    _CheckPath(path, path2, parentPath, pathStr, pathElems, PropertyType | PrimPropertyType | NamespacedPropertyType, name, Sdf.Path.emptyPath) 

def CheckTargetPath(path, path2, parentPath, pathStr, pathElems, targetPath):
    _CheckPath(path, path2, parentPath, pathStr, pathElems, TargetType, "", targetPath) 

def CheckRelationalAttributePath(path, path2, parentPath, pathStr, pathElems, name):
    _CheckPath(path, path2, parentPath, pathStr, pathElems, PropertyType | RelationalAttributeType, name, parentPath.targetPath) 

def CheckRelationalNamespacedAttributePath(path, path2, parentPath, pathStr, pathElems, name):
    _CheckPath(path, path2, parentPath, pathStr, pathElems, PropertyType | RelationalAttributeType | NamespacedPropertyType, name, parentPath.targetPath) 

def CheckMapperPath(path, path2, parentPath, pathStr, pathElems, targetPath):
    _CheckPath(path, path2, parentPath, pathStr, pathElems, MapperType, "", targetPath) 

def CheckMapperArgPath(path, path2, parentPath, pathStr, pathElems, name):
    _CheckPath(path, path2, parentPath, pathStr, pathElems, MapperArgType, name, parentPath.targetPath) 

def CheckExpressionPath(path, path2, parentPath, pathStr, pathElems):
    _CheckPath(path, path2, parentPath, pathStr, pathElems, ExpressionType, "expression", parentPath.targetPath) 

def CheckOrdering(*args):
    for i in range(len(args)-1):
        Assert(args[i] < args[i+1], "Expected <%s> to compare less-than <%s>, but it did not"%(args[i], args[i+1]))
    
# ========================================================================
# Test some basic construction
# ========================================================================

# --- Check Empty path
empty = Sdf.Path.emptyPath

CheckEmptyPath(empty)
AssertFalse(empty.IsPrimPropertyPath())
AssertFalse(empty.IsNamespacedPropertyPath())
AssertFalse(empty.IsRelationalAttributePath())
AssertFalse(empty < empty, "Less-than check failed")

# --- Check absolute root path
absRoot = Sdf.Path.absoluteRootPath

CheckRootPath(absRoot, Sdf.Path.absoluteRootPath, "/")
CheckOrdering(empty, absRoot)

# --- Check relative root path
relRoot = Sdf.Path.reflexiveRelativePath

CheckRootPath(relRoot, Sdf.Path.reflexiveRelativePath, ".")
CheckOrdering(empty, absRoot, relRoot)

# --- Make a root prim path
rootPrim = absRoot.AppendChild("Foo")

CheckPrimPath(rootPrim, absRoot.AppendChild("Foo"), absRoot, "/Foo", ["Foo"], "Foo")
CheckOrdering(empty, absRoot, rootPrim, relRoot)

# --- Make another root prim path
rootPrim2 = absRoot.AppendChild("Bar")

CheckPrimPath(rootPrim2, absRoot.AppendChild("Bar"), absRoot, "/Bar", ["Bar"], "Bar")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, relRoot)

# --- Make a prim child of the root prim path
childPrim = rootPrim.AppendChild("Bar")

CheckPrimPath(childPrim, rootPrim.AppendChild("Bar"), rootPrim, "/Foo/Bar", ["Foo", "Bar"], "Bar")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, relRoot)

# --- Make another prim child of the root prim path
childPrim2 = rootPrim.AppendChild("Foo")

CheckPrimPath(childPrim2, rootPrim.AppendChild("Foo"), rootPrim, "/Foo/Foo", ["Foo", "Foo"], "Foo")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, relRoot)

# --- Make a property of the root prim path
rootProp = rootPrim.AppendProperty("prop1")

CheckPrimPropertyPath(rootProp, rootPrim.AppendProperty("prop1"), rootPrim, "/Foo.prop1", ["Foo", ".prop1"], "prop1")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, relRoot)

# --- Make another property of the root prim path
rootProp2 = rootPrim.AppendProperty("prop2")

CheckPrimPropertyPath(rootProp2, rootPrim.AppendProperty("prop2"), rootPrim, "/Foo.prop2", ["Foo", ".prop2"], "prop2")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootProp2, relRoot)

# --- Make a namespaced property of the root prim path
rootProp3 = rootPrim.AppendProperty("prop3:bar:baz")

CheckPrimNamespacedPropertyPath(rootProp3, rootPrim.AppendProperty("prop3:bar:baz"), rootPrim, "/Foo.prop3:bar:baz", ["Foo", ".prop3:bar:baz"], "prop3:bar:baz")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootProp3, relRoot)

# --- Make a property of the child prim path
childProp = childPrim.AppendProperty("foo")

CheckPrimPropertyPath(childProp, childPrim.AppendProperty("foo"), childPrim, "/Foo/Bar.foo", ["Foo", "Bar", ".foo"], "foo")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childProp, childPrim2, rootProp, relRoot)

# --- Make another property of the child prim path
childProp2 = childPrim.AppendProperty("bar")

CheckPrimPropertyPath(childProp2, childPrim.AppendProperty("bar"), childPrim, "/Foo/Bar.bar", ["Foo", "Bar", ".bar"], "bar")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childProp2, childProp, childPrim2, rootProp, relRoot)

# --- Make a target of the root property path
rootPropTarg = rootProp.AppendTarget("/Target1")

CheckTargetPath(rootPropTarg, rootProp.AppendTarget("/Target1"), rootProp, "/Foo.prop1[/Target1]", ["Foo", ".prop1", "[/Target1]"], "/Target1")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg, relRoot)

# --- Make another target of the root property path
rootPropTarg2 = rootProp.AppendTarget("/FooTarget.bar")

CheckTargetPath(rootPropTarg2, rootProp.AppendTarget("/FooTarget.bar"), rootProp, "/Foo.prop1[/FooTarget.bar]", ["Foo", ".prop1", "[/FooTarget.bar]"], "/FooTarget.bar")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropTarg, relRoot)

# --- Make a rel attr of the root property target path
rootPropRelAttr = rootPropTarg.AppendRelationalAttribute("fooAttr")

CheckRelationalAttributePath(rootPropRelAttr, rootPropTarg.AppendRelationalAttribute("fooAttr"), rootPropTarg, "/Foo.prop1[/Target1].fooAttr", ["Foo", ".prop1", "[/Target1]", ".fooAttr"], "fooAttr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropTarg, rootPropRelAttr, relRoot)

# --- Make another rel attr of the root property path, same target
rootPropRelAttr2 = rootPropTarg.AppendRelationalAttribute("barAttr")

CheckRelationalAttributePath(rootPropRelAttr2, rootPropTarg.AppendRelationalAttribute("barAttr"), rootPropTarg, "/Foo.prop1[/Target1].barAttr", ["Foo", ".prop1", "[/Target1]", ".barAttr"], "barAttr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, relRoot)

# --- Make another rel attr of the root property path, 2nd target
rootPropRelAttr3 = rootPropTarg2.AppendRelationalAttribute("barAttr")

CheckRelationalAttributePath(rootPropRelAttr3, rootPropTarg2.AppendRelationalAttribute("barAttr"), rootPropTarg2, "/Foo.prop1[/FooTarget.bar].barAttr", ["Foo", ".prop1", "[/FooTarget.bar]", ".barAttr"], "barAttr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, relRoot)

# --- Make yet another rel attr of the root property path, 2nd target
rootPropRelAttr4 = rootPropTarg2.AppendRelationalAttribute("fooAttr")

CheckRelationalAttributePath(rootPropRelAttr4, rootPropTarg2.AppendRelationalAttribute("fooAttr"), rootPropTarg2, "/Foo.prop1[/FooTarget.bar].fooAttr", ["Foo", ".prop1", "[/FooTarget.bar]", ".fooAttr"], "fooAttr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, relRoot)

# --- Make a namespaced rel attr of the root property path, 2nd target
rootPropRelAttr5 = rootPropTarg2.AppendRelationalAttribute("fooAttr:bar:baz")

CheckRelationalAttributePath(rootPropRelAttr5, rootPropTarg2.AppendRelationalAttribute("fooAttr:bar:baz"), rootPropTarg2, "/Foo.prop1[/FooTarget.bar].fooAttr:bar:baz", ["Foo", ".prop1", "[/FooTarget.bar]", ".fooAttr:bar:baz"], "fooAttr:bar:baz")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr5, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, relRoot)

# --- Make a target of a rel attr path
rootPropRelAttrTarget = rootPropRelAttr.AppendTarget("/ConnTarget1.attr")

CheckTargetPath(rootPropRelAttrTarget, rootPropRelAttr.AppendTarget("/ConnTarget1.attr"), rootPropRelAttr, "/Foo.prop1[/Target1].fooAttr[/ConnTarget1.attr]", ["Foo", ".prop1", "[/Target1]", ".fooAttr", "[/ConnTarget1.attr]"], "/ConnTarget1.attr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, relRoot)

# --- Make another target of a rel attr path
rootPropRelAttrTarget2 = rootPropRelAttr.AppendTarget("/ConnTarget2.attr")

CheckTargetPath(rootPropRelAttrTarget2, rootPropRelAttr.AppendTarget("/ConnTarget2.attr"), rootPropRelAttr, "/Foo.prop1[/Target1].fooAttr[/ConnTarget2.attr]", ["Foo", ".prop1", "[/Target1]", ".fooAttr", "[/ConnTarget2.attr]"], "/ConnTarget2.attr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, relRoot)

# --- Make a mapper path on a prim property
rootPropMapper1 = rootProp.AppendMapper("/ConnTarget1.attr")

CheckMapperPath(rootPropMapper1, rootProp.AppendMapper("/ConnTarget1.attr"), rootProp, "/Foo.prop1.mapper[/ConnTarget1.attr]", ["Foo", ".prop1", ".mapper[/ConnTarget1.attr]"], "/ConnTarget1.attr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropMapper1, relRoot)

# --- Make another mapper path on a prim property
rootPropMapper2 = rootProp.AppendMapper("/ConnTarget2.attr")

CheckMapperPath(rootPropMapper2, rootProp.AppendMapper("/ConnTarget2.attr"), rootProp, "/Foo.prop1.mapper[/ConnTarget2.attr]", ["Foo", ".prop1", ".mapper[/ConnTarget2.attr]"], "/ConnTarget2.attr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropMapper1, rootPropMapper2, relRoot)

# --- Make an expression path on a prim property
rootPropExpression = rootProp.AppendExpression()

CheckExpressionPath(rootPropExpression, rootProp.AppendExpression(), rootProp, "/Foo.prop1.expression", ["Foo", ".prop1", ".expression"])
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropMapper1, rootPropMapper2, rootPropExpression, relRoot)

# --- Make a mapper arg path on a prim property
rootPropMapperArg1 = rootPropMapper1.AppendMapperArg("fooArg")

CheckMapperArgPath(rootPropMapperArg1, rootPropMapper1.AppendMapperArg("fooArg"), rootPropMapper1, "/Foo.prop1.mapper[/ConnTarget1.attr].fooArg", ["Foo", ".prop1", ".mapper[/ConnTarget1.attr]", ".fooArg"], "fooArg")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropMapper1, rootPropMapperArg1, rootPropMapper2, rootPropExpression, relRoot)

# --- Make another mapper arg path on a prim property
rootPropMapperArg2 = rootPropMapper1.AppendMapperArg("barArg")

CheckMapperArgPath(rootPropMapperArg2, rootPropMapper1.AppendMapperArg("barArg"), rootPropMapper1, "/Foo.prop1.mapper[/ConnTarget1.attr].barArg", ["Foo", ".prop1", ".mapper[/ConnTarget1.attr]", ".barArg"], "barArg")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropMapper1, rootPropMapperArg2, rootPropMapperArg1, rootPropMapper2, rootPropExpression, relRoot)

# --- Make a mapper path on a rel attr
rootPropRelAttrMapper1 = rootPropRelAttr.AppendMapper("/ConnTarget1.attr")

CheckMapperPath(rootPropRelAttrMapper1, rootPropRelAttr.AppendMapper("/ConnTarget1.attr"), rootPropRelAttr, "/Foo.prop1[/Target1].fooAttr.mapper[/ConnTarget1.attr]", ["Foo", ".prop1", "[/Target1]", ".fooAttr", ".mapper[/ConnTarget1.attr]"], "/ConnTarget1.attr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropRelAttrMapper1, rootPropMapper1, rootPropMapperArg2, rootPropMapperArg1, rootPropMapper2, rootPropExpression, relRoot)

# --- Make another mapper path on a rel attr
rootPropRelAttrMapper2 = rootPropRelAttr.AppendMapper("/ConnTarget2.attr")

CheckMapperPath(rootPropRelAttrMapper2, rootPropRelAttr.AppendMapper("/ConnTarget2.attr"), rootPropRelAttr, "/Foo.prop1[/Target1].fooAttr.mapper[/ConnTarget2.attr]", ["Foo", ".prop1", "[/Target1]", ".fooAttr", ".mapper[/ConnTarget2.attr]"], "/ConnTarget2.attr")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropRelAttrMapper1, rootPropRelAttrMapper2, rootPropMapper1, rootPropMapperArg2, rootPropMapperArg1, rootPropMapper2, rootPropExpression, relRoot)

# --- Make a expression path on a rel attr
rootPropRelAttrExpression = rootPropRelAttr.AppendExpression()

CheckExpressionPath(rootPropRelAttrExpression, rootPropRelAttr.AppendExpression(), rootPropRelAttr, "/Foo.prop1[/Target1].fooAttr.expression", ["Foo", ".prop1", "[/Target1]", ".fooAttr", ".expression"])
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropRelAttrMapper1, rootPropRelAttrMapper2, rootPropRelAttrExpression, rootPropMapper1, rootPropMapperArg2, rootPropMapperArg1, rootPropMapper2, rootPropExpression, relRoot)

# --- Make a mapper arg path on a rel attr
rootPropRelAttrMapperArg1 = rootPropRelAttrMapper1.AppendMapperArg("fooArg")

CheckMapperArgPath(rootPropRelAttrMapperArg1, rootPropRelAttrMapper1.AppendMapperArg("fooArg"), rootPropRelAttrMapper1, "/Foo.prop1[/Target1].fooAttr.mapper[/ConnTarget1.attr].fooArg", ["Foo", ".prop1", "[/Target1]", ".fooAttr", ".mapper[/ConnTarget1.attr]", ".fooArg"], "fooArg")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropRelAttrMapper1, rootPropRelAttrMapperArg1, rootPropRelAttrMapper2, rootPropRelAttrExpression, rootPropMapper1, rootPropMapperArg2, rootPropMapperArg1, rootPropMapper2, rootPropExpression, relRoot)

# --- Make another mapper arg path on a rel attr
rootPropRelAttrMapperArg2 = rootPropRelAttrMapper1.AppendMapperArg("barArg")

CheckMapperArgPath(rootPropRelAttrMapperArg2, rootPropRelAttrMapper1.AppendMapperArg("barArg"), rootPropRelAttrMapper1, "/Foo.prop1[/Target1].fooAttr.mapper[/ConnTarget1.attr].barArg", ["Foo", ".prop1", "[/Target1]", ".fooAttr", ".mapper[/ConnTarget1.attr]", ".barArg"], "barArg")
CheckOrdering(empty, absRoot, rootPrim2, rootPrim, childPrim, childPrim2, rootProp, rootPropTarg2, rootPropRelAttr3, rootPropRelAttr4, rootPropTarg, rootPropRelAttr2, rootPropRelAttr, rootPropRelAttrTarget, rootPropRelAttrTarget2, rootPropRelAttrMapper1, rootPropRelAttrMapperArg2, rootPropRelAttrMapperArg1, rootPropRelAttrMapper2, rootPropRelAttrExpression, rootPropMapper1, rootPropMapperArg2, rootPropMapperArg1, rootPropMapper2, rootPropExpression, relRoot)

# test GetPrimPath

# AssertEqual(Path(".").GetPrimPath(),                   ".")
AssertEqual(Sdf.Path("foo").GetPrimPath(),                 "foo")
AssertEqual(Sdf.Path("foo/bar").GetPrimPath(),             "foo/bar")
AssertEqual(Sdf.Path("foo.bar").GetPrimPath(),             "foo")
AssertEqual(Sdf.Path("foo.bar[foo].bar").GetPrimPath(),    "foo")

# test ReplacePrefix

AssertEqual(Sdf.Path("/a").ReplacePrefix("/a", "/b"), "/b")
AssertEqual(Sdf.Path("/a/b").ReplacePrefix("/a", "/b"), "/b/b")
AssertEqual(Sdf.Path("a").ReplacePrefix("a", "b"), "b")
AssertEqual(Sdf.Path("foo/bar").ReplacePrefix("foo", "bar"), "bar/bar")
AssertEqual(Sdf.Path("foo.bar").ReplacePrefix("foo", "bar"), "bar.bar")
AssertEqual(Sdf.Path("foo.bar:baz").ReplacePrefix("foo", "bar"), "bar.bar:baz")
AssertEqual(Sdf.Path("foo.bar[/foo].attr").ReplacePrefix("foo.bar", "a.b"), "a.b[/foo].attr")
AssertEqual(Sdf.Path("/foo.bar[foo.bar].attr").ReplacePrefix("foo.bar", "a.b"), "/foo.bar[a.b].attr")
AssertEqual(Sdf.Path("/foo.bar[foo.bar].attr:baz").ReplacePrefix("foo.bar", "a.b"), "/foo.bar[a.b].attr:baz")
AssertEqual(Sdf.Path("/foo.bar[/foo.bar].attr").ReplacePrefix("/foo.bar", "/a.b"), "/a.b[/a.b].attr")
AssertEqual(Sdf.Path("/foo.bar[/foo.bar[/foo.bar].attr].attr").ReplacePrefix("/foo.bar", "/a.b"), "/a.b[/a.b[/a.b].attr].attr")
AssertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]").ReplacePrefix("/foo", "/a/b"), "/a/b.rel[/a/b/prim].relAttr.mapper[/a/b/prim.attr]")
AssertEqual(Sdf.Path("bar.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]").ReplacePrefix("/foo", "/a/b"), "bar.rel[/a/b/prim].relAttr.mapper[/a/b/prim.attr]")
AssertEqual(Sdf.Path("../bar").ReplacePrefix("..", "bar"), "bar/bar")

# test GetCommonPrefix

AssertEqual(Sdf.Path("/a").GetCommonPrefix("/a"), "/a")
AssertEqual(Sdf.Path("/a/b").GetCommonPrefix("/a/b"), "/a/b")
AssertEqual(Sdf.Path("/a.b").GetCommonPrefix("/a.b"), "/a.b")
AssertEqual(Sdf.Path("/a/b").GetCommonPrefix("/b/a"), "/")
AssertEqual(Sdf.Path("/a/b/c").GetCommonPrefix("/a/b"), "/a/b")
AssertEqual(Sdf.Path("/a/b").GetCommonPrefix("/a/b/c"), "/a/b")
AssertEqual(Sdf.Path("/a/b.c").GetCommonPrefix("/a/b"), "/a/b")
AssertEqual(Sdf.Path("/a/b").GetCommonPrefix("/a/b.c"), "/a/b")
AssertEqual(Sdf.Path("/foo.bar[/foo].attr").GetCommonPrefix("/foo.foo[/foo].attr"), "/foo")
AssertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]").GetCommonPrefix("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]"), "/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]")
AssertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr].x").GetCommonPrefix("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr].y"), "/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]")
AssertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr1]").GetCommonPrefix("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr2]"), "/foo.rel[/foo/prim].relAttr")
AssertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]").GetCommonPrefix("/foo.rel[/foo/prim2].relAttr.mapper[/foo/prim.attr]"), "/foo.rel")

# test MakeAbsolutePath

AssertEqual(Sdf.Path("..").MakeAbsolutePath("/foo/bar"), "/foo")
AssertEqual(Sdf.Path("/A/B/C/D.rel[../../E.rel[F.attr].attr].attr").MakeAbsolutePath("/A/B"), "/A/B/C/D.rel[/A/B/E.rel[/A/B/E/F.attr].attr].attr")
AssertEqual(Sdf.Path("C/D.rel[../../E.rel[F.attr].attr].attr").MakeAbsolutePath("/A/B"), "/A/B/C/D.rel[/A/B/E.rel[/A/B/E/F.attr].attr].attr")
AssertEqual(Sdf.Path("../.attr").MakeAbsolutePath("/foo/bar"), "/foo.attr")
AssertEqual(Sdf.Path("../.attr[1 & 2]").MakeAbsolutePath("/foo/bar"), "/foo.attr[1 & 2]")
AssertEqual(Sdf.Path.emptyPath.MakeAbsolutePath("/foo/bar"), Sdf.Path.emptyPath)
AssertEqual(Sdf.Path(".attr.mapper[/connection.target]").MakeAbsolutePath("/foo/bar"), "/foo/bar.attr.mapper[/connection.target]")
AssertEqual(Sdf.Path(".attr.mapper[/connection.target].arg").MakeAbsolutePath("/foo/bar"), "/foo/bar.attr.mapper[/connection.target].arg")
AssertEqual(Sdf.Path(".attr.expression").MakeAbsolutePath("/foo/bar"), "/foo/bar.attr.expression")

# test MakeRelativePath

AssertEqual(Sdf.Path("/foo/bar").MakeRelativePath("/foo"), "bar")
AssertEqual(Sdf.Path("/foo").MakeRelativePath("/foo"), ".")
AssertEqual(Sdf.Path("/foo").MakeRelativePath("/foo/bar"), "..")
AssertEqual(Sdf.Path.emptyPath.MakeRelativePath("/foo/bar"), Sdf.Path.emptyPath)

# test GetPrefixes

AssertEqual(Sdf.Path("/foo/bar/goo/loo").GetPrefixes(),
   ["/foo", "/foo/bar", "/foo/bar/goo", "/foo/bar/goo/loo"])

# test ReplaceTargetPath

AssertEqual(Sdf.Path("/prim.rel[foo].attr").ReplaceTargetPath("bar"), "/prim.rel[bar].attr")
AssertEqual(Sdf.Path("/prim.rel[foo].attr:baz").ReplaceTargetPath("bar"), "/prim.rel[bar].attr:baz")
AssertEqual(Sdf.Path("/prim.rel[foo].attr[1]").ReplaceTargetPath("bar"), "/prim.rel[bar].attr[1]")
AssertEqual(Sdf.Path("foo").ReplaceTargetPath("bar"), "foo")
AssertEqual(Sdf.Path("/prim.attr.mapper[/foo.target]").ReplaceTargetPath("/bar.target"), "/prim.attr.mapper[/bar.target]")
AssertEqual(Sdf.Path("/prim.attr.mapper[/foo.target].arg").ReplaceTargetPath("/bar.target"), "/prim.attr.mapper[/bar.target].arg")
AssertEqual(Sdf.Path("/prim.rel[foo].attr.expression").ReplaceTargetPath("bar"), "/prim.rel[bar].attr.expression")
AssertEqual(Sdf.Path("/prim.rel[foo].attr.expression").ReplaceTargetPath("bar:baz"), "/prim.rel[bar:baz].attr.expression")
AssertEqual(Sdf.Path("/prim.rel[/prim].relAttr.mapper[/prim.attr]").ReplaceTargetPath("/bar.attr"), "/prim.rel[/prim].relAttr.mapper[/bar.attr]")
AssertEqual(Sdf.Path("/prim.rel[/prim].relAttr.mapper[/prim.attr]").ReplaceTargetPath("/bar.attr:baz"), "/prim.rel[/prim].relAttr.mapper[/bar.attr:baz]")

# test AppendPath

AssertEqual(Sdf.Path("/prim").AppendPath("."), "/prim")
AssertEqual(Sdf.Path("/").AppendPath("foo/bar.attr"), "/foo/bar.attr")
AssertEqual(Sdf.Path("/").AppendPath("foo/bar.attr:argle:bargle"), "/foo/bar.attr:argle:bargle")
AssertEqual(Sdf.Path("/foo").AppendPath("bar.attr"), "/foo/bar.attr")
AssertEqual(Sdf.Path("/foo").AppendPath("bar.attr:argle:bargle"), "/foo/bar.attr:argle:bargle")
AssertEqual(Sdf.Path("/foo").AppendPath("bar.rel[/target].attr"), "/foo/bar.rel[/target].attr")
AssertEqual(Sdf.Path("/foo").AppendPath("bar.rel[/target].attr:argle:bargle"), "/foo/bar.rel[/target].attr:argle:bargle")
AssertEqual(Sdf.Path("/foo").AppendPath("bar.attr[/target.attr]"), "/foo/bar.attr[/target.attr]")
AssertEqual(Sdf.Path("/foo").AppendPath("bar.attr[/target.attr:argle:bargle]"), "/foo/bar.attr[/target.attr:argle:bargle]")
AssertEqual(Sdf.Path("/foo").AppendPath("bar.attr.mapper[/target].arg"), "/foo/bar.attr.mapper[/target].arg")

ExpectedErrorBegin(3)
AssertEqual(Sdf.Path("/prim").AppendPath("/absolute"), empty)
AssertEqual(Sdf.Path("/prim.attr").AppendPath("prim"), empty)
AssertEqual(Sdf.Path("/").AppendPath(".prop"), empty)
ExpectedErrorEnd()

# test various error cases
ExpectedErrorBegin()
AssertEqual( Sdf.Path(".").GetPrimPath(), empty )
AssertEqual( Sdf.Path(".foo").GetPrimPath(), empty )
AssertEqual( Sdf.Path("foo.bar").AppendChild("a"), empty )
AssertEqual( Sdf.Path("foo.bar").AppendProperty("a"), empty )
AssertEqual( Sdf.Path("foo").AppendTarget("/foo"), empty )
AssertEqual( Sdf.Path("foo").ReplacePrefix("bar", "bar"), Sdf.Path("foo") )
AssertEqual( Sdf.Path("foo").ReplacePrefix(empty, "bar"), empty )
AssertEqual( Sdf.Path("foo").ReplacePrefix("foo", empty), empty )
AssertEqual( Sdf.Path("foo").GetCommonPrefix(empty), empty )
AssertEqual( Sdf.Path("foo").MakeAbsolutePath("foo"), empty )
AssertEqual( Sdf.Path("foo").MakeAbsolutePath(empty), empty )
AssertEqual( Sdf.Path("foo").MakeRelativePath("foo"), empty )
AssertEqual( Sdf.Path("foo").MakeRelativePath(empty ), empty )
AssertEqual( Sdf.Path("foo").targetPath, empty )
AssertEqual( Sdf.Path("foo").ReplaceTargetPath(empty), empty)
AssertEqual( empty.ReplaceTargetPath("foo"), empty)
AssertEqual( Sdf.Path("foo").AppendChild("!@#$$>."), empty)
AssertEqual( Sdf.Path("foo").AppendProperty("."), empty )
AssertEqual( Sdf.Path("foo.prop[foo]").AppendRelationalAttribute("."), empty )
AssertEqual( Sdf.Path("foo").AppendMapper("/conn.target"), empty )
AssertEqual( Sdf.Path("foo.attr").AppendMapper(empty), empty )
AssertEqual( Sdf.Path("foo.attr.mapper[/conn.target]").AppendMapperArg("./bad"), empty )
AssertEqual( Sdf.Path("foo").AppendMapperArg("arg"), empty )
AssertEqual( Sdf.Path("foo").AppendExpression(), empty )

AssertEqual(Sdf.Path("foo.bar").AppendTarget(empty), empty)
AssertEqual(Sdf.Path().MakeRelativePath("foo"), empty)

# Retest all of the above Append negatives with the equivalent AppendElementString()
AssertEqual( Sdf.Path("foo.bar").AppendElementString("a"), empty )
AssertEqual( Sdf.Path("foo.bar").AppendElementString(".a"), empty )
AssertEqual( Sdf.Path("foo").AppendElementString("[/foo]"), empty )
AssertEqual( Sdf.Path("foo").AppendElementString("!@#$$>."), empty)
AssertEqual( Sdf.Path("foo").AppendElementString(".."), empty )
AssertEqual( Sdf.Path("foo.prop[foo]").AppendElementString("[.]"), empty )
AssertEqual( Sdf.Path("foo").AppendElementString(".mapper[/conn.target]"), empty )
AssertEqual( Sdf.Path("foo.attr").AppendElementString(".mapper[]"), empty )
AssertEqual( Sdf.Path("foo.attr.mapper[/conn.target]").AppendElementString("./bad"), empty )
# The following tests don't translate to element appending, because the
# textual syntax for mapper args is the same as for ordinary properties,
# which *are* legal to append to prims, and "expression" *is* a legal
# property name for a prim.
#AssertEqual( Sdf.Path("foo").AppendElementString(".arg"), empty )
#AssertEqual( Sdf.Path("foo").AppendElementString(".expression"), empty )
AssertEqual(Sdf.Path("foo.bar").AppendElementString("[]"), empty)

ExpectedErrorEnd()

AssertEqual(Sdf.Path("/foo").targetPath, empty)

# IsBuiltinMarker

Assert(all([Sdf.Path.IsBuiltInMarker(x) 
            for x in ['', 'current', 'authored', 'final', 'initial']]))

Assert(not any([Sdf.Path.IsBuiltInMarker(x) 
                for x in ['XXX', 'YYY', 'ZZZ']]))

# test GetVariantSelection, 
#      IsPrimVariantSelectionPath,
#      ContainsPrimVariantSelection
#      StripAllVariantSelections

Assert(not Sdf.Path('/foo/bar').ContainsPrimVariantSelection())

Assert(Sdf.Path('/foo/bar{var=sel}').ContainsPrimVariantSelection())
Assert(Sdf.Path('/foo/bar{var=sel}').IsPrimVariantSelectionPath())

Assert(Sdf.Path('/foo{var=sel}bar').ContainsPrimVariantSelection())
Assert(not Sdf.Path('/foo{var=sel}bar').IsPrimVariantSelectionPath())

Assert(Sdf.Path('/foo{var=sel}bar').GetVariantSelection() == ('var', 'sel'))
Assert(Sdf.Path('/foo/bar{var=sel}').GetVariantSelection() == ('var', 'sel'))

Assert(Sdf.Path('/foo/bar').StripAllVariantSelections() == (Sdf.Path('/foo/bar')))
Assert(Sdf.Path('/foo/bar{var=sel}').StripAllVariantSelections() 
       == (Sdf.Path('/foo/bar')))

# XXX work around Path parser failure:
p = Sdf.Path('/foo/bar{var=sel}baz/frob')
Assert(p.StripAllVariantSelections() == Sdf.Path('/foo/bar/baz/frob'))
p = Sdf.Path('/foo{bond=connery}bar{captain=picard}baz/frob{doctor=tennent}')
Assert(p.StripAllVariantSelections() == Sdf.Path('/foo/bar/baz/frob'))

AssertEqual(Sdf.Path.TokenizeIdentifier(""), [])
AssertEqual(Sdf.Path.TokenizeIdentifier("foo"), ["foo"])
AssertEqual(Sdf.Path.TokenizeIdentifier("foo::baz"), [])
AssertEqual(Sdf.Path.TokenizeIdentifier("foo:bar:baz"), ["foo","bar","baz"])

AssertEqual(Sdf.Path.JoinIdentifier([]), "")
AssertEqual(Sdf.Path.JoinIdentifier(["foo"]), "foo")
AssertEqual(Sdf.Path.JoinIdentifier(["foo","bar","baz"]), "foo:bar:baz")
AssertEqual(Sdf.Path.StripNamespace(Sdf.Path.JoinIdentifier([])), "")
AssertEqual(Sdf.Path.StripNamespace(Sdf.Path.JoinIdentifier(["foo"])), "foo")
AssertEqual(Sdf.Path.StripNamespace(Sdf.Path.JoinIdentifier(["foo","bar","baz"])), "baz")

AssertEqual(Sdf.Path.JoinIdentifier("", ""), "")
AssertEqual(Sdf.Path.JoinIdentifier("foo", ""), "foo")
AssertEqual(Sdf.Path.JoinIdentifier("", "foo"), "foo")
AssertEqual(Sdf.Path.JoinIdentifier("foo","bar"), "foo:bar")
AssertEqual(Sdf.Path.JoinIdentifier("foo:bar","baz"), "foo:bar:baz")
AssertEqual(Sdf.Path.JoinIdentifier("foo","baz:blah"), "foo:baz:blah")
AssertEqual(Sdf.Path.JoinIdentifier("foo:bar","baz:blah"), "foo:bar:baz:blah")
