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
import unittest

from pxr import Gf
from pxr import UsdGeom
from maya import cmds
from maya import standalone
from maya.api import OpenMaya


class testUsdImportColorSets(unittest.TestCase):

    def _AssertColorSet(self, mesh, colorSetName, expectedColorRepresentation,
            expectedValues, expectedNumValues=None, expectedIsClamped=False):
        """
        Verifies that the color values for the color set named colorSetName on
        the MFnMesh mesh match the values in expectedValues. expectedValues
        should be a dictionary mapping mesh-level face vertex indices to color
        values. A face vertex index missing from the dictionary means that that
        face vertex should NOT have an assigned color value on the Maya mesh.
        If multiple face vertices map to the same color value and the number of
        color values in the color set is smaller than the number of assignments,
        pass in the expected number of color values in the color set as
        expectedNumValues.
        """
        if expectedNumValues is None:
            expectedNumValues = len(expectedValues)
        actualNumValues = mesh.numColors(colorSetName)
        self.assertEqual(expectedNumValues, actualNumValues)

        self.assertEqual(mesh.getColorRepresentation(colorSetName),
            expectedColorRepresentation)

        self.assertEqual(mesh.isColorClamped(colorSetName), expectedIsClamped)

        mesh.setCurrentColorSetName(colorSetName)

        itMeshFV = OpenMaya.MItMeshFaceVertex(mesh.object())
        itMeshFV.reset()
        fvi = 0
        while not itMeshFV.isDone():
            expectedValue = expectedValues.get(fvi)
            if expectedValue is None:
                self.assertFalse(itMeshFV.hasColor())
            else:
                self.assertTrue(itMeshFV.hasColor())

                actualValue = itMeshFV.getColor(colorSetName)

                if expectedColorRepresentation == OpenMaya.MFnMesh.kAlpha:
                    actualValue = actualValue[3]
                elif expectedColorRepresentation == OpenMaya.MFnMesh.kRGB:
                    actualValue = Gf.Vec3f(actualValue[0], actualValue[1],
                        actualValue[2])
                elif expectedColorRepresentation == OpenMaya.MFnMesh.kRGBA:
                    actualValue = Gf.Vec4f(actualValue[0], actualValue[1],
                        actualValue[2], actualValue[3])

                self.assertAlmostEqual(expectedValue, actualValue)

            itMeshFV.next()
            fvi += 1

    def _MakeExpectedValuesSparse(self, expectedValues, interpolation):
        """
        Given an expectedValues dictionary containing a complete set of values
        for all 24 face vertices on a test cube, return a copy of the dictionary
        that has odd-numbered components removed. The resulting dictionary
        should be correct for the sparse data sets in the test scene.
        """
        self.assertEqual(len(expectedValues), 24)

        sparseExpectedValues = dict(expectedValues)

        if interpolation == UsdGeom.Tokens.uniform:
            # Remove the face vertices that are part of odd-numbered faces.
            for i in xrange(4, 8):
                sparseExpectedValues.pop(i)
            for i in xrange(12, 16):
                sparseExpectedValues.pop(i)
            for i in xrange(20, 24):
                sparseExpectedValues.pop(i)
        elif interpolation == UsdGeom.Tokens.vertex:
            # Remove the face vertices that are on odd-numbered mesh-level
            # vertices.
            sparseExpectedValues.pop(1)
            sparseExpectedValues.pop(2)
            sparseExpectedValues.pop(5)
            sparseExpectedValues.pop(6)
            sparseExpectedValues.pop(9)
            sparseExpectedValues.pop(10)
            sparseExpectedValues.pop(13)
            sparseExpectedValues.pop(14)
            sparseExpectedValues.pop(16)
            sparseExpectedValues.pop(17)
            sparseExpectedValues.pop(18)
            sparseExpectedValues.pop(19)
        elif interpolation == UsdGeom.Tokens.faceVarying:
            # Remove odd-numbered face vertices.
            for i in xrange(1, 24, 2):
                sparseExpectedValues.pop(i)

        return sparseExpectedValues


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

        usdFile = os.path.abspath('UsdImportColorSetsTest.usda')
        cmds.usdImport(file=usdFile, shadingMode='none')

    def _GetMayaMesh(self, meshName):
        selectionList = OpenMaya.MSelectionList()
        selectionList.add(meshName)
        mObj = selectionList.getDependNode(0)

        return OpenMaya.MFnMesh(mObj)

    def testImportNoColorSets(self):
        """
        Tests that importing a USD cube mesh with no color set primvars results
        in no color sets on the Maya mesh.
        """
        mayaCubeMesh = self._GetMayaMesh('NoColorSetsCubeShape')
        self.assertEqual(mayaCubeMesh.numColorSets, 0)

    def testImportConstantInterpolationColorSets(self):
        """
        Tests that color set primvars on a USD cube mesh that have constant
        interpolation are imported correctly.
        """
        mayaCubeMesh = self._GetMayaMesh('ColorSetsCubeShape')

        colorSetName = 'ConstantColor_kAlpha'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = 0.5
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kAlpha,
            expectedValues, expectedNumValues=1)

        colorSetName = 'ConstantColor_kRGB'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = Gf.Vec3f(1.0, 0.0, 1.0)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=1)

        colorSetName = 'ConstantColor_kRGBA'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = Gf.Vec4f(0.0, 1.0, 0.0, 0.5)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGBA,
            expectedValues, expectedNumValues=1)

    def testImportUniformInterpolationColorSets(self):
        """
        Tests that color set primvars on a USD cube mesh that have uniform
        interpolation are imported correctly.
        """
        mayaCubeMesh = self._GetMayaMesh('ColorSetsCubeShape')

        interpolation  = UsdGeom.Tokens.uniform

        colorSetName = 'FaceColor_Full_kAlpha'
        expectedValues = {}
        for i in xrange(0, 4):
            expectedValues[i] = 1.0
        for i in xrange(4, 8):
            expectedValues[i] = 0.9
        for i in xrange(8, 12):
            expectedValues[i] = 0.8
        for i in xrange(12, 16):
            expectedValues[i] = 0.7
        for i in xrange(16, 20):
            expectedValues[i] = 0.6
        for i in xrange(20, 24):
            expectedValues[i] = 0.5
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kAlpha,
            expectedValues, expectedNumValues=6)

        colorSetName = 'FaceColor_Sparse_kAlpha'
        expectedValues = self._MakeExpectedValuesSparse(expectedValues,
            interpolation)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kAlpha,
            expectedValues, expectedNumValues=3)

        colorSetName = 'FaceColor_Full_kRGB'
        expectedValues = {}
        for i in xrange(0, 4):
            expectedValues[i] = Gf.Vec3f(1.0, 0.0, 0.0)
        for i in xrange(4, 8):
            expectedValues[i] = Gf.Vec3f(0.0, 1.0, 0.0)
        for i in xrange(8, 12):
            expectedValues[i] = Gf.Vec3f(0.0, 0.0, 1.0)
        for i in xrange(12, 16):
            expectedValues[i] = Gf.Vec3f(1.0, 1.0, 0.0)
        for i in xrange(16, 20):
            expectedValues[i] = Gf.Vec3f(1.0, 0.0, 1.0)
        for i in xrange(20, 24):
            expectedValues[i] = Gf.Vec3f(0.0, 1.0, 1.0)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=6)

        colorSetName = 'FaceColor_Sparse_kRGB'
        expectedValues = self._MakeExpectedValuesSparse(expectedValues,
            interpolation)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=3)

        colorSetName = 'FaceColor_Full_kRGBA'
        expectedValues = {}
        for i in xrange(0, 4):
            expectedValues[i] = Gf.Vec4f(1.0, 0.0, 0.0, 1.0)
        for i in xrange(4, 8):
            expectedValues[i] = Gf.Vec4f(0.0, 1.0, 0.0, 0.9)
        for i in xrange(8, 12):
            expectedValues[i] = Gf.Vec4f(0.0, 0.0, 1.0, 0.8)
        for i in xrange(12, 16):
            expectedValues[i] = Gf.Vec4f(1.0, 1.0, 0.0, 0.7)
        for i in xrange(16, 20):
            expectedValues[i] = Gf.Vec4f(1.0, 0.0, 1.0, 0.6)
        for i in xrange(20, 24):
            expectedValues[i] = Gf.Vec4f(0.0, 1.0, 1.0, 0.5)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGBA,
            expectedValues, expectedNumValues=6)

        colorSetName = 'FaceColor_Sparse_kRGBA'
        expectedValues = self._MakeExpectedValuesSparse(expectedValues,
            interpolation)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGBA,
            expectedValues, expectedNumValues=3)

    def testImportVertexInterpolationColorSets(self):
        """
        Tests that color set primvars on a USD cube mesh that have vertex
        interpolation are imported correctly.
        """
        mayaCubeMesh = self._GetMayaMesh('ColorSetsCubeShape')

        interpolation  = UsdGeom.Tokens.vertex

        colorSetName = 'VertexColor_Full_kAlpha'
        expectedValues = {
            0: 1.0,
            1: 0.9,
            2: 0.7,
            3: 0.8,
            4: 0.8,
            5: 0.7,
            6: 0.5,
            7: 0.6,
            8: 0.6,
            9: 0.5,
            10: 0.3,
            11: 0.4,
            12: 0.4,
            13: 0.3,
            14: 0.9,
            15: 1.0,
            16: 0.9,
            17: 0.3,
            18: 0.5,
            19: 0.7,
            20: 0.4,
            21: 1.0,
            22: 0.8,
            23: 0.6,
        }
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kAlpha,
            expectedValues, expectedNumValues=8)

        colorSetName = 'VertexColor_Sparse_kAlpha'
        expectedValues = self._MakeExpectedValuesSparse(expectedValues,
            interpolation)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kAlpha,
            expectedValues, expectedNumValues=4)

        colorSetName = 'VertexColor_Full_kRGB'
        expectedValues = {
            0: Gf.Vec3f(1.0, 0.0, 0.0),
            1: Gf.Vec3f(0.0, 1.0, 0.0),
            2: Gf.Vec3f(1.0, 1.0, 0.0),
            3: Gf.Vec3f(0.0, 0.0, 1.0),
            4: Gf.Vec3f(0.0, 0.0, 1.0),
            5: Gf.Vec3f(1.0, 1.0, 0.0),
            6: Gf.Vec3f(0.0, 1.0, 1.0),
            7: Gf.Vec3f(1.0, 0.0, 1.0),
            8: Gf.Vec3f(1.0, 0.0, 1.0),
            9: Gf.Vec3f(0.0, 1.0, 1.0),
            10: Gf.Vec3f(1.0, 1.0, 1.0),
            11: Gf.Vec3f(0.0, 0.0, 0.0),
            12: Gf.Vec3f(0.0, 0.0, 0.0),
            13: Gf.Vec3f(1.0, 1.0, 1.0),
            14: Gf.Vec3f(0.0, 1.0, 0.0),
            15: Gf.Vec3f(1.0, 0.0, 0.0),
            16: Gf.Vec3f(0.0, 1.0, 0.0),
            17: Gf.Vec3f(1.0, 1.0, 1.0),
            18: Gf.Vec3f(0.0, 1.0, 1.0),
            19: Gf.Vec3f(1.0, 1.0, 0.0),
            20: Gf.Vec3f(0.0, 0.0, 0.0),
            21: Gf.Vec3f(1.0, 0.0, 0.0),
            22: Gf.Vec3f(0.0, 0.0, 1.0),
            23: Gf.Vec3f(1.0, 0.0, 1.0)
        }
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=8)

        colorSetName = 'VertexColor_Sparse_kRGB'
        expectedValues = self._MakeExpectedValuesSparse(expectedValues,
            interpolation)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=4)

        colorSetName = 'VertexColor_Full_kRGBA'
        expectedValues = {
            0: Gf.Vec4f(1.0, 0.0, 0.0, 1.0),
            1: Gf.Vec4f(0.0, 1.0, 0.0, 0.9),
            2: Gf.Vec4f(1.0, 1.0, 0.0, 0.7),
            3: Gf.Vec4f(0.0, 0.0, 1.0, 0.8),
            4: Gf.Vec4f(0.0, 0.0, 1.0, 0.8),
            5: Gf.Vec4f(1.0, 1.0, 0.0, 0.7),
            6: Gf.Vec4f(0.0, 1.0, 1.0, 0.5),
            7: Gf.Vec4f(1.0, 0.0, 1.0, 0.6),
            8: Gf.Vec4f(1.0, 0.0, 1.0, 0.6),
            9: Gf.Vec4f(0.0, 1.0, 1.0, 0.5),
            10: Gf.Vec4f(1.0, 1.0, 1.0, 0.3),
            11: Gf.Vec4f(0.0, 0.0, 0.0, 0.4),
            12: Gf.Vec4f(0.0, 0.0, 0.0, 0.4),
            13: Gf.Vec4f(1.0, 1.0, 1.0, 0.3),
            14: Gf.Vec4f(0.0, 1.0, 0.0, 0.9),
            15: Gf.Vec4f(1.0, 0.0, 0.0, 1.0),
            16: Gf.Vec4f(0.0, 1.0, 0.0, 0.9),
            17: Gf.Vec4f(1.0, 1.0, 1.0, 0.3),
            18: Gf.Vec4f(0.0, 1.0, 1.0, 0.5),
            19: Gf.Vec4f(1.0, 1.0, 0.0, 0.7),
            20: Gf.Vec4f(0.0, 0.0, 0.0, 0.4),
            21: Gf.Vec4f(1.0, 0.0, 0.0, 1.0),
            22: Gf.Vec4f(0.0, 0.0, 1.0, 0.8),
            23: Gf.Vec4f(1.0, 0.0, 1.0, 0.6)
        }
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGBA,
            expectedValues, expectedNumValues=8)

        colorSetName = 'VertexColor_Sparse_kRGBA'
        expectedValues = self._MakeExpectedValuesSparse(expectedValues,
            interpolation)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGBA,
            expectedValues, expectedNumValues=4)

    def testImportFaceVaryingInterpolationColorSets(self):
        """
        Tests that color set primvars on a USD cube mesh that have faceVarying
        interpolation are imported correctly.
        """
        mayaCubeMesh = self._GetMayaMesh('ColorSetsCubeShape')

        interpolation  = UsdGeom.Tokens.faceVarying

        colorSetName = 'FaceVertexColor_Full_kAlpha'
        expectedValues = {
            0: 1.00,
            1: 0.99,
            2: 0.98,
            3: 0.97,
            4: 0.90,
            5: 0.89,
            6: 0.88,
            7: 0.87,
            8: 0.80,
            9: 0.79,
            10: 0.78,
            11: 0.77,
            12: 0.70,
            13: 0.69,
            14: 0.68,
            15: 0.67,
            16: 0.60,
            17: 0.59,
            18: 0.58,
            19: 0.57,
            20: 0.50,
            21: 0.49,
            22: 0.48,
            23: 0.47
        }
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kAlpha,
            expectedValues, expectedNumValues=24)

        colorSetName = 'FaceVertexColor_Sparse_kAlpha'
        expectedValues = self._MakeExpectedValuesSparse(expectedValues,
            interpolation)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kAlpha,
            expectedValues, expectedNumValues=12)

        colorSetName = 'FaceVertexColor_Full_kRGB'
        expectedValues = {
            0: Gf.Vec3f(1.00, 1.00, 0.00),
            1: Gf.Vec3f(1.00, 0.90, 0.00),
            2: Gf.Vec3f(1.00, 0.80, 0.00),
            3: Gf.Vec3f(1.00, 0.70, 0.00),
            4: Gf.Vec3f(0.90, 1.00, 0.00),
            5: Gf.Vec3f(0.90, 0.90, 0.00),
            6: Gf.Vec3f(0.90, 0.80, 0.00),
            7: Gf.Vec3f(0.90, 0.70, 0.00),
            8: Gf.Vec3f(0.80, 1.00, 0.00),
            9: Gf.Vec3f(0.80, 0.90, 0.00),
            10: Gf.Vec3f(0.80, 0.80, 0.00),
            11: Gf.Vec3f(0.80, 0.70, 0.00),
            12: Gf.Vec3f(0.70, 1.00, 0.00),
            13: Gf.Vec3f(0.70, 0.90, 0.00),
            14: Gf.Vec3f(0.70, 0.80, 0.00),
            15: Gf.Vec3f(0.70, 0.70, 0.00),
            16: Gf.Vec3f(0.60, 1.00, 0.00),
            17: Gf.Vec3f(0.60, 0.90, 0.00),
            18: Gf.Vec3f(0.60, 0.80, 0.00),
            19: Gf.Vec3f(0.60, 0.70, 0.00),
            20: Gf.Vec3f(0.50, 1.00, 0.00),
            21: Gf.Vec3f(0.50, 0.90, 0.00),
            22: Gf.Vec3f(0.50, 0.80, 0.00),
            23: Gf.Vec3f(0.50, 0.70, 0.00)
        }
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=24)

        colorSetName = 'FaceVertexColor_Sparse_kRGB'
        expectedValues = self._MakeExpectedValuesSparse(expectedValues,
            interpolation)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=12)

        colorSetName = 'FaceVertexColor_Full_kRGBA'
        expectedValues = {
            0: Gf.Vec4f(1.00, 1.00, 0.00, 1.00),
            1: Gf.Vec4f(1.00, 0.90, 0.00, 0.99),
            2: Gf.Vec4f(1.00, 0.80, 0.00, 0.98),
            3: Gf.Vec4f(1.00, 0.70, 0.00, 0.97),
            4: Gf.Vec4f(0.90, 1.00, 0.00, 0.90),
            5: Gf.Vec4f(0.90, 0.90, 0.00, 0.89),
            6: Gf.Vec4f(0.90, 0.80, 0.00, 0.88),
            7: Gf.Vec4f(0.90, 0.70, 0.00, 0.87),
            8: Gf.Vec4f(0.80, 1.00, 0.00, 0.80),
            9: Gf.Vec4f(0.80, 0.90, 0.00, 0.79),
            10: Gf.Vec4f(0.80, 0.80, 0.00, 0.78),
            11: Gf.Vec4f(0.80, 0.70, 0.00, 0.77),
            12: Gf.Vec4f(0.70, 1.00, 0.00, 0.70),
            13: Gf.Vec4f(0.70, 0.90, 0.00, 0.69),
            14: Gf.Vec4f(0.70, 0.80, 0.00, 0.68),
            15: Gf.Vec4f(0.70, 0.70, 0.00, 0.67),
            16: Gf.Vec4f(0.60, 1.00, 0.00, 0.60),
            17: Gf.Vec4f(0.60, 0.90, 0.00, 0.59),
            18: Gf.Vec4f(0.60, 0.80, 0.00, 0.58),
            19: Gf.Vec4f(0.60, 0.70, 0.00, 0.57),
            20: Gf.Vec4f(0.50, 1.00, 0.00, 0.50),
            21: Gf.Vec4f(0.50, 0.90, 0.00, 0.49),
            22: Gf.Vec4f(0.50, 0.80, 0.00, 0.48),
            23: Gf.Vec4f(0.50, 0.70, 0.00, 0.47)
        }
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGBA,
            expectedValues, expectedNumValues=24)

        colorSetName = 'FaceVertexColor_Sparse_kRGBA'
        expectedValues = self._MakeExpectedValuesSparse(expectedValues,
            interpolation)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGBA,
            expectedValues, expectedNumValues=12)

    def testImportClampedColorSet(self):
        """
        Tests that a color set primvar on a USD cube mesh tagged as 'Maya[clamped]'
        is imported correctly.
        """
        mayaCubeMesh = self._GetMayaMesh('ColorSetsCubeShape')

        colorSetName = 'ConstantColorClamped_kRGBA'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = Gf.Vec4f(1.0, 1.0, 0.0, 0.5)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGBA,
            expectedValues, expectedNumValues=1, expectedIsClamped=True)

    def testImportNonAuthoredDisplayColor(self):
        """
        Tests that a color set primvar on a USD cube mesh named 'displayColor'
        is NOT imported if it is tagged as 'Maya[generated] = 1'.
        """
        mayaCubeMesh = self._GetMayaMesh('DisplayColorNotAuthoredCubeShape')
        self.assertEqual(mayaCubeMesh.numColorSets, 0)

    def testImportAuthoredDisplayColor(self):
        """
        Tests that a color set primvar on a USD cube mesh named 'displayColor'
        IS imported when it does not have an opinion for 'Maya[generated]'
        """
        mayaCubeMesh = self._GetMayaMesh('DisplayColorAuthoredCubeShape')

        colorSetName = 'displayColor'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = Gf.Vec3f(1.0, 0.0, 1.0)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=1)

    def testImportNonAuthoredDisplayOpacityColor(self):
        """
        Tests that a color set primvar on a USD cube mesh named 'displayOpacity'
        is NOT imported if it is tagged as 'Maya[generated] = 1'.
        """
        mayaCubeMesh = self._GetMayaMesh('DisplayOpacityNotAuthoredCubeShape')
        self.assertEqual(mayaCubeMesh.numColorSets, 0)

    def testImportAuthoredDisplayOpacity(self):
        """
        Tests that a color set primvar on a USD cube mesh named 'displayOpacity'
        S imported and called 'displayColor' when it does not have an opinion
        for 'Maya[generated]' and there is no authored displayColor primvar.
        """
        mayaCubeMesh = self._GetMayaMesh('DisplayOpacityAuthoredCubeShape')

        # Note that the displayOpacity primvar should have been imported as a
        # displayColor color set since the mesh has no displayColor primvar.
        # It should still be an alpha-only color set.
        colorSetName = 'displayColor'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = 0.5
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kAlpha,
            expectedValues, expectedNumValues=1)

    def testImportAuthoredDisplayColorAndDisplayOpacity(self):
        """
        Tests that a USD cube mesh that has both 'displayColor' and
        'displayOpacity' primvars authored is imported correctly.
        """
        mayaCubeMesh = self._GetMayaMesh('DisplayColorAndDisplayOpacityAuthoredCubeShape')

        colorSetName = 'displayColor'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = Gf.Vec3f(1.0, 0.0, 1.0)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=1)

        colorSetName = 'displayOpacity'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = 0.5
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kAlpha,
            expectedValues, expectedNumValues=1)

    def testImportAuthoredDisplayColorConvertToDisplay(self):
        """
        Tests that a color set primvar on a USD cube mesh named 'displayColor'
        that is tagged as 'Authored' is imported correctly and has its values
        converted from linear space to display space.
        """
        mayaCubeMesh = self._GetMayaMesh(
            'DisplayColorAuthoredConvertToDisplayCubeShape')

        # This mesh has a display color authored in linear space such that it
        # becomes Gf.Vec3f(0.5) when converted to display space.
        colorSetName = 'displayColor'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = Gf.Vec3f(0.5, 0.5, 0.5)
        self._AssertColorSet(mayaCubeMesh, colorSetName, OpenMaya.MFnMesh.kRGB,
            expectedValues, expectedNumValues=1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
