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
from pxr import Sdf
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt

from maya import OpenMaya
from maya import cmds
from maya import standalone


class testUsdExportColorSets(unittest.TestCase):

    COLOR_BLACK = Gf.Vec4f(0.0, 0.0, 0.0, 1.0)
    COLOR_RED = Gf.Vec4f(1.0, 0.0, 0.0, 1.0)
    COLOR_GREEN = Gf.Vec4f(0.0, 1.0, 0.0, 1.0)
    COLOR_BLUE = Gf.Vec4f(0.0, 0.0, 1.0, 1.0)
    COLOR_YELLOW = Gf.Vec4f(1.0, 1.0, 0.0, 1.0)
    COLOR_MAGENTA = Gf.Vec4f(1.0, 0.0, 1.0, 1.0)
    COLOR_CYAN = Gf.Vec4f(0.0, 1.0, 1.0, 1.0)
    COLOR_WHITE = Gf.Vec4f(1.0, 1.0, 1.0, 1.0)

    FACE_COLOR_MAP = {
        0: COLOR_RED,
        1: COLOR_GREEN,
        2: COLOR_BLUE,
        3: COLOR_YELLOW,
        4: COLOR_MAGENTA,
        5: COLOR_CYAN}

    VERTEX_COLOR_MAP = {
        0: COLOR_RED,
        1: COLOR_GREEN,
        2: COLOR_BLUE,
        3: COLOR_YELLOW,
        4: COLOR_MAGENTA,
        5: COLOR_CYAN,
        6: COLOR_BLACK,
        7: COLOR_WHITE}

    COLOR_COLOR_SET_DEFAULT = Gf.Vec4f(1.0, 1.0, 1.0, 1.0)
    COLOR_SHADER_DEFAULT = Gf.Vec4f(0.5, 0.5, 0.5, 0.0)

    # This is the list of color sets that should be on the ColorSetSourceCube.
    # It should include all combinations of face/vertex/faceVertex, sparse or
    # not (Full), and color representation (alpha, RGB, or RGBA).
    SOURCE_COLOR_SET_NAMES = [
        'FaceColor_Full_kAlpha',
        'FaceColor_Full_kRGB',
        'FaceColor_Full_kRGBA',
        'FaceColor_Sparse_kAlpha',
        'FaceColor_Sparse_kRGB',
        'FaceColor_Sparse_kRGBA',
        'FaceVertexColor_Full_kAlpha',
        'FaceVertexColor_Full_kRGB',
        'FaceVertexColor_Full_kRGBA',
        'FaceVertexColor_Sparse_kAlpha',
        'FaceVertexColor_Sparse_kRGB',
        'FaceVertexColor_Sparse_kRGBA',
        'VertexColor_Full_kAlpha',
        'VertexColor_Full_kRGB',
        'VertexColor_Full_kRGBA',
        'VertexColor_Sparse_kAlpha',
        'VertexColor_Sparse_kRGB',
        'VertexColor_Sparse_kRGBA']

    @staticmethod
    def _IsColorSetSparse(colorSetName):
        return '_Sparse_' in colorSetName

    @staticmethod
    def _IsColorSetAlpha(colorSetName):
        return colorSetName.endswith('kAlpha')

    @staticmethod
    def _IsColorSetRGB(colorSetName):
        return colorSetName.endswith('kRGB')

    @staticmethod
    def _IsColorSetRGBA(colorSetName):
        return colorSetName.endswith('kRGBA')

    @staticmethod
    def _ColorFromVec4f(value):
        return Gf.Vec3f(value[0], value[1], value[2])

    @staticmethod
    def _AlphaFromVec4f(value):
        return value[3]

    def _AssertPrimvar(self, primvar, expectedTypeName=None,
            expectedValues=None, expectedInterpolation=None,
            expectedIndices=None, expectedUnauthoredValuesIndex=None):
        self.assertTrue(primvar)

        if expectedInterpolation is None:
            expectedInterpolation = UsdGeom.Tokens.constant
        if expectedIndices is None:
            expectedIndices = Vt.IntArray()
        elif isinstance(expectedIndices, list):
            expectedIndices = Vt.IntArray(expectedIndices)
        if expectedUnauthoredValuesIndex is None:
            expectedUnauthoredValuesIndex = -1

        # This should work for undefined primvars.
        self.assertEqual(primvar.GetIndices(), expectedIndices)
        self.assertEqual(primvar.GetUnauthoredValuesIndex(),
            expectedUnauthoredValuesIndex)

        if expectedTypeName is None:
            self.assertFalse(primvar.IsDefined())
            # No further testing for primvars that we expect not to exist.
            return

        self.assertTrue(primvar.IsDefined())
        self.assertEqual(primvar.GetTypeName(), expectedTypeName)
        self.assertEqual(primvar.GetInterpolation(), expectedInterpolation)

        if expectedValues is None:
            self.assertFalse(primvar.GetAttr().HasAuthoredValueOpinion())
            self.assertEqual(primvar.Get(), None)
        else:
            for idx in range(len(primvar.Get())):
                val1 = primvar.Get()[idx]
                val2 = expectedValues[idx]
                if isinstance(val1, Gf.Vec3f):
                    self.assertEqual(val1, val2)  # both are 3-vectors
                    return
                self.assertAlmostEqual(val1, val2, places=5)  # default==7

    def _AssertMeshDisplayColorAndOpacity(self, mesh, expectedColors=None,
            expectedOpacities=None, expectedInterpolation=None,
            expectedIndices=None, expectedUnauthoredValuesIndex=None):
        displayColorPrimvar = mesh.GetDisplayColorPrimvar()
        self._AssertPrimvar(
            displayColorPrimvar, Sdf.ValueTypeNames.Color3fArray,
            expectedColors, expectedInterpolation,
            expectedIndices, expectedUnauthoredValuesIndex)

        displayOpacityPrimvar = mesh.GetDisplayOpacityPrimvar()
        self._AssertPrimvar(
            displayOpacityPrimvar, Sdf.ValueTypeNames.FloatArray,
            expectedOpacities, expectedInterpolation,
            expectedIndices, expectedUnauthoredValuesIndex)

    @staticmethod
    def _ClearColorSets(mesh):
        """
        Removes all color sets on the given MFnMesh.
        """
        colorSetNames = []
        mesh.getColorSetNames(colorSetNames)
        for colorSetName in colorSetNames:
            mesh.deleteColorSet(colorSetName)

    def _CopyColorSetToMeshAsDisplayColor(self, srcMesh, colorSetName, dstMesh):
        """
        Copies a color set named colorSetName from the MFnMesh srcMesh to
        the MFnMesh dstMesh. All existing color sets on dstMesh will be removed.
        """
        testUsdExportColorSets._ClearColorSets(dstMesh)

        colorSetData = OpenMaya.MColorArray()
        unsetColor = OpenMaya.MColor(-999, -999, -999, -999)
        srcMesh.getFaceVertexColors(colorSetData, colorSetName, unsetColor)

        colorRep = srcMesh.getColorRepresentation(colorSetName)
        colorRepString = 'RGBA'
        if colorRep == OpenMaya.MFnMesh.kAlpha:
            colorRepString = 'A'
        elif colorRep == OpenMaya.MFnMesh.kRGB:
            colorRepString = 'RGB'

        isClamped = srcMesh.isColorClamped(colorSetName)

        cmds.polyColorSet(dstMesh.name(),
                                create=True,
                                colorSet='displayColor',
                                representation=colorRepString,
                                clamped=isClamped)
        dstMesh.setCurrentColorSetName('displayColor')

        # XXX: The Maya setFaceVertexColor() API seems to somehow still author
        # faceVertex colors we don't want it to, so after setting the ones we
        # intend, we also remove the ones we don't.
        removeFaces = OpenMaya.MIntArray()
        removeVertices = OpenMaya.MIntArray()

        itMeshFV = OpenMaya.MItMeshFaceVertex(srcMesh.object())
        itMeshFV.reset()
        while not itMeshFV.isDone():
            faceId = itMeshFV.faceId()
            vertId = itMeshFV.vertId()
            faceVertId = itMeshFV.faceVertId()

            itMeshFV.next()

            colorIndexPtr = OpenMaya.intPtr()
            srcMesh.getFaceVertexColorIndex(faceId, faceVertId,
                colorIndexPtr, colorSetName)
            colorSetValue = colorSetData[colorIndexPtr.value()]
            if colorSetValue == unsetColor:
                removeFaces.append(faceId)
                removeVertices.append(vertId)
                continue

            dstMesh.setFaceVertexColor(colorSetValue, faceId, vertId, None,
                                       colorRep)

        if removeFaces.length() > 0 and removeVertices.length() > 0:
            dstMesh.removeFaceVertexColors(removeFaces, removeVertices)

    def _GetExpectedColorSetValues(self, colorSetName):
        """
        Given a color set name, generate the color set value list we'd expect
        to see when we export those color sets as USD primvars. The naming
        convention should identify what the color set contains:

        FaceColor: Color assignments per face (MFnMesh::setFaceColor), 0-5
        VertexColor: Color assignments per vertex (MFnMesh::setVertexColor), 0-7
        FaceVertexColor: Color assignments per face vertex
            (MFnMesh::setFaceVertexColor), 0-23

        Full vs. Sparse: There is no API in Maya to determine at what
        granularity a color set was authored, so usdExport will always get the
        colors from Maya at the faceVertex level, but then it tries to find
        equivalent values and compress the data to vertex, uniform, or constant
        interpolation if possible.
        FaceColor: Every odd-numbered face is unassigned.
        VertexColor: Every odd-numbered vertex is unassigned.
        FaceVertexColor: Every odd-numbered local faceVertex is unassigned.
            Local faceVertices are numbered 0-3 (for our cubes, at least) and
            uniquely identify a faceVertex when combined with a face ID.

        kAlpha/kRGB/kRGBA: Maps to values of MFnMesh::MColorRepresentation.

        Note that the color sets need to be authored in Maya to match the values
        produced here. This code can be adapted to author them in Maya.
        """
        isFaceColor = colorSetName.startswith('FaceColor_')
        isVertexColor = colorSetName.startswith('VertexColor_')
        isFaceVertexColor = colorSetName.startswith('FaceVertexColor_')

        isAlpha = self._IsColorSetAlpha(colorSetName)
        isRGB = self._IsColorSetRGB(colorSetName)
        isSparse = self._IsColorSetSparse(colorSetName)

        colorSetValues = []

        # For uniform and vertex interpolations, make sure that we visit each
        # component only once, so initialize all components as unvisited.
        faceVisited = [False] * self._colorSetSourceMesh.numPolygons()
        vertexVisited = [False] * self._colorSetSourceMesh.numVertices()

        # The export uses an MItMeshFaceVertex iterator when getting color set
        # values which will visit vertices somewhat "out-of-order" (it iterates
        # over faces and then local faceVertices within that face). To make
        # sure we match that ordering, we build up the list of colorSetValues
        # using the iterator too.
        itMeshFV = OpenMaya.MItMeshFaceVertex(self._colorSetSourceMesh.object())
        itMeshFV.reset()
        while not itMeshFV.isDone():
            faceId = itMeshFV.faceId()
            vertexId = itMeshFV.vertId()
            faceVertexId = itMeshFV.faceVertId()

            # Pre-emptively advance the iterator, in case we skip a component
            # in a sparse color set.
            itMeshFV.next()

            # Skip odd-numbered components for sparse color sets.
            if isSparse:
                if isFaceColor and (faceId % 2 != 0):
                    continue
                elif isVertexColor and (vertexId % 2 != 0):
                    continue
                elif isFaceVertexColor and (faceVertexId % 2 != 0):
                    continue

            # Don't append face or vertex colors more than once for uniform
            # or vertex interpolations, respectively.
            if isFaceColor and faceVisited[faceId]:
                continue
            if isVertexColor and vertexVisited[vertexId]:
                continue

            faceVisited[faceId] = True
            vertexVisited[vertexId] = True

            # Note that we make a copy of the default color here, since we'll
            # be manipulating individual values on it.
            colorSetValue = Gf.Vec4f(self.COLOR_COLOR_SET_DEFAULT)

            if isFaceColor:
                colorSetValue = self.FACE_COLOR_MAP[faceId]
                colorSetValue[3] = 1.0 - (faceId * 0.1)
            elif isVertexColor:
                colorSetValue = self.VERTEX_COLOR_MAP[vertexId]
                colorSetValue[3] = 1.0 - (vertexId * 0.1)
            elif isFaceVertexColor:
                colorSetValue[0] = 1.0 - (faceId * 0.1)
                colorSetValue[1] = 1.0 - (faceVertexId * 0.1)
                colorSetValue[2] = 0.0
                colorSetValue[3] = 1.0 - (faceId * 0.1 + faceVertexId * 0.01)

            if isAlpha:
                colorSetValue = self._AlphaFromVec4f(colorSetValue)
            elif isRGB:
                colorSetValue = self._ColorFromVec4f(colorSetValue)

            colorSetValues.append(colorSetValue)

        if isSparse:
            # Add the unassigned color value.
            colorSetValue = self.COLOR_COLOR_SET_DEFAULT
            if isAlpha:
                colorSetValue = self._AlphaFromVec4f(colorSetValue)
            elif isRGB:
                colorSetValue = self._ColorFromVec4f(colorSetValue)

            colorSetValues.append(colorSetValue)

        return colorSetValues

    def _GetExpectedColorSetIndices(self, colorSetName):
        """
        Given a color set name, return the assignment indices we'd expect to
        see when we export those color sets as USD primvars. The naming
        convention should identify what the color set contains. See above for
        more info.
        """
        isFaceColor = colorSetName.startswith('FaceColor_')
        isVertexColor = colorSetName.startswith('VertexColor_')
        isFaceVertexColor = colorSetName.startswith('FaceVertexColor_')

        isSparse = self._IsColorSetSparse(colorSetName)

        assignmentIndices = []

        if isSparse:
            # Every other component is unassigned.
            if isFaceColor:
                assignmentIndices = [0, 3, 1, 3, 2, 3]
            elif isVertexColor:
                assignmentIndices = [0, 4, 1, 4, 2, 4, 3, 4]
            elif isFaceVertexColor:
                assignmentIndices = [0, 12, 1, 12, 2, 12, 3, 12, 4, 12, 5, 12, 6, 12, 7, 12, 8, 12, 9, 12, 10, 12, 11, 12]
        else:
            # Every component has an assignment.
            if isFaceColor:
                assignmentIndices = [i for i in xrange(self._colorSetSourceMesh.numPolygons())]
            elif isVertexColor:
                # The assignments for vertex color are a little different
                # due to MItMeshFaceVertex visiting components in faceVertex
                # order, in which case we actually visit vertices somewhat
                # out of order. 
                assignmentIndices = [0, 1, 3, 2, 5, 4, 7, 6]
            elif isFaceVertexColor:
                assignmentIndices = [i for i in xrange(self._colorSetSourceMesh.numFaceVertices())]

        return assignmentIndices

    def _GetExpectedColorSetUnassignedIndex(self, colorSetName):
        """
        Given a color set name, return the index that represents unauthored
        values when the color set is exported as a primvar.
        """
        isSparse = self._IsColorSetSparse(colorSetName)

        unassignedIndex = -1

        if not isSparse:
            return unassignedIndex

        isFaceColor = colorSetName.startswith('FaceColor_')
        isVertexColor = colorSetName.startswith('VertexColor_')
        isFaceVertexColor = colorSetName.startswith('FaceVertexColor_')

        # When we skip components, we'll only be assigning half of them.
        # In the case of our cube, we have 6 faces, 8 vertices, and 24
        # faceVertices, so the unassigned indices should be 3, 4, and
        # 12, respectively.
        if isFaceColor:
            unassignedIndex = self._colorSetSourceMesh.numPolygons()
        elif isVertexColor:
            unassignedIndex = self._colorSetSourceMesh.numVertices()
        elif isFaceVertexColor:
            unassignedIndex = self._colorSetSourceMesh.numFaceVertices()

        unassignedIndex /= 2

        return unassignedIndex

    def _GetExpectedColorSetInterpolation(self, colorSetName):
        isFaceColor = colorSetName.startswith('FaceColor_')
        isVertexColor = colorSetName.startswith('VertexColor_')
        isFaceVertexColor = colorSetName.startswith('FaceVertexColor_')

        interpolation = None

        if isFaceColor:
            interpolation = UsdGeom.Tokens.uniform
        elif isVertexColor:
            interpolation = UsdGeom.Tokens.vertex
        elif isFaceVertexColor:
            interpolation = UsdGeom.Tokens.faceVarying

        return interpolation

    def _GetUsdStage(self, testName):
        usdFilePath = os.path.abspath('%s.usda' % testName)
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            exportDisplayColor=True,
            exportUVs=False)

        self._stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(self._stage)

    def _GetCubeMayaMesh(self, cubeName):
        selectionList = OpenMaya.MSelectionList()
        selectionList.add(cubeName)
        mObj = OpenMaya.MObject()
        selectionList.getDependNode(0, mObj)

        mesh = OpenMaya.MFnMesh(mObj)
        self.assertTrue(mesh)

        return mesh

    def _GetCubeUsdMesh(self, cubeName):
        cubePrimPath = '/UsdExportColorSetsTest/Geom/CubeMeshes/%s' % cubeName
        cubePrim = self._stage.GetPrimAtPath(cubePrimPath)
        self.assertTrue(cubePrim)

        usdMesh = UsdGeom.Mesh(cubePrim)
        self.assertTrue(usdMesh)

        return usdMesh

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    def setUp(self):
        cmds.file(os.path.abspath('UsdExportColorSetsTest.ma'),
                        open=True,
                        force=True)

        self._colorSetSourceMesh = self._GetCubeMayaMesh(
            'ColorSetSourceCubeShape')

    def testExportSourceColorSets(self):
        """
        Tests that the authored color sets on the source cube mesh export correctly.
        """
        cubeName = 'ColorSetSourceCube'
        self._GetUsdStage(cubeName)
        usdCubeMesh = self._GetCubeUsdMesh(cubeName)

        for colorSetName in self.SOURCE_COLOR_SET_NAMES:
            expectedTypeName = Sdf.ValueTypeNames.Color4fArray
            if self._IsColorSetAlpha(colorSetName):
                expectedTypeName = Sdf.ValueTypeNames.FloatArray
            elif self._IsColorSetRGB(colorSetName):
                expectedTypeName = Sdf.ValueTypeNames.Color3fArray

            expectedValues = self._GetExpectedColorSetValues(colorSetName)
            expectedIndices = self._GetExpectedColorSetIndices(colorSetName)

            expectedInterpolation = self._GetExpectedColorSetInterpolation(colorSetName)

            expectedUnauthoredValuesIndex = self._GetExpectedColorSetUnassignedIndex(colorSetName)

            primvar = usdCubeMesh.GetPrimvar(colorSetName)

            self._AssertPrimvar(primvar, expectedTypeName=expectedTypeName,
                expectedValues=expectedValues,
                expectedInterpolation=expectedInterpolation,
                expectedIndices=expectedIndices,
                expectedUnauthoredValuesIndex=expectedUnauthoredValuesIndex)

    def _VerifyColorSetsAsDisplayColorExportForCube(self, cubeName,
        colorSetName, expectedColors, expectedAlphas, expectedIndices):
        mayaCubeMesh = self._GetCubeMayaMesh('%sShape' % cubeName)

        self._CopyColorSetToMeshAsDisplayColor(self._colorSetSourceMesh,
            colorSetName, mayaCubeMesh)

        expectedInterpolation = self._GetExpectedColorSetInterpolation(colorSetName)

        self._GetUsdStage('%s-%s' % (cubeName, colorSetName))
        usdCubeMesh = self._GetCubeUsdMesh(cubeName)

        self._AssertMeshDisplayColorAndOpacity(usdCubeMesh,
            expectedColors=expectedColors,
            expectedOpacities=expectedAlphas,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

    def _ConvertColorsToLinear(self, colors):
        results = []
        for color in colors:
            results.append(Gf.ConvertDisplayToLinear(color))
        return results

    def testExportColorSetsAsDisplayColorOnObjectLevelCube(self):
        """
        Tests that color sets authored as displayColor on a cube with a
        material assignment at the object level export correctly.
        """
        cubeName = 'ObjectLevelCube'
        colorSetName = 'FaceColor_Sparse_kAlpha'
        expectedColors = [Gf.Vec3f(1, 1, 0)] * 4
        expectedAlphas = [1.0, 0.4, 0.8, 0.6]
        expectedIndices = [0, 1, 2, 1, 3, 1]
        self._VerifyColorSetsAsDisplayColorExportForCube(cubeName, colorSetName,
            expectedColors, expectedAlphas, expectedIndices)

        colorSetName = 'FaceVertexColor_Full_kRGB'
        expectedColors = self._GetExpectedColorSetValues(colorSetName)
        expectedColors = self._ConvertColorsToLinear(expectedColors)
        expectedAlphas = [0.4] * 24
        expectedIndices = self._GetExpectedColorSetIndices(colorSetName)
        self._VerifyColorSetsAsDisplayColorExportForCube(cubeName, colorSetName,
            expectedColors, expectedAlphas, expectedIndices)

    def testExportColorSetsAsDisplayColorOnUniquePerFaceCube(self):
        """
        Tests that color sets authored as displayColor on a cube with a unique
        material assignment per face export correctly.
        """
        cubeName = 'UniquePerFaceCube'

        colorSetName = 'FaceColor_Sparse_kRGB'
        expectedColors = [
            Gf.Vec3f(1.0, 0.0, 0.0),
            Gf.Vec3f(0.0, 0.0, 1.0),
            Gf.Vec3f(0.0, 0.0, 1.0),
            Gf.Vec3f(1.0, 0.0, 0.0),
            Gf.Vec3f(1.0, 0.0, 1.0),
            Gf.Vec3f(1.0, 1.0, 0.0)]
        expectedAlphas = [0.25, 0.55, 0.10, 0.85, 0.70, 0.40]
        expectedIndices = [0, 1, 2, 3, 4, 5]
        self._VerifyColorSetsAsDisplayColorExportForCube(cubeName, colorSetName,
            expectedColors, expectedAlphas, expectedIndices)

    def testExportColorSetsAsDisplayColorOnUnassignedCube(self):
        """
        Tests that color sets authored as displayColor on a cube with no
        material assignments export correctly.
        """
        cubeName = 'UnassignedCube'

        colorSetName = 'FaceVertexColor_Full_kRGB'
        expectedColors = self._GetExpectedColorSetValues(colorSetName)
        expectedColors = self._ConvertColorsToLinear(expectedColors)
        expectedAlphas = [self._AlphaFromVec4f(self.COLOR_SHADER_DEFAULT)] * 24
        expectedIndices = self._GetExpectedColorSetIndices(colorSetName)
        self._VerifyColorSetsAsDisplayColorExportForCube(cubeName, colorSetName,
            expectedColors, expectedAlphas, expectedIndices)

        colorSetName = 'FaceVertexColor_Full_kAlpha'
        expectedColors = [self._ColorFromVec4f(self.COLOR_SHADER_DEFAULT)] * 24
        expectedAlphas = self._GetExpectedColorSetValues(colorSetName)
        expectedIndices = self._GetExpectedColorSetIndices(colorSetName)
        self._VerifyColorSetsAsDisplayColorExportForCube(cubeName, colorSetName,
            expectedColors, expectedAlphas, expectedIndices)


if __name__ == '__main__':
    unittest.main(verbosity=2)
