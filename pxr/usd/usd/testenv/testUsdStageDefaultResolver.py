#!/pxrpythonsubst
#
# Copyright 2024 Pixar
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