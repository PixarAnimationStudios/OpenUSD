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
from pxr import Sdf, Usd, Tf

allFormats = ['usd' + x for x in 'ac']

class TestUsdVariants(unittest.TestCase):
    def test_VariantSetAPI(self):
        f = 'MilkCartonA.usda'
        layer = Sdf.Layer.FindOrOpen(f)
        self.assertTrue(layer)

        stage = Usd.Stage.Open(f)
        self.assertTrue(stage)

        prim = stage.GetPrimAtPath('/MilkCartonA')
        self.assertTrue(prim)

        self.assertTrue(prim.HasVariantSets())
        self.assertTrue('modelingVariant' in prim.GetVariantSets().GetNames())
        self.assertEqual(prim.GetVariantSet('modelingVariant').GetVariantSelection(),
                         'Carton_Opened')
        self.assertEqual(prim.GetVariantSets().GetVariantSelection('modelingVariant'),
                         'Carton_Opened')
        self.assertEqual(prim.GetVariantSet('modelingVariant').GetVariantNames(),
                         ['ALL_VARIANTS', 'Carton_Opened', 'Carton_Sealed'])
        self.assertEqual(prim.GetVariantSet('modelingVariant').GetName(),
                         'modelingVariant')

    def test_VariantSelectionPathAbstraction(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestVariantSelectionPathAbstraction.'+fmt)
            p = s.OverridePrim("/Foo")
            vss = p.GetVariantSets()
            self.assertFalse(p.HasVariantSets())
            vs = vss.AddVariantSet("LOD")
            self.assertTrue(p.HasVariantSets())
            self.assertTrue(vs)
            self.assertTrue(vs.AddVariant("High"))
            self.assertTrue(p.HasVariantSets())

            # This call triggers the bug. This happens because it triggers the
            # computation of a PcpPrimIndex for the variant prim, which then causes
            # the prim with a variant selection to be included in the UsdStage's
            # scene graph later when the next round of change processing occurs.
            #
            # XXX: WBN to indicate the bug # above.  This code changed when the
            # variant API changed during the switch to using EditTargets instead of
            # UsdPrimVariant.  It's unclear whether or not the mystery bug is still
            # reproduced. Leaving the test in place as much as possible..
            self.assertFalse(p.GetAttribute("bar").IsDefined())

            # This triggers change processing which will include the prim with the
            # variant selection and put it on the stage.
            vs.SetVariantSelection('High')
            editTarget = vs.GetVariantEditTarget()
            self.assertTrue(editTarget)
            with Usd.EditContext(s, editTarget):
                s.DefinePrim(p.GetPath().AppendChild('Foobar'), 'Scope')

            self.assertTrue(s.GetPrimAtPath(p.GetPath().AppendChild('Foobar')))

            # Here's the actual manifestation of the bug: We should still not have
            # this prim on the stage, but when the bug is present, we do. Paths
            # containing variant selections can never identify objects on a stage.
            # Verify that the stage does not contain a prim for the variant prim
            # spec we just created at </Foo{LOD=High}Foobar>
            testPath = p.GetPath().AppendVariantSelection(
                'LOD', 'High').AppendChild('Foobar')
            self.assertFalse(s.GetPrimAtPath(testPath))

    def test_NestedVariantSets(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestNestedVariantSets.'+fmt)
            p = s.DefinePrim('/Foo', 'Scope')
            vss = p.GetVariantSets()
            vs_lod = vss.AddVariantSet("LOD")
            vs_lod.AddVariant("High")
            vs_lod.SetVariantSelection('High')
            with vs_lod.GetVariantEditContext():
                # Create a directly nested variant set.
                vs_costume = vss.AddVariantSet("Costume")
                vs_costume.AddVariant("Spooky")
                vs_costume.SetVariantSelection('Spooky')
                with vs_costume.GetVariantEditContext():
                    s.DefinePrim(p.GetPath().AppendChild('SpookyHat'), 'Cone')

                # Create a child prim with its own variant set.
                p2 = s.DefinePrim(p.GetPath().AppendChild('DetailedStuff'), 'Scope')
                vss_p2 = p2.GetVariantSets()
                vs_p2 = vss_p2.AddVariantSet("StuffVariant")
                vs_p2.AddVariant("A")
                vs_p2.SetVariantSelection('A')
                with vs_p2.GetVariantEditContext():
                    s.DefinePrim(p2.GetPath().AppendChild('StuffA'), 'Sphere')

            self.assertTrue(vss.GetNames() == ['LOD', 'Costume'])
            self.assertTrue(s.GetPrimAtPath('/Foo/SpookyHat'))
            self.assertTrue(s.GetRootLayer().GetPrimAtPath(
                '/Foo{LOD=High}{Costume=Spooky}SpookyHat'))


if __name__ == '__main__':
    unittest.main()
