#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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
from pxr import Usd, UsdGeom, UsdVol, Sdf

class TestUsdVolVolume(unittest.TestCase):

    def test_VolumeFieldManagement(self):
        # We'll set up a stage where we have a referenced volume and fields,
        # so that we can test blocking field rels.
        stage = Usd.Stage.CreateInMemory('volumeFieldManagement.usda')
        base = UsdGeom.Xform.Define(stage, '/base')
        vol = UsdVol.Volume.Define(stage, '/base/volume')
        f1 = UsdVol.Field3DAsset.Define(stage, '/base/volume/f3dField')
        f2 = UsdVol.OpenVDBAsset.Define(stage, '/base/volume/vdbField')

        vol.CreateFieldRelationship('diffuse', f1.GetPath())
        vol.CreateFieldRelationship('emissive', f2.GetPath())

        refed = stage.DefinePrim('/volModel')
        refed.GetReferences().AddInternalReference(base.GetPath())
        refVol = UsdVol.Volume.Get(stage, '/volModel/volume')
        
        self.assertTrue(refVol)
        self.assertTrue(refVol.HasFieldRelationship('diffuse'))
        self.assertEqual(refVol.GetFieldPath('diffuse'), 
                         Sdf.Path('/volModel/volume/f3dField'))
        
        fieldMap = refVol.GetFieldPaths()
        self.assertEqual(len(fieldMap), 2)
        self.assertEqual(fieldMap['diffuse'], 
                        Sdf.Path('/volModel/volume/f3dField'))
        self.assertEqual(fieldMap['emissive'], 
                        Sdf.Path('/volModel/volume/vdbField'))
        
        self.assertEqual(refVol.GetFieldPath('diffuse'), 
                        Sdf.Path('/volModel/volume/f3dField'))
        self.assertTrue(refVol.HasFieldRelationship('diffuse'))
        
        # Now block diffuse, and ensure API reflects it.
        refVol.BlockFieldRelationship('diffuse')
        fieldMap2 = refVol.GetFieldPaths()
        self.assertEqual(len(fieldMap2), 1)
        self.assertTrue(refVol.GetFieldPath('diffuse').isEmpty)
        self.assertTrue(refVol.HasFieldRelationship('diffuse'))

        # Add a field to a bogus prim path.  We do not attempt to 
        # validate bad paths in the API
        refVol.CreateFieldRelationship('specular', '/no/such/prim')
        fieldMap3 = refVol.GetFieldPaths()
        self.assertEqual(len(fieldMap3), 2)
        self.assertFalse(refVol.GetFieldPath('specular').isEmpty)
        self.assertTrue(refVol.HasFieldRelationship('specular'))
        
        # re-establish diffuse, locally
        refVol.CreateFieldRelationship('diffuse', '/volModel/volume/f3dField')
        fieldMap4 = refVol.GetFieldPaths()
        self.assertEqual(len(fieldMap4), 3)
        self.assertEqual(fieldMap4['diffuse'],
                        Sdf.Path('/volModel/volume/f3dField'))
        self.assertEqual(fieldMap4['emissive'],
                        Sdf.Path('/volModel/volume/vdbField'))
        


if __name__ == "__main__":
    unittest.main()
