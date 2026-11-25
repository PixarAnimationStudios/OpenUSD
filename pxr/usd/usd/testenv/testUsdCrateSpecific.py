#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest, tempfile, os
from pxr import Sdf, Usd

class TestUsdCrateSpecific(unittest.TestCase):

    def test_SdfCopySpecWithRelationshipTargets(self):
        """Tests overwriting relationship target from source to target in crate
        layers. This test checks that Sdf.CopySpec() works correctly with the
        "procedural" dynamic creation of relationship children fields that the
        crate data implementation does.
        """
        import textwrap

        layerContent = '''#usda 1.0

        def "target" {}

        def "source" {
            rel myrel
        }

        def "dest" {
            rel myrel = </target>
        }
        '''

        layer = Sdf.Layer.CreateAnonymous(".usdc")
        layer.ImportFromString(textwrap.dedent(layerContent))
        self.assertEqual(
            list(layer.GetRelationshipAtPath(
                '/dest.myrel').targetPathList.GetAppliedItems()),
            [Sdf.Path('/target')])
        self.assertTrue(Sdf.CopySpec(layer, "/source", layer, "/dest"))
        self.assertEqual(
            list(layer.GetRelationshipAtPath(
                '/dest.myrel').targetPathList.GetAppliedItems()),
            [])

    def test_SamplesExportScalability(self):
        """There was a bug (USD-10028) where a hash table in the crate file
        writer used 'double' as a key and the default hash function, which on
        clang/libc++ just returns the floating point bit pattern as an integer,
        and the hash table just masks bits to produce an index.  This led to
        essentially 100% collisions, and a hang on export of more than about 30k
        samples.  This test just ensures we can write 100k samples.
        """
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/test")
        attr = prim.CreateAttribute("foo", Sdf.ValueTypeNames.Float)
        numSamples = 100_000
        for i in range(numSamples):
            attr.Set(float(i), i)
        with tempfile.TemporaryDirectory() as tmpdir:
            usdc = os.path.join(tmpdir, "foo.usdc")
            stage.Export(usdc)

    def test_TargetAndConnectionPathKinds(self):
        """Relationship targets and attribute connections may be target,
        mapper, mapper arg and relational attribute paths, including nested
        target paths.  Crate synthesizes target and connection specs from the
        list ops rather than storing them, so check that a round trip through
        a crate file preserves both the list ops and the set of specs.
        """
        import textwrap

        layerContent = '''#usda 1.0

        def "TargetA" {
            int foo
        }

        def "TargetB" {
            int bar
        }

        def "Connected" {
            int summed.connect = [</TargetA.foo>, </TargetB.bar>]
            rel r2 = </TargetA>
            rel r = [</TargetA>, </Connected.r2[/TargetA]>]
        }

        def "Dot" {
            rel bundles = [
                </Connected.summed[/TargetB.bar]>,
                </Connected.r[/Connected.r2[/TargetA]]>,
                </Connected.summed.mapper[/TargetB.bar]>,
                </Connected.summed.mapper[/TargetB.bar].arg>,
                </Connected.r[/TargetA].relAttr>,
            ]
            int in.connect = [
                </Connected.summed[/TargetB.bar]>,
                </Connected.r[/Connected.r2[/TargetA]]>,
                </Connected.summed.mapper[/TargetB.bar]>,
                </Connected.summed.mapper[/TargetB.bar].arg>,
                </Connected.r[/TargetA].relAttr>,
            ]
        }
        '''

        def _GetSpecPaths(layer):
            paths = []
            layer.Traverse('/', paths.append)
            return sorted(paths)

        def _GetLists(layer):
            return (
                list(layer.GetRelationshipAtPath(
                    '/Dot.bundles').targetPathList.explicitItems),
                list(layer.GetAttributeAtPath(
                    '/Dot.in').connectionPathList.explicitItems))

        textLayer = Sdf.Layer.CreateAnonymous(".usda")
        self.assertTrue(textLayer.ImportFromString(
            textwrap.dedent(layerContent)))

        # Spot-check that the nested target spec exists in the text layer,
        # so the comparison below is meaningful.
        self.assertIn(
            Sdf.Path('/Dot.bundles[/Connected.r[/Connected.r2[/TargetA]]]'),
            _GetSpecPaths(textLayer))

        with tempfile.TemporaryDirectory() as tmpdir:
            usdc = os.path.join(tmpdir, "targets.usdc")
            self.assertTrue(textLayer.Export(usdc))
            crateLayer = Sdf.Layer.OpenAsAnonymous(usdc)
            self.assertTrue(crateLayer)
            self.assertEqual(_GetLists(crateLayer), _GetLists(textLayer))
            self.assertEqual(
                _GetSpecPaths(crateLayer), _GetSpecPaths(textLayer))
            # Release the layer so its file can be removed with tmpdir.
            del crateLayer

if __name__ == "__main__":
    unittest.main()
