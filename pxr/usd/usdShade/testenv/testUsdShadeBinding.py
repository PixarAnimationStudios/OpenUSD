#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

from pxr import Sdf, Usd, UsdShade
import unittest

# TODO: add tests for binding with UsdMaterial
class TestUsdShadeBinding(unittest.TestCase): 
    def test_Basic(self):
        s = Usd.Stage.CreateInMemory()
        rl = s.GetRootLayer()

        # set up so the weaker subtree binds gprim to look1, and
        # stronger subtree to look2
        lw1 = UsdShade.Material.Define(s, "/weaker/look1")
        lw2 = UsdShade.Material.Define(s, "/weaker/look2")
        gpw = s.OverridePrim("/weaker/gprim")
        UsdShade.MaterialBindingAPI.Apply(gpw).Bind(lw1)
        self.assertEqual(
            UsdShade.MaterialBindingAPI(gpw).GetDirectBindingRel().GetTargets(),
            [Sdf.Path("/weaker/look1")])

        ls1 = UsdShade.Material.Define(s, "/stronger/look1")
        ls2 = UsdShade.Material.Define(s, "/stronger/look2")
        gps = s.OverridePrim("/stronger/gprim")
        UsdShade.MaterialBindingAPI.Apply(gps).Bind(ls2)
        self.assertEqual(
            UsdShade.MaterialBindingAPI(gps).GetDirectBindingRel().GetTargets(), 
            [Sdf.Path("/stronger/look2")])

        cr = s.OverridePrim("/composed")

        cr.GetReferences().AddReference(rl.identifier, "/stronger")
        cr.GetReferences().AddReference(rl.identifier, "/weaker")

        gpc = s.GetPrimAtPath("/composed/gprim")
        lb = UsdShade.MaterialBindingAPI(gpc).GetDirectBindingRel()

        # validate we get look2, the stronger binding
        self.assertEqual(lb.GetTargets(), [Sdf.Path("/composed/look2")])

        # upon unbinding *in* the stronger site (i.e. "/stronger/gprim"),
        # we should still be unbound in the fully composed view
        UsdShade.MaterialBindingAPI(gps).UnbindAllBindings()
        self.assertEqual(lb.GetTargets(), [])

        # but *clearing* the target on the referenced prim should allow
        # the weaker binding to shine through
        UsdShade.MaterialBindingAPI(gps).GetDirectBindingRel().ClearTargets(True)

        print(rl.ExportToString())

        self.assertEqual(lb.GetTargets(), [Sdf.Path("/composed/look1")])

    # Test ComputeBoundMaterial() API
    def test_GetBoundMaterial(self):
        stage = Usd.Stage.CreateInMemory()
        look = UsdShade.Material.Define(stage, "/World/Material")
        self.assertTrue(look)
        gprim = stage.OverridePrim("/World/Gprim")
        self.assertTrue(gprim)

        gprimBindingAPI = UsdShade.MaterialBindingAPI.Apply(gprim)
        self.assertFalse(gprimBindingAPI.ComputeBoundMaterial()[0])
        gprimBindingAPI.Bind(look)
        (mat, rel) = gprimBindingAPI.ComputeBoundMaterial()
        self.assertTrue(mat and rel)

        # Now add one more target to mess things up
        rel = gprimBindingAPI.GetDirectBindingRel()
        rel.AddTarget(Sdf.Path("/World"))
        self.assertFalse(gprimBindingAPI.ComputeBoundMaterial()[0])

    def test_BlockingOnOver(self):
        stage = Usd.Stage.CreateInMemory()
        over = stage.OverridePrim('/World/over')
        look = UsdShade.Material.Define(stage, "/World/Material")
        self.assertTrue(look)
        gprim = stage.DefinePrim("/World/gprim")

        UsdShade.MaterialBindingAPI(over).UnbindDirectBinding()
        UsdShade.MaterialBindingAPI.Apply(gprim).Bind(look)
        # This will compose in gprim's binding, but should still be blocked
        over.GetInherits().AddInherit("/World/gprim")
        self.assertFalse(UsdShade.MaterialBindingAPI(over).ComputeBoundMaterial()[0])

if __name__ == "__main__":
    unittest.main()
