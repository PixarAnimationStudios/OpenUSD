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
        lw1.Bind(gpw)
        self.assertEqual(UsdShade.Material.GetBindingRel(gpw).GetTargets(),
                         [Sdf.Path("/weaker/look1")])

        ls1 = UsdShade.Material.Define(s, "/stronger/look1")
        ls2 = UsdShade.Material.Define(s, "/stronger/look2")
        gps = s.OverridePrim("/stronger/gprim")
        ls2.Bind(gps)
        self.assertEqual(UsdShade.Material.GetBindingRel(gps).GetTargets(), 
                         [Sdf.Path("/stronger/look2")])

        cr = s.OverridePrim("/composed")

        cr.GetReferences().AddReference(rl.identifier, "/stronger")
        cr.GetReferences().AddReference(rl.identifier, "/weaker")

        gpc = s.GetPrimAtPath("/composed/gprim")
        lb = UsdShade.Material.GetBindingRel(gpc)

        # validate we get look2, the stronger binding
        self.assertEqual(lb.GetTargets(), [Sdf.Path("/composed/look2")])

        # upon unbinding *in* the stronger site (i.e. "/stronger/gprim"),
        # we should still be unbound in the fully composed view
        UsdShade.Material.Unbind(gps)
        self.assertEqual(lb.GetTargets(), [])
        # but *clearing* the target on the referenced prim should allow
        # the weaker binding to shine through
        UsdShade.Material.GetBindingRel(gps).ClearTargets(True)
        self.assertEqual(lb.GetTargets(), [Sdf.Path("/composed/look1")])

    # Test GetBoundMaterial() API
    def test_GetBoundMaterial(self):
        stage = Usd.Stage.CreateInMemory()
        look = UsdShade.Material.Define(stage, "/World/Material")
        self.assertTrue(look)
        gprim = stage.OverridePrim("/World/Gprim")
        self.assertTrue(gprim)

        self.assertFalse(UsdShade.Material.GetBoundMaterial(gprim))
        look.Bind(gprim)
        self.assertTrue(UsdShade.Material.GetBoundMaterial(gprim))

        # Now add one more target to mess things up
        rel = UsdShade.Material.GetBindingRel(gprim)
        rel.AddTarget(Sdf.Path("/World"))
        self.assertFalse(UsdShade.Material.GetBoundMaterial(gprim))

    def test_BlockingOnOver(self):
        stage = Usd.Stage.CreateInMemory()
        over = stage.OverridePrim('/World/over')
        look = UsdShade.Material.Define(stage, "/World/Material")
        self.assertTrue(look)
        gprim = stage.DefinePrim("/World/gprim")

        UsdShade.Material.Unbind(over)
        look.Bind(gprim)
        # This will compose in gprim's binding, but should still be blocked
        over.GetInherits().AddInherit("/World/gprim")
        self.assertFalse(UsdShade.Material.GetBoundMaterial(over))

if __name__ == "__main__":
    unittest.main()
