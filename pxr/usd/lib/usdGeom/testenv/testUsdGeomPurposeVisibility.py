#!/pxrpythonsubst

import sys, os
from pxr import Sdf, Usd, UsdGeom

import Mentor.Runtime
from Mentor.Runtime import (AssertEqual, FindDataFile)

def TestCompute():
    stage = Usd.Stage.CreateInMemory()

    print "Ensuring non-imageable prims with no opinions evaluate to defaults"

    ni_Root  = stage.DefinePrim("/ni_Root")
    ni_sub   =  stage.DefinePrim("/ni_Root/ni_Sub")
    ni_leaf  =  stage.DefinePrim("/ni_Root/ni_Sub/ni_leaf")
    img = UsdGeom.Imageable(ni_Root)
    AssertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                img.GetPrim().GetPath())
    AssertEqual(img.ComputePurpose(), UsdGeom.Tokens.default_, 
                img.GetPrim().GetPath())
    img = UsdGeom.Imageable(ni_sub)
    AssertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                img.GetPrim().GetPath())
    AssertEqual(img.ComputePurpose(), UsdGeom.Tokens.default_, 
                img.GetPrim().GetPath())
    img = UsdGeom.Imageable(ni_leaf)
    AssertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                img.GetPrim().GetPath())
    AssertEqual(img.ComputePurpose(), UsdGeom.Tokens.default_, 
                img.GetPrim().GetPath())
    
    print "Ensuring non-imageable prims WITH opinions STILL evaluate to defaults"
    ni_sub.CreateAttribute(UsdGeom.Tokens.visibility, Sdf.ValueTypeNames.Token).Set(UsdGeom.Tokens.invisible)
    ni_sub.CreateAttribute(UsdGeom.Tokens.purpose, Sdf.ValueTypeNames.Token).Set(UsdGeom.Tokens.render)
    
    img = UsdGeom.Imageable(ni_sub)
    AssertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                img.GetPrim().GetPath())
    AssertEqual(img.ComputePurpose(), UsdGeom.Tokens.default_, 
                img.GetPrim().GetPath())
    img = UsdGeom.Imageable(ni_leaf)
    AssertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                img.GetPrim().GetPath())
    AssertEqual(img.ComputePurpose(), UsdGeom.Tokens.default_, 
                img.GetPrim().GetPath())
    
    print "Ensuring imageable leaf prim can have opinions"
    i_Root  = UsdGeom.Scope.Define(stage,  "/i_Root")
    i_sub   =  UsdGeom.Scope.Define(stage, "/i_Root/i_Sub")
    i_leaf  =  UsdGeom.Scope.Define(stage, "/i_Root/i_Sub/i_leaf")
    
    i_leaf.GetPurposeAttr().Set(UsdGeom.Tokens.guide)
    i_leaf.GetVisibilityAttr().Set(UsdGeom.Tokens.invisible)
    
    AssertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                i_leaf.GetPrim().GetPath())
    AssertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.guide, 
                i_leaf.GetPrim().GetPath())
    
    print "Ensuring imageable leaf prim is shadowed by Imageable parent opinions"
    i_leaf.GetVisibilityAttr().Set(UsdGeom.Tokens.inherited)
    i_sub.GetPurposeAttr().Set(UsdGeom.Tokens.proxy)
    i_sub.GetVisibilityAttr().Set(UsdGeom.Tokens.invisible)
    
    AssertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                i_leaf.GetPrim().GetPath())
    AssertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.proxy, 
                i_leaf.GetPrim().GetPath())
    
    print "Ensuring imageable leaf prim is NOT shadowed by non-Imageable parent opinions"
    i_sub.GetPrim().SetTypeName('')
    
    AssertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                i_leaf.GetPrim().GetPath())
    AssertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.guide, 
                i_leaf.GetPrim().GetPath())
    
    print "Ensuring imageable most ancestral imageable opinion wins when there are many"
    i_sub.GetPrim().SetTypeName('Scope')
    # The fallbacks are special, let's make sure authoring them doesn't change 
    # results - i.e. values authored on i_sub should win
    i_Root.GetPurposeAttr().Set(UsdGeom.Tokens.default_)
    i_Root.GetVisibilityAttr().Set(UsdGeom.Tokens.inherited)
    
    AssertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                i_leaf.GetPrim().GetPath())
    AssertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.proxy, 
                i_leaf.GetPrim().GetPath())
    
    i_Root.GetPurposeAttr().Set(UsdGeom.Tokens.render)
    i_Root.GetVisibilityAttr().Set(UsdGeom.Tokens.invisible)
    
    AssertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                i_leaf.GetPrim().GetPath())
    AssertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.render, 
                i_leaf.GetPrim().GetPath())
    
    
