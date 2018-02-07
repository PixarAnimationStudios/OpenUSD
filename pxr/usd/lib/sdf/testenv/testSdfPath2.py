#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

import sys, os, traceback, unittest
from pxr import Sdf, Tf

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
    assert path == path2

    # Check properties
    assert path.pathString == pathStr
    assert path.name == name
    assert path.pathElementCount == len(pathElems)
    assert path.targetPath == targetPath

    # Check queries
    assert path.IsAbsolutePath() == pathStr.startswith("/")
    assert path.IsPrimPath() == (((pathFlags & PrimType) or (path == Sdf.Path.reflexiveRelativePath)) != 0 )
    assert path.IsRootPrimPath() == ((pathFlags & PrimType) != 0 and (len(pathElems) is 1))
    assert path.IsPropertyPath() == ((pathFlags & PropertyType) != 0)
    if (pathFlags & NamespacedPropertyType) != 0:
        assert path.IsNamespacedPropertyPath() == ((pathFlags & NamespacedPropertyType) != 0)
    if (pathFlags & PrimPropertyType) != 0:
        assert path.IsPrimPropertyPath() == ((pathFlags & PrimPropertyType) != 0)
    if (pathFlags & RelationalAttributeType) != 0:
        assert path.IsRelationalAttributePath() == ((pathFlags & RelationalAttributeType) != 0)
        assert path.IsTargetPath() == ((pathFlags & TargetType) != 0)
        assert path.IsMapperPath() == ((pathFlags & MapperType) != 0)
        assert path.IsMapperArgPath() == ((pathFlags & MapperArgType) != 0)
        assert path.IsExpressionPath() == ((pathFlags & ExpressionType) != 0)

    if pathFlags & (TargetType | MapperType | MapperArgType):
        assert path.ContainsTargetPath()

    # Check path elements
    prefixes = Sdf._PathElemsToPrefixes(path.IsAbsolutePath(), pathElems)
    assert path.GetPrefixes() == prefixes

    # Check parent
    assert path.GetParentPath() == parentPath
    
    # Make sure all parent prefixes are recognized as prefixes, and that
    # each prefix (including terminal) is reconstructible from its string
    # element representation
    currPath = path
    while (parentPath != Sdf.Path.emptyPath) and (parentPath.name != Sdf.Path.parentPathElement):
        currElement = currPath.elementString
        reconPath = parentPath.AppendElementString(currElement)
        assert(path.HasPrefix(parentPath))
        assert reconPath == currPath
        currPath = parentPath
        parentPath = parentPath.GetParentPath()

def CheckEmptyPath(path):
    _CheckPath(path, Sdf.Path.emptyPath, Sdf.Path.emptyPath, "", [], EmptyType, "", Sdf.Path.emptyPath)
    assert path == Sdf.Path()
    
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
        assert args[i] < args[i+1], "Expected <%s> to compare less-than <%s>, but it did not"%(args[i], args[i+1])
    
