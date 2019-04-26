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

import sys, unittest
from pxr import Usd, UsdShade, UsdGeom, Sdf

class TestUsdShadeCoordSysAPI(unittest.TestCase):
    def test_CoordSys(self):
        stage = Usd.Stage.Open('test.usda')

        world = stage.GetPrimAtPath('/World')
        model = stage.GetPrimAtPath('/World/Model')
        geom = stage.GetPrimAtPath('/World/Model/Geom')
        box = stage.GetPrimAtPath('/World/Model/Geom/Box')

        worldCoords = UsdShade.CoordSysAPI(world)
        modelCoords = UsdShade.CoordSysAPI(model)
        geomCoords = UsdShade.CoordSysAPI(geom)
        boxCoords = UsdShade.CoordSysAPI(box)

        # Bindings
        self.assertEqual(
            worldCoords.GetLocalBindings(),
            [('worldSpace', Sdf.Path('/World/Space'))])
        self.assertEqual(
            modelCoords.GetLocalBindings(),
            [('instanceSpace', Sdf.Path('/World/Model')),
             ('modelSpace', Sdf.Path('/World/Model/Geom')),
             ('paintSpace', Sdf.Path('/World/Model/Place3dTexture'))])

        # GetCoordinateSystems
        self.assertEqual(
            [(name, xf.GetPath()) for (name, xf)
            in worldCoords.GetCoordinateSystems()],
            [('worldSpace', Sdf.Path('/World/Space'))])
        self.assertEqual(
            [(name, xf.GetPath()) for (name, xf)
            in modelCoords.GetCoordinateSystems()],
            [('instanceSpace', Sdf.Path('/World/Model')),
             ('modelSpace', Sdf.Path('/World/Model/Geom')),
             ('paintSpace', Sdf.Path('/World/Model/Place3dTexture'))])

        # FindCoordinateSystemsWithInheritance
        self.assertEqual(
            [(name, xf.GetPath()) for (name, xf)
            in worldCoords.FindCoordinateSystemsWithInheritance()],
            [('worldSpace', Sdf.Path('/World/Space'))])
        print modelCoords.FindCoordinateSystemsWithInheritance()
        self.assertEqual(
            [(name, xf.GetPath()) for (name, xf)
            in modelCoords.FindCoordinateSystemsWithInheritance()],
            [('instanceSpace', Sdf.Path('/World/Model')),
             ('modelSpace', Sdf.Path('/World/Model/Geom')),
             ('paintSpace', Sdf.Path('/World/Model/Place3dTexture')),
             ('worldSpace', Sdf.Path('/World/Space'))])

        # Bind
        relName = UsdShade.CoordSysAPI.GetCoordSysRelationshipName('boxSpace')
        self.assertFalse(geom.HasRelationship(relName))
        geomCoords.Bind('boxSpace', box.GetPath())
        self.assertTrue(geom.HasRelationship(relName))
        self.assertEqual(
            geomCoords.GetLocalBindings(),
            [('boxSpace', box.GetPath())])

        # BlockBinding
        self.assertTrue(geom.HasRelationship(relName))
        self.assertTrue(geom.GetRelationship(relName).HasAuthoredTargets())
        self.assertNotEqual(geom.GetRelationship(relName).GetTargets(), [])
        geomCoords.BlockBinding('boxSpace')
        self.assertEqual(
            geomCoords.GetLocalBindings(), [])
        self.assertEqual(geom.GetRelationship(relName).GetTargets(), [])
        self.assertTrue(geom.GetRelationship(relName).HasAuthoredTargets())

        # ClearBinding
        geomCoords.ClearBinding('boxSpace', False)
        self.assertTrue(geom.HasRelationship(relName))
        self.assertFalse(geom.GetRelationship(relName).HasAuthoredTargets())
        geomCoords.ClearBinding('boxSpace', True)
        self.assertFalse(geom.HasRelationship(relName))

        # Bindings to a non-Xformable are not resolved
        relName = UsdShade.CoordSysAPI.GetCoordSysRelationshipName('invalid')
        scope = UsdGeom.Scope.Define(stage, '/World/Model/Geom/Scope')
        geomCoords.Bind('invalid', scope.GetPath())
        self.assertTrue(geom.HasRelationship(relName))
        self.assertTrue(geom.GetRelationship(relName).HasAuthoredTargets())
        self.assertNotEqual(geomCoords.GetLocalBindings(), [])
        self.assertEqual(geomCoords.GetCoordinateSystems(), [])

if __name__ == '__main__':
    unittest.main()
