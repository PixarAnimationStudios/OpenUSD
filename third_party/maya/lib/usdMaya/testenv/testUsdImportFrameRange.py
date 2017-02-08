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

from pxr import Usd

from maya import cmds
from maya import standalone


class testUsdImportFrameRange(unittest.TestCase):

    def _LoadUsdWithRange(self, start=None, end=None):
        # Import the USD file.
        usdFilePath = os.path.abspath('MovingCube.usda')
        cmds.loadPlugin('pxrUsd')
        if start is not None and end is not None:
            cmds.usdImport(file=usdFilePath, readAnimData=True,
                frameRange=(start, end))
        else:
            cmds.usdImport(file=usdFilePath, readAnimData=True)

        self.stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(self.stage)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

    def setUp(self):
        cmds.file(new=True, force=True)

    def testUsdImport(self):
        """
        Tests a simple import with frame range specified.
        """
        self._LoadUsdWithRange(1, 15)

        numKeyFrames = cmds.keyframe("pCube1.translateX", query=True,
            keyframeCount=True)
        self.assertEqual(numKeyFrames, 14)

        keyTimes = cmds.keyframe("pCube1.translateX", index=(0,14), query=True)
        self.assertEqual(keyTimes, [float(x) for x in range(1, 16) if x != 5.0])

    def testUsdImportNoRangeSpecified(self):
        """
        Tests an import with animation but no range specified.
        """
        self._LoadUsdWithRange()

        numKeyFrames = cmds.keyframe("pCube1.translateX", query=True,
            keyframeCount=True)
        self.assertEqual(numKeyFrames, 29)

        keyTimes = cmds.keyframe("pCube1.translateX", index=(0,29), query=True)
        self.assertEqual(keyTimes, [float(x) for x in range(1, 31) if x != 5.0])

    def testUsdImportOverlyLargeRange(self):
        """
        Tests an import frame range that is larger than the time range of
        animation available in USD prims.
        """
        self._LoadUsdWithRange(-100, 100)

        numKeyFrames = cmds.keyframe("pCube1.translateX", query=True,
            keyframeCount=True)
        self.assertEqual(numKeyFrames, 29)

        keyTimes = cmds.keyframe("pCube1.translateX", index=(0,29), query=True)
        self.assertEqual(keyTimes, [float(x) for x in range(1, 31) if x != 5.0])

    def testUsdImportOutOfRange(self):
        """
        Tests an import frame range that doesn't intersect with the time range
        of animation available in USD prims.
        """
        self._LoadUsdWithRange(-200, -100)

        numKeyFrames = cmds.keyframe("pCube1.translateX", query=True,
            keyframeCount=True)
        self.assertEqual(numKeyFrames, 0)

    def testUsdImportSingle(self):
        """
        Tests an import frame range that is only one frame.
        """
        self._LoadUsdWithRange(29, 29)

        xValue = cmds.getAttr("pCube1.translateX")
        self.assertAlmostEqual(xValue, 11.7042500857406)

        numKeyFrames = cmds.keyframe("pCube1.translateX", query=True,
            keyframeCount=True)
        self.assertEqual(numKeyFrames, 0) # Only one frame, so no real animation.

        keyTimes = cmds.keyframe("pCube1.translateX", index=(0,0), query=True)
        self.assertEqual(keyTimes, None) # Only one frame, so no real animation.


if __name__ == '__main__':
    unittest.main(verbosity=2)