class TestSdfPath2(unittest.TestCase):
    def test_Basic(self):
        # ========================================================================
        # Test some basic construction
        # ========================================================================
        
        # --- Check Empty path
        empty = Sdf.Path.emptyPath
        
        CheckEmptyPath(empty)
        self.assertFalse(empty.IsPrimPropertyPath())
        self.assertFalse(empty.IsNamespacedPropertyPath())
        self.assertFalse(empty.IsRelationalAttributePath())
        self.assertFalse(empty < empty, "Less-than check failed")
        
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
        
        # self.assertEqual(Path(".").GetPrimPath(),                   ".")
        self.assertEqual(Sdf.Path("foo").GetPrimPath(),                 "foo")
        self.assertEqual(Sdf.Path("foo/bar").GetPrimPath(),             "foo/bar")
        self.assertEqual(Sdf.Path("foo.bar").GetPrimPath(),             "foo")
        self.assertEqual(Sdf.Path("foo.bar[foo].bar").GetPrimPath(),    "foo")
        
        # test ReplacePrefix
        
        self.assertEqual(Sdf.Path("/a").ReplacePrefix("/a", "/b"), "/b")
        self.assertEqual(Sdf.Path("/a/b").ReplacePrefix("/a", "/b"), "/b/b")
        self.assertEqual(Sdf.Path("a").ReplacePrefix("a", "b"), "b")
        self.assertEqual(Sdf.Path("foo/bar").ReplacePrefix("foo", "bar"), "bar/bar")
        self.assertEqual(Sdf.Path("foo.bar").ReplacePrefix("foo", "bar"), "bar.bar")
        self.assertEqual(Sdf.Path("foo.bar:baz").ReplacePrefix("foo", "bar"), "bar.bar:baz")
        self.assertEqual(Sdf.Path("foo.bar[/foo].attr").ReplacePrefix("foo.bar", "a.b"), "a.b[/foo].attr")
        self.assertEqual(Sdf.Path("/foo.bar[foo.bar].attr").ReplacePrefix("foo.bar", "a.b"), "/foo.bar[a.b].attr")
        self.assertEqual(Sdf.Path("/foo.bar[foo.bar].attr:baz").ReplacePrefix("foo.bar", "a.b"), "/foo.bar[a.b].attr:baz")
        self.assertEqual(Sdf.Path("/foo.bar[/foo.bar].attr").ReplacePrefix("/foo.bar", "/a.b"), "/a.b[/a.b].attr")
        self.assertEqual(Sdf.Path("/foo.bar[/foo.bar[/foo.bar].attr].attr").ReplacePrefix("/foo.bar", "/a.b"), "/a.b[/a.b[/a.b].attr].attr")
        self.assertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]").ReplacePrefix("/foo", "/a/b"), "/a/b.rel[/a/b/prim].relAttr.mapper[/a/b/prim.attr]")
        self.assertEqual(Sdf.Path("bar.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]").ReplacePrefix("/foo", "/a/b"), "bar.rel[/a/b/prim].relAttr.mapper[/a/b/prim.attr]")
        self.assertEqual(Sdf.Path("../bar").ReplacePrefix("..", "bar"), "bar/bar")
        
        # test GetCommonPrefix
        
        self.assertEqual(Sdf.Path("/a").GetCommonPrefix("/a"), "/a")
        self.assertEqual(Sdf.Path("/a/b").GetCommonPrefix("/a/b"), "/a/b")
        self.assertEqual(Sdf.Path("/a.b").GetCommonPrefix("/a.b"), "/a.b")
        self.assertEqual(Sdf.Path("/a/b").GetCommonPrefix("/b/a"), "/")
        self.assertEqual(Sdf.Path("/a/b/c").GetCommonPrefix("/a/b"), "/a/b")
        self.assertEqual(Sdf.Path("/a/b").GetCommonPrefix("/a/b/c"), "/a/b")
        self.assertEqual(Sdf.Path("/a/b.c").GetCommonPrefix("/a/b"), "/a/b")
        self.assertEqual(Sdf.Path("/a/b").GetCommonPrefix("/a/b.c"), "/a/b")
        self.assertEqual(Sdf.Path("/foo.bar[/foo].attr").GetCommonPrefix("/foo.foo[/foo].attr"), "/foo")
        self.assertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]").GetCommonPrefix("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]"), "/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]")
        self.assertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr].x").GetCommonPrefix("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr].y"), "/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]")
        self.assertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr1]").GetCommonPrefix("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr2]"), "/foo.rel[/foo/prim].relAttr")
        self.assertEqual(Sdf.Path("/foo.rel[/foo/prim].relAttr.mapper[/foo/prim.attr]").GetCommonPrefix("/foo.rel[/foo/prim2].relAttr.mapper[/foo/prim.attr]"), "/foo.rel")
        
        # test MakeAbsolutePath
        
        self.assertEqual(Sdf.Path("..").MakeAbsolutePath("/foo/bar"), "/foo")
        self.assertEqual(Sdf.Path("/A/B/C/D.rel[../../E.rel[F.attr].attr].attr").MakeAbsolutePath("/A/B"), "/A/B/C/D.rel[/A/B/E.rel[/A/B/E/F.attr].attr].attr")
        self.assertEqual(Sdf.Path("C/D.rel[../../E.rel[F.attr].attr].attr").MakeAbsolutePath("/A/B"), "/A/B/C/D.rel[/A/B/E.rel[/A/B/E/F.attr].attr].attr")
        self.assertEqual(Sdf.Path("../.attr").MakeAbsolutePath("/foo/bar"), "/foo.attr")
        self.assertEqual(Sdf.Path("../.attr[1 & 2]").MakeAbsolutePath("/foo/bar"), "/foo.attr[1 & 2]")
        self.assertEqual(Sdf.Path.emptyPath.MakeAbsolutePath("/foo/bar"), Sdf.Path.emptyPath)
        self.assertEqual(Sdf.Path(".attr.mapper[/connection.target]").MakeAbsolutePath("/foo/bar"), "/foo/bar.attr.mapper[/connection.target]")
        self.assertEqual(Sdf.Path(".attr.mapper[/connection.target].arg").MakeAbsolutePath("/foo/bar"), "/foo/bar.attr.mapper[/connection.target].arg")
        self.assertEqual(Sdf.Path(".attr.expression").MakeAbsolutePath("/foo/bar"), "/foo/bar.attr.expression")
        
        # test MakeRelativePath
        

        self.assertEqual(Sdf.Path("/foo/bar").MakeRelativePath("/foo"), "bar")
        self.assertEqual(Sdf.Path("/foo").MakeRelativePath("/foo"), ".")
        self.assertEqual(Sdf.Path("/foo").MakeRelativePath("/foo/bar"), "..")
        self.assertEqual(Sdf.Path.emptyPath.MakeRelativePath("/foo/bar"), Sdf.Path.emptyPath)
        
        # test GetPrefixes
        
        self.assertEqual(Sdf.Path("/foo/bar/goo/loo").GetPrefixes(),
           ["/foo", "/foo/bar", "/foo/bar/goo", "/foo/bar/goo/loo"])
        
        # test ReplaceTargetPath
        
        self.assertEqual(Sdf.Path("/prim.rel[foo].attr").ReplaceTargetPath("bar"), "/prim.rel[bar].attr")
        self.assertEqual(Sdf.Path("/prim.rel[foo].attr:baz").ReplaceTargetPath("bar"), "/prim.rel[bar].attr:baz")
        self.assertEqual(Sdf.Path("/prim.rel[foo].attr[1]").ReplaceTargetPath("bar"), "/prim.rel[bar].attr[1]")
        self.assertEqual(Sdf.Path("foo").ReplaceTargetPath("bar"), "foo")
        self.assertEqual(Sdf.Path("/prim.attr.mapper[/foo.target]").ReplaceTargetPath("/bar.target"), "/prim.attr.mapper[/bar.target]")
        self.assertEqual(Sdf.Path("/prim.attr.mapper[/foo.target].arg").ReplaceTargetPath("/bar.target"), "/prim.attr.mapper[/bar.target].arg")
        self.assertEqual(Sdf.Path("/prim.rel[foo].attr.expression").ReplaceTargetPath("bar"), "/prim.rel[bar].attr.expression")
        self.assertEqual(Sdf.Path("/prim.rel[foo].attr.expression").ReplaceTargetPath("bar:baz"), "/prim.rel[bar:baz].attr.expression")
        self.assertEqual(Sdf.Path("/prim.rel[/prim].relAttr.mapper[/prim.attr]").ReplaceTargetPath("/bar.attr"), "/prim.rel[/prim].relAttr.mapper[/bar.attr]")
        self.assertEqual(Sdf.Path("/prim.rel[/prim].relAttr.mapper[/prim.attr]").ReplaceTargetPath("/bar.attr:baz"), "/prim.rel[/prim].relAttr.mapper[/bar.attr:baz]")
        
        # test AppendPath
        
        self.assertEqual(Sdf.Path("/prim").AppendPath("."), "/prim")
        self.assertEqual(Sdf.Path("/").AppendPath("foo/bar.attr"), "/foo/bar.attr")
        self.assertEqual(Sdf.Path("/").AppendPath("foo/bar.attr:argle:bargle"), "/foo/bar.attr:argle:bargle")
        self.assertEqual(Sdf.Path("/foo").AppendPath("bar.attr"), "/foo/bar.attr")
        self.assertEqual(Sdf.Path("/foo").AppendPath("bar.attr:argle:bargle"), "/foo/bar.attr:argle:bargle")
        self.assertEqual(Sdf.Path("/foo").AppendPath("bar.rel[/target].attr"), "/foo/bar.rel[/target].attr")
        self.assertEqual(Sdf.Path("/foo").AppendPath("bar.rel[/target].attr:argle:bargle"), "/foo/bar.rel[/target].attr:argle:bargle")
        self.assertEqual(Sdf.Path("/foo").AppendPath("bar.attr[/target.attr]"), "/foo/bar.attr[/target.attr]")
        self.assertEqual(Sdf.Path("/foo").AppendPath("bar.attr[/target.attr:argle:bargle]"), "/foo/bar.attr[/target.attr:argle:bargle]")
        self.assertEqual(Sdf.Path("/foo").AppendPath("bar.attr.mapper[/target].arg"), "/foo/bar.attr.mapper[/target].arg")
        
        self.assertEqual(Sdf.Path("/prim").AppendPath("/absolute"), empty)
        self.assertEqual(Sdf.Path("/prim.attr").AppendPath("prim"), empty)
        self.assertEqual(Sdf.Path("/").AppendPath(".prop"), empty)
        
        # test various error cases
        self.assertEqual( Sdf.Path(".").GetPrimPath(), empty )
        self.assertEqual( Sdf.Path(".foo").GetPrimPath(), empty )
        self.assertEqual( Sdf.Path("foo.bar").AppendChild("a"), empty )
        self.assertEqual( Sdf.Path("foo.bar").AppendProperty("a"), empty )
        self.assertEqual( Sdf.Path("foo").AppendTarget("/foo"), empty )
        self.assertEqual( Sdf.Path("foo").ReplacePrefix("bar", "bar"), Sdf.Path("foo") )
        self.assertEqual( Sdf.Path("foo").ReplacePrefix(empty, "bar"), empty )
        self.assertEqual( Sdf.Path("foo").ReplacePrefix("foo", empty), empty )
        self.assertEqual( Sdf.Path("foo").GetCommonPrefix(empty), empty )
        self.assertEqual( Sdf.Path("foo").MakeAbsolutePath("foo"), empty )
        self.assertEqual( Sdf.Path("foo").MakeAbsolutePath(empty), empty )
        self.assertEqual( Sdf.Path("foo").MakeRelativePath("foo"), empty )
        self.assertEqual( Sdf.Path("foo").MakeRelativePath(empty ), empty )
        self.assertEqual( Sdf.Path("foo").targetPath, empty )
        self.assertEqual( Sdf.Path("foo").ReplaceTargetPath(empty), empty)
        self.assertEqual( empty.ReplaceTargetPath("foo"), empty)
        self.assertEqual( Sdf.Path("foo").AppendChild("!@#$$>."), empty)
        self.assertEqual( Sdf.Path("foo").AppendProperty("."), empty )
        self.assertEqual( Sdf.Path("foo.prop[foo]").AppendRelationalAttribute("."), empty )
        self.assertEqual( Sdf.Path("foo").AppendMapper("/conn.target"), empty )
        self.assertEqual( Sdf.Path("foo.attr").AppendMapper(empty), empty )
        self.assertEqual( Sdf.Path("foo.attr.mapper[/conn.target]").AppendMapperArg("./bad"), empty )
        self.assertEqual( Sdf.Path("foo").AppendMapperArg("arg"), empty )
        self.assertEqual( Sdf.Path("foo").AppendExpression(), empty )
        
        self.assertEqual(Sdf.Path("foo.bar").AppendTarget(empty), empty)
        self.assertEqual(Sdf.Path().MakeRelativePath("foo"), empty)
        
        # Retest all of the above Append negatives with the equivalent AppendElementString()
        self.assertEqual( Sdf.Path("foo.bar").AppendElementString("a"), empty )
        self.assertEqual( Sdf.Path("foo.bar").AppendElementString(".a"), empty )
        self.assertEqual( Sdf.Path("foo").AppendElementString("[/foo]"), empty )
        self.assertEqual( Sdf.Path("foo").AppendElementString("!@#$$>."), empty)
        self.assertEqual( Sdf.Path("foo").AppendElementString(".."), empty )
        self.assertEqual( Sdf.Path("foo.prop[foo]").AppendElementString("[.]"), empty )
        self.assertEqual( Sdf.Path("foo").AppendElementString(".mapper[/conn.target]"), empty )
        self.assertEqual( Sdf.Path("foo.attr").AppendElementString(".mapper[]"), empty )
        self.assertEqual( Sdf.Path("foo.attr.mapper[/conn.target]").AppendElementString("./bad"), empty )
        # The following tests don't translate to element appending, because the
        # textual syntax for mapper args is the same as for ordinary properties,
        # which *are* legal to append to prims, and "expression" *is* a legal
        # property name for a prim.
        #self.assertEqual( Sdf.Path("foo").AppendElementString(".arg"), empty )
        #self.assertEqual( Sdf.Path("foo").AppendElementString(".expression"), empty )
        self.assertEqual(Sdf.Path("foo.bar").AppendElementString("[]"), empty)
        
        
        self.assertEqual(Sdf.Path("/foo").targetPath, empty)
        
        # IsBuiltinMarker
        
        self.assertTrue(all([Sdf.Path.IsBuiltInMarker(x) 
                    for x in ['', 'current', 'authored', 'final', 'initial']]))
        
        self.assertTrue(not any([Sdf.Path.IsBuiltInMarker(x) 
                          for x in ['XXX', 'YYY', 'ZZZ']]))
        
        # test GetVariantSelection, 
        #      IsPrimVariantSelectionPath,
        #      ContainsPrimVariantSelection
        #      StripAllVariantSelections
        
        self.assertTrue(not Sdf.Path('/foo/bar').ContainsPrimVariantSelection())
        self.assertTrue(Sdf.Path('/foo/bar{var=sel}').ContainsPrimVariantSelection())
        self.assertTrue(Sdf.Path('/foo/bar{var=sel}').IsPrimVariantSelectionPath())
        self.assertTrue(Sdf.Path('/foo{var=sel}bar').ContainsPrimVariantSelection())
        self.assertTrue(not Sdf.Path('/foo{var=sel}bar').IsPrimVariantSelectionPath())
        self.assertEqual(Sdf.Path('/foo{var=sel}bar').GetVariantSelection(), ('', ''))
        self.assertEqual(Sdf.Path('/foo{var=sel}bar').GetParentPath().GetVariantSelection(), ('var', 'sel'))
        self.assertEqual(Sdf.Path('/foo/bar{var=sel}').GetVariantSelection(), ('var', 'sel'))
        self.assertEqual(Sdf.Path('/foo/bar').StripAllVariantSelections(), (Sdf.Path('/foo/bar')))
        self.assertEqual(Sdf.Path('/foo/bar{var=sel}').StripAllVariantSelections(),
              (Sdf.Path('/foo/bar')))
        
        # XXX work around Path parser failure:
        p = Sdf.Path('/foo/bar{var=sel}baz/frob')
        self.assertEqual(p.StripAllVariantSelections(), Sdf.Path('/foo/bar/baz/frob'))
        p = Sdf.Path('/foo{bond=connery}bar{captain=picard}baz/frob{doctor=tennent}')
        self.assertEqual(p.StripAllVariantSelections(), Sdf.Path('/foo/bar/baz/frob'))
        
        self.assertEqual(Sdf.Path.TokenizeIdentifier(""), [])
        self.assertEqual(Sdf.Path.TokenizeIdentifier("foo"), ["foo"])
        self.assertEqual(Sdf.Path.TokenizeIdentifier("foo::baz"), [])
        self.assertEqual(Sdf.Path.TokenizeIdentifier("foo:bar:baz"), ["foo","bar","baz"])
        
        self.assertEqual(Sdf.Path.JoinIdentifier([]), "")
        self.assertEqual(Sdf.Path.JoinIdentifier(["foo"]), "foo")
        self.assertEqual(Sdf.Path.JoinIdentifier(["foo","bar","baz"]), "foo:bar:baz")
        self.assertEqual(Sdf.Path.JoinIdentifier(["foo","","baz"]), "foo:baz")
        self.assertEqual(Sdf.Path.JoinIdentifier(["foo","bar",""]), "foo:bar")
        self.assertEqual(Sdf.Path.JoinIdentifier(["","bar","baz"]), "bar:baz")
        self.assertEqual(Sdf.Path.JoinIdentifier(["","bar",""]), "bar")
        
        self.assertEqual(Sdf.Path.StripNamespace(Sdf.Path.JoinIdentifier([])), "")
        self.assertEqual(Sdf.Path.StripNamespace(Sdf.Path.JoinIdentifier(["foo"])), "foo")
        self.assertEqual(Sdf.Path.StripNamespace(Sdf.Path.JoinIdentifier(["foo","bar","baz"])), "baz")
        
        self.assertEqual(Sdf.Path.JoinIdentifier("", ""), "")
        self.assertEqual(Sdf.Path.JoinIdentifier("foo", ""), "foo")
        self.assertEqual(Sdf.Path.JoinIdentifier("", "foo"), "foo")
        self.assertEqual(Sdf.Path.JoinIdentifier("foo","bar"), "foo:bar")
        self.assertEqual(Sdf.Path.JoinIdentifier("foo:bar","baz"), "foo:bar:baz")
        self.assertEqual(Sdf.Path.JoinIdentifier("foo","baz:blah"), "foo:baz:blah")
        self.assertEqual(Sdf.Path.JoinIdentifier("foo:bar","baz:blah"), "foo:bar:baz:blah")
        
        ########################################################################
        #
        # Nested variant selections
        #
        
        p1 = Sdf.Path('/a')
        p2 = p1.AppendVariantSelection('x','a')
        p3 = p2.AppendVariantSelection('y','b')
        
        # It is allowable to make repeated selections for the same variant set.
        p2x = p2.AppendVariantSelection('x','a')
        p3x = p3.AppendVariantSelection('x','a')
        self.assertEqual( p2x, Sdf.Path('/a{x=a}{x=a}') )
        self.assertEqual( p3x, Sdf.Path('/a{x=a}{y=b}{x=a}') )
        
        self.assertTrue( not p1.ContainsPrimVariantSelection() )
        self.assertTrue( p2.ContainsPrimVariantSelection() )
        self.assertTrue( p3.ContainsPrimVariantSelection() )
        
        self.assertEqual( p2.GetVariantSelection(), ('x','a') )
        self.assertEqual( p3.GetVariantSelection(), ('y','b') )
        
        self.assertEqual( p1, Sdf.Path('/a') )
        self.assertEqual( p2, Sdf.Path('/a{x=a}') )
        self.assertEqual( p3, Sdf.Path('/a{x=a}{y=b}') )
        
        self.assertEqual( p1, eval(repr(p1)) )
        self.assertEqual( p2, eval(repr(p2)) )
        self.assertEqual( p3, eval(repr(p3)) )
        
        self.assertEqual( p1, p2.GetParentPath() )
        self.assertEqual( p2, p3.GetParentPath() )
        
        self.assertTrue( p3.HasPrefix(p2) )
        self.assertTrue( p2.HasPrefix(p1) )
        
        self.assertEqual( p3.ReplacePrefix(p2, '/b'), Sdf.Path('/b{y=b}') )

if __name__ == "__main__":
    unittest.main()
