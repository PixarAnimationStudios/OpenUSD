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

import os
import unittest

from pxr import Gf
from pxr import Kind
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt

from maya import cmds, mel, standalone
from maya import OpenMaya as OM
from maya import OpenMayaFX as OMFX

class testUsdExportPointInstancer(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 400.0

    EPSILON = 1e-3

    @classmethod
    def _GetDagPath(cls, dagPath):
        sel = OM.MSelectionList()
        sel.add(dagPath)
        dagPath = OM.MDagPath()
        sel.getDagPath(0, dagPath)
        return dagPath

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.file(os.path.abspath('InstancerTest.ma'),
                  open=True,
                  force=True)

        # Create nCache. This is to be on the safe side and ensure that the
        # particles have the same positions for all test cases.
        # It doesn't look like doCreateNclothCache is Python-wrapped.
        cmds.select("nParticle1")
        cacheLocation = os.path.abspath("nCache")
        mel.eval('doCreateNclothCache 5 { "2", "1", "10", "OneFile", "1", "%s","0","","0", "add", "0", "1", "1","0","1","mcx" }'
                % cacheLocation)

        # Export to USD. (Don't load the reference assembly yet; this should
        # work without the reference assembly.)
        usdFilePath = os.path.abspath('InstancerTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            frameRange=(cls.START_TIMECODE, cls.END_TIMECODE))

        cls.stage = Usd.Stage.Open(usdFilePath)

        # Load the USD reference assembly so that it draws instanced.
        assembly = OM.MFnAssembly(cls._GetDagPath("referencePrototype"))
        assembly.activate("Full")

    def _AssertPrototype(self, prim, typeName, numChildren,
            hasInstancerTranslate):
        self.assertEqual(prim.GetTypeName(), typeName)
        self.assertEqual(len(prim.GetChildren()), numChildren)
        self.assertEqual(Usd.ModelAPI(prim).GetKind(), Kind.Tokens.subcomponent)

        xformOps = UsdGeom.Xformable(prim).GetOrderedXformOps()
        foundInstancerTranslate = False
        for op in xformOps:
            if op.GetOpName() == "xformOp:translate:instancerTranslate":
                foundInstancerTranslate = True
                break
        self.assertEqual(foundInstancerTranslate, hasInstancerTranslate)

    def testPrototypes(self):
        """
        Tests that all of the instancer prototypes made it to USD.
        """
        self.assertTrue(self.stage)
        instancer = OMFX.MFnInstancer(self._GetDagPath("instancer1"))

        # Move to the last frame so that we can ensure all of the prototypes
        # are in use.
        cmds.currentTime(self.END_TIMECODE, edit=True)
        paths = OM.MDagPathArray()
        matrices = OM.MMatrixArray()
        particlePathStartIndices = OM.MIntArray()
        pathIndices = OM.MIntArray()
        instancer.allInstances(
                paths, matrices, particlePathStartIndices, pathIndices)

        # Check that the Maya instanced objects are what we think they are.
        pathStrings = [paths[i].fullPathName() for i in xrange(paths.length())]
        self.assertEqual(pathStrings, [
            # 0 (logical 0) - Cube
            "|dummyGroup|pCube1|pCubeShape1",
            "|dummyGroup|pCube1|pSphere1|pSphereShape1",
            # 1 (logical 2) - Sphere
            "|InstancerTest|instancer1|prototypeUnderInstancer|prototypeUnderInstancerShape",
            # 2 (logical 3) - Reference
            "|referencePrototype|NS_referencePrototype:Geom|NS_referencePrototype:Cone|NS_referencePrototype:ConeShape"
        ])

        # Check that the USD prims have correct type name, references, kinds,
        # kinds, instancerTranslate xformOps.
        prototypesPrim = self.stage.GetPrimAtPath(
                "/InstancerTest/instancer1/Prototypes")
        self.assertEqual(len(prototypesPrim.GetChildren()), 3)

        prototype0 = prototypesPrim.GetChild("prototype_0")
        self._AssertPrototype(prototype0, "Xform", 2, True)

        prototype1 = prototypesPrim.GetChild("prototype_1")
        self._AssertPrototype(prototype1, "Mesh", 0, False)

        prototype2 = prototypesPrim.GetChild("prototype_2")
        self._AssertPrototype(prototype2, "Xform", 1, True)
        self.assertEqual(
                Usd.ModelAPI(prototype2).GetAssetName(),
                "ConeAssetName")

    def _MayaToGfMatrix(self, mayaMatrix):
        scriptUtil = OM.MScriptUtil()
        values = [[scriptUtil.getDouble4ArrayItem(mayaMatrix.matrix, r, c)
                for c in xrange(4)] for r in xrange(4)]
        return Gf.Matrix4d(values)

    def _GetWorldSpacePosition(self, path):
        return self._MayaToGfMatrix(self._GetDagPath(path).inclusiveMatrix())

    def _AssertMatricesRaw(self, mat1, mat2):
        """
        Asserts that mat1 and mat2 are element-wise close within EPSILON.
        """
        for i in xrange(4):
            for j in xrange(4):
                self.assertTrue(abs(mat1[i][j] - mat2[i][j]) < self.EPSILON,
                        "%s\n%s" % (mat1, mat2))

    def _AssertXformMatrices(self, mat1, mat2):
        """
        Asserts that two matrices that represent transforms are the same,
        taking into account the precision of half-precision rotations.
        The matrices are factored into translation/rotation/scale/etc. and
        the components are compared. Rotation is handled by converting to
        half-precision quaternions before comparing.
        """
        _, r1, s1, u1, t1, p1 = mat2.Factor()
        _, r2, s2, u2, t2, p2 = mat2.Factor()
        self.assertTrue(Gf.IsClose(t1, t2, self.EPSILON)) # translation
        self.assertTrue(Gf.IsClose(s1, s2, self.EPSILON)) # scale
        self._AssertMatricesRaw(u1, u2) # shear

        # Deal with rotations weirdly because of half-precision floats.
        # Extract the rotations as Quath's and compare them for closeness.
        rot1 = r1.ExtractRotation()
        rot2 = r2.ExtractRotation()
        quatd1 = rot1.GetQuat()
        quatd2 = rot2.GetQuat()
        quath1 = Gf.Quath(quatd1.real, *quatd1.imaginary)
        quath2 = Gf.Quath(quatd2.real, *quatd2.imaginary)
        self.assertTrue(abs(quath1.real - quath2.real) < self.EPSILON)
        self.assertTrue(Gf.IsClose(quath1.imaginary, quath2.imaginary,
                self.EPSILON))

    def testTransforms(self):
        """
        Check that the point transforms are correct.
        """
        mayaInstancer = OMFX.MFnInstancer(self._GetDagPath("instancer1"))
        usdInstancer = UsdGeom.PointInstancer(
                self.stage.GetPrimAtPath("/InstancerTest/instancer1"))

        time = self.START_TIMECODE
        while time <= self.END_TIMECODE:
            cmds.currentTime(time, edit=True)

            # Need to do this because MFnInstancer will give instance matrices
            # as offsets from prototypes' original world space positions.
            worldPositions = [
                self._GetWorldSpacePosition("|dummyGroup|pCube1"),
                self._GetWorldSpacePosition("|InstancerTest|instancer1|prototypeUnderInstancer"),
                self._GetWorldSpacePosition("|referencePrototype")
            ]

            paths = OM.MDagPathArray()
            matrices = OM.MMatrixArray()
            particlePathStartIndices = OM.MIntArray()
            pathIndices = OM.MIntArray()
            mayaInstancer.allInstances(
                    paths, matrices, particlePathStartIndices, pathIndices)

            usdInstanceTransforms = \
                    usdInstancer.ComputeInstanceTransformsAtTime(time, time)
            usdProtoIndices = usdInstancer.GetProtoIndicesAttr().Get(time)

            self.assertEqual(matrices.length(), len(usdInstanceTransforms))

            # Compute the instancer-space position of instances in Maya
            # (including the protos' transforms). By default, this is what
            # UsdGeomPointInstancer::ComputeInstanceTransformsAtTime already
            # gives us.
            mayaWorldPositions = [worldPositions[protoIndex]
                    for protoIndex in usdProtoIndices]
            mayaGfMatrices = [
                    mayaWorldPositions[i] * self._MayaToGfMatrix(matrices[i])
                    for i in xrange(matrices.length())]
            usdGfMatrices = [
                    usdInstanceTransforms[i]
                    for i in xrange(len(usdInstanceTransforms))]
            for i in xrange(len(usdGfMatrices)):
                self._AssertXformMatrices(
                        mayaGfMatrices[i], usdGfMatrices[i])

            time += 1.0

    def testInstancePaths(self):
        """
        Checks that the proto index assigned for each point is correct.
        """
        mayaInstancer = OMFX.MFnInstancer(self._GetDagPath("instancer1"))
        usdInstancer = UsdGeom.PointInstancer(
                self.stage.GetPrimAtPath("/InstancerTest/instancer1"))

        time = self.START_TIMECODE
        while time <= self.END_TIMECODE:
            cmds.currentTime(time, edit=True)

            paths = OM.MDagPathArray()
            matrices = OM.MMatrixArray()
            particlePathStartIndices = OM.MIntArray()
            pathIndices = OM.MIntArray()
            mayaInstancer.allInstances(
                    paths, matrices, particlePathStartIndices, pathIndices)
            usdProtoIndices = usdInstancer.GetProtoIndicesAttr().Get(time)

            # Mapping of proto index to index(es) in the paths array.
            # Note that in the Maya instancer a single point may map to multiple
            # DAG paths, which correspond to all the shapes in the instanced
            # hierarchy.
            usdIndicesToMayaIndices = {
                0: [0, 1], # the first prototype has two shapes in hierarchy
                1: [2],    # this prototype only has one shape
                2: [3],    # the reference prototype only has one shape
            }

            for i in xrange(len(usdProtoIndices)):
                usdProtoIndex = usdProtoIndices[i]
                expectedMayaIndices = usdIndicesToMayaIndices[usdProtoIndex]

                mayaIndicesStart = particlePathStartIndices[i]
                mayaIndicesEnd = particlePathStartIndices[i + 1]

                self.assertEqual(mayaIndicesEnd - mayaIndicesStart,
                        len(expectedMayaIndices))
                actualPathIndices = [pathIndices[i]
                        for i in xrange(mayaIndicesStart, mayaIndicesEnd)]
                self.assertEqual(actualPathIndices, expectedMayaIndices)

            time += 1.0


if __name__ == '__main__':
    unittest.main(verbosity=2)
