#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import sys, os, unittest
from pxr import Sdf, Tf, Usd, UsdGeom

class TestUsdGeomPurposeVisibility(unittest.TestCase):
    def test_ComputeVisibility(self):
        stage = Usd.Stage.CreateInMemory()

        print("Ensuring non-imageable prims with no opinions evaluate to defaults")

        ni_Root  = stage.DefinePrim("/ni_Root")
        ni_sub   =  stage.DefinePrim("/ni_Root/ni_Sub")
        ni_leaf  =  stage.DefinePrim("/ni_Root/ni_Sub/ni_leaf")

        img = UsdGeom.Imageable(ni_Root)
        self.assertEqual(
            img.ComputeVisibility(),
            UsdGeom.Tokens.inherited, 
            img.GetPrim().GetPath())

        img = UsdGeom.Imageable(ni_sub)
        self.assertEqual(
            img.ComputeVisibility(),
            UsdGeom.Tokens.inherited, 
            img.GetPrim().GetPath())

        img = UsdGeom.Imageable(ni_leaf)
        self.assertEqual(
            img.ComputeVisibility(),
            UsdGeom.Tokens.inherited, 
            img.GetPrim().GetPath())
        
        print("Ensuring non-imageable prims WITH opinions STILL evaluate to defaults")
        ni_sub.CreateAttribute(UsdGeom.Tokens.visibility, Sdf.ValueTypeNames.Token).Set(UsdGeom.Tokens.invisible)
        
        img = UsdGeom.Imageable(ni_sub)
        self.assertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                    img.GetPrim().GetPath())
        img = UsdGeom.Imageable(ni_leaf)
        self.assertEqual(img.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                    img.GetPrim().GetPath())
        
        print("Ensuring imageable leaf prim can have opinions")
        i_Root  = UsdGeom.Scope.Define(stage,  "/i_Root")
        i_sub   =  UsdGeom.Scope.Define(stage, "/i_Root/i_Sub")
        i_leaf  =  UsdGeom.Scope.Define(stage, "/i_Root/i_Sub/i_leaf")
        
        i_leaf.GetVisibilityAttr().Set(UsdGeom.Tokens.invisible)
        
        self.assertEqual(
            i_leaf.ComputeVisibility(),
            UsdGeom.Tokens.invisible, 
            i_leaf.GetPrim().GetPath())
        
        print("Ensuring imageable leaf prim is shadowed by Imageable parent opinions")
        i_leaf.GetVisibilityAttr().Set(UsdGeom.Tokens.inherited)
        i_sub.GetVisibilityAttr().Set(UsdGeom.Tokens.invisible)
        
        self.assertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                    i_leaf.GetPrim().GetPath())
        
        print("Ensuring imageable leaf prim is NOT shadowed by non-Imageable parent opinions")
        i_sub.GetPrim().SetTypeName('')
        
        self.assertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.inherited, 
                    i_leaf.GetPrim().GetPath())
        
        print("Ensuring imageable most ancestral imageable opinion wins when there are many")
        i_sub.GetPrim().SetTypeName('Scope')
        # The fallbacks are special, let's make sure authoring them doesn't change 
        # results - i.e. values authored on i_sub should win
        i_Root.GetVisibilityAttr().Set(UsdGeom.Tokens.inherited)
        
        self.assertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                    i_leaf.GetPrim().GetPath())
        
        i_Root.GetVisibilityAttr().Set(UsdGeom.Tokens.invisible)
        
        self.assertEqual(i_leaf.ComputeVisibility(), UsdGeom.Tokens.invisible, 
                    i_leaf.GetPrim().GetPath())

        # Verify that the Compute*() API that takes parent visibility
        # works correctly.
        self.assertEqual(
            i_leaf.ComputeVisibility(),
            UsdGeom.Tokens.invisible, 
            i_leaf.GetPrim().GetPath())
        

    def test_ComputePurposeVisibility(self):
        """
        Test purpose visibility functionality, which allows purpose-specific
        visibility control.
        """

        stage = Usd.Stage.CreateInMemory()

        rootPrim = stage.DefinePrim("/Root", "Scope")
        imageablePrim = stage.DefinePrim("/Root/Imageable", "Scope")
        nonImageablePrim = stage.DefinePrim("/Root/NonImageable")

        root = UsdGeom.Imageable(rootPrim)
        self.assertTrue(root)
        imageable = UsdGeom.Imageable(imageablePrim)
        self.assertTrue(imageable)

        # VisibilityAPI is not applied to Imageable prims by default 
        rootVisAPI = UsdGeom.VisibilityAPI(rootPrim)
        self.assertFalse(rootVisAPI)
        imageableVisAPI = UsdGeom.VisibilityAPI(imageablePrim)
        self.assertFalse(imageableVisAPI)

        # Test that overall visibility is initially visible and that purpose
        # visibility gives use the expected fallback values.
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(),
            UsdGeom.Tokens.visible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.proxy),
            UsdGeom.Tokens.inherited)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.render),
            UsdGeom.Tokens.inherited)

        # Test that GeomVisibilityAPI can only apply to Imageable
        # prims.
        self.assertTrue(UsdGeom.VisibilityAPI.CanApply(imageablePrim))
        self.assertFalse(UsdGeom.VisibilityAPI.CanApply(nonImageablePrim))
        
        # None of the purpose visibilty attributes exist yet when queried via
        # either UsdGeomImageable or UsdGeomVisibilityAPI.
        self.assertTrue(
            imageable.GetPurposeVisibilityAttr())
        # Note that UsdGeomVisibilityAPI does not allow "default" as an input
        # purpose to GetPurposeVisibilityAttr even though UsdGeomImageable does
        # allow it. This will likely change when all visibility properties are
        # moved out of Imageable into VisibilityAPI.
        with self.assertRaises(Tf.ErrorException):
            imageableVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.default_)

        self.assertFalse(
            imageable.GetPurposeVisibilityAttr(UsdGeom.Tokens.guide))
        self.assertFalse(
            imageableVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.guide))

        self.assertFalse(
            imageable.GetPurposeVisibilityAttr(UsdGeom.Tokens.proxy))
        self.assertFalse(
            imageableVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.proxy))

        self.assertFalse(
            imageable.GetPurposeVisibilityAttr(UsdGeom.Tokens.render))
        self.assertFalse(
            imageableVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.render))

        # Test that GeomVisibilityAPI adds the expected purpose
        # visibility attributes.
        UsdGeom.VisibilityAPI.Apply(imageablePrim)
        self.assertTrue(imageableVisAPI)

        guideVisibility = \
            imageable.GetPurposeVisibilityAttr(UsdGeom.Tokens.guide)
        self.assertEqual(guideVisibility, 
            imageableVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.guide))
        self.assertEqual(guideVisibility.Get(), UsdGeom.Tokens.invisible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)

        proxyVisibility = \
            imageable.GetPurposeVisibilityAttr(UsdGeom.Tokens.proxy)
        self.assertEqual(proxyVisibility, 
            imageableVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.proxy))
        self.assertEqual(proxyVisibility.Get(), UsdGeom.Tokens.inherited)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.proxy),
            UsdGeom.Tokens.inherited)

        renderVisibility = \
            imageable.GetPurposeVisibilityAttr(UsdGeom.Tokens.render)
        self.assertEqual(renderVisibility, 
            imageableVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.render))
        self.assertEqual(renderVisibility.Get(), UsdGeom.Tokens.inherited)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.render),
            UsdGeom.Tokens.inherited)


        # Set purpose visibility on the root and ensure that it inherits as
        # expected.
        UsdGeom.VisibilityAPI.Apply(rootPrim)
        self.assertTrue(rootVisAPI)

        guideVisibility = root.GetPurposeVisibilityAttr(UsdGeom.Tokens.guide)
        self.assertEqual(guideVisibility, 
            rootVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.guide))
        guideVisibility.Set(UsdGeom.Tokens.visible)
        self.assertEqual(
            root.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.visible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.visible)


        # Set overall visibility to invisible, causing purpose visibility to
        # also become invisible.
        overallVisibility = root.GetPurposeVisibilityAttr()
        overallVisibility.Set(UsdGeom.Tokens.invisible)
        self.assertEqual(
            root.ComputeEffectiveVisibility(),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            root.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            root.ComputeEffectiveVisibility(UsdGeom.Tokens.render),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.render),
            UsdGeom.Tokens.invisible)

        # Unapply the VisibilityAPI from the root prim.
        rootPrim.RemoveAPI(UsdGeom.VisibilityAPI)
        self.assertFalse(rootPrim.HasAPI(UsdGeom.VisibilityAPI))

        # Since VisibilityAPI is no longer applied, there are no longer 
        # fallback attributes from the VisibilityAPI schema for the purpose
        # visibility.
        self.assertFalse(rootVisAPI)
        # However, note that the guideVisibility attribute was authored above 
        # in this test so VisibilityAPI's GetPurposeVisibilityAttr still returns
        # it even though it is no longer applied.
        self.assertTrue(
            rootVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.guide))
        self.assertFalse(
            rootVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.proxy))
        self.assertFalse(
            rootVisAPI.GetPurposeVisibilityAttr(UsdGeom.Tokens.render))

        # UsdGeomImageable's GetPurposedVisibilityAttr still returns the
        # overall visibility attribute for default purpose since it is not tied
        # to VisibilityAPI. However it does not return valid atttributes for the
        # other purposes without VisibilityAPI. Note that this includes 
        # guideVisibility even though the attribute exists and is authored.
        self.assertTrue(
            root.GetPurposeVisibilityAttr())
        self.assertFalse(
            root.GetPurposeVisibilityAttr(UsdGeom.Tokens.guide))
        self.assertFalse(
            root.GetPurposeVisibilityAttr(UsdGeom.Tokens.proxy))
        self.assertFalse(
            root.GetPurposeVisibilityAttr(UsdGeom.Tokens.render))

        # Even without VisibilityAPI applied to root, root still has overall
        # visibility set to "invisible" which overrides all purpose visibility
        # and is inherited by the child Imageable.
        self.assertEqual(
            root.ComputeEffectiveVisibility(),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            root.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            root.ComputeEffectiveVisibility(UsdGeom.Tokens.render),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.render),
            UsdGeom.Tokens.invisible)

        # Set root's overall visibility back to visible.
        overallVisibility.Set(UsdGeom.Tokens.visible)

        # Default and render visibility are back to the inherited defaults 
        # again.
        self.assertEqual(
            root.ComputeEffectiveVisibility(),
            UsdGeom.Tokens.visible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(),
            UsdGeom.Tokens.visible)
        self.assertEqual(
            root.ComputeEffectiveVisibility(UsdGeom.Tokens.render),
            UsdGeom.Tokens.inherited)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.render),
            UsdGeom.Tokens.inherited)

        # Guide visibility is back to the default of "invisible" again even 
        # though guideVisibility exists on the root and has a value of 
        # "visible". An authored guideVisibility attribute is ignored when 
        # VisibilityAPI is not applied.
        guideVisibility = rootVisAPI.GetPurposeVisibilityAttr(
            UsdGeom.Tokens.guide)
        self.assertEqual(guideVisibility.Get(), UsdGeom.Tokens.visible)
        self.assertEqual(
            root.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)
        self.assertEqual(
            imageable.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)

    def test_ComputePurposeVisibilityWithInstancing(self):
        """
        Test inherited purpose visibility in combination with native
        instancing.
        """

        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().ImportFromString('''#usda 1.0
        def Scope "_prototype" (
            prepend apiSchemas = ["VisibilityAPI"]
        )
        {
            token guideVisibility = "visible"

            def Scope "child" (
                prepend apiSchemas = ["VisibilityAPI"]
            )
            {
            }
        }

        def Scope "instance" (
            instanceable = true
            references = </_prototype>
        )
        {
        }
        ''')

        # The instance child prim (which proxies _prototype/child) is
        # initially visible, due to the guideVisibility opinion on _prototype.
        childPrim = stage.GetPrimAtPath('/instance/child')
        self.assertTrue(childPrim)
        self.assertTrue(childPrim.IsInstanceProxy())

        child = UsdGeom.Imageable(childPrim)
        self.assertEqual(
            child.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.visible)

        # If we invis guides on the instance root, this is inherited by the
        # instance child.
        instancePrim = stage.GetPrimAtPath('/instance')
        self.assertTrue(instancePrim)
        self.assertFalse(instancePrim.IsInstanceProxy())

        instance = UsdGeom.Imageable(instancePrim)
        guideVisibility = \
            instance.GetPurposeVisibilityAttr(UsdGeom.Tokens.guide)
        guideVisibility.Set(UsdGeom.Tokens.invisible)
        self.assertEqual(
            child.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)

        # ...but the prototype child doesn't inherit guide visibility, since
        # it doesn't have any properties, etc.
        prototypeChildPrim = \
            stage.GetPrimAtPath('/instance').GetPrototype().GetChild('child')
        self.assertTrue(prototypeChildPrim)
        self.assertFalse(prototypeChildPrim.IsInstanceProxy())

        prototypeChild = UsdGeom.Imageable(prototypeChildPrim)
        self.assertEqual(
            prototypeChild.ComputeEffectiveVisibility(UsdGeom.Tokens.guide),
            UsdGeom.Tokens.invisible)


    def test_ComputePurpose(self):
        stage = Usd.Stage.CreateInMemory()

        # Create a hierarchy of imageable and non-imageable prims with authored
        # and not authored purpose opinions to test all purpose inheritance 
        # cases. This is the equivalent hierarchy.
        #  def "Root" {
        #      token purpose = "proxy"
        #
        #      def Scope "RenderScope" {
        #          token purpose = "render"
        #
        #          def "DefPrim {
        #              token purpose = "default"
        #
        #              def Scope "Scope {
        #
        #                  def Xform "InheritXform" {
        #                  }
        #
        #                  def Xform "GuideXform" {
        #                      token purpose = "guide"
        #                  }
        #              }
        #          }
        #      }
        #
        #      def Xform "Xform {
        #      }
        #  }

        # Non-imageable with purpose attribute opinion
        root = stage.DefinePrim("/Root")
        root.CreateAttribute(UsdGeom.Tokens.purpose, 
                             Sdf.ValueTypeNames.Token).Set(UsdGeom.Tokens.proxy)
        rootImg = UsdGeom.Imageable(root)

        # Imageable with purpose attribute opinion
        renderScope = stage.DefinePrim("/Root/RenderScope", "Scope")
        renderScopeImg = UsdGeom.Imageable(renderScope)
        renderScopeImg.GetPurposeAttr().Set(UsdGeom.Tokens.render)

        # Non-imageable with purpose attribute opinion
        defaultPrim = stage.DefinePrim("/Root/RenderScope/DefPrim")
        defaultPrim.CreateAttribute(
            UsdGeom.Tokens.purpose, Sdf.ValueTypeNames.Token).Set(
                UsdGeom.Tokens.default_)
        defaultPrimImg = UsdGeom.Imageable(defaultPrim)

        # Imageable with no purpose opinion
        scope = stage.DefinePrim("/Root/RenderScope/DefPrim/Scope", "Scope")
        scopeImg = UsdGeom.Imageable(scope)

        # Imageable with no purpose opinion
        inheritXform = stage.DefinePrim(
            "/Root/RenderScope/DefPrim/Scope/InheritXform", "Xform")
        inheritXformImg = UsdGeom.Imageable(inheritXform)

        # Imageable with purpose opinion
        guideXform = stage.DefinePrim(
            "/Root/RenderScope/DefPrim/Scope/GuideXform", "Xform")
        guideXformImg = UsdGeom.Imageable(guideXform)
        guideXformImg.GetPurposeAttr().Set(UsdGeom.Tokens.guide)

        # Imageable with no purpose opinion
        xform = stage.DefinePrim("/Root/Xform", "Xform")
        xformImg = UsdGeom.Imageable(xform)

        # Non-imageable root evaluates to default.
        self.assertFalse(rootImg)
        self.assertEqual(rootImg.GetPurposeAttr().Get(), UsdGeom.Tokens.proxy)
        rootInfo = rootImg.ComputePurposeInfo()
        self.assertEqual(rootInfo.purpose, UsdGeom.Tokens.default_)
        self.assertFalse(rootInfo.isInheritable)

        # Imageable with authored opinion always evaluates to authored purpose
        self.assertTrue(renderScopeImg)
        self.assertEqual(renderScopeImg.GetPurposeAttr().Get(), 
                         UsdGeom.Tokens.render)
        renderScopeInfo = renderScopeImg.ComputePurposeInfo()
        self.assertEqual(renderScopeInfo.purpose, UsdGeom.Tokens.render)
        self.assertTrue(renderScopeInfo.isInheritable)

        # Non-imageable under imageable prim with authored opinion will inherit
        # its purpose from the authored imageable parent always
        self.assertFalse(defaultPrimImg)
        self.assertEqual(defaultPrimImg.GetPurposeAttr().Get(), 
                         UsdGeom.Tokens.default_)
        defaultPrimInfo = defaultPrimImg.ComputePurposeInfo()
        self.assertEqual(defaultPrimInfo.purpose, UsdGeom.Tokens.render)
        self.assertTrue(defaultPrimInfo.isInheritable)

        # Imageable with no opinion inherits its purpose from nearest imageable
        # ancestor with authored purpose opinion.
        self.assertTrue(scopeImg)
        self.assertEqual(scopeImg.GetPurposeAttr().Get(), 
                         UsdGeom.Tokens.default_)
        scopeInfo = scopeImg.ComputePurposeInfo()
        self.assertEqual(scopeInfo.purpose, UsdGeom.Tokens.render)
        self.assertTrue(scopeInfo.isInheritable)

        # Imageable with no opinion whose parent is also an imageable with no 
        # opinion still inherits its purpose from nearest possible imageable
        # ancestor that does have an authored purpose opinion.
        self.assertTrue(inheritXformImg)
        self.assertEqual(inheritXformImg.GetPurposeAttr().Get(), 
                         UsdGeom.Tokens.default_)
        inheritXformInfo = inheritXformImg.ComputePurposeInfo()
        self.assertEqual(inheritXformInfo.purpose, UsdGeom.Tokens.render)
        self.assertTrue(inheritXformInfo.isInheritable)

        # Imageable with a different authored opinion than ancestor's purpose
        # will always uses its own authored purpose.
        self.assertTrue(guideXformImg)
        self.assertEqual(guideXformImg.GetPurposeAttr().Get(), 
                         UsdGeom.Tokens.guide)
        guideXformInfo = guideXformImg.ComputePurposeInfo()
        self.assertEqual(guideXformInfo.purpose, UsdGeom.Tokens.guide)
        self.assertTrue(guideXformInfo.isInheritable)

        # Imageable with no opinion and no inheritable ancestor opinion, always
        # uses the fallback purpose attribute value.
        self.assertTrue(xformImg)
        self.assertEqual(xformImg.GetPurposeAttr().Get(), 
                         UsdGeom.Tokens.default_)
        xformInfo = xformImg.ComputePurposeInfo()
        self.assertEqual(xformInfo.purpose, UsdGeom.Tokens.default_)
        self.assertFalse(xformInfo.isInheritable)

        # For testing the ComputePurposeInfo API that takes a precomputed parent
        # purpose.
        inheritableDefault = UsdGeom.Imageable.PurposeInfo(
            UsdGeom.Tokens.default_, True)
        nonInheritableDefault = UsdGeom.Imageable.PurposeInfo(
            UsdGeom.Tokens.default_, False)

        inheritableProxy = UsdGeom.Imageable.PurposeInfo(
            UsdGeom.Tokens.proxy, True)
        nonInheritableProxy = UsdGeom.Imageable.PurposeInfo(
            UsdGeom.Tokens.proxy, False)

        # This scope has an authored opinion so parent purpose info is alwasy
        # ignored.
        self.assertEqual(
            renderScopeImg.ComputePurposeInfo(inheritableDefault),
            UsdGeom.Imageable.PurposeInfo(UsdGeom.Tokens.render, True))
        self.assertEqual(
            renderScopeImg.ComputePurposeInfo(nonInheritableDefault),
            UsdGeom.Imageable.PurposeInfo(UsdGeom.Tokens.render, True))
        self.assertEqual(
            renderScopeImg.ComputePurposeInfo(inheritableProxy),
            UsdGeom.Imageable.PurposeInfo(UsdGeom.Tokens.render, True))
        self.assertEqual(
            renderScopeImg.ComputePurposeInfo(nonInheritableProxy),
            UsdGeom.Imageable.PurposeInfo(UsdGeom.Tokens.render, True))

        # This scope is imageable but with no purpose opinion. It uses the 
        # passed in parent purpose if the parent purpose is inheritable. It 
        # uses its fallback opinion otherwise.
        self.assertEqual(
            scopeImg.ComputePurposeInfo(inheritableDefault),
            UsdGeom.Imageable.PurposeInfo(UsdGeom.Tokens.default_, True))
        self.assertEqual(
            scopeImg.ComputePurposeInfo(nonInheritableDefault),
            UsdGeom.Imageable.PurposeInfo(UsdGeom.Tokens.default_, False))
        self.assertEqual(
            scopeImg.ComputePurposeInfo(inheritableProxy),
            UsdGeom.Imageable.PurposeInfo(UsdGeom.Tokens.proxy, True))
        self.assertEqual(
            scopeImg.ComputePurposeInfo(nonInheritableProxy),
            UsdGeom.Imageable.PurposeInfo(UsdGeom.Tokens.default_, False))

    def test_MakeVisInvis(self):
        stage = Usd.Stage.CreateInMemory()

        root  = UsdGeom.Scope.Define(stage,  "/Root")
        sub   =  UsdGeom.Scope.Define(stage, "/Root/Sub")
        leaf  =  UsdGeom.Scope.Define(stage, "/Root/Sub/leaf")

        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())

        print('Test that making a root invisible makes all the prims invisible.')
        root.MakeInvisible()
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    leaf.GetPrim().GetPath())

        print('Test that making the leaf visible causes everything to become visible.')
        leaf.MakeVisible(Usd.TimeCode.Default())
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    leaf.GetPrim().GetPath())
        
        print('Test that making the subscope invisible causes only the subscope and the leaf to be invisible. Not the root.')
        sub.MakeInvisible()
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    leaf.GetPrim().GetPath())

        print('Test invising just the leaf.')
        leaf.MakeInvisible()
        sub.MakeVisible()
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.invisible,
                    leaf.GetPrim().GetPath())

        print('Test vising everything again.')
        root.MakeVisible()
        leaf.MakeVisible()
        self.assertEqual(root.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    root.GetPrim().GetPath())
        self.assertEqual(sub.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    sub.GetPrim().GetPath())
        self.assertEqual(leaf.ComputeVisibility(), UsdGeom.Tokens.inherited,
                    leaf.GetPrim().GetPath())

        print('Test with a couple of new subtrees.')
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
        # Make A invisible and then make E visible. Test that D remains
        # invisible.
        print('Test preservation of visibility state.')
        a = UsdGeom.Scope.Define(stage, "/A")
        b = UsdGeom.Scope.Define(stage, "/A/B")
        c = UsdGeom.Scope.Define(stage, "/A/B/C")
        d = UsdGeom.Scope.Define(stage, "/A/B/D")
        e = UsdGeom.Scope.Define(stage, "/A/B/C/E")
        a.MakeInvisible()
        e.MakeVisible()
        self.assertEqual(a.ComputeVisibility(), UsdGeom.Tokens.inherited,
                         a.GetPath())
        self.assertEqual(b.ComputeVisibility(), UsdGeom.Tokens.inherited,
                         b.GetPath())
        self.assertEqual(c.ComputeVisibility(), UsdGeom.Tokens.inherited,
                         c.GetPath())
        self.assertEqual(d.ComputeVisibility(), UsdGeom.Tokens.invisible,
                         d.GetPath())
        self.assertEqual(e.ComputeVisibility(), UsdGeom.Tokens.inherited,
                         e.GetPath())

        print('Test non-default visibility authoring.')
        d.MakeVisible()
        a.MakeInvisible(1.0)
        e.MakeVisible(1.0)
        
        self.assertEqual(a.ComputeVisibility(1.0), UsdGeom.Tokens.inherited,
                         a.GetPath())
        self.assertEqual(b.ComputeVisibility(1.0), UsdGeom.Tokens.inherited,
                         b.GetPath())
        self.assertEqual(c.ComputeVisibility(1.0), UsdGeom.Tokens.inherited,
                         c.GetPath())
        self.assertEqual(e.ComputeVisibility(1.0), UsdGeom.Tokens.inherited,
                         e.GetPath())
        self.assertEqual(d.ComputeVisibility(1.0), UsdGeom.Tokens.invisible,
                         d.GetPath())

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
        print('Test authoring and computing proxyPrim.')
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
        self.assertEqual(c.ComputeProxyPrim(), (d.GetPrim(), c.GetPrim()),
                         c.GetPath())
        self.assertEqual(e.ComputeProxyPrim(), (d.GetPrim(), c.GetPrim()),
                         e.GetPath())

        # Compute the proxy prim in a case that is invalid because the targeted
        # prim doesn't have 'proxy' purpose.
        self.assertEqual(f.ComputeProxyPrim(), None, f.GetPath())

        # Set purpose 'guide' on A, to make sure that D's purpose value isn't
        # determined by an ancestor opinion (i.e., ensure the purpose value
        # used here *doesn't* reflect pruning purpose semantics).
        a.CreatePurposeAttr(UsdGeom.Tokens.guide)
        self.assertEqual(e.ComputeProxyPrim(), (d.GetPrim(), c.GetPrim()),
                         e.GetPath())

if __name__ == "__main__":
    unittest.main()
