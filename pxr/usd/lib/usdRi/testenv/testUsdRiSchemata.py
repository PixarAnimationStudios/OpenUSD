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
    def _SingleTargetRel(self, schema, getRelFn, createRelFn, 
                         getTargetFn, validTargetObjectPath):
        rel = getRelFn(schema)
        assert not rel
        rel = createRelFn(schema)
        assert rel

        # Add a valid target object
        rel.AppendTarget(validTargetObjectPath)
        assert getTargetFn(schema)
        # Add one more target to the rel which should cause it to 
        # return nothing since it expects exactly one valid target.
        rel.AppendTarget(schema.GetPrim().GetPath())
        assert not getTargetFn(schema)
        # Clear targets and add one again that is not a prim path, that should
        # also be an error condition.
        # XXX:Note here is where we'd like a validation scheme for USD.
        rel.ClearTargets(True)
        rel.AppendTarget(schema.GetPrim().GetPath())
        assert not getTargetFn(schema)
        # Clean up
        rel.ClearTargets(True)
        assert not getTargetFn(schema)

    def _MultiTargetRel(self, schema, getRelFn, createRelFn, 
                        getTargetFn, validTargetObjectPath):
        rel = getRelFn(schema)
        assert not rel
        rel = createRelFn(schema)
        assert rel

        rel.AppendTarget(validTargetObjectPath)
        assert rel
        assert len(getTargetFn(schema)) == 1
        # Add an invalid target and make sure that we still get one valid object.
        rel.AppendTarget(schema.GetPrim().GetPath())
        assert len(getTargetFn(schema)) == 1
        # Clean up
        rel.ClearTargets(True)


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

        print ("Test look")
        material = UsdShade.Material.Define(stage, "/World/Group/Model/Look")
        assert material
        assert material.GetPrim()
        material.Bind(p)

        print ("Test shader")
        shader = UsdRi.RslShader.Define(stage, '/World/Group/Model/Shader')
        assert shader
        assert shader.GetPrim()
        assert not UsdRi.Statements.IsRiAttribute(shader.GetSloPathAttr())
        shader.GetSloPathAttr().Set('foo')

        print ("Test RiLookAPI")
        rilook = UsdRi.LookAPI(material)
        assert rilook 
        assert rilook.GetPrim()

        # Test surface rel
        self._SingleTargetRel(rilook, 
            UsdRi.LookAPI.GetSurfaceRel, 
            UsdRi.LookAPI.CreateSurfaceRel,
            UsdRi.LookAPI.GetSurface,
            shader.GetPath())

        # Test displacement rel
        self._SingleTargetRel(rilook, 
            UsdRi.LookAPI.GetDisplacementRel, 
            UsdRi.LookAPI.CreateDisplacementRel,
            UsdRi.LookAPI.GetDisplacement,
            shader.GetPath())

        # Test volume rel
        self._SingleTargetRel(rilook, 
            UsdRi.LookAPI.GetVolumeRel, 
            UsdRi.LookAPI.CreateVolumeRel,
            UsdRi.LookAPI.GetVolume,
            shader.GetPath())

        # Test coshaders rel
        self._MultiTargetRel(rilook,
            UsdRi.LookAPI.GetCoshadersRel,
            UsdRi.LookAPI.CreateCoshadersRel,
            UsdRi.LookAPI.GetCoshaders,
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

        print ("Test RIS Look")
        rislook = UsdRi.LookAPI(material.GetPrim())
        assert rislook 
        assert rislook.GetPrim()
        assert not rislook.GetBxdf()
        assert len(rislook.GetPatterns()) == 0

        # Test the bxdf relationship
        self._SingleTargetRel(rislook,
            UsdRi.LookAPI.GetBxdfRel,
            UsdRi.LookAPI.CreateBxdfRel,
            UsdRi.LookAPI.GetBxdf,
            bxdf.GetPath())

        # Test the patterns relationship
        self._MultiTargetRel(rislook,
            UsdRi.LookAPI.GetPatternsRel,
            UsdRi.LookAPI.CreatePatternsRel,
            UsdRi.LookAPI.GetPatterns,
            pattern.GetPath())

        print ("Test riStatements")
        riStatements = UsdRi.Statements(shader.GetPrim())
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
        self.assertEqual(UsdRi.Statements.GetRiAttributeName(attr), 'ModelName')
        self.assertEqual(UsdRi.Statements.GetRiAttributeNameSpace(attr), 'user')
        assert UsdRi.Statements.IsRiAttribute(attr)

        self.assertEqual(UsdRi.Statements.MakeRiAttributePropertyName('myattr'),
                    'ri:attributes:user:myattr')
        self.assertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice:myattr'),
                    'ri:attributes:dice:myattr')
        self.assertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice.myattr'),
                    'ri:attributes:dice:myattr')
        self.assertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice_myattr'),
                    'ri:attributes:dice:myattr')
        # period is stronger separator than underscore, when both are present
        self.assertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice_my.attr'),
                    'ri:attributes:dice_my:attr')
        # multiple tokens concatted with underscores
        self.assertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice:my1:long:attr'),
                    'ri:attributes:dice:my1_long_attr')
        self.assertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice.my2.long.attr'),
                    'ri:attributes:dice:my2_long_attr')
        self.assertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice_my3_long_attr'),
                    'ri:attributes:dice:my3_long_attr')

        self.assertEqual(riStatements.GetCoordinateSystem(), '')
        self.assertEqual(UsdRi.Statements(model).GetModelCoordinateSystems(), [])
        self.assertEqual(UsdRi.Statements(model).GetModelScopedCoordinateSystems(), [])
        riStatements.SetCoordinateSystem('LEyeSpace')
        self.assertEqual(riStatements.GetCoordinateSystem(), 'LEyeSpace')
        self.assertEqual(UsdRi.Statements(model).GetModelCoordinateSystems(),
                    [Sdf.Path('/World/Group/Model/Shader')])
        riStatements.SetScopedCoordinateSystem('ScopedLEyeSpace')
        self.assertEqual(riStatements.GetScopedCoordinateSystem(), 'ScopedLEyeSpace')
        self.assertEqual(UsdRi.Statements(model).GetModelScopedCoordinateSystems(),
                    [Sdf.Path('/World/Group/Model/Shader')])
        self.assertEqual(UsdRi.Statements(group).GetModelCoordinateSystems(), [])
        self.assertEqual(UsdRi.Statements(group).GetModelScopedCoordinateSystems(), [])
        self.assertEqual(UsdRi.Statements(world).GetModelCoordinateSystems(), [])
        self.assertEqual(UsdRi.Statements(world).GetModelScopedCoordinateSystems(), [])

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
