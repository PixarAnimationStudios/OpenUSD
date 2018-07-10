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

from maya import cmds
from maya import standalone

from pxr import Sdf, Usd, UsdGeom, Gf, Vt, UsdUtils


class testUsdExportAsClip(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.file(os.path.abspath('UsdExportAsClipTest.ma'), open=True,
            force=True)
        print os.path.abspath('UsdExportAsClipTest.ma')

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _ValidateNumSamples(self, stage, primPath, attrName, expectedNumSamples):
        """Check that the expected number of samples have been written to a clip"""
        cube = stage.GetPrimAtPath(primPath)
        self.assertTrue(cube)
        attr = cube.GetAttribute(attrName)
        self.assertTrue(Gf.IsClose(attr.GetNumTimeSamples(), expectedNumSamples, 1e-6))

    def _ValidateSamples(self, canonicalStage, testStage, primPath, attrName, frameRange):
        """
        Check that an attibute's values between two stages are equivalent over a
        certain frame range.
        """
        def getValues(stage):
            cube = stage.GetPrimAtPath(primPath)
            self.assertTrue(cube)
            attr = cube.GetAttribute(attrName)
            return [attr.Get(time=tc) for tc in xrange(*frameRange)]

        for frame, x,y in zip(xrange(*frameRange),
                              getValues(canonicalStage),
                              getValues(testStage)):
            msg = ('different values found on frame: {frame}\n'
                   'non clip: {x}\n'
                   'clips:    {y}'.format(frame=frame, x=x, y=y))
            if isinstance(x, basestring):
                self.assertEqual(x, y, msg=msg)
            elif isinstance(x, Vt.Vec3fArray):
                self.assertEqual(len(x), len(y), msg)
                for xpart, ypart in zip(x,y):
                    self.assertTrue(Gf.IsClose(xpart, ypart, 1e-6), msg=msg)
            else:
                self.assertTrue(Gf.IsClose(x, y, 1e-6), msg=msg)

    def testExportAsClip(self):
        """
        Test that a maya scene exports to usd the same way if it is exported
        all at once, or in 5 frame clips and then stitched back together.
        """
        # generate clip files and validate num samples on points attribute
        clipFiles = []
        # first 5 frames have no animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.001.usda')
        clipFiles.append(usdFile)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, frameRange=(1, 5))
        stage = Usd.Stage.Open(usdFile)
        self._ValidateNumSamples(stage,'/world/pCube1', 'points',  1)

        # next 5 frames have no animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.005.usda')
        clipFiles.append(usdFile)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, frameRange=(5, 10))
        stage = Usd.Stage.Open(usdFile)
        self._ValidateNumSamples(stage, '/world/pCube1', 'points', 1)

        # next 5 frames have deformation animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.010.usda')
        clipFiles.append(usdFile)
        frames = (10, 15)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, frameRange=frames)
        stage = Usd.Stage.Open(usdFile)
        self._ValidateNumSamples(stage, '/world/pCube1', 'points', frames[1] + 1 - frames[0])

        # next 5 frames have no animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.015.usda')
        clipFiles.append(usdFile)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, frameRange=(15, 20))
        stage = Usd.Stage.Open(usdFile)
        self._ValidateNumSamples(stage, '/world/pCube1', 'points', 1)

        stitchedPath = os.path.abspath('result.usda')
        stitchedLayer = Sdf.Layer.CreateNew(stitchedPath)
        UsdUtils.StitchClips(stitchedLayer, clipFiles, '/world', 1, 20, 'default')

        # export a non clip version for comparison
        canonicalUsdFile = os.path.abspath('canonical.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=canonicalUsdFile, frameRange=(1, 20))

        print 'comparing: \nnormal: {}\nstitched: {}'.format(canonicalUsdFile, stitchedPath)
        canonicalStage = Usd.Stage.Open(canonicalUsdFile)
        clipsStage = Usd.Stage.Open(stitchedPath)
        # visible
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube1', 'visibility', (0, 21))
        # animated visibility
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube2', 'visibility', (0, 21))
        # hidden, non animated:
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube4', 'visibility', (0, 21))
        # constant points:
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube2', 'points', (0, 21))
        # blend shape driven animated points:
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube3', 'points', (0, 21))
        # animated points:
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube1', 'points', (0, 21))


if __name__ == '__main__':
    unittest.main(verbosity=2)
