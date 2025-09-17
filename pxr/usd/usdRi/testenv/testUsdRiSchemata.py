#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function

from pxr import Tf, Sdf, Usd, UsdRi, UsdShade
import unittest

class TestUsdRiSchemata(unittest.TestCase):

    def _TestOutput(self, schema, getOutputFn, setOutputSrcFn, getFn, 
                    validTargetObjectPath):
        output = getOutputFn(schema)
        assert 'ri:' in output.GetBaseName()

        assert setOutputSrcFn(schema, validTargetObjectPath)

        output = getOutputFn(schema)
        assert output.GetAttr()

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
        UsdShade.MaterialBindingAPI.Apply(p).Bind(material)

        print ("Test RiMaterialAPI")
        riMaterial = UsdRi.MaterialAPI.Apply(material.GetPrim())
        assert riMaterial
        assert riMaterial.GetPrim()

        print ("Test RIS Material")
        risMaterial = UsdRi.MaterialAPI(material.GetPrim())
        assert risMaterial 
        assert risMaterial.GetPrim()

        print ("Test riStatements")
        riStatements = UsdRi.StatementsAPI.Apply(model.GetPrim())
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
        prefix = 'primvars:'
        self.assertEqual(attr.GetName(),
            prefix+'ri:attributes:user:ModelName')
        self.assertEqual(attr.Get(), 'someModelName')
        self.assertEqual(UsdRi.StatementsAPI.GetRiAttributeName(attr), 'ModelName')
        self.assertEqual(UsdRi.StatementsAPI.GetRiAttributeNameSpace(attr), 'user')
        assert UsdRi.StatementsAPI.IsRiAttribute(attr)

        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('myattr'),
                    prefix+'ri:attributes:user:myattr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice:myattr'),
                    prefix+'ri:attributes:dice:myattr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice.myattr'),
                    prefix+'ri:attributes:dice:myattr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice_myattr'),
                    prefix+'ri:attributes:dice:myattr')
        # period is stronger separator than underscore, when both are present
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice_my.attr'),
                    prefix+'ri:attributes:dice_my:attr')
        # multiple tokens concatted with underscores
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice:my1:long:attr'),
                    prefix+'ri:attributes:dice:my1_long_attr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice.my2.long.attr'),
                    prefix+'ri:attributes:dice:my2_long_attr')
        self.assertEqual(UsdRi.StatementsAPI.MakeRiAttributePropertyName('dice_my3_long_attr'),
                    prefix+'ri:attributes:dice:my3_long_attr')

        self.assertEqual(riStatements.GetCoordinateSystem(), '')
        self.assertEqual(UsdRi.StatementsAPI(model).GetModelCoordinateSystems(), [])
        self.assertEqual(UsdRi.StatementsAPI(model).GetModelScopedCoordinateSystems(), [])
        riStatements.SetCoordinateSystem('LEyeSpace')
        self.assertEqual(riStatements.GetCoordinateSystem(), 'LEyeSpace')
        self.assertEqual(UsdRi.StatementsAPI(model).GetModelCoordinateSystems(),
                    [Sdf.Path('/World/Group/Model')])
        riStatements.SetScopedCoordinateSystem('ScopedLEyeSpace')
        self.assertEqual(riStatements.GetScopedCoordinateSystem(), 'ScopedLEyeSpace')
        self.assertEqual(UsdRi.StatementsAPI(model).GetModelScopedCoordinateSystems(),
                    [Sdf.Path('/World/Group/Model')])
        self.assertEqual(UsdRi.StatementsAPI(group).GetModelCoordinateSystems(), [])
        self.assertEqual(UsdRi.StatementsAPI(group).GetModelScopedCoordinateSystems(), [])
        self.assertEqual(UsdRi.StatementsAPI(world).GetModelCoordinateSystems(), [])
        self.assertEqual(UsdRi.StatementsAPI(world).GetModelScopedCoordinateSystems(), [])

        # Test mixed old & new style encodings
        if Tf.GetEnvSetting('USDRI_STATEMENTS_READ_OLD_ATTR_ENCODING'):
            prim  = stage.DefinePrim("/prim")
            riStatements = UsdRi.StatementsAPI.Apply(prim)
            self.assertEqual(len(riStatements.GetRiAttributes()), 0)
            # Add new-style
            newStyleAttr = riStatements.CreateRiAttribute('newStyle', 'string')
            newStyleAttr.Set('new')
            self.assertEqual(len(riStatements.GetRiAttributes()), 1)
            # Add old-style (note that we can't use UsdRi API for this,
            # since it doesn't let the caller choose the encoding)
            oldStyleAttr = prim.CreateAttribute('ri:attributes:user:oldStyle',
                Sdf.ValueTypeNames.String)
            oldStyleAttr.Set('old')
            self.assertEqual(len(riStatements.GetRiAttributes()), 2)
            # Exercise the case of an Ri attribute encoded in both
            # old and new styles.
            ignoredAttr = prim.CreateAttribute('ri:attributes:user:newStyle',
                Sdf.ValueTypeNames.String)
            self.assertEqual(len(riStatements.GetRiAttributes()), 2)
            self.assertFalse(ignoredAttr in riStatements.GetRiAttributes())

if __name__ == "__main__":
    unittest.main()
