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

if __name__ == "__main__":
    unittest.main()
