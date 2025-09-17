#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
        # GetAllVariantSelections returns the union of all strongest variant
        # selection opinions, even if the variant set doesn't exist.
        self.assertEqual(prim.GetVariantSets().GetAllVariantSelections(),
                         {"modelingVariant" : "Carton_Opened", 
                          "shadingComplexity" : "full",
                          "localDanglingVariant" : "local",
                          "referencedDanglingVariant" : "ref"})
        self.assertTrue(prim.GetVariantSets().HasVariantSet(
                        "shadingComplexity"))
        self.assertFalse(prim.GetVariantSets().HasVariantSet(
                         "localDanglingVariant"))
        self.assertFalse(prim.GetVariantSets().HasVariantSet(
                         "referencedDanglingVariant"))
        # ClearVariantSelection clears the variant set selection from the edit target,
        # permitting any weaker layer selection to take effect
        stage.SetEditTarget(stage.GetSessionLayer())
        prim.GetVariantSet('modelingVariant').SetVariantSelection('Carton_Sealed')
        self.assertEqual(prim.GetVariantSet('modelingVariant').GetVariantSelection(),
                        'Carton_Sealed')
        prim.GetVariantSet('modelingVariant').ClearVariantSelection()
        self.assertEqual(prim.GetVariantSet('modelingVariant').GetVariantSelection(),
                        'Carton_Opened')
        # BlockVariantSelection sets the selection to empty, which blocks weaker variant
        # selection opinions
        prim.GetVariantSet('modelingVariant').BlockVariantSelection()
        self.assertEqual(prim.GetVariantSet('modelingVariant').GetVariantSelection(), '')

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

    def test_USD_5189(self):
        for fmt in allFormats:
            l = Sdf.Layer.CreateAnonymous('.'+fmt)
            l.ImportFromString('''#usda 1.0
(
   defaultPrim = "prim"
)

def "prim" (
    inherits = </class>
    prepend variantSets = "myVariantSet"
    variants = {
        string myVariantSet = "light"
    }
)
{
    variantSet "myVariantSet" = {
        "full"
        {
            string bar = "full"
        }
        
        "light"
        {
            string bar = "light"
        }
    }
}

over "refprim" (
    references = </prim>
    delete variantSets = "myVariantSet"
    prepend variantSets = "myRefVariantSet"
)
{
    variantSet "myRefVariantSet" = {
        "open"
        {
        }
    }
}

over "refrefprim" (
    references = </refprim>
    delete variantSets = "myRefVariantSet"
    variants = {
        string myVariantSet = "full"
    }
    prepend variantSets = "myRefRefVariantSet"
)
{
    variantSet "myRefRefVariantSet" = {
        "closed"
        {
        }
    }
}
''')

            s = Usd.Stage.Open(l)
            p = s.GetPrimAtPath('/prim')
            rp = s.GetPrimAtPath('/refprim')
            rrp = s.GetPrimAtPath('/refrefprim')

            # With bug USD-5189, only the first would return 'myVariantSet', the
            # others would be empty.
            self.assertEqual(p.GetVariantSets().GetNames(),
                             ['myVariantSet'])
            self.assertEqual(rp.GetVariantSets().GetNames(),
                             ['myRefVariantSet', 'myVariantSet'])
            self.assertEqual(rrp.GetVariantSets().GetNames(),
                             ['myRefRefVariantSet', 'myRefVariantSet', 'myVariantSet'])

    def test_UnselectedVariantEditsNotification(self):
        """Tests the expected contents of UsdObjectsChanged notice when 
        unselected variants are added or removed from specs contributing to
        a composed prim"""

        # Setup: payload layer has a single root prim with a variant set 
        # "standin" that has three variants, "a", "b", and "c". The root
        # layer has a single prim with a payload to the payload layer and the
        # same variant set "standin" with only two variants "a" and "b". The
        # root prim sets the variant selection to "a".
        payloadLayer = Sdf.Layer.CreateAnonymous("payload.usda")
        payloadLayer.ImportFromString('''#usda 1.0
            def "Payload" (
                append variantSets = ["standin"]
            )
            {
                variantSet "standin" = {
                    "a" {
                        int a_attr
                    }
                    "b" {
                        int b_attr
                    }
                    "c" {
                        int c_attr
                    }
                }
            }
        ''')

        rootLayer = Sdf.Layer.CreateAnonymous("root.usda")
        rootLayer.ImportFromString('''#usda 1.0
            def "Root" (
                payload = @'''  + payloadLayer.identifier + '''@</Payload>
                append variantSets = "standin"
                variants = {
                    string standin = "a"
                }
            )
            {
                variantSet "standin" = {
                    "a" {

                    }
                    "b" {

                    }
                }
            }
        ''')

        # Open the root layer as a stage and verify it has the prim /Root with
        # the variant set "standin" that has the composed variant options "a",
        # "b", and "c".
        stage = Usd.Stage.Open(rootLayer)
        prim = stage.GetPrimAtPath("/Root")
        self.assertTrue(prim)
        composedVarSet = prim.GetVariantSet("standin")
        self.assertTrue(composedVarSet)
        self.assertEqual(composedVarSet.GetVariantNames(), ['a', 'b', 'c'])

        # Our notice handling callback for the Objects changed notice. This 
        # callback verifies that for the following edits, the notice will report
        # that we haven't resynced any prim paths but that we do get an info
        # changed on the /Root prim indicating that 'variantChildren' has 
        # changed.
        numNoticesReceived = 0
        def _OnObjectsChanged(notice, sender):
            # Increment the numNoticesReceived so that we can sanity check that
            # this notice callback is indeed called after the expected 
            # operations.
            nonlocal numNoticesReceived
            numNoticesReceived = numNoticesReceived + 1
            self.assertEqual(notice.GetStage(), stage)
            self.assertEqual(notice.GetResyncedPaths(), [])
            self.assertEqual(notice.GetChangedInfoOnlyPaths(), 
                             [Sdf.Path('/Root')])
            self.assertEqual(notice.GetChangedFields('/Root'), 
                             ['variantChildren'])

        # Register out ObjectsChanged notice listener.
        objectsChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.ObjectsChanged, _OnObjectsChanged)

        # Get the Sdf variant set for "standin" in both the payload and root
        # layers.
        payloadPrimSpec = payloadLayer.GetPrimAtPath('/Payload')
        payloadVarSet = payloadPrimSpec.variantSets['standin']
        rootPrimSpec = rootLayer.GetPrimAtPath('/Root')
        rootVarSet = rootPrimSpec.variantSets['standin']

        # Remove variant 'c' in the payload layer and verify the notice
        # handler verifications were triggered. This edit (as well as the 
        # subsequent edits) only results in an info changed notification that
        # the composed variants for at least one variant set has change for the
        # root prim, but there is no resync and the composed prim index for the
        # prim has no actual dependency on this particular variant.
        payloadVarSet.RemoveVariant(payloadVarSet.variants['c'])
        self.assertEqual(numNoticesReceived, 1)

        # The composed variant names no longer contain 'c'
        self.assertEqual(composedVarSet.GetVariantNames(), ['a', 'b'])

        # Remove variant 'b' in the root layer and verify the notice
        # handler verifications were triggered.
        rootVarSet.RemoveVariant(rootVarSet.variants['b'])
        self.assertEqual(numNoticesReceived, 2)

        # The composed variant names are still "a" and "b" since the payload
        # still has "b", but the info changed notice was still correct.
        self.assertEqual(composedVarSet.GetVariantNames(), ['a', 'b'])

        # Add a new variant spec 'root' to "standin" in the root layer and 
        # verify the notice handler verifications were triggered.
        Sdf.VariantSpec(rootVarSet, "root")
        self.assertEqual(numNoticesReceived, 3)

        # The composed variant names now contain 'root'
        self.assertEqual(composedVarSet.GetVariantNames(), ['a', 'b', 'root'])

        # Add a new variant spec 'payload' to "standin" in the payload layer and 
        # verify the notice handler verifications were triggered.
        Sdf.VariantSpec(payloadVarSet, "payload")
        self.assertEqual(numNoticesReceived, 4)

        # The composed variant names now contain 'payload'
        self.assertEqual(composedVarSet.GetVariantNames(), 
                         ['a', 'b', 'payload', 'root'])

        # Revoke the existing ObjectsChanged notice handler and replace it
        # with this new one that verifies that we get a resync change for 
        # /Root and no info changes.
        objectsChanged.Revoke()

        def _OnSelectedObjectChanged(notice, sender):
            nonlocal numNoticesReceived
            numNoticesReceived = numNoticesReceived + 1
            self.assertEqual(notice.GetStage(), stage)
            self.assertEqual(notice.GetResyncedPaths(), ['/Root'])
            self.assertEqual(notice.GetChangedInfoOnlyPaths(), [])

        objectsChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.ObjectsChanged, _OnSelectedObjectChanged)

        # Remove variant "a" from the payload layer and verify the notice
        # handler verifications were triggered. This edit results in a resync
        # because the prim index for /Root does depend on the "a" variant 
        # because of the {standin=a} variant selection.
        payloadVarSet.RemoveVariant(payloadVarSet.variants['a'])
        self.assertEqual(numNoticesReceived, 5)

        objectsChanged.Revoke()

if __name__ == '__main__':
    unittest.main()
