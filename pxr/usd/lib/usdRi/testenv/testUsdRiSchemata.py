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
#

from pxr import Sdf, Usd, UsdRi, UsdShade
import unittest

class TestUsdRiSchemata(unittest.TestCase):

    def _TestOutput(self, schema, getOutputFn, setOutputSrcFn, getFn, 
                    validTargetObjectPath):
        output = getOutputFn(schema)
        assert 'ri:' not in output.GetBaseName()

        assert setOutputSrcFn(schema, validTargetObjectPath)

        output = getOutputFn(schema)
        assert output.GetProperty()

        targetObj = getFn(schema)
        self.assertEqual(targetObj.GetPath(), validTargetObjectPath)

    def test_Basic(self):
        l = Sdf.Layer.CreateAnonymous()
        stage = Usd.Stage.Open(l.identifier)

        world = stage.DefinePrim("/World", "Xform")
        assert world
        world.SetMetadata('kind', 'group')
        assert world.IsModel()
        assert world.IsGroup()

        group = stage.DefinePrim("/World/Group", "Xform")
        assert group
        group.SetMetadata('kind', 'group')
        assert group.IsModel()
        assert group.IsGroup()

        model = stage.DefinePrim("/World/Group/Model", "Xform")
        assert model
        model.SetMetadata('kind', 'component')
        assert model.IsModel()

        p = stage.DefinePrim("/World/Group/Model/Mesh", "Scope")
        assert p

        print ("Test Material")
        material = UsdShade.Material.Define(stage, "/World/Group/Model/Material")
        assert material
        assert material.GetPrim()
        material.Bind(p)

        print ("Test shader")
        shader = UsdRi.RslShader.Define(stage, '/World/Group/Model/Shader')
        assert shader
        assert shader.GetPrim()
        assert not UsdRi.StatementsAPI.IsRiAttribute(shader.GetSloPathAttr())
        shader.GetSloPathAttr().Set('foo')

        print ("Test RiMaterialAPI")
        riMaterial = UsdRi.MaterialAPI.Apply(material.GetPrim())
        assert riMaterial
        assert riMaterial.GetPrim()

        # Test surface output
        self._TestOutput(riMaterial, 
            UsdRi.MaterialAPI.GetSurfaceOutput, 
            UsdRi.MaterialAPI.SetSurfaceSource,
            UsdRi.MaterialAPI.GetSurface,
            shader.GetPath())

        # Test displacement output
        self._TestOutput(riMaterial, 
            UsdRi.MaterialAPI.GetDisplacementOutput, 
            UsdRi.MaterialAPI.SetDisplacementSource,
            UsdRi.MaterialAPI.GetDisplacement,
            shader.GetPath())

        # Test volume output
        self._TestOutput(riMaterial, 
            UsdRi.MaterialAPI.GetVolumeOutput, 
            UsdRi.MaterialAPI.SetVolumeSource,
            UsdRi.MaterialAPI.GetVolume,
            shader.GetPath())

        print ("Test pattern")
        pattern = UsdRi.RisPattern.Define(stage, '/World/Group/Model/Pattern')
        assert pattern
        assert pattern.GetPrim()
        pattern.GetFilePathAttr().Set('foo')
        self.assertEqual (pattern.GetFilePathAttr().Get(), 'foo')
        pattern.GetArgsPathAttr().Set('argspath')
        self.assertEqual (pattern.GetArgsPathAttr().Get(), 'argspath')

        print ("Test oslPattern")
        oslPattern = UsdRi.RisOslPattern.Define(
            stage, '/World/Group/Model/OslPattern')
        assert oslPattern
        assert oslPattern.GetPrim()
        self.assertEqual (oslPattern.GetFilePathAttr().Get(), 'PxrOSL')

        print ("Test bxdf")
        bxdf = UsdRi.RisBxdf.Define(stage, '/World/Group/Model/Bxdf')
        assert bxdf
        assert bxdf.GetPrim()
        bxdf.GetFilePathAttr().Set('foo')
        bxdf.GetArgsPathAttr().Set('argspath')

        print ("Test RIS Material")
        risMaterial = UsdRi.MaterialAPI(material.GetPrim())
        assert risMaterial 
        assert risMaterial.GetPrim()

        print ("Test riStatements")
        riStatements = UsdRi.StatementsAPI.Apply(shader.GetPrim())
        assert riStatements
        assert riStatements.GetPrim()
        attr = riStatements.CreateRiAttribute("ModelName", "string").\
            Set('someModelName')
        assert attr
        props = riStatements.GetRiAttributes()
        assert props
        # this is so convoluted
        attr = riStatements.GetPrim().GetAttribute(props[0].GetName())
        assert attr
        self.assertEqual(attr.GetName(), 'ri:attributes:user:ModelName')
        self.assertEqual(attr.Get(), 'someModelName')
        self.assertEqual(UsdRi.StatementsAPI.GetRiAttributeName(attr), 'ModelName')
        self.assertEqual(UsdRi.StatementsAPI.GetRiAttributeNameSpace(attr), 'user')
        assert UsdRi.StatementsAPI.IsRiAttribute(attr)

        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('myattr'),
                    'ri:attributes:user:myattr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice:myattr'),
                    'ri:attributes:dice:myattr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice.myattr'),
                    'ri:attributes:dice:myattr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice_myattr'),
                    'ri:attributes:dice:myattr')
        # period is stronger separator than underscore, when both are present
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice_my.attr'),
                    'ri:attributes:dice_my:attr')
        # multiple tokens concatted with underscores
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice:my1:long:attr'),
                    'ri:attributes:dice:my1_long_attr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice.my2.long.attr'),
                    'ri:attributes:dice:my2_long_attr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice_my3_long_attr'),
                    'ri:attributes:dice:my3_long_attr')

        self.assertEqual(riStatements.GetCoordinateSystem(), '')
        self.assertEqual(UsdRi.StatementsAPI(model).GetModelCoordinateSystems(), [])
        self.assertEqual(UsdRi.StatementsAPI(model).GetModelScopedCoordinateSystems(), [])
        riStatements.SetCoordinateSystem('LEyeSpace')
        self.assertEqual(riStatements.GetCoordinateSystem(), 'LEyeSpace')
        self.assertEqual(UsdRi.StatementsAPI(model).GetModelCoordinateSystems(),
                    [Sdf.Path('/World/Group/Model/Shader')])
        riStatements.SetScopedCoordinateSystem('ScopedLEyeSpace')
        self.assertEqual(riStatements.GetScopedCoordinateSystem(), 'ScopedLEyeSpace')
        self.assertEqual(UsdRi.StatementsAPI(model).GetModelScopedCoordinateSystems(),
                    [Sdf.Path('/World/Group/Model/Shader')])
        self.assertEqual(UsdRi.StatementsAPI(group).GetModelCoordinateSystems(), [])
        self.assertEqual(UsdRi.StatementsAPI(group).GetModelScopedCoordinateSystems(), [])
        self.assertEqual(UsdRi.StatementsAPI(world).GetModelCoordinateSystems(), [])
        self.assertEqual(UsdRi.StatementsAPI(world).GetModelScopedCoordinateSystems(), [])

        self.assertFalse(riStatements.GetFocusRegionAttr().IsValid())
        assert(riStatements.CreateFocusRegionAttr() is not None)
        assert(riStatements.GetFocusRegionAttr() is not None)
        self.assertTrue(riStatements.GetFocusRegionAttr().IsValid())
        self.assertEqual(riStatements.GetFocusRegionAttr().Get(), None)
        riStatements.CreateFocusRegionAttr(9.0, True)
        self.assertEqual(riStatements.GetFocusRegionAttr().Get(), 9.0)
        
    def test_Metadata(self):
        stage = Usd.Stage.CreateInMemory()
        osl = UsdRi.RisOslPattern.Define(stage, "/osl")
        self.assertTrue( osl.GetFilePathAttr().IsHidden() )

if __name__ == "__main__":
    unittest.main()