def TestMakeVisInvis():
    stage = Usd.Stage.CreateInMemory()

    root  = UsdGeom.Scope.Define(stage,  "/Root")
    sub   =  UsdGeom.Scope.Define(stage, "/Root/Sub")
    leaf  =  UsdGeom.Scope.Define(stage, "/Root/Sub/leaf")

    AssertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                root.GetPrim().GetPath())

    print 'Test that making a root invisible makes all the prims invisible.'
    root.MakeInvisible()
    AssertEqual(root.ComputeVisibility(), UsdGeom.Tokens.invisible,
                root.GetPrim().GetPath())
    AssertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.invisible,
                sub.GetPrim().GetPath())
    AssertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.invisible,
                leaf.GetPrim().GetPath())

    print 'Test that making the leaf visible causes everything to become visible.'
    leaf.MakeVisible(Usd.TimeCode.Default())
    AssertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                root.GetPrim().GetPath())
    AssertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                sub.GetPrim().GetPath())
    AssertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.inherited,
                leaf.GetPrim().GetPath())
    
    print 'Test that making the subscope invisible causes only the subscope and the leaf to be invisisible. Not the root.'
    sub.MakeInvisible()
    AssertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                root.GetPrim().GetPath())
    AssertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.invisible,
                sub.GetPrim().GetPath())
    AssertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.invisible,
                leaf.GetPrim().GetPath())

    print 'Test invising just the leaf.'
    leaf.MakeInvisible()
    sub.MakeVisible()
    AssertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                root.GetPrim().GetPath())
    AssertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                sub.GetPrim().GetPath())
    AssertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.invisible,
                leaf.GetPrim().GetPath())

    print 'Test vising everything again.'
    root.MakeVisible()
    leaf.MakeVisible()
    AssertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                root.GetPrim().GetPath())
    AssertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                sub.GetPrim().GetPath())
    AssertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.inherited,
                leaf.GetPrim().GetPath())

    print 'Test with a couple of new subtrees.'
    root2  =  UsdGeom.Scope.Define(stage, "/Root2")
    sub2   =  UsdGeom.Scope.Define(stage, "/Root/Sub2")
    leaf2  =  UsdGeom.Scope.Define(stage, "/Root/Sub2/Leaf2")
    sub3   =  UsdGeom.Scope.Define(stage, "/Root/Sub3")
    leaf3  =  UsdGeom.Scope.Define(stage, "/Root/Sub3/Leaf3")

    root.MakeInvisible()
    leaf.MakeVisible()
    leaf3.MakeVisible()
    AssertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                root.GetPrim().GetPath())
    AssertEqual(root2.ComputeVisibility(), UsdGeom.Tokens.inherited,
                root2.GetPrim().GetPath())
    AssertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                sub.GetPrim().GetPath())
    AssertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.inherited,
                leaf.GetPrim().GetPath())
    AssertEqual(sub2.ComputeVisibility(), UsdGeom.Tokens.invisible,
                sub2.GetPrim().GetPath())
    AssertEqual(leaf2.ComputeVisibility(), UsdGeom.Tokens.invisible,
                leaf2.GetPrim().GetPath())
    AssertEqual(sub3.ComputeVisibility(), UsdGeom.Tokens.inherited,
                sub3.GetPrim().GetPath())
    AssertEqual(leaf3.ComputeVisibility(), UsdGeom.Tokens.inherited,
                leaf3.GetPrim().GetPath())
    
    # Create the following prim structure:
    #       A
    #       |
    #       B
    #      / \
    #     C   D
    #     |
    #     E
    # Make A invisible and then make E visible. Test that D remains invisible.
    print 'Test preservation of visibility state.'
    a = UsdGeom.Scope.Define(stage, "/A")
    b = UsdGeom.Scope.Define(stage, "/A/B")
    c = UsdGeom.Scope.Define(stage, "/A/B/C")
    d = UsdGeom.Scope.Define(stage, "/A/B/D")
    e = UsdGeom.Scope.Define(stage, "/A/B/C/E")
    a.MakeInvisible()
    e.MakeVisible()
    AssertEqual(a.ComputeVisibility(), UsdGeom.Tokens.inherited, a.GetPath())
    AssertEqual(b.ComputeVisibility(), UsdGeom.Tokens.inherited, b.GetPath())
    AssertEqual(c.ComputeVisibility(), UsdGeom.Tokens.inherited, c.GetPath())
    AssertEqual(d.ComputeVisibility(), UsdGeom.Tokens.invisible, d.GetPath())
    AssertEqual(e.ComputeVisibility(), UsdGeom.Tokens.inherited, e.GetPath())

    print 'Test non-default visibility authoring.'
    d.MakeVisible()
    a.MakeInvisible(1.0)
    e.MakeVisible(1.0)
    
    AssertEqual(a.ComputeVisibility(1.0), UsdGeom.Tokens.inherited, a.GetPath())
    AssertEqual(b.ComputeVisibility(1.0), UsdGeom.Tokens.inherited, b.GetPath())
    AssertEqual(c.ComputeVisibility(1.0), UsdGeom.Tokens.inherited, c.GetPath())
    AssertEqual(e.ComputeVisibility(1.0), UsdGeom.Tokens.inherited, e.GetPath())
    AssertEqual(d.ComputeVisibility(1.0), UsdGeom.Tokens.invisible, d.GetPath())

if __name__ == "__main__":
    Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)
    TestCompute()
    TestMakeVisInvis()
