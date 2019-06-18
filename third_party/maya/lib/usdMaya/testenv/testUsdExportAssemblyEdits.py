#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from pxr import Gf
from pxr import Sdf
from pxr import Usd
from pxr import UsdGeom

from maya import cmds
from maya import standalone

import os
import unittest


class testUsdExportAssemblyEdits(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        mayaFile = os.path.abspath('UsdExportAssemblyEditsTest.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('AssemblyEditsTest.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            exportRefsAsInstanceable=True, shadingMode='none')

        cls._stage = Usd.Stage.Open(usdFilePath)
        cls._rootLayer = cls._stage.GetRootLayer()
        cls._modelPrimPath = Sdf.Path.absoluteRootPath.AppendChild(
            'UsdExportAssemblyEditsTest')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

    def _AssertPrimHasReference(self, primPath):
        """
        Verifies that a prim exists at the given path on the USD stage, and
        that it has a single reference in the stage's root layer.

        The prim is returned.
        """
        prim = self._stage.GetPrimAtPath(primPath)
        self.assertTrue(prim)

        primSpec = self._rootLayer.GetPrimAtPath(primPath)
        self.assertTrue(primSpec)

        references = primSpec.referenceList.GetAddedOrExplicitItems()
        self.assertEqual(len(references), 1)

        return prim

    def _AssertPrimHasEdits(self, primPath, hasEdits):
        """
        Verifies that a prim exists at the given path on the USD stage, and
        that there is or is not scene description for it in the stage's root
        layer based on the value of hasEdits.

        The prim is returned.
        """
        prim = self._stage.GetPrimAtPath(primPath)
        self.assertTrue(prim)

        primSpec = self._rootLayer.GetPrimAtPath(primPath)

        if hasEdits:
            self.assertTrue(primSpec)
        else:
            self.assertFalse(primSpec)

        return prim

    def _ValidateXformOp(self, primPath, opName, opType, opValue):
        """
        Verifies that an Xformable prim exists at the given path on the USD
        stage, and that it has a single XformOp with the given opName and
        opType. The XformOp's value must match opValue.
        """
        prim = self._stage.GetPrimAtPath(primPath)
        self.assertTrue(prim)

        xformable = UsdGeom.Xformable(prim)
        self.assertTrue(xformable)

        xformOps = xformable.GetOrderedXformOps()
        foundXformOp = False

        for xformOp in xformOps:
            if xformOp.GetName() == opName and xformOp.GetOpType() == opType:
                self.assertFalse(foundXformOp)
                foundXformOp = True
                break

        self.assertTrue(foundXformOp)

        self.assertEqual(xformOp.Get(), opValue)

    def testExportModelNoEdits(self):
        """
        Validates that an assembly node that references a model USD file
        exports correctly when it does not have any Maya assembly edits.
        """
        primPath = self._modelPrimPath.AppendChild('ModelNoEdits')
        prim = self._AssertPrimHasReference(primPath)

        # The target of the reference has kind 'component' (which is not a
        # group), and there are no edits, so this prim should be instanceable.
        self.assertTrue(prim.IsInstanceable())

        # There should be no additional scene description in the root layer
        # below the model prim with the reference.
        geomPrimPath = primPath.AppendChild('Geom')
        self._AssertPrimHasEdits(geomPrimPath, False)

    def testExportModelWithEdits(self):
        """
        Validates that an assembly node that references a model USD file
        exports correctly when it has Maya assembly edits.
        """
        primPath = self._modelPrimPath.AppendChild('ModelWithEdits')
        prim = self._AssertPrimHasReference(primPath)

        # The target of the reference has kind 'component' (which is not a
        # group), but the assembly does have edits, so this prim should not be
        # instanceable.
        self.assertFalse(prim.IsInstanceable())

        geomPrimPath = primPath.AppendChild('Geom')

        shapePrimPath = geomPrimPath.AppendChild('Cone')
        self._AssertPrimHasEdits(shapePrimPath, True)
        self._ValidateXformOp(shapePrimPath, 'xformOp:rotateXYZ',
            UsdGeom.XformOp.TypeRotateXYZ, Gf.Vec3d(30.0, 45.0, 60.0))

        shapePrimPath = geomPrimPath.AppendChild('Cube')
        self._AssertPrimHasEdits(shapePrimPath, True)
        self._ValidateXformOp(shapePrimPath, 'xformOp:translate',
            UsdGeom.XformOp.TypeTranslate, Gf.Vec3d(7.0, 8.0, 9.0))

        # The cylinder should not have been edited.
        shapePrimPath = geomPrimPath.AppendChild('Cylinder')
        self._AssertPrimHasEdits(shapePrimPath, False)

        shapePrimPath = geomPrimPath.AppendChild('Sphere')
        self._AssertPrimHasEdits(shapePrimPath, True)
        self._ValidateXformOp(shapePrimPath, 'xformOp:translate',
            UsdGeom.XformOp.TypeTranslate, Gf.Vec3d(0.0, 0.0, 10.0))

        shapePrimPath = geomPrimPath.AppendChild('Torus')
        self._AssertPrimHasEdits(shapePrimPath, True)
        self._ValidateXformOp(shapePrimPath, 'xformOp:rotateXYZ',
            UsdGeom.XformOp.TypeRotateXYZ, Gf.Vec3d(0.0, 45.0, 0.0))

    def testExportSetNoEdits(self):
        """
        Validates that an assembly node that references a set USD file exports
        correctly when it does not have any Maya assembly edits.
        """
        primPath = self._modelPrimPath.AppendChild('SetNoEdits')
        prim = self._AssertPrimHasReference(primPath)

        # The target of the reference has kind 'assembly' which is a group, and
        # groups cannot be instanced, so this prim should not be instanceable.
        self.assertFalse(prim.IsInstanceable())

        # There should be no additional scene description in the root layer
        # below the set prim with the reference.
        for i in range(1, 6):
            setPrimPath = primPath.AppendChild('Shapes_%d' % i)
            self._AssertPrimHasEdits(setPrimPath, False)

    def testExportSetWithSetEdits(self):
        """
        Validates that an assembly node that references a set USD file exports
        correctly when it has Maya assembly edits on nodes in the first level
        of assembly hierarchy (nodes *not* underneath nested assemblies).
        """
        primPath = self._modelPrimPath.AppendChild('SetWithSetEdits')
        prim = self._AssertPrimHasReference(primPath)

        # The target of the reference has kind 'assembly' which is a group, and
        # groups cannot be instanced, so this prim should not be instanceable.
        self.assertFalse(prim.IsInstanceable())

        setPrimPath = primPath.AppendChild('Shapes_1')
        self._AssertPrimHasEdits(setPrimPath, True)
        self._ValidateXformOp(setPrimPath, 'xformOp:rotateXYZ',
            UsdGeom.XformOp.TypeRotateXYZ, Gf.Vec3d(30.0, 45.0, 60.0))

        setPrimPath = primPath.AppendChild('Shapes_2')
        self._AssertPrimHasEdits(setPrimPath, True)
        self._ValidateXformOp(setPrimPath, 'xformOp:translate',
            UsdGeom.XformOp.TypeTranslate, Gf.Vec3d(7.0, 8.0, 9.0))

        setPrimPath = primPath.AppendChild('Shapes_3')
        self._AssertPrimHasEdits(setPrimPath, True)
        self._ValidateXformOp(setPrimPath, 'xformOp:translate',
            UsdGeom.XformOp.TypeTranslate, Gf.Vec3d(0.0, 0.0, 10.0))

        setPrimPath = primPath.AppendChild('Shapes_4')
        self._AssertPrimHasEdits(setPrimPath, True)
        self._ValidateXformOp(setPrimPath, 'xformOp:rotateXYZ',
            UsdGeom.XformOp.TypeRotateXYZ, Gf.Vec3d(0.0, 45.0, 0.0))

        # The fifth shape set should not have been edited.
        setPrimPath = primPath.AppendChild('Shapes_5')
        self._AssertPrimHasEdits(setPrimPath, False)

    def testExportSetWithComponentEdits(self):
        """
        Validates that an assembly node that references a set USD file exports
        correctly when it has Maya assembly edits on nodes underneath nested
        assemblies.
        """
        primPath = self._modelPrimPath.AppendChild('SetWithComponentEdits')
        prim = self._AssertPrimHasReference(primPath)

        # The target of the reference has kind 'assembly' which is a group, and
        # groups cannot be instanced, so this prim should not be instanceable.
        self.assertFalse(prim.IsInstanceable())

        # Four out of the five shape sets should not have had edits applied to
        # them.
        for i in [1, 2, 4, 5]:
            setPrimPath = primPath.AppendChild('Shapes_%d' % i)
            self._AssertPrimHasEdits(setPrimPath, False)

        geomPrimPath = primPath.AppendChild('Shapes_3').AppendChild('Geom')

        shapePrimPath = geomPrimPath.AppendChild('Cone')
        self._AssertPrimHasEdits(shapePrimPath, True)
        self._ValidateXformOp(shapePrimPath, 'xformOp:rotateXYZ',
            UsdGeom.XformOp.TypeRotateXYZ, Gf.Vec3d(30.0, 45.0, 60.0))

        shapePrimPath = geomPrimPath.AppendChild('Cube')
        self._AssertPrimHasEdits(shapePrimPath, True)
        self._ValidateXformOp(shapePrimPath, 'xformOp:translate',
            UsdGeom.XformOp.TypeTranslate, Gf.Vec3d(7.0, 8.0, 9.0))

        # The cylinder should not have been edited.
        shapePrimPath = geomPrimPath.AppendChild('Cylinder')
        self._AssertPrimHasEdits(shapePrimPath, False)

        shapePrimPath = geomPrimPath.AppendChild('Sphere')
        self._AssertPrimHasEdits(shapePrimPath, True)
        self._ValidateXformOp(shapePrimPath, 'xformOp:translate',
            UsdGeom.XformOp.TypeTranslate, Gf.Vec3d(0.0, 0.0, 10.0))

        shapePrimPath = geomPrimPath.AppendChild('Torus')
        self._AssertPrimHasEdits(shapePrimPath, True)
        self._ValidateXformOp(shapePrimPath, 'xformOp:rotateXYZ',
            UsdGeom.XformOp.TypeRotateXYZ, Gf.Vec3d(0.0, 45.0, 0.0))


if __name__ == '__main__':
    unittest.main(verbosity=2)
