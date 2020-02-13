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
from pxr import Usd, UsdGeom, Gf, Vt
import unittest

class TestUsdAbcUvWrite(unittest.TestCase):
    def test_ReadOldP(self):
      testFile = 'testUsdAbcP_OldEncoding.abc'
      stage = Usd.Stage.Open(testFile)
      self.assertTrue(stage)

      pCylinderShape1 = UsdGeom.Mesh.Get(stage, '/pCylinder1/pCylinderShape1')
      self.assertTrue(pCylinderShape1)
      P = pCylinderShape1.GetPrim().GetProperty('points').Get(0)
      self.assertEqual(len(P), 42)

      knownP = Vt.Vec3fArray(42,
        (Gf.Vec3f(3.8914246559143066, -4.887899398803711, -1.2644004821777344),
         Gf.Vec3f(3.310243606567383, -4.887899398803711, -2.4050326347351074),
         Gf.Vec3f(2.4050326347351074, -4.887899398803711, -3.3102433681488037),
         Gf.Vec3f(1.2644003629684448, -4.887899398803711, -3.8914241790771484),
         Gf.Vec3f(0.0, -4.887899398803711, -4.0916852951049805),
         Gf.Vec3f(-1.2644003629684448, -4.887899398803711, -3.8914239406585693),
         Gf.Vec3f(-2.405032157897949, -4.887899398803711, -3.3102426528930664),
         Gf.Vec3f(-3.3102424144744873, -4.887899398803711, -2.40503191947937),
         Gf.Vec3f(-3.891423225402832, -4.887899398803711, -1.2644000053405762),
         Gf.Vec3f(-4.091684341430664, -4.887899398803711, 0.0),
         Gf.Vec3f(-3.891423225402832, -4.887899398803711, 1.2644000053405762),
         Gf.Vec3f(-3.310242176055908, -4.887899398803711, 2.405031681060791),
         Gf.Vec3f(-2.405031681060791, -4.887899398803711, 3.310241937637329),
         Gf.Vec3f(-1.2644000053405762, -4.887899398803711, 3.891422748565674),
         Gf.Vec3f(-1.21941667430292e-07, -4.887899398803711, 4.091683864593506),
         Gf.Vec3f(1.2643996477127075, -4.887899398803711, 3.8914225101470947),
         Gf.Vec3f(2.405031204223633, -4.887899398803711, 3.31024169921875),
         Gf.Vec3f(3.310241460800171, -4.887899398803711, 2.405031442642212),
         Gf.Vec3f(3.8914222717285156, -4.887899398803711, 1.264399766921997),
         Gf.Vec3f(4.091683387756348, -4.887899398803711, 0.0),
         Gf.Vec3f(3.8914246559143066, 4.887899398803711, -1.2644004821777344),
         Gf.Vec3f(3.310243606567383, 4.887899398803711, -2.4050326347351074),
         Gf.Vec3f(2.4050326347351074, 4.887899398803711, -3.3102433681488037),
         Gf.Vec3f(1.2644003629684448, 4.887899398803711, -3.8914241790771484),
         Gf.Vec3f(0.0, 4.887899398803711, -4.0916852951049805),
         Gf.Vec3f(-1.2644003629684448, 4.887899398803711, -3.8914239406585693),
         Gf.Vec3f(-2.405032157897949, 4.887899398803711, -3.3102426528930664),
         Gf.Vec3f(-3.3102424144744873, 4.887899398803711, -2.40503191947937),
         Gf.Vec3f(-3.891423225402832, 4.887899398803711, -1.2644000053405762),
         Gf.Vec3f(-4.091684341430664, 4.887899398803711, 0.0),
         Gf.Vec3f(-3.891423225402832, 4.887899398803711, 1.2644000053405762),
         Gf.Vec3f(-3.310242176055908, 4.887899398803711, 2.405031681060791),
         Gf.Vec3f(-2.405031681060791, 4.887899398803711, 3.310241937637329),
         Gf.Vec3f(-1.2644000053405762, 4.887899398803711, 3.891422748565674),
         Gf.Vec3f(-1.21941667430292e-07, 4.887899398803711, 4.091683864593506),
         Gf.Vec3f(1.2643996477127075, 4.887899398803711, 3.8914225101470947),
         Gf.Vec3f(2.405031204223633, 4.887899398803711, 3.31024169921875),
         Gf.Vec3f(3.310241460800171, 4.887899398803711, 2.405031442642212),
         Gf.Vec3f(3.8914222717285156, 4.887899398803711, 1.264399766921997),
         Gf.Vec3f(4.091683387756348, 4.887899398803711, 0.0),
         Gf.Vec3f(0.0, -4.887899398803711, 0.0),
         Gf.Vec3f(0.0, 4.887899398803711, 0.0))
      )

      self.assertEqual(P, knownP)


if __name__ == '__main__':
    unittest.main()
