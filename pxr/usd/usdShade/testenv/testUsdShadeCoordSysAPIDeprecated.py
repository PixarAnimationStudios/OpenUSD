#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Usd, UsdShade, UsdGeom, Sdf, Tf

class TestUsdShadeCoordSysAPI(unittest.TestCase):
    def test_CoordSysNonAppliedDeprecated(self):
        stage = Usd.Stage.Open('test.usda')

        # Test CanContainPropertyName
        self.assertTrue(UsdShade.CoordSysAPI.CanContainPropertyName(
            stage.GetPropertyAtPath('/World_old.coordSys:worldSpace').GetName()))
        self.assertFalse(UsdShade.CoordSysAPI.CanContainPropertyName(
            stage.GetPropertyAtPath('/World_old.xformOp:translate').GetName()))

        world = stage.GetPrimAtPath('/World_old')
        model = stage.GetPrimAtPath('/World_old/Model')
        geom = stage.GetPrimAtPath('/World_old/Model/Geom')
        box = stage.GetPrimAtPath('/World_old/Model/Geom/Box')

        # Updated these to use constructor for multi-apply API with a noop
        # instanceName
        worldCoords = UsdShade.CoordSysAPI(world, 'noop')
        modelCoords = UsdShade.CoordSysAPI(model, 'noop')
        geomCoords = UsdShade.CoordSysAPI(geom, 'noop')
        boxCoords = UsdShade.CoordSysAPI(box, 'noop')

        # Local Bindings
        self.assertEqual(
            worldCoords.GetLocalBindings(),
            [('worldSpace', '/World_old.coordSys:worldSpace', '/World_old/Space')])
        self.assertEqual(
            modelCoords.GetLocalBindings(),
            [('instanceSpace', '/World_old/Model.coordSys:instanceSpace',
              '/World_old/Model'),
             ('modelSpace', '/World_old/Model.coordSys:modelSpace',
              '/World_old/Model/Geom'),
             ('paintSpace', '/World_old/Model.coordSys:paintSpace',
               '/World_old/Model/Place3dTexture')])
        self.assertEqual(
            geomCoords.GetLocalBindings(), [])
        self.assertEqual(
            boxCoords.GetLocalBindings(), [])

        # Full (including inherited) Bindings
        self.assertEqual(
            worldCoords.FindBindingsWithInheritance(),
            [('worldSpace', '/World_old.coordSys:worldSpace', '/World_old/Space')])
        self.assertEqual(
            modelCoords.FindBindingsWithInheritance(),
            [('instanceSpace', '/World_old/Model.coordSys:instanceSpace',
              '/World_old/Model'),
             ('modelSpace', '/World_old/Model.coordSys:modelSpace',
              '/World_old/Model/Geom'),
             ('paintSpace', '/World_old/Model.coordSys:paintSpace',
               '/World_old/Model/Place3dTexture'),
             ('worldSpace', '/World_old.coordSys:worldSpace', '/World_old/Space')])
        self.assertEqual(
            geomCoords.FindBindingsWithInheritance(),
            [('instanceSpace', '/World_old/Model.coordSys:instanceSpace',
              '/World_old/Model'),
             ('modelSpace', '/World_old/Model.coordSys:modelSpace',
              '/World_old/Model/Geom'),
             ('paintSpace', '/World_old/Model.coordSys:paintSpace',
               '/World_old/Model/Place3dTexture'),
             ('worldSpace', '/World_old.coordSys:worldSpace', '/World_old/Space')])
        self.assertEqual(
            boxCoords.FindBindingsWithInheritance(),
            [('instanceSpace', '/World_old/Model.coordSys:instanceSpace',
              '/World_old/Model'),
             ('modelSpace', '/World_old/Model.coordSys:modelSpace',
              '/World_old/Model/Geom'),
             ('paintSpace', '/World_old/Model.coordSys:paintSpace',
               '/World_old/Model/Place3dTexture'),
             ('worldSpace', '/World_old.coordSys:worldSpace', '/World_old/Space')])

        # Bind
        relName = UsdShade.CoordSysAPI.GetCoordSysRelationshipName('boxSpace')
        self.assertFalse(geom.HasRelationship(relName))
        geomCoords.Bind('boxSpace', box.GetPath())
        self.assertTrue(geom.HasRelationship(relName))
        self.assertEqual(
            geomCoords.GetLocalBindings(),
            [('boxSpace', '/World_old/Model/Geom.coordSys:boxSpace',
              box.GetPath())])
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

if __name__ == '__main__':
    unittest.main()
