#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Ar, Sdf, Usd

class TestUsdStageDefaultResolver(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Force Ar to use the default resolver implementation.
        Ar.SetPreferredResolver('ArDefaultResolver')

    def test_StageReload(self):
        def CreateTestRef(stagePath, strVal):
            stage = Usd.Stage.CreateNew(stagePath)
            prim = stage.DefinePrim('/test')
            stage.SetDefaultPrim(prim)
            attr = prim.CreateAttribute('testStr', Sdf.ValueTypeNames.String)
            attr.Set(strVal)
            stage.GetRootLayer().Save()

        Ar.DefaultResolver.SetDefaultSearchPath(['dirA'])

        CreateTestRef('dirA/layer.usda', 'A')
        CreateTestRef('dirB/layer.usda', 'B')

        stage = Usd.Stage.CreateNew("reload.usda")
        prim = stage.DefinePrim('/world')
        stage.SetDefaultPrim(prim)
        prim.GetReferences().AddReference('layer.usda')
        stage.Save()
        
        self._CheckPrimAttrValue(stage, '/world', 'testStr', 'A')

        # Check that updating the default search path will trigger the scene
        # to process the resolver changed notice and update
        Ar.DefaultResolver.SetDefaultSearchPath(['dirB'])
        self._CheckPrimAttrValue(stage, '/world', 'testStr', 'B')


    def _CheckPrimAttrValue(self, stage, primPath, attr, val):
        prim = stage.GetPrimAtPath(primPath)
        self.assertTrue(prim.IsValid())
        attr = prim.GetAttribute(attr)
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), val)

if __name__ == "__main__":
    unittest.main()