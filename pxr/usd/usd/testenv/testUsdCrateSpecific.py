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

if __name__ == "__main__":
    unittest.main()
