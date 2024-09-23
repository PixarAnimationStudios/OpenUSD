#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd, Sdf, Ts, Tf
import unittest, shutil


class TestUsdSplines(unittest.TestCase):

    def _GetTestSpline(self, attrType = Sdf.ValueTypeNames.Double):
        """
        Return a spline for cases where we don't particularly care about the
        contents.  This one exercises both standard and Maya tangent forms.
        """
        typeName = str(attrType)
        spline = Ts.Spline(typeName)
        spline.SetKnot(Ts.Knot(
            typeName = typeName,
            time = 1,
            value = 8,
            nextInterp = Ts.InterpCurve,
            postTanMayaWidth = 4,
            postTanMayaHeight = 0.5))
        spline.SetKnot(Ts.Knot(
            typeName = typeName,
            time = 6,
            value = 20,
            nextInterp = Ts.InterpCurve,
            preTanMayaWidth = 2,
            preTanMayaHeight = 0.4,
            postTanWidth = 2,
            postTanSlope = 0.3))
        return spline

    def _DoSerializationTest(
            self, case, spline,
            attrType = Sdf.ValueTypeNames.Double,
            isEmpty = False):
        """
        Write a spline to a file, copy the file, read the copy, and verify the
        original and round-tripped spline are identical.
        """
        for format in ["usda", "usdc"]:

            filename1 = f"test_Serialization_{case}.{format}"
            filename2 = f"test_Serialization_{case}.copy.{format}"

            stage = Usd.Stage.CreateNew(filename1)
            prim = stage.DefinePrim(Sdf.Path("/MyPrim"))
            attr = prim.CreateAttribute("myAttr", attrType)
            self.assertFalse(attr.HasSpline())
            attr.SetSpline(spline)
            self.assertTrue(attr.HasSpline())
            print(f"Original spline, {case}, {format}:")
            print(spline)

            stage.Save()
            shutil.copyfile(filename1, filename2)
            stage2 = Usd.Stage.Open(filename2)

            attr2 = stage2.GetAttributeAtPath("/MyPrim.myAttr")

            if isEmpty:
                self.assertFalse(attr2.HasSpline())
            else:
                self.assertTrue(attr2.HasSpline())
                spline2 = attr2.GetSpline()
                print(f"Round-tripped spline, {case}, {format}:")
                print(spline2)

                self.assertEqual(spline, spline2)

    def test_Serialization_Empty(self):
        """
        Test serialization of empty splines.
        """
        self._DoSerializationTest("Empty", Ts.Spline(), isEmpty = True)

    def test_Serialization_Museum(self):
        """
        Test serialization with some Museum cases.
        """
        for exhibit in ["TwoKnotBezier", "ComplexParams"]:

            splineData = Ts.TsTest_Museum.GetDataByName(exhibit)
            spline = Ts.TsTest_TsEvaluator().SplineDataToSpline(splineData)
            self._DoSerializationTest(f"Museum.{exhibit}", spline)

    def test_Serialization_Complex(self):
        """
        Test serialization with some programmatically built splines.  Exercise
        features not available via Museum cases.
        """
        # Bezier, Maya tangent form.
        # Custom data dictionary.
        spline = Ts.Spline()
        spline.SetKnot(Ts.Knot(
            time = -14.7,
            value = 11.30752,
            nextInterp = Ts.InterpCurve,
            postTanMayaWidth = 5.4,
            postTanMayaHeight = 7.7,
            customData = { "a": "yes", "b": 4, "c": { "d": "ugh" } } ))
        spline.SetKnot(Ts.Knot(
            time = -1.2,
            value = 22.994037,
            nextInterp = Ts.InterpCurve,
            preTanMayaWidth = 4.7,
            preTanMayaHeight = -3.3,
            postTanMayaWidth = 7,
            postTanMayaHeight = 40))
        self._DoSerializationTest("Complex.1", spline)

        # Bezier, mixed standard and Maya tangent forms.
        spline = Ts.Spline()
        spline.SetKnot(Ts.Knot(
            time = 1,
            value = 8,
            nextInterp = Ts.InterpCurve,
            postTanMayaWidth = 4,
            postTanMayaHeight = 0.5))
        spline.SetKnot(Ts.Knot(
            time = 6,
            value = 20,
            nextInterp = Ts.InterpCurve,
            preTanMayaWidth = 2,
            preTanMayaHeight = 0.4,
            postTanWidth = 2,
            postTanSlope = 0.3))
        spline.SetKnot(Ts.Knot(
            time = 40,
            value = -44,
            nextInterp = Ts.InterpCurve,
            preTanWidth = 1,
            preTanSlope = 0,
            postTanMayaWidth = 22,
            postTanMayaHeight = 0))
        self._DoSerializationTest("Complex.2", spline)

        # Hermite, mixed standard and Maya tangent forms.
        spline = Ts.Spline()
        spline.SetCurveType(Ts.CurveTypeHermite)
        spline.SetKnot(Ts.Knot(
            curveType = Ts.CurveTypeHermite,
            time = 1,
            value = 8,
            nextInterp = Ts.InterpCurve,
            postTanSlope = 4))
        spline.SetKnot(Ts.Knot(
            curveType = Ts.CurveTypeHermite,
            time = 6,
            value = 20,
            nextInterp = Ts.InterpCurve,
            preTanMayaHeight = 2,
            postTanSlope = 1))
        self._DoSerializationTest("Complex.3", spline)

    def test_Serialization_ValueTypes(self):
        """
        Test serialization of splines with different value types.
        """
        # Double is exercised by all the other tests.

        # Float.
        attrType = Sdf.ValueTypeNames.Float
        spline = self._GetTestSpline(attrType)
        self._DoSerializationTest(
            "ValueTypes.Float", spline, attrType)

        # Half.
        attrType = Sdf.ValueTypeNames.Half
        spline = self._GetTestSpline(attrType)
        self._DoSerializationTest(
            "ValueTypes.Half", spline, attrType)

        # Timecode.
        # Double-typed, and stamped as time-valued.
        spline = self._GetTestSpline(Sdf.ValueTypeNames.Double)
        spline.SetTimeValued(True)
        self._DoSerializationTest(
            "ValueTypes.TimeCode", spline, Sdf.ValueTypeNames.TimeCode)

    def test_Serialization_Loops(self):
        """
        Test serialization of inner loop params.
        """
        spline = Ts.Spline()
        spline.SetKnot(Ts.Knot(
            time = 1,
            value = 5,
            nextInterp = Ts.InterpCurve,
            preTanWidth = 1,
            preTanSlope = 1,
            postTanWidth = 1,
            postTanSlope = 1))
        lp = Ts.LoopParams()
        lp.protoStart = 1
        lp.protoEnd = 10
        lp.numPostLoops = 1
        spline.SetInnerLoopParams(lp)
        self._DoSerializationTest("Loops.Valid", spline)

        # In this version, there is no knot at the prototype start time, so the
        # loop params are invalid.  They should be serialized and read back
        # anyway.
        lp.protoStart = 2
        spline.SetInnerLoopParams(lp)
        self._DoSerializationTest("Loops.Invalid", spline)

    def _DoLayerOffsetTest(self, case, attrType, timeValued, scale):
        """
        Test writing and reading splines across layer offsets.
        """
        stage = Usd.Stage.CreateInMemory()
        rootLayer = stage.GetRootLayer()
        deepLayer = Sdf.Layer.CreateAnonymous()

        rootLayer.subLayerPaths = [deepLayer.identifier]
        rootLayer.subLayerOffsets[0] = \
            Sdf.LayerOffset(offset = 5.0, scale = scale)

        stage.SetEditTarget(stage.GetEditTargetForLocalLayer(deepLayer))

        spline = self._GetTestSpline(Sdf.ValueTypeNames.Double)
        spline.SetTimeValued(timeValued)
        print(f"Original spline, {case}:")
        print(spline)

        prim = stage.DefinePrim(Sdf.Path("/MyPrim"))
        attr = prim.CreateAttribute("myAttr", attrType)
        attr.SetSpline(spline)

        sdfAttr = deepLayer.GetAttributeAtPath("/MyPrim.myAttr")
        sdfSpline = sdfAttr.GetInfo("spline")
        print(f"Deep spline, {case}:")
        print(sdfSpline)

        attr2 = stage.GetAttributeAtPath("/MyPrim.myAttr")
        spline2 = attr2.GetSpline()
        print(f"Retrieved spline, {case}:")
        print(spline2)

        spline3 = attr2.GetMetadata("spline")
        print("Retrieved spline, generic, {case}:")
        print(spline3)

        self.assertEqual(spline2, spline)
        self.assertEqual(spline3, spline)
        self.assertNotEqual(sdfSpline, spline)

    def test_LayerOffsets(self):
        """
        Test writing and reading splines across layer offsets.
        """
        self._DoLayerOffsetTest(
            "test_LayerOffsets",
            attrType = Sdf.ValueTypeNames.Double, timeValued = False,
            scale = 2.0)

    def test_LayerOffsets_TimeCode(self):
        """
        Test writing and reading time-valued splines across layer offsets.
        """
        self._DoLayerOffsetTest(
            "test_LayerOffsets_TimeCode",
            attrType = Sdf.ValueTypeNames.TimeCode, timeValued = True,
            scale = 2.0)

    def test_LayerOffsets_Reversed(self):
        """
        Test writing and reading splines across time-reversing layer offsets.
        """
        self._DoLayerOffsetTest(
            "test_LayerOffsets_Reversed",
            attrType = Sdf.ValueTypeNames.Double, timeValued = False,
            scale = -2.0)

    def test_InvalidType(self):
        """
        Verify that a spline cannot be assigned to an attribute of an
        unsupported value type.
        """
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim(Sdf.Path("/MyPrim"))
        attr = prim.CreateAttribute("myAttr", Sdf.ValueTypeNames.String)
        spline = self._GetTestSpline()

        gotException = False
        try:
            attr.SetSpline(spline)
        except Tf.ErrorException as e:
            gotException = True
            print("got exception:")
            print(e)
        except:
            pass

        self.assertTrue(gotException)


if __name__ == "__main__":

    # 'buffer' means that all stdout will be captured and swallowed, unless
    # there is an error, in which case the stdout of the erroring case will be
    # printed on stderr along with the test results.  Suppressing the output of
    # passing cases makes it easier to find the output of failing ones.
    unittest.main(testRunner = unittest.TextTestRunner(buffer = True))
