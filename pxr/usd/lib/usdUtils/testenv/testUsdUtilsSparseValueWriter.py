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

from pxr import UsdUtils, Usd, Sdf, UsdGeom, Gf, Tf

import unittest

class TestUsdUtilsSparseAuthoring(unittest.TestCase):
    def TestAttrOfType(self, typeName, arrayTypeName, defaultValue, epsilon):
        s = Usd.Stage.CreateInMemory()
        scope = UsdGeom.Scope.Define(s, "/Scope")

        # Test scalar valued attribute.
        attr = scope.GetPrim().CreateAttribute("attr", typeName)
        self.assertFalse(attr.HasAuthoredValueOpinion())

        valueWriter = UsdUtils.SparseAttrValueWriter(attr, 
                defaultValue=defaultValue)
        self.assertTrue(attr.HasAuthoredValueOpinion())

        valueType = type(defaultValue)
        valueWriter.SetTimeSample(value=valueType(10.), time=0.0)
        valueWriter.SetTimeSample(value=valueType(10.), time=0.5)
        valueWriter.SetTimeSample(value=valueType(10.), time=1.0)
        valueWriter.SetTimeSample(value=valueType(20.), time=2.0)
        valueWriter.SetTimeSample(value=valueType(20.), time=3.0)
        valueWriter.SetTimeSample(value=valueType(20.), time=4.0)
        valueWriter.SetTimeSample(value=valueType(30.), time=5.0)
        valueWriter.SetTimeSample(value=valueType(30. + (epsilon / 2)), 
                                  time=6.0)
        valueWriter.SetTimeSample(value=valueType(30.), time=7.0)
        valueWriter.SetTimeSample(value=valueType(30. + (epsilon * 2)), 
                                  time=8.0)
        expectedTimeSamples = [1.0, 2.0, 4.0, 5.0, 7.0, 8.0] if \
                              defaultValue == valueType(10.0) else \
                              [0.0, 1.0, 2.0, 4.0, 5.0, 7.0, 8.0]
        self.assertEqual(attr.GetTimeSamples(), expectedTimeSamples)

        # Test array valued attribute.
        arrayAttr = scope.GetPrim().CreateAttribute("arrayAttr", arrayTypeName)
        self.assertFalse(arrayAttr.HasAuthoredValueOpinion())
        
        arrayValueWriter = UsdUtils.SparseAttrValueWriter(arrayAttr, 
                defaultValue=[defaultValue, defaultValue])
        self.assertTrue(arrayAttr.HasAuthoredValueOpinion())

        arrayValueWriter.SetTimeSample(value=[valueType(10.), valueType(10.)], 
                                       time=0.0)
        arrayValueWriter.SetTimeSample(value=[valueType(10.), valueType(10.)], 
                                       time=0.5)
        arrayValueWriter.SetTimeSample(value=[valueType(10.), valueType(10.)], 
                                       time=1.0)
        arrayValueWriter.SetTimeSample(value=[valueType(20.), valueType(10.)], 
                                       time=2.0)
        arrayValueWriter.SetTimeSample(value=[valueType(20.), valueType(10.)],
                                       time=3.0)
        arrayValueWriter.SetTimeSample(value=[valueType(20.), valueType(10.)],
                                       time=4.0)
        arrayValueWriter.SetTimeSample(value=[valueType(30.), valueType(30.)],
                                       time=5.0)
        arrayValueWriter.SetTimeSample(value=[valueType(30.), 
                                       valueType(30. + (epsilon / 2))],
                                       time=6.0)
        arrayValueWriter.SetTimeSample(value=[valueType(30.), valueType(30.)],
                                       time=7.0)
        arrayValueWriter.SetTimeSample(value=[valueType(30. + (epsilon * 2)), 
                                       valueType(30.)],
                                       time=8.0)

        expectedTimeSamples = [1.0, 2.0, 4.0, 5.0, 7.0, 8.0] if \
                              defaultValue == valueType(10.0) else \
                              [0.0, 1.0, 2.0, 4.0, 5.0, 7.0, 8.0]
        self.assertEqual(attr.GetTimeSamples(), expectedTimeSamples)

    def test_Double(self):
        self.TestAttrOfType(Sdf.ValueTypeNames.Double, 
            Sdf.ValueTypeNames.DoubleArray, 0.0, 1e-12)

    def test_Float(self):
        # default value matches first time-sample.
        self.TestAttrOfType(Sdf.ValueTypeNames.Float, 
            Sdf.ValueTypeNames.FloatArray, 10.0, 1e-6)

    def test_Half(self):
        self.TestAttrOfType(Sdf.ValueTypeNames.Half, 
            Sdf.ValueTypeNames.HalfArray, 20.0, 1e-2)

    def test_Vectors(self):
        self.TestAttrOfType(Sdf.ValueTypeNames.Float2, 
            Sdf.ValueTypeNames.Float2Array, Gf.Vec2f(0.0), 1e-6)
        self.TestAttrOfType(Sdf.ValueTypeNames.Float3, 
            Sdf.ValueTypeNames.Float3Array, Gf.Vec3f(0.0), 1e-6)
        self.TestAttrOfType(Sdf.ValueTypeNames.Float4, 
            Sdf.ValueTypeNames.Float4Array, Gf.Vec4f(0.0), 1e-6)

        self.TestAttrOfType(Sdf.ValueTypeNames.Double2, 
            Sdf.ValueTypeNames.Double2Array, Gf.Vec2d(0.0), 1e-12)
        self.TestAttrOfType(Sdf.ValueTypeNames.Double3, 
            Sdf.ValueTypeNames.Double3Array, Gf.Vec3d(0.0), 1e-12)
        self.TestAttrOfType(Sdf.ValueTypeNames.Double4, 
            Sdf.ValueTypeNames.Double4Array, Gf.Vec4d(0.0), 1e-12)

        self.TestAttrOfType(Sdf.ValueTypeNames.Half2, 
            Sdf.ValueTypeNames.Half2Array, Gf.Vec2h(0.0), 1e-2)
        self.TestAttrOfType(Sdf.ValueTypeNames.Half3, 
            Sdf.ValueTypeNames.Half3Array, Gf.Vec3h(0.0), 1e-2)
        self.TestAttrOfType(Sdf.ValueTypeNames.Half4, 
            Sdf.ValueTypeNames.Half4Array, Gf.Vec4h(0.0), 1e-2)

    def test_Matrices(self):
        self.TestAttrOfType(Sdf.ValueTypeNames.Matrix2d, 
            Sdf.ValueTypeNames.Matrix2dArray, Gf.Matrix2d(0.0), 1e-12)
        self.TestAttrOfType(Sdf.ValueTypeNames.Matrix3d, 
            Sdf.ValueTypeNames.Matrix3dArray, Gf.Matrix3d(0.0), 1e-12)
        self.TestAttrOfType(Sdf.ValueTypeNames.Matrix4d, 
            Sdf.ValueTypeNames.Matrix4dArray, Gf.Matrix4d(0.0), 1e-12)

    def TestQuats(self, typeName, arrayTypeName, defaultValue, epsilon):
        s = Usd.Stage.CreateInMemory()
        scope = UsdGeom.Scope.Define(s, "/Scope")

        # Test scalar valued quat attribute.
        attr = scope.GetPrim().CreateAttribute("quat", typeName)
        self.assertFalse(attr.HasAuthoredValueOpinion())

        valueWriter = UsdUtils.SparseAttrValueWriter(attr, 
                defaultValue=defaultValue)
        self.assertTrue(attr.HasAuthoredValueOpinion())

        valueType = type(defaultValue)
        realType = type(defaultValue.GetReal())
        imaginaryType = type(defaultValue.GetImaginary())

        valueWriter.SetTimeSample(value=valueType(realType(10.), 
                                                  imaginaryType(10.0)), 
                                  time=1.0)
        valueWriter.SetTimeSample(value=valueType(realType(20.), 
                                                  imaginaryType(20.0)), 
                                  time=2.0)
        valueWriter.SetTimeSample(value=valueType(realType(20.), 
                                                  imaginaryType(20.0)), 
                                  time=3.0)
        valueWriter.SetTimeSample(value=valueType(realType(20.), 
                                                  imaginaryType(20.0)), 
                                  time=4.0)
        valueWriter.SetTimeSample(value=valueType(realType(30.), 
                                                  imaginaryType(30.0)), 
                                  time=5.0)
        valueWriter.SetTimeSample(value=valueType(realType(30. + epsilon / 2), 
                                    imaginaryType(30.0 + epsilon / 2)), 
                                  time=6.0)
        valueWriter.SetTimeSample(value=valueType(realType(30.), 
                                                  imaginaryType(30.0)), 
                                  time=7.0)
        valueWriter.SetTimeSample(value=valueType(realType(30. + epsilon * 2), 
                                    imaginaryType(30.0 + epsilon * 2)), 
                                  time=8.0)

        self.assertEqual(attr.GetTimeSamples(), 
                         [1.0, 2.0, 4.0, 5.0, 7.0, 8.0])


        # Test array valued quats.
        arrayAttr = scope.GetPrim().CreateAttribute("arrayQuat", arrayTypeName)
        self.assertFalse(arrayAttr.HasAuthoredValueOpinion())
        
        arrayValueWriter = UsdUtils.SparseAttrValueWriter(arrayAttr, 
                defaultValue=[defaultValue, defaultValue])
        self.assertTrue(arrayAttr.HasAuthoredValueOpinion())

        arrayValueWriter.SetTimeSample(
                value=[valueType(realType(10.), imaginaryType(10.0)), 
                       valueType(realType(10.), imaginaryType(10.0))], 
                time=1.0)
        arrayValueWriter.SetTimeSample(
                value=[valueType(realType(20.), imaginaryType(10.0)), 
                       valueType(realType(10.), imaginaryType(20.0))], 
                time=2.0)
        arrayValueWriter.SetTimeSample(
                value=[valueType(realType(20.), imaginaryType(10.0)), 
                       valueType(realType(10.), imaginaryType(20.0))], 
                time=3.0)
        arrayValueWriter.SetTimeSample(
                value=[valueType(realType(20.), imaginaryType(10.0)), 
                       valueType(realType(10.), imaginaryType(20.0))], 
                time=4.0)
        arrayValueWriter.SetTimeSample(
                value=[valueType(realType(30.), imaginaryType(30.0)), 
                       valueType(realType(30.), imaginaryType(30.0))], 
                time=5.0)
        arrayValueWriter.SetTimeSample(
                value=[valueType(realType(30. + epsilon/2), 
                                 imaginaryType(30.0 + epsilon/2)), 
                       valueType(realType(30.+ epsilon/2), 
                                 imaginaryType(30.0 + epsilon/2))], 
                time=6.0)
        arrayValueWriter.SetTimeSample(
                value=[valueType(realType(30.), imaginaryType(30.0)), 
                       valueType(realType(30.), imaginaryType(30.0))], 
                time=7.0)
        arrayValueWriter.SetTimeSample(
                value=[valueType(realType(30.), 
                                 imaginaryType(30.0 + epsilon*2)), 
                       valueType(realType(30.+ epsilon*2), 
                                 imaginaryType(30.0))], 
                time=8.0)

        expectedTimeSamples = [1.0, 2.0, 4.0, 5.0, 7.0, 8.0]
        self.assertEqual(attr.GetTimeSamples(), expectedTimeSamples)

    def test_testQuats(self):
        self.TestQuats(Sdf.ValueTypeNames.Quatd, 
                       Sdf.ValueTypeNames.QuatdArray, 
                       Gf.Quatd(0.0), 1e-12)
        self.TestQuats(Sdf.ValueTypeNames.Quatf, 
                       Sdf.ValueTypeNames.QuatfArray, 
                       Gf.Quatf(0.0), 1e-6)
        self.TestQuats(Sdf.ValueTypeNames.Quath, 
                       Sdf.ValueTypeNames.QuathArray, 
                       Gf.Quath(0.0), 1e-2)

    def test_MatchDefault(self):
        s = Usd.Stage.CreateInMemory()
        from pxr import UsdGeom
        sphere = UsdGeom.Sphere.Define(s, Sdf.Path("/Sphere"))
        radius = sphere.CreateRadiusAttr()
        attrValueWriter = UsdUtils.SparseAttrValueWriter(radius, 
            defaultValue=1.0)

        # Default value isn't authored into scene description since it matches
        # the fallback value of radius.
        self.assertFalse(radius.HasAuthoredValueOpinion())

        attrValueWriter.SetTimeSample(10.0, 1.0)
        attrValueWriter.SetTimeSample(10.0, 2.0)
        attrValueWriter.SetTimeSample(10.0, 3.0)
        attrValueWriter.SetTimeSample(20.0, 4.0)

        self.assertEqual(radius.GetTimeSamples(), [1.0, 3.0, 4.0])
        
    def test_SparseValueWriter(self):
        s = Usd.Stage.CreateInMemory()
        from pxr import UsdGeom
        cylinder = UsdGeom.Cylinder.Define(s, Sdf.Path("/Cylinder"))
        radius = cylinder.CreateRadiusAttr()
        height = cylinder.CreateHeightAttr()

        valueWriter = UsdUtils.SparseValueWriter()

        valueWriter.SetAttribute(radius, 2.0, Usd.TimeCode.Default())
        valueWriter.SetAttribute(height, 2.0, Usd.TimeCode.Default())

        # Default value isn't authored for the height attribute into scene
        # description since it matches the fallback value of height.
        self.assertFalse(height.HasAuthoredValueOpinion())

        # Default value is authored for the radius attribute into scene
        # description since it does not match the fallback value of radius, 
        # which is 1.0.
        self.assertTrue(radius.HasAuthoredValueOpinion())

        self.assertEqual(radius.Get(), 2.0)
        # Calling SetAttribute again with time-Default will update the default
        # value.
        valueWriter.SetAttribute(radius, 5.0, Usd.TimeCode.Default())
        self.assertEqual(radius.Get(), 5.0)

        valueWriter.SetAttribute(radius, 10.0, 1.0)
        valueWriter.SetAttribute(radius, 20.0, 2.0)
        valueWriter.SetAttribute(radius, 20.0, 3.0)
        valueWriter.SetAttribute(radius, 20.0, 4.0)

        valueWriter.SetAttribute(height, 2.0, 1.0)
        valueWriter.SetAttribute(height, 2.0, 2.0)
        valueWriter.SetAttribute(height, 3.0, 3.0)
        valueWriter.SetAttribute(height, 3.0, 4.0)

        self.assertEqual(radius.GetTimeSamples(), [1.0, 2.0])
        self.assertEqual(height.GetTimeSamples(), [2.0, 3.0])

    def test_ErrorCases(self):
        s = Usd.Stage.CreateInMemory()
        scope = UsdGeom.Scope.Define(s, "/Scope")

        attr1 = scope.GetPrim().CreateAttribute("attr1", 
                Sdf.ValueTypeNames.Float)
        self.assertFalse(attr1.HasAuthoredValueOpinion())

        attrValueWriter = UsdUtils.SparseAttrValueWriter(attr1)
        # No default value was specified. So no authored value yet.
        self.assertFalse(attr1.HasAuthoredValueOpinion())

        attrValueWriter.SetTimeSample(10.0, time=10.0)
        # Authoring an earlier time-sample than the previous one results in 
        # a coding error also.

        # Calling SetTimeSample with time=default raises a coding error.
        with self.assertRaises(Tf.ErrorException):
            attrValueWriter.SetTimeSample(10.0, Usd.TimeCode.Default())

        with self.assertRaises(Tf.ErrorException):
            attrValueWriter.SetTimeSample(20.0, time=5.0)

if __name__=="__main__":
    unittest.main()
