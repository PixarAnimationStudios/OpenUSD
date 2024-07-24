#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Usd, UsdShade, UsdGeom, Sdf, Tf

class TestUsdShadeCoordSysAPI(unittest.TestCase):
    def test_CoordSysMultiApply(self):
        stage = Usd.Stage.Open('test.usda')

        # Test CanContainPropertyName
        self.assertTrue(UsdShade.CoordSysAPI.CanContainPropertyName(
            stage.GetPropertyAtPath('/World.coordSys:worldSpace:binding'). \
                    GetName()))
        self.assertFalse(UsdShade.CoordSysAPI.CanContainPropertyName(
            stage.GetPropertyAtPath('/World.xformOp:translate').GetName()))

        world = stage.GetPrimAtPath('/World')
        model = stage.GetPrimAtPath('/World/Model')
        geom = stage.GetPrimAtPath('/World/Model/Geom')
        box = stage.GetPrimAtPath('/World/Model/Geom/Box')

        worldSpace = UsdShade.CoordSysAPI(world, 'worldSpace')
        modelInstanceSpace = UsdShade.CoordSysAPI(model, 'instanceSpace')
        modelModelSpace = UsdShade.CoordSysAPI(model, 'modelSpace')
        modelPaintSpace = UsdShade.CoordSysAPI(model, 'paintSpace')
        modelWorldSpace = UsdShade.CoordSysAPI(model, 'worldSpace')

        # Local Bindings
        self.assertEqual(worldSpace.GetLocalBinding(), 
                ('worldSpace:binding', '/World.coordSys:worldSpace:binding',
                '/World/Space'))
        self.assertEqual(modelInstanceSpace.GetLocalBinding(),
            ('instanceSpace:binding', 
              '/World/Model.coordSys:instanceSpace:binding', '/World/Model'))
        self.assertEqual(modelModelSpace.GetLocalBinding(),
            ('modelSpace:binding', '/World/Model.coordSys:modelSpace:binding',
              '/World/Model/Geom'))
        self.assertEqual(modelPaintSpace.GetLocalBinding(),
            ('paintSpace:binding', '/World/Model.coordSys:paintSpace:binding',
              '/World/Model/Place3dTexture'))
        self.assertEqual(modelWorldSpace.GetLocalBinding(), ('', '', ''))
        self.assertEqual(
            UsdShade.CoordSysAPI.GetLocalBindingsForPrim(model),
            [('instanceSpace:binding',
              '/World/Model.coordSys:instanceSpace:binding',
              '/World/Model'),
             ('modelSpace:binding', '/World/Model.coordSys:modelSpace:binding',
              '/World/Model/Geom'),
             ('paintSpace:binding', '/World/Model.coordSys:paintSpace:binding',
              '/World/Model/Place3dTexture')])
        self.assertEqual(
            UsdShade.CoordSysAPI.GetLocalBindingsForPrim(geom), [])
        self.assertEqual(
            UsdShade.CoordSysAPI.GetLocalBindingsForPrim(box), [])

        # Full (including inherited) Bindings
        self.assertEqual(
            UsdShade.CoordSysAPI.FindBindingsWithInheritanceForPrim(world),
            [('worldSpace:binding', '/World.coordSys:worldSpace:binding', 
              '/World/Space')])
        self.assertEqual(
            UsdShade.CoordSysAPI.FindBindingsWithInheritanceForPrim(model),
            [('instanceSpace:binding',
              '/World/Model.coordSys:instanceSpace:binding',
              '/World/Model'),
             ('modelSpace:binding', '/World/Model.coordSys:modelSpace:binding',
              '/World/Model/Geom'),
             ('paintSpace:binding', '/World/Model.coordSys:paintSpace:binding',
              '/World/Model/Place3dTexture'),
             ('worldSpace:binding', '/World.coordSys:worldSpace:binding', 
              '/World/Space')])
        self.assertEqual(
            UsdShade.CoordSysAPI.FindBindingsWithInheritanceForPrim(geom),
            [('instanceSpace:binding',
              '/World/Model.coordSys:instanceSpace:binding',
              '/World/Model'),
             ('modelSpace:binding', '/World/Model.coordSys:modelSpace:binding',
              '/World/Model/Geom'),
             ('paintSpace:binding', '/World/Model.coordSys:paintSpace:binding',
              '/World/Model/Place3dTexture'),
             ('worldSpace:binding', '/World.coordSys:worldSpace:binding', 
              '/World/Space')])
        self.assertEqual(
            UsdShade.CoordSysAPI.FindBindingsWithInheritanceForPrim(box),
            [('instanceSpace:binding',
              '/World/Model.coordSys:instanceSpace:binding',
              '/World/Model'),
             ('modelSpace:binding', '/World/Model.coordSys:modelSpace:binding',
              '/World/Model/Geom'),
             ('paintSpace:binding', '/World/Model.coordSys:paintSpace:binding',
               '/World/Model/Place3dTexture'),
             ('worldSpace:binding', '/World.coordSys:worldSpace:binding', 
              '/World/Space')])

        # Find binding in self or parent.
        self.assertEqual(
            worldSpace.FindBindingWithInheritance(),
            worldSpace.GetLocalBinding())
        # no targets are specified for /World/Model.coordSys:worldSpace:binding
        # searches above in ancestor for same named binding
        self.assertEqual(
            modelWorldSpace.FindBindingWithInheritance(),
            worldSpace.GetLocalBinding())

        # create binding
        UsdShade.CoordSysAPI.Apply(world, 'modelSpace')
        worldModelSpace = UsdShade.CoordSysAPI(world, 'modelSpace')
        self.assertTrue(worldModelSpace.CreateBindingRel())
        # Bind and Clear using non-static APIs
        self.assertTrue(worldModelSpace.Bind(geom.GetPath()))
        self.assertEqual(worldModelSpace.GetLocalBinding(),
            ('modelSpace:binding', '/World.coordSys:modelSpace:binding',
                '/World/Model/Geom'))
        worldModelSpace.ClearBinding(True)
        self.assertEqual(worldModelSpace.GetLocalBinding(), ('','',''));
        # Bind and Clear Binding using static method APIs

        self.assertTrue(
                UsdShade.CoordSysAPI.Apply(world, 'modelSpace').Bind(
                        geom.GetPath()))
        self.assertEqual(worldModelSpace.GetLocalBinding(),
            ('modelSpace:binding', '/World.coordSys:modelSpace:binding',
                '/World/Model/Geom'))
        self.assertTrue(
                UsdShade.CoordSysAPI.Apply(world, "modelSpace").ClearBinding(
                    True))
        self.assertEqual(worldModelSpace.GetLocalBinding(), ('','',''));

        # BlockBinding using non-static API
        self.assertEqual(worldSpace.GetLocalBinding(),
            ('worldSpace:binding', '/World.coordSys:worldSpace:binding', 
              '/World/Space'))
        self.assertTrue(worldSpace.BlockBinding())
        self.assertEqual(worldSpace.GetLocalBinding(), ('','',''))

        # BlockBinding using static API
        self.assertEqual(
            UsdShade.CoordSysAPI.GetLocalBindingsForPrim(model),
            [('instanceSpace:binding',
              '/World/Model.coordSys:instanceSpace:binding',
              '/World/Model'),
             ('modelSpace:binding', '/World/Model.coordSys:modelSpace:binding',
              '/World/Model/Geom'),
             ('paintSpace:binding', '/World/Model.coordSys:paintSpace:binding',
              '/World/Model/Place3dTexture')])
        self.assertTrue(
            UsdShade.CoordSysAPI.Apply(model, 'instanceSpace').BlockBinding())
        self.assertEqual(
            UsdShade.CoordSysAPI.GetLocalBindingsForPrim(model),
            [('modelSpace:binding', '/World/Model.coordSys:modelSpace:binding',
              '/World/Model/Geom'),
             ('paintSpace:binding', '/World/Model.coordSys:paintSpace:binding',
              '/World/Model/Place3dTexture')])

if __name__ == '__main__':
    unittest.main()
