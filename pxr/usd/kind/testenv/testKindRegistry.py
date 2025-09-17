#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Gf, Tf, Kind, Plug
import os, unittest, shutil

class TestKindRegistry(unittest.TestCase):
    def test_Basic(self):
        # Register python module plugins
        Plug.Registry().RegisterPlugins(os.getcwd() + "/**/")

        reg = Kind.Registry()
        self.assertTrue(reg)

        # Test factory default kinds + config file contributions
        expectedDefaultKinds = [
            'group',
            'model',
            'test_model_kind',
            'test_root_kind',
            ]
        actualDefaultKinds = Kind.Registry.GetAllKinds()

        # We cannot expect actual to be equal to expected, because there is no
        # way to prune the site's extension plugins from actual.
        # assertEqual(sorted(expectedDefaultKinds), sorted(actualDefaultKinds))

        for expected in expectedDefaultKinds:
            self.assertTrue( Kind.Registry.HasKind(expected) )
            self.assertTrue( expected in actualDefaultKinds )

        # Check the 'test_model_kind' kind from the TestKindModule_plugInfo.json
        self.assertTrue(Kind.Registry.HasKind('test_root_kind'))
        self.assertEqual(Kind.Registry.GetBaseKind('test_root_kind'), '')

        # Check the 'test_model_kind' kind from the TestKindModule_plugInfo.json
        self.assertTrue(Kind.Registry.HasKind('test_model_kind'))
        self.assertEqual(Kind.Registry.GetBaseKind('test_model_kind'), 'model')

if __name__ == "__main__":
    unittest.main()
