#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

import os
import math
import unittest

from pxr import Sdf
from pxr import Usd
from pxr import UsdGeom
from pxr import UsdRi
from pxr import Vt
from pxr import Gf

from maya import cmds
from maya import standalone


class testUsdMayaUserExportedAttributes(unittest.TestCase):

    COMMON_ATTR_NAME = 'userProperties:transformAndShapeAttr'

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.file(os.path.abspath('UserExportedAttributesTest.ma'), open=True, force=True)

        usdFilePathFormat = 'UserExportedAttributesTest_EXPORTED_%s.usda'
        
        mergedUsdFilePath = os.path.abspath(usdFilePathFormat % 'MERGED')
        unmergedUsdFilePath = os.path.abspath(usdFilePathFormat % 'UNMERGED')

        cmds.loadPlugin('pxrUsd', quiet=True)

        cmds.usdExport(file=mergedUsdFilePath, mergeTransformAndShape=True)
        cmds.usdExport(file=unmergedUsdFilePath, mergeTransformAndShape=False)

    def _GetExportedStage(self, mergeTransformAndShape=True):
        usdFilePathFormat = 'UserExportedAttributesTest_EXPORTED_%s.usda'
        if mergeTransformAndShape:
            usdFilePath = usdFilePathFormat % 'MERGED'
        else:
            usdFilePath = usdFilePathFormat % 'UNMERGED'
        usdFilePath = os.path.abspath(usdFilePath)

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        return stage

    def testExportAttributes(self):
        """
        Tests that attributes tagged to be exported as USD attributes get
        exported correctly.
        """
        stage = self._GetExportedStage()

        prim = stage.GetPrimAtPath('/UserExportedAttributesTest/Geom/Cube')
        self.assertTrue(prim)

        exportedAttrs = {
            'userProperties:realAttrOne':
                {'value': 42,
                 'typeName': Sdf.ValueTypeNames.Int},
            'my:namespace:realAttrTwo':
                {'value': 3.14,
                 'typeName': Sdf.ValueTypeNames.Double},
            'my:namespace:someNewAttr':
                {'value': 'a string value',
                 'typeName': Sdf.ValueTypeNames.String},
        }

        for attrName in exportedAttrs:
            attr = prim.GetAttribute(attrName)
            self.assertTrue(attr)
            self.assertEqual(attr.Get(), exportedAttrs[attrName]['value'])
            self.assertEqual(attr.GetTypeName(), exportedAttrs[attrName]['typeName'])
            self.assertTrue(attr.IsCustom())

        # This attribute did not specify a USD attribute name which means it
        # should go in the userProperties namespace as tested above. Make sure
        # it does NOT appear outside that namespace.
        attrName = 'realAttrOne'
        attr = prim.GetAttribute(attrName)
        self.assertFalse(attr.IsValid())

        # These attributes are in the JSON but do not exist on the Maya node,
        # so we do NOT expect attributes by any of these names to exist.
        bogusAttrNames = [
            'bogusAttrOne',
            'userProperties:bogusAttrOne',

            'bogusAttrTwo',
            'userProperties:bogusAttrTwo',
            'my:namespace:bogusAttrTwo',

            'bogusRemapAttr',
            'userProperties:bogusRemapAttr',
            'my:namespace:someNewBogusAttr']

        for bogusAttrName in bogusAttrNames:
            attr = prim.GetAttribute(bogusAttrName)
            self.assertFalse(attr.IsValid())

        # An attribute that has multiple tags all specifying the same USD
        # attribute name should result in the first Maya attribute name
        # lexicographically taking precedence.
        attr = prim.GetAttribute('userProperties:multiplyTaggedAttr')
        self.assertTrue(attr)
        self.assertEqual(attr.Get(), 20)

        # Since this test is merging transform and shape nodes, the value on
        # the USD prim for an attribute that is tagged on BOTH Maya nodes
        # should end up coming from the shape node.
        commonAttr = prim.GetAttribute(
            testUsdMayaUserExportedAttributes.COMMON_ATTR_NAME)
        self.assertTrue(commonAttr)
        self.assertEqual(commonAttr.Get(), 'this node is a mesh')

    def testExportAttributesUnmerged(self):
        """
        Tests that attributes tagged to be exported as USD attributes get
        exported correctly when we are NOT merging Maya transforms and shapes.
        """
        stage = self._GetExportedStage(mergeTransformAndShape=False)

        # Since this test is NOT merging transform and shape nodes, there
        # should be a USD prim for each node, and they should have distinct
        # values for similarly named tagged attributes.
        prim = stage.GetPrimAtPath('/UserExportedAttributesTest/Geom/Cube')
        self.assertTrue(prim)
        commonAttr = prim.GetAttribute(
            testUsdMayaUserExportedAttributes.COMMON_ATTR_NAME)
        self.assertTrue(commonAttr)
        self.assertEqual(commonAttr.Get(), 'this node is a transform')

        prim = stage.GetPrimAtPath('/UserExportedAttributesTest/Geom/Cube/CubeShape')
        self.assertTrue(prim)
        commonAttr = prim.GetAttribute(
            testUsdMayaUserExportedAttributes.COMMON_ATTR_NAME)
        self.assertTrue(commonAttr)
        self.assertEqual(commonAttr.Get(), 'this node is a mesh')

    def testExportAttributeTypes(self):
        """
        Tests that attributes tagged to be exported as different attribute types
        (Usd attributes, primvars, UsdRi attributes) are exported correctly.
        """
        stage = self._GetExportedStage()

        prim = stage.GetPrimAtPath('/UserExportedAttributesTest/Geom/CubeTypedAttrs')
        self.assertTrue(prim)

        # Validate Usd attributes.
        attrName = 'userProperties:myIntArrayAttr'
        attr = prim.GetAttribute(attrName)
        self.assertTrue(attr)
        expectedValue = Vt.IntArray([99, 98, 97, 96, 95, 94, 93, 92, 91, 90])
        self.assertEqual(attr.Get(), expectedValue)
        self.assertEqual(attr.GetTypeName(), Sdf.ValueTypeNames.IntArray)
        self.assertTrue(attr.IsCustom())

        # Validate UsdRi attributes.
        expectedRiAttrs = {
            'ri:attributes:user:myIntRiAttr':
                {'value': 42,
                 'typeName': Sdf.ValueTypeNames.Int},
            'ri:attributes:user:myNamespace:myAttr':
                {'value': 'UsdRi string',
                 'typeName': Sdf.ValueTypeNames.String},
            'ri:attributes:user:myStringArrayRiAttr':
                {'value': Vt.StringArray(['the', 'quick', 'brown', 'fox']),
                 'typeName': Sdf.ValueTypeNames.StringArray},
        }
        expectedRiAttrNames = set(expectedRiAttrs.keys())

        riStatements = UsdRi.Statements(prim)
        self.assertTrue(riStatements)

        riAttrs = riStatements.GetRiAttributes()
        self.assertEqual(len(riAttrs), 3)

        riAttrNames = {attr.GetName() for attr in riAttrs}
        self.assertEqual(riAttrNames, expectedRiAttrNames)

        for riAttrName in expectedRiAttrs:
            riAttr = prim.GetAttribute(riAttrName)
            self.assertTrue(riAttr)
            self.assertTrue(UsdRi.Statements.IsRiAttribute(riAttr))
            self.assertEqual(riAttr.Get(), expectedRiAttrs[riAttrName]['value'])
            self.assertEqual(riAttr.GetTypeName(), expectedRiAttrs[riAttrName]['typeName'])

        # Validate primvars.
        expectedPrimvars = {
            'myConstantIntPrimvar':
                {'value': 123,
                 'typeName': Sdf.ValueTypeNames.Int,
                 'interpolation': UsdGeom.Tokens.constant},
            'myFaceVaryingIntPrimvar':
                {'value': 999,
                 'typeName': Sdf.ValueTypeNames.Int,
                 'interpolation': UsdGeom.Tokens.faceVarying},
            'myFloatArrayPrimvar':
                {'value': Vt.FloatArray([1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8]),
                 'typeName': Sdf.ValueTypeNames.FloatArray,
                 'interpolation': UsdGeom.Tokens.vertex},
            'myStringPrimvar':
                {'value': 'no interp string',
                 'typeName': Sdf.ValueTypeNames.String,
                 'interpolation': None},
            'myUniformDoublePrimvar':
                {'value': 3.140,
                 'typeName': Sdf.ValueTypeNames.Double,
                 'interpolation': UsdGeom.Tokens.uniform},
            'myVertexStringPrimvar':
                {'value': 'a vertex string',
                 'typeName': Sdf.ValueTypeNames.String,
                 'interpolation': UsdGeom.Tokens.vertex},
        }
        expectedPrimvarNames = set(expectedPrimvars.keys())
        # Getting all primvars will also include the built-in displayColor,
        # displayOpacity, and st.
        expectedPrimvarNames.update(['displayColor', 'displayOpacity', 'st'])

        gprim = UsdGeom.Gprim(prim)
        self.assertTrue(gprim)

        primvars = gprim.GetPrimvars()
        self.assertEqual(len(primvars), 9)

        primvarNames = {primvar.GetBaseName() for primvar in primvars}
        self.assertEqual(primvarNames, expectedPrimvarNames)

        for primvarName in expectedPrimvars:
            primvar = gprim.GetPrimvar(primvarName)
            self.assertTrue(primvar)
            self.assertTrue(UsdGeom.Primvar.IsPrimvar(primvar))
            self._assertAlmostEqualWithFallback(
                primvar.Get(),
                expectedPrimvars[primvarName]['value'])
            self.assertEqual(primvar.GetTypeName(), expectedPrimvars[primvarName]['typeName'])

            expectedInterpolation = expectedPrimvars[primvarName]['interpolation']
            if expectedInterpolation is None:
                self.assertFalse(primvar.HasAuthoredInterpolation())
            else:
                self.assertTrue(primvar.HasAuthoredInterpolation())
                self.assertEqual(primvar.GetInterpolation(), expectedInterpolation)

    @staticmethod
    def _GetExportedAttributesDict(doubleToSinglePrecision=False):
        """
        Gets a dictionary that maps attribute name prefixes to their values
        and types given the value of doubleToSinglePrecision.
        """
        # To simplify testing, the Maya scene is authored so that attributes of
        # similar types all have the same value, so we define those expected
        # values here. We use self.assertAlmostEqual to compare the values and ensure that
        # the specific Sdf.ValueTypeName of the attribute matches, so we can
        # be a little sloppy with the types of these values.
        BOOL_VALUE = True
        INT_1_VALUE = 42
        INT_2_VALUE = Gf.Vec2i(1, 2)
        INT_3_VALUE = Gf.Vec3i(1, 2, 3)
        ENUM_VALUE = 'Two'
        FLOAT_DOUBLE_1_VALUE = 1.1
        FLOAT_DOUBLE_2_VALUE = Gf.Vec2d(1.1, 2.2)
        FLOAT_DOUBLE_3_VALUE = Gf.Vec3d(1.1, 2.2, 3.3)
        FLOAT_DOUBLE_4_VALUE = Gf.Vec4d(1.1, 2.2, 3.3, 4.4)
        FLOAT_DOUBLE_ANGLE_VALUE = math.pi
        STRING_VALUE = 'foo'
        STRING_ARRAY_VALUE = ['foo', 'bar', 'baz']
        FLOAT_DOUBLE_MATRIX_VALUE = Gf.Matrix4d(
            1.1, 1.1, 1.1, 1.0,
            2.2, 2.2, 2.2, 1.0,
            3.3, 3.3, 3.3, 1.0,
            1.0, 1.0, 1.0, 1.0)
        FLOAT_DOUBLE_ARRAY_VALUE = [1.1, 2.2, 3.3]
        INT_ARRAY_VALUE = [1, 2, 3]
        FLOAT_DOUBLE_VECTOR_POINT_ARRAY_VALUE = Vt.Vec3dArray([
            (1.1, 1.1, 1.1),
            (2.2, 2.2, 2.2),
            (3.3, 3.3, 3.3)])

        exportedAttrsDict = {
            'myBool':         {'value': BOOL_VALUE,
                               'typeName': Sdf.ValueTypeNames.Bool},
            'myByte':         {'value': INT_1_VALUE,
                               'typeName': Sdf.ValueTypeNames.Int},
            'myChar':         {'value': INT_1_VALUE,
                               'typeName': Sdf.ValueTypeNames.Int},
            'myShort':        {'value': INT_1_VALUE,
                               'typeName': Sdf.ValueTypeNames.Int},
            'myLong':         {'value': INT_1_VALUE,
                               'typeName': Sdf.ValueTypeNames.Int},
            'myEnum':         {'value': ENUM_VALUE,
                               'typeName': Sdf.ValueTypeNames.Token},
            'myFloat':        {'value': FLOAT_DOUBLE_1_VALUE,
                               'typeName': Sdf.ValueTypeNames.Float},
            'myDouble':       {'value': FLOAT_DOUBLE_1_VALUE,
                               'typeName': Sdf.ValueTypeNames.Double},
            'myDoubleAngle':  {'value': FLOAT_DOUBLE_ANGLE_VALUE,
                               'typeName': Sdf.ValueTypeNames.Double},
            'myDoubleLinear': {'value': FLOAT_DOUBLE_1_VALUE,
                               'typeName': Sdf.ValueTypeNames.Double},
            'myString':       {'value': STRING_VALUE,
                               'typeName': Sdf.ValueTypeNames.String},
            'myStringArray':  {'value': STRING_ARRAY_VALUE,
                               'typeName': Sdf.ValueTypeNames.StringArray},
            'myDoubleMatrix': {'value': FLOAT_DOUBLE_MATRIX_VALUE,
                               'typeName': Sdf.ValueTypeNames.Matrix4d},
            'myFloatMatrix':  {'value': FLOAT_DOUBLE_MATRIX_VALUE,
                               'typeName': Sdf.ValueTypeNames.Matrix4d},
            'myFloat2':       {'value': FLOAT_DOUBLE_2_VALUE,
                               'typeName': Sdf.ValueTypeNames.Float2},
            'myFloat3':       {'value': FLOAT_DOUBLE_3_VALUE,
                               'typeName': Sdf.ValueTypeNames.Float3},
            'myDouble2':      {'value': FLOAT_DOUBLE_2_VALUE,
                               'typeName': Sdf.ValueTypeNames.Double2},
            'myDouble3':      {'value': FLOAT_DOUBLE_3_VALUE,
                               'typeName': Sdf.ValueTypeNames.Double3},
            'myDouble4':      {'value': FLOAT_DOUBLE_4_VALUE,
                               'typeName': Sdf.ValueTypeNames.Double4},
            'myLong2':        {'value': INT_2_VALUE,
                               'typeName': Sdf.ValueTypeNames.Int2},
            'myLong3':        {'value': INT_3_VALUE,
                               'typeName': Sdf.ValueTypeNames.Int3},
            'myShort2':       {'value': INT_2_VALUE,
                               'typeName': Sdf.ValueTypeNames.Int2},
            'myShort3':       {'value': INT_3_VALUE,
                               'typeName': Sdf.ValueTypeNames.Int3},
            'myDoubleArray':  {'value': FLOAT_DOUBLE_ARRAY_VALUE,
                               'typeName': Sdf.ValueTypeNames.DoubleArray},
            'myFloatArray':   {'value': FLOAT_DOUBLE_ARRAY_VALUE,
                               'typeName': Sdf.ValueTypeNames.FloatArray},
            'myIntArray':     {'value': INT_ARRAY_VALUE,
                               'typeName': Sdf.ValueTypeNames.IntArray},
            'myVectorArray':  {'value': FLOAT_DOUBLE_VECTOR_POINT_ARRAY_VALUE,
                               'typeName': Sdf.ValueTypeNames.Vector3dArray},
            'myPointArray':   {'value': FLOAT_DOUBLE_VECTOR_POINT_ARRAY_VALUE,
                               'typeName': Sdf.ValueTypeNames.Point3dArray}
        }

        if doubleToSinglePrecision:
            # Patch the dictionary with float-based types for the appropriate
            # attributes.
            exportedAttrsDict['myDouble']['typeName'] = Sdf.ValueTypeNames.Float
            exportedAttrsDict['myDoubleAngle']['typeName'] = Sdf.ValueTypeNames.Float
            exportedAttrsDict['myDoubleLinear']['typeName'] = Sdf.ValueTypeNames.Float
            exportedAttrsDict['myDouble2']['typeName'] = Sdf.ValueTypeNames.Float2
            exportedAttrsDict['myDouble3']['typeName'] = Sdf.ValueTypeNames.Float3
            exportedAttrsDict['myDouble4']['typeName'] = Sdf.ValueTypeNames.Float4
            exportedAttrsDict['myDoubleArray']['typeName'] = Sdf.ValueTypeNames.FloatArray
            exportedAttrsDict['myVectorArray']['typeName'] = Sdf.ValueTypeNames.Vector3fArray
            exportedAttrsDict['myPointArray']['typeName'] = Sdf.ValueTypeNames.Point3fArray

        return exportedAttrsDict

    def _ValidateAllAttributes(self, usdPrim, exportedAttrsDict):
        """
        Validates that USD attributes, primvars, and UsdRi attributes exist,
        have the correct value, and have the correct type for each attribute
        prefix in exportedAttrsDict.
        """
        self.assertTrue(usdPrim)

        gprim = UsdGeom.Gprim(usdPrim)
        self.assertTrue(gprim)

        # Do a quick check to see whether we have the right number of attributes
        # by counting the primvars and UsdRi attributes.
        # Getting all primvars will also include the built-in displayColor,
        # displayOpacity, and st, so we add 3 to the length of the
        # exportedAttrsDict for that check.
        primvars = gprim.GetPrimvars()
        self.assertEqual(len(primvars), len(exportedAttrsDict) + 3)

        riStatements = UsdRi.Statements(usdPrim)
        self.assertTrue(riStatements)

        riAttrs = riStatements.GetRiAttributes()
        self.assertEqual(len(riAttrs), len(exportedAttrsDict))

        # UsdRi attributes store vector and point array data as just arrays of
        # three doubles/floats.
        riAttrMapping = {
            Sdf.ValueTypeNames.Vector3dArray: Sdf.ValueTypeNames.Double3Array,
            Sdf.ValueTypeNames.Vector3fArray: Sdf.ValueTypeNames.Float3Array,
            Sdf.ValueTypeNames.Point3dArray: Sdf.ValueTypeNames.Double3Array,
            Sdf.ValueTypeNames.Point3fArray: Sdf.ValueTypeNames.Float3Array
        }

        for attrPrefix in exportedAttrsDict:
            # Test regular USD attributes.
            usdAttr = usdPrim.GetAttribute('userProperties:%sUsdAttr' % attrPrefix)
            value = exportedAttrsDict[attrPrefix]['value']
            self.assertTrue(usdAttr)
            self.assertEqual(usdAttr.GetTypeName(),
                             exportedAttrsDict[attrPrefix]['typeName'])
            self.assertTrue(usdAttr.IsCustom())
            self._assertAlmostEqualWithFallback(usdAttr.Get(), value)

            # Test primvars.
            primvar = gprim.GetPrimvar('%sPrimvar' % attrPrefix)
            self.assertTrue(primvar)
            self.assertTrue(UsdGeom.Primvar.IsPrimvar(primvar))
            self._assertAlmostEqualWithFallback(primvar.Get(), value)
            self.assertEqual(primvar.GetTypeName(), exportedAttrsDict[attrPrefix]['typeName'])

            # Test UsdRi attributes.
            riAttr = usdPrim.GetAttribute('ri:attributes:user:%sUsdRiAttr' % attrPrefix)
            self.assertTrue(riAttr)
            self.assertTrue(UsdRi.Statements.IsRiAttribute(riAttr))
            self._assertAlmostEqualWithFallback(riAttr.Get(), value)

            riAttrType = exportedAttrsDict[attrPrefix]['typeName']
            riAttrType = riAttrMapping.get(riAttrType, riAttrType)
            self.assertEqual(riAttr.GetTypeName(), riAttrType)

    def _assertAlmostEqualWithFallback(self, left, right):
        try:
            self.assertAlmostEqual(left, right, delta=5)
        except TypeError as err:
            len_left = len(left)
            self.assertEqual(len_left, len(right))

            for idx in range(len_left):

                if type(left[idx]) in {Gf.Vec3f, Gf.Vec3d}:
                    a, b = Gf.Vec3d(left[idx]), Gf.Vec3d(right[idx])
                    self.assertTrue(Gf.IsClose(a, b, 1.0e-5))
                    continue

                if type(left[idx]) in {Gf.Vec4d, Gf.Vec4f}:
                    a, b = Gf.Vec4d(left[idx]), Gf.Vec4d(right[idx])
                    self.assertTrue(Gf.IsClose(a, b, 1.0e-5))
                    continue

                self.assertAlmostEqual(left[idx], right[idx], delta=5)

    def testAllMayaAttributeTypesExport(self):
        """
        Tests that (almost) all Maya attributes that can be created with addAttr
        and are tagged to be exported as USD attributes get exported correctly.
        """
        usdStage = self._GetExportedStage()
        usdPrim = usdStage.GetPrimAtPath(
            '/UserExportedAttributesTest/Geom/AllMayaTypesTestingCubes/AllTypesCube')

        exportedAttrsDict = self._GetExportedAttributesDict()

        self._ValidateAllAttributes(usdPrim, exportedAttrsDict)

    def testAllMayaAttributeTypesExportTranslateDoubleToSinglePrecision(self):
        """
        Tests that (almost) all Maya attributes that can be created with addAttr
        and are tagged to be exported as USD attributes get exported correctly,
        and that when they are tagged with translateMayaDoubleToUsdSinglePrecision,
        any attributes that are double-based in Maya should be float-based in USD.
        """
        usdStage = self._GetExportedStage()
        usdPrim = usdStage.GetPrimAtPath(
            '/UserExportedAttributesTest/Geom/AllMayaTypesTestingCubes/AllTypesCastDoubleToFloatCube')

        exportedAttrsDict = self._GetExportedAttributesDict(True)

        self._ValidateAllAttributes(usdPrim, exportedAttrsDict)


if __name__ == '__main__':
    unittest.main(verbosity=2)
