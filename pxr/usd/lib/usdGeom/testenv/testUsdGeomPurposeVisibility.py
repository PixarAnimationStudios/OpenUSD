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

import sys, os, unittest
from pxr import Sdf, Usd, UsdGeom

class TestUsdGeomPurposeVisibility(unittest.TestCase):
    def test_Compute(self):
        stage = Usd.Stage.CreateInMemory()

        print "Ensuring non-imageable prims with no opinions evaluate to defaults"

        ni_Root  = stage.DefinePrim("/ni_Root")
        ni_sub   =  stage.DefinePrim("/ni_Root/ni_Sub")
        ni_leaf  =  stage.DefinePrim("/ni_Root/ni_Sub/ni_leaf")
        img = UsdGeom.Imageable(ni_Root)
        self.assertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                    img.GetPrim().GetPath())
        self.assertEqual(img.ComputePurpose(), UsdGeom.Tokens.default_, 
                    img.GetPrim().GetPath())
        img = UsdGeom.Imageable(ni_sub)
        self.assertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                    img.GetPrim().GetPath())
        self.assertEqual(img.ComputePurpose(), UsdGeom.Tokens.default_, 
                    img.GetPrim().GetPath())
        img = UsdGeom.Imageable(ni_leaf)
        self.assertEqual(img.ComputeVisibility(
                    parentVisibility=UsdGeom.Tokens.inherited),
                UsdGeom.Tokens.inherited, 
                img.GetPrim().GetPath())
        self.assertEqual(img.ComputePurpose(
                        parentPurpose=UsdGeom.Tokens.default_), 
                UsdGeom.Tokens.default_, 
                img.GetPrim().GetPath())
        
        print "Ensuring non-imageable prims WITH opinions STILL evaluate to defaults"
        ni_sub.CreateAttribute(UsdGeom.Tokens.visibility, Sdf.ValueTypeNames.Token).Set(UsdGeom.Tokens.invisible)
        ni_sub.CreateAttribute(UsdGeom.Tokens.purpose, Sdf.ValueTypeNames.Token).Set(UsdGeom.Tokens.render)
        
        img = UsdGeom.Imageable(ni_sub)
        self.assertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                    img.GetPrim().GetPath())
        self.assertEqual(img.ComputePurpose(), UsdGeom.Tokens.default_, 
                    img.GetPrim().GetPath())
        img = UsdGeom.Imageable(ni_leaf)
        self.assertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                    img.GetPrim().GetPath())
        self.assertEqual(img.ComputePurpose(), UsdGeom.Tokens.default_, 
                    img.GetPrim().GetPath())
        
        print "Ensuring imageable leaf prim can have opinions"
        i_Root  = UsdGeom.Scope.Define(stage,  "/i_Root")
        i_sub   =  UsdGeom.Scope.Define(stage, "/i_Root/i_Sub")
        i_leaf  =  UsdGeom.Scope.Define(stage, "/i_Root/i_Sub/i_leaf")
        
        i_leaf.GetPurposeAttr().Set(UsdGeom.Tokens.guide)
        i_leaf.GetVisibilityAttr().Set(UsdGeom.Tokens.invisible)
        
        self.assertEqual(i_leaf.ComputeVisibility(
                    parentVisibility=UsdGeom.Tokens.inherited), 
                UsdGeom.Tokens.invisible, 
                i_leaf.GetPrim().GetPath())

        self.assertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.guide, 
                    i_leaf.GetPrim().GetPath())
        
        print "Ensuring imageable leaf prim is shadowed by Imageable parent opinions"
        i_leaf.GetVisibilityAttr().Set(UsdGeom.Tokens.inherited)
        i_sub.GetPurposeAttr().Set(UsdGeom.Tokens.proxy)
        i_sub.GetVisibilityAttr().Set(UsdGeom.Tokens.invisible)
        
        self.assertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                    i_leaf.GetPrim().GetPath())
        self.assertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.proxy, 
                    i_leaf.GetPrim().GetPath())
        
        print "Ensuring imageable leaf prim is NOT shadowed by non-Imageable parent opinions"
        i_sub.GetPrim().SetTypeName('')
        
        self.assertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                    i_leaf.GetPrim().GetPath())
        self.assertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.guide, 
                    i_leaf.GetPrim().GetPath())
        
        print "Ensuring imageable most ancestral imageable opinion wins when there are many"
        i_sub.GetPrim().SetTypeName('Scope')
        # The fallbacks are special, let's make sure authoring them doesn't change 
        # results - i.e. values authored on i_sub should win
        i_Root.GetPurposeAttr().Set(UsdGeom.Tokens.default_)
        i_Root.GetVisibilityAttr().Set(UsdGeom.Tokens.inherited)
        
        self.assertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                    i_leaf.GetPrim().GetPath())
        self.assertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.proxy, 
                    i_leaf.GetPrim().GetPath())
        
        i_Root.GetPurposeAttr().Set(UsdGeom.Tokens.render)
        i_Root.GetVisibilityAttr().Set(UsdGeom.Tokens.invisible)
        
        self.assertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                    i_leaf.GetPrim().GetPath())
        self.assertEqual(i_leaf.ComputePurpose(), UsdGeom.Tokens.render, 
                    i_leaf.GetPrim().GetPath())

        # Verify that the Compute*() API that takes parent visibility/purpose 
        # works correctly.
        self.assertEqual(i_leaf.ComputeVisibility(
                    parentVisibility=i_sub.ComputeVisibility()), 
                UsdGeom.Tokens.invisible, 
                i_leaf.GetPrim().GetPath())
        self.assertEqual(i_leaf.ComputePurpose(
                    parentPurpose= i_sub.ComputePurpose()), 
                UsdGeom.Tokens.render, 
                i_leaf.GetPrim().GetPath())
        
    def test_MakeVisInvis(self):
        stage = Usd.Stage.CreateInMemory()

        root  = UsdGeom.Scope.Define(stage,  "/Root")
        sub   =  UsdGeom.Scope.Define(stage, "/Root/Sub")
        leaf  =  UsdGeom.Scope.Define(stage, "/Root/Sub/leaf")

        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())

        print 'Test that making a root invisible makes all the prims invisible.'
        root.MakeInvisible()
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    leaf.GetPrim().GetPath())

        print 'Test that making the leaf visible causes everything to become visible.'
        leaf.MakeVisible(Usd.TimeCode.Default())
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    leaf.GetPrim().GetPath())
        
        print 'Test that making the subscope invisible causes only the subscope and the leaf to be invisisible. Not the root.'
        sub.MakeInvisible()
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    leaf.GetPrim().GetPath())

        print 'Test invising just the leaf.'
        leaf.MakeInvisible()
        sub.MakeVisible()
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    leaf.GetPrim().GetPath())

        print 'Test vising everything again.'
        root.MakeVisible()
        leaf.MakeVisible()
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.inherited,
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
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())
        self.assertEqual(root2.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root2.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    leaf.GetPrim().GetPath())
        self.assertEqual(sub2.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    sub2.GetPrim().GetPath())
        self.assertEqual(leaf2.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    leaf2.GetPrim().GetPath())
        self.assertEqual(sub3.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    sub3.GetPrim().GetPath())
        self.assertEqual(leaf3.ComputeVisibility(), UsdGeom.Tokens.inherited,
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
        self.assertEqual(a.ComputeVisibility(), UsdGeom.Tokens.inherited, a.GetPath())
        self.assertEqual(b.ComputeVisibility(), UsdGeom.Tokens.inherited, b.GetPath())
        self.assertEqual(c.ComputeVisibility(), UsdGeom.Tokens.inherited, c.GetPath())
        self.assertEqual(d.ComputeVisibility(), UsdGeom.Tokens.invisible, d.GetPath())
        self.assertEqual(e.ComputeVisibility(), UsdGeom.Tokens.inherited, e.GetPath())

        print 'Test non-default visibility authoring.'
        d.MakeVisible()
        a.MakeInvisible(1.0)
        e.MakeVisible(1.0)
        
        self.assertEqual(a.ComputeVisibility(1.0), UsdGeom.Tokens.inherited, a.GetPath())
        self.assertEqual(b.ComputeVisibility(1.0), UsdGeom.Tokens.inherited, b.GetPath())
        self.assertEqual(c.ComputeVisibility(1.0), UsdGeom.Tokens.inherited, c.GetPath())
        self.assertEqual(e.ComputeVisibility(1.0), UsdGeom.Tokens.inherited, e.GetPath())
        self.assertEqual(d.ComputeVisibility(1.0), UsdGeom.Tokens.invisible, d.GetPath())

    def test_ProxyPrim(self):
        stage = Usd.Stage.CreateInMemory()
        
        # Create the following prim structure:
        #         A
        #        / \
        #       B   F
        #      / \
        #     C   D
        #     |
        #     E
        # with: C has purpose 'render' and proxyPrim targets D
        #       D has purpose proxy
        #       F has purpose render and proxyPrim targets B (which is default)
        print 'Test authoring and computing renderProxy.'
        a = UsdGeom.Scope.Define(stage, "/A")
        b = UsdGeom.Scope.Define(stage, "/A/B")
        c = UsdGeom.Scope.Define(stage, "/A/B/C")
        d = UsdGeom.Scope.Define(stage, "/A/B/D")
        e = UsdGeom.Scope.Define(stage, "/A/B/C/E")
        f = UsdGeom.Scope.Define(stage, "/A/F")

        c.CreatePurposeAttr(UsdGeom.Tokens.render)
        c.SetProxyPrim(d)
        d.CreatePurposeAttr(UsdGeom.Tokens.proxy)
        f.CreatePurposeAttr(UsdGeom.Tokens.render)
        f.SetProxyPrim(b)
        
        self.assertEqual(a.ComputeProxyPrim(), None, a.GetPath())
        self.assertEqual(c.ComputeProxyPrim(), (d.GetPrim(), c.GetPrim()), c.GetPath())
        self.assertEqual(e.ComputeProxyPrim(), (d.GetPrim(), c.GetPrim()), e.GetPath())
        
if __name__ == "__main__":
    unittest.main()
