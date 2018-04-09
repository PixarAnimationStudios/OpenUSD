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

import logging
import unittest
import sys

from pxr import Gf

fbAxis = Gf.Vec3d(1, 0, 0)
lrAxis = Gf.Vec3d(0, 1, 0)
swAxis = Gf.Vec3d(0, 0, 1)
axes = {'XYZ' : (fbAxis, lrAxis, swAxis),
        'YXZ' : (lrAxis, fbAxis, swAxis),
        'ZXY' : (swAxis, fbAxis, lrAxis),
        'XZY' : (fbAxis, swAxis, lrAxis),
        'YZX' : (lrAxis, swAxis, fbAxis),
        'ZYX' : (swAxis, lrAxis, fbAxis) }

def ComposeRotation(rotOrder, rotVals):
    """Recomposes a rotation using the given rotation order and a list of
    angles in radians."""
    twRot = Gf.Rotation(axes[rotOrder][2], Gf.RadiansToDegrees(rotVals[0]))
    fbRot = Gf.Rotation(axes[rotOrder][0], Gf.RadiansToDegrees(rotVals[1]))
    lrRot = Gf.Rotation(axes[rotOrder][1], Gf.RadiansToDegrees(rotVals[2]))
    swRot = Gf.Rotation(axes[rotOrder][2], Gf.RadiansToDegrees(rotVals[3]))
    twMat = Gf.Matrix3d(twRot)
    fbMat = Gf.Matrix3d(fbRot)
    lrMat = Gf.Matrix3d(lrRot)
    swMat = Gf.Matrix3d(swRot)
    return Gf.Matrix4d(twMat * fbMat * lrMat * swMat, Gf.Vec3d(0))

def IsMatrixClose (x, y, epsilon=1e-6):
    for i in range(4):
        if not Gf.IsClose(x.GetRow(i), y.GetRow(i), epsilon):
            return False
    return True

class TestGfDecomposeRotation(unittest.TestCase):
    
    def setUp(self):
        self.log = logging.getLogger()

    # This function runs the standards tests for decomposing a rotation and
    # verifying that the resulting angles recompose back into the same rotation
    # again.  It also can check that the decomposed angles match expectedResult
    # if that is passed in.
    # 
    # XXX: For further testing we should test for left handed matrices as well,
    # but the results of left handed matrices decomposed into right handed
    # coordinate spaces are inconsistent right now.  Would like to get these
    # results more consistent but am currently worried about the fallout from
    # consumers of the DecomposeRotation function if we change this right now.
    def _TestDecomposeRotation(self,
                               rot, 
                               rotOrder = "XYZ", 
                               thetaTwHint = 300, 
                               thetaFBHint = 300, 
                               thetaLRHint = 300, 
                               thetaSwHint = 300, 
                               useHint = False, 
                               expectedResult = None):
        self.log.info("_TestDecomposeRotation")
        self.log.info("    rot = " + str(rot))
        self.log.info("    rotOrder = " + str(rotOrder))
        self.log.info("    thetaTwHint = " + str(thetaTwHint))
        self.log.info("    thetaFBHint = " + str(thetaFBHint))
        self.log.info("    thetaLRHint = " + str(thetaLRHint))
        self.log.info("    thetaSwHint = " + str(thetaSwHint))
        self.log.info("    useHint = " + str(useHint))
        self.log.info("    expectedResult = " + str(expectedResult))
        self.log.info("")

        # Do a special test for the passed in rotation order if expectedResult
        # was passed in for checking.
        if expectedResult != None:
            result = \
                Gf.Rotation.DecomposeRotation(rot,
                                              twAxis = axes[rotOrder][2],
                                              fbAxis = axes[rotOrder][0],
                                              lrAxis = axes[rotOrder][1],
                                              handedness = 1.0,
                                              thetaTwHint = thetaTwHint,
                                              thetaFBHint = thetaFBHint,
                                              thetaLRHint = thetaLRHint,
                                              thetaSwHint = thetaSwHint,
                                              useHint = useHint)
            for x,y in zip(result, expectedResult):
                self.assertAlmostEqual(x, y, places=5)
            self.assertTrue(IsMatrixClose(ComposeRotation(rotOrder, result), rot))

        # Test that decomposing and then recomposing yields the same matrix
        # for all rotation orders.
        for key in axes.keys():
            result = \
                Gf.Rotation.DecomposeRotation(rot,
                                              twAxis = axes[key][2],
                                              fbAxis = axes[key][0],
                                              lrAxis = axes[key][1],
                                              handedness = 1.0,
                                              thetaTwHint = thetaTwHint,
                                              thetaFBHint = thetaFBHint,
                                              thetaLRHint = thetaLRHint,
                                              thetaSwHint = thetaSwHint,
                                              useHint = useHint)
            self.assertTrue(IsMatrixClose(ComposeRotation(key, result), rot))

    def test_DecomposeRotationsAllAngles(self):
        """Test decomposing all four angles"""

        rot = Gf.Matrix4d(
            -0.77657773110174733, 0.41334436580878597, -0.47547183177449759, 0.0,
            0.53325734733674601, 0.029348545343396093, -0.8454438268729656, 0.0,
            -0.33550503583141722, -0.91010169659239692, -0.24321034680169412, 0.0,
            0.0, 0.0, 0.0, 1.0)

        # No hints
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = 300,
            thetaLRHint = 300,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (-1.641680, 1.998063, 0.943548, 0.0))

        # Use hints variation 1
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = -2.0,
            thetaFBHint = 2.0,
            thetaLRHint = 1.0,
            thetaSwHint = 0.0,
            useHint = True,
            expectedResult = (-1.641680, 1.998063, 0.943548, 0.0))

        # Use hints variation 2
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 1.5,
            thetaFBHint = 1.0,
            thetaLRHint = -2.0,
            thetaSwHint = 0.0,
            useHint = True,
            expectedResult = (1.499912, 1.143529, -2.198045, 0.0))

        # Use hints variation 3
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 1.5,
            thetaFBHint = -2.0,
            thetaLRHint = -1.0,
            thetaSwHint = 3.0,
            useHint = True,
            expectedResult = (1.499912, -1.998063, -0.943548, 3.141592))

        # Use hints variation 4
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = -1.5,
            thetaFBHint = -1.0,
            thetaLRHint = 2.0,
            thetaSwHint = 3.0,
            useHint = True,
            expectedResult = (-1.641680, -1.143529, 2.198045, 3.141592))

        # Use hints shifted by multiples of 2pi
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 14.0,
            thetaFBHint = 4.0,
            thetaLRHint = -20.0,
            thetaSwHint = -16,
            useHint = True,
            expectedResult = (14.066283, 4.285122, -19.793104, -15.707963))

        # Test gimbal locked rotations
        rot = Gf.Matrix4d(
            -0.70710678118654746, 0.0, 0.70710678118654768, 0.0,
            -0.70710678118654768, 0.0, -0.70710678118654746, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = 300,
            thetaLRHint = 300,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (1.178097, 1.570796, -1.178097, 0.0))

        rot = Gf.Matrix4d(
            -0.70710678118654746, 0.0, -0.70710678118654768, 0.0,
            -0.70710678118654768, 0.0, 0.70710678118654746, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = 300,
            thetaLRHint = 300,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (1.178097, -1.570796, 1.178097, 0.0))

    # Test zero Tw
    def TestDecomposeRotationsZeroTw(self):
        rot = Gf.Matrix4d(
            -0.77657773110174733, 0.41334436580878597, -0.47547183177449759, 0.0,
            0.53325734733674601, 0.029348545343396093, -0.8454438268729656, 0.0,
            -0.33550503583141722, -0.91010169659239692, -0.24321034680169412, 0.0,
            0.0, 0.0, 0.0, 1.0)

        # No hints
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = None,
            thetaFBHint = 300,
            thetaLRHint = 300,
            thetaSwHint = 300,
            useHint = False,
            expectedResult =(0.0, 1.290688, 2.646092, -0.489124))

        # Hints variation 1
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = None,
            thetaFBHint = 1.0,
            thetaLRHint = 3.0,
            thetaSwHint = 0.0,
            useHint = True,
            expectedResult = (0.0, 1.290688, 2.646092, -0.489124))

        # Hints variation 2
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = None,
            thetaFBHint = -2.0,
            thetaLRHint = 1.0,
            thetaSwHint = 3.0,
            useHint = True,
            expectedResult = (0.0, -1.850905, 0.495501, 2.652469))

        # Use hints shifted by multiples of 2pi
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = None,
            thetaFBHint = -14.0,
            thetaLRHint = 19.0,
            thetaSwHint = -4.0,
            useHint = True,
            expectedResult = (0.0, -14.417276, 19.345057, -3.630717))

        # Test gimbal locked rotations
        rot = Gf.Matrix4d(
            0.0, 0.0, -1.0, 0.0,
            -0.70710678118654746, -0.70710678118654768, 0.0, 0.0,
            -0.70710678118654768, 0.70710678118654746, 0.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = None,
            thetaFBHint = 300,
            thetaLRHint = 300,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (0.0, -1.178097, 1.570796, 1.178097))

        rot = Gf.Matrix4d(
            0.0, 0.0, 1.0, 0.0,
            -0.70710678118654746, 0.70710678118654768, 0.0, 0.0,
            -0.70710678118654768, -0.70710678118654746, 0.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = None,
            thetaFBHint = 300,
            thetaLRHint = 300,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (0.0, 0.392699, -1.570796, 0.392699))

    def test_DecomposeRotationsZeroFB(self):
        rot = Gf.Matrix4d(
            -0.77657773110174733, 0.41334436580878597, -0.47547183177449759, 0.0,
            0.53325734733674601, 0.029348545343396093, -0.8454438268729656, 0.0,
            -0.33550503583141722, -0.91010169659239692, -0.24321034680169412, 0.0,
            0.0, 0.0, 0.0, 1.0)

        # No hints
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = None,
            thetaLRHint = 300,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (-1.058488, 0.0, 1.816471, -1.923984))

        # Hints variation 1
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = -1.0,
            thetaFBHint = None,
            thetaLRHint = 2.0,
            thetaSwHint = -2.0,
            useHint = True,
            expectedResult = (-1.058488, 0.0, 1.816471, -1.923984))

        # Hints variation 2
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 2.0,
            thetaFBHint = None,
            thetaLRHint = -2.0,
            thetaSwHint = 1.0,
            useHint = True,
            expectedResult = (2.083105, 0.0, -1.816471, 1.217609))

        # Use hints shifted by multiples of 2pi
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 21.0,
            thetaFBHint = None,
            thetaLRHint = 30.0,
            thetaSwHint = -12.0,
            useHint = True,
            expectedResult = (20.932661, 0.0, 29.599456, -11.348762))

        # Test gimbal locked rotations
        rot = Gf.Matrix4d(
            -0.70710678118654746, 0.70710678118654768, 0.0, 0.0,
            -0.70710678118654768, -0.70710678118654746, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = None,
            thetaLRHint = 300,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (1.178097, 0.0, 0.0, 1.178097))

        rot = Gf.Matrix4d(
            -0.70710678118654746, -0.70710678118654768, 0.0, 0.0,
            -0.70710678118654768, 0.70710678118654746, 0.0, 0.0,
            0.0, 0.0, -1.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = None,
            thetaLRHint = 300,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (-0.392699, 0.0, -3.141593, 0.392699))

    def test_DecomposeRotationsZeroLR(self):
        rot = Gf.Matrix4d(
            -0.77657773110174733, 0.41334436580878597, -0.47547183177449759, 0.0,
            0.53325734733674601, 0.029348545343396093, -0.8454438268729656, 0.0,
            -0.33550503583141722, -0.91010169659239692, -0.24321034680169412, 0.0,
            0.0, 0.0, 0.0, 1.0)

        # No hints
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = 300,
            thetaLRHint = None,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (-2.629284, 1.816471, 0.0, -0.353188))

        # Hints variation 1
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = -3.0,
            thetaFBHint = 2.0,
            thetaLRHint = None,
            thetaSwHint = 0.0,
            useHint = True,
            expectedResult = (-2.629284, 1.816471, 0.0, -0.353188))

        # Hints variation 2
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 0.0,
            thetaFBHint = -2.0,
            thetaLRHint = None,
            thetaSwHint = 3.0,
            useHint = True,
            expectedResult = (0.512309, -1.816471, 0.0, 2.788405))

        # Use hints shifted by multiples of 2pi
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 13.0,
            thetaFBHint = 5.0,
            thetaLRHint = None,
            thetaSwHint = -23.0,
            useHint = True,
            expectedResult = (13.078679, 4.466714, 0.0, -22.344336))

        # Test gimbal locked rotations
        rot = Gf.Matrix4d(
            -0.70710678118654746, 0.70710678118654768, 0.0, 0.0,
            -0.70710678118654768, -0.70710678118654746, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = 300,
            thetaLRHint = None,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (1.178097, 0.0, 0.0, 1.178097))

        rot = Gf.Matrix4d(
            -0.70710678118654746, -0.70710678118654768, 0.0, 0.0,
            -0.70710678118654768, 0.70710678118654746, 0.0, 0.0,
            0.0, 0.0, -1.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = 300,
            thetaLRHint = None,
            thetaSwHint = 300,
            useHint = False,
            expectedResult = (1.178097, -3.141593, 0.0, -1.178097))

    def test_DecomposeRotationsZeroSw(self):
        rot = Gf.Matrix4d(
            -0.77657773110174733, 0.41334436580878597, -0.47547183177449759, 0.0,
            0.53325734733674601, 0.029348545343396093, -0.8454438268729656, 0.0,
            -0.33550503583141722, -0.91010169659239692, -0.24321034680169412, 0.0,
            0.0, 0.0, 0.0, 1.0)

        # No hints
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = 300,
            thetaLRHint = 300,
            thetaSwHint = None,
            useHint = False,
            expectedResult = (-1.641680, 1.998063, 0.943548, 0.0))

        # Hints variation 1
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = -2.0,
            thetaFBHint = 2,
            thetaLRHint = 1.0,
            thetaSwHint = None,
            useHint = True,
            expectedResult = (-1.641680, 1.998063, 0.943548, 0.0))

        # Hints variation 2
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 2.0,
            thetaFBHint = -1.0,
            thetaLRHint = -2.0,
            thetaSwHint = None,
            useHint = True,
            expectedResult = (1.499913, 1.143529, -2.198045, 0.0))

        # Use hints shifted by multiples of 2pi
        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = -20.0,
            thetaFBHint = 8,
            thetaLRHint = -12,
            thetaSwHint = None,
            useHint = True,
            expectedResult = (-20.491236, 8.281248, -11.622823, 0.0))

        # Test gimbal locked rotations
        rot = Gf.Matrix4d(
            -0.70710678118654746, 0.0, 0.70710678118654768, 0.0,
            -0.70710678118654768, 0.0, -0.70710678118654746, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = 300,
            thetaLRHint = 300,
            thetaSwHint = None,
            useHint = False,
            expectedResult = (1.178097, 1.570796, -1.178097, 0.0))

        rot = Gf.Matrix4d(
            -0.70710678118654746, 0.0, -0.70710678118654768, 0.0,
            -0.70710678118654768, 0.0, 0.70710678118654746, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 1.0)

        self._TestDecomposeRotation(rot, 'XYZ',
            thetaTwHint = 300,
            thetaFBHint = 300,
            thetaLRHint = 300,
            thetaSwHint = None,
            useHint = False,
            expectedResult = (1.178097, -1.570796, 1.178097, 0.0))

    def test_DecomposeWithSwingShift(self):
        """Tests decomposing into four angles using swing shift"""

        # Rotation matrix without swing
        rot = Gf.Matrix4d(
            -0.77657773110174733, 0.41334436580878597, -0.47547183177449759, 0.0,
            0.53325734733674601, 0.029348545343396093, -0.8454438268729656, 0.0,
            -0.33550503583141722, -0.91010169659239692, -0.24321034680169412, 0.0,
            0.0, 0.0, 0.0, 1.0)

        # Additional swing rotation angle
        swShift = 2.132495

        # Final rotation with swing applied to the matrix
        rotWithSwing = rot * \
             Gf.Matrix4d(
                Gf.Matrix3d(
                    Gf.Rotation(Gf.Vec3d(0,0,1), Gf.RadiansToDegrees(swShift))),
                Gf.Vec3d(0))

        # No hints
        result = \
            Gf.Rotation.DecomposeRotation(rot,
                                          twAxis = Gf.Vec3d(0, 0, 1),
                                          fbAxis = Gf.Vec3d(1, 0, 0),
                                          lrAxis = Gf.Vec3d(0, 1, 0),
                                          handedness = 1,
                                          thetaTwHint = 300,
                                          thetaFBHint = 300,
                                          thetaLRHint = 300,
                                          thetaSwHint = 300,
                                          swShift = swShift)

        for x,y in zip(result, (1.499913, -1.998063, -0.943548, -1.009098)):
            self.assertAlmostEqual(x, y, places=5)
        self.assertTrue(IsMatrixClose(ComposeRotation('XYZ', result), rotWithSwing))

        # Hints variation 1
        result = \
            Gf.Rotation.DecomposeRotation(rot,
                                          twAxis = Gf.Vec3d(0, 0, 1),
                                          fbAxis = Gf.Vec3d(1, 0, 0),
                                          lrAxis = Gf.Vec3d(0, 1, 0),
                                          handedness = 1,
                                          thetaTwHint = 0,
                                          thetaFBHint = 0,
                                          thetaLRHint = 0,
                                          thetaSwHint = 0,
                                          useHint = True,
                                          swShift = swShift)

        for x,y in zip(result, (1.499913, -1.998063, -0.943548, -1.009098)):
            self.assertAlmostEqual(x, y, places=5)
        self.assertTrue(IsMatrixClose(ComposeRotation('XYZ', result), rotWithSwing))

        # Hints variation 2
        result = \
            Gf.Rotation.DecomposeRotation(rot,
                                          twAxis = Gf.Vec3d(0, 0, 1),
                                          fbAxis = Gf.Vec3d(1, 0, 0),
                                          lrAxis = Gf.Vec3d(0, 1, 0),
                                          handedness = 1,
                                          thetaTwHint = -2.0,
                                          thetaFBHint = -2.0,
                                          thetaLRHint = 2.0,
                                          thetaSwHint = -1.0,
                                          useHint = True,
                                          swShift = swShift)

        for x,y in zip(result, (-1.641680, -1.143530, 2.198045, -1.009098)):
            self.assertAlmostEqual(x, y, places=5)
        self.assertTrue(IsMatrixClose(ComposeRotation('XYZ', result), rotWithSwing))

        # Hints variation 3
        result = \
            Gf.Rotation.DecomposeRotation(rot,
                                          twAxis = Gf.Vec3d(0, 0, 1),
                                          fbAxis = Gf.Vec3d(1, 0, 0),
                                          lrAxis = Gf.Vec3d(0, 1, 0),
                                          handedness = 1,
                                          thetaTwHint = -2.0,
                                          thetaFBHint = 2.0,
                                          thetaLRHint = 1.0,
                                          thetaSwHint = 3.0,
                                          useHint = True,
                                          swShift = swShift)

        for x,y in zip(result, (-1.641680, 1.998063, 0.943548, 2.132495)):
            self.assertAlmostEqual(x, y, places=5)
        self.assertTrue(IsMatrixClose(ComposeRotation('XYZ', result), rotWithSwing))

        # Hints variation 4
        result = \
            Gf.Rotation.DecomposeRotation(rot,
                                          twAxis = Gf.Vec3d(0, 0, 1),
                                          fbAxis = Gf.Vec3d(1, 0, 0),
                                          lrAxis = Gf.Vec3d(0, 1, 0),
                                          handedness = 1,
                                          thetaTwHint = 2.0,
                                          thetaFBHint = 1.0,
                                          thetaLRHint = -2.0,
                                          thetaSwHint = 3.0,
                                          useHint = True,
                                          swShift = swShift)

        for x,y in zip(result, (1.499913, 1.143530, -2.198045, 2.132495)):
            self.assertAlmostEqual(x, y, places=5)
        self.assertTrue(IsMatrixClose(ComposeRotation('XYZ', result), rotWithSwing))

        # Hints variation 3
        result = \
            Gf.Rotation.DecomposeRotation(rot,
                                          twAxis = Gf.Vec3d(0, 0, 1),
                                          fbAxis = Gf.Vec3d(1, 0, 0),
                                          lrAxis = Gf.Vec3d(0, 1, 0),
                                          handedness = 1,
                                          thetaTwHint = -2.0,
                                          thetaFBHint = 2.0,
                                          thetaLRHint = 1.0,
                                          thetaSwHint = 3.0,
                                          useHint = True,
                                          swShift = swShift)

        for x,y in zip(result, (-1.641680, 1.998063, 0.943548, 2.132495)):
            self.assertAlmostEqual(x, y, places=5)
        self.assertTrue(IsMatrixClose(ComposeRotation('XYZ', result), rotWithSwing))

        # Use hints shifted by multiples of 2pi
        result = \
            Gf.Rotation.DecomposeRotation(rot,
                                          twAxis = Gf.Vec3d(0, 0, 1),
                                          fbAxis = Gf.Vec3d(1, 0, 0),
                                          lrAxis = Gf.Vec3d(0, 1, 0),
                                          handedness = 1,
                                          thetaTwHint = 14.0,
                                          thetaFBHint = 4.0,
                                          thetaLRHint = -20.0,
                                          thetaSwHint = -16,
                                          useHint = True,
                                          swShift = swShift)

        for x,y in zip(result, (14.066283, 4.285122, -19.793104, -13.575468)):
            self.assertAlmostEqual(x, y, places=5)
        self.assertTrue(IsMatrixClose(ComposeRotation('XYZ', result), rotWithSwing))

    def test_DecomposeRotation3(self):
        """Test DecomposeRotation3, should be identical to zero Sw except it returns
        a 3-tuple.  Minimally tested with and without hints as the functionality
        is more thoroughly tested in the zero Sw case above."""

        rot = Gf.Matrix4d(
            -0.77657773110174733, 0.41334436580878597, -0.47547183177449759, 0.0,
            0.53325734733674601, 0.029348545343396093, -0.8454438268729656, 0.0,
            -0.33550503583141722, -0.91010169659239692, -0.24321034680169412, 0.0,
            0.0, 0.0, 0.0, 1.0)

        # No hints
        thetaTw, thetaFB, thetaLR = \
            Gf.Rotation.DecomposeRotation3(rot,
                                           twAxis = Gf.Vec3d(0, 0, 1),
                                           fbAxis = Gf.Vec3d(1, 0, 0),
                                           lrAxis = Gf.Vec3d(0, 1, 0),
                                           handedness = 1)

        self.assertAlmostEqual(thetaTw, -1.641680, places=5)
        self.assertAlmostEqual(thetaFB, 1.998063, places=5)
        self.assertAlmostEqual(thetaLR, 0.943548, places=5)
        self.assertTrue(IsMatrixClose(ComposeRotation('XYZ', (thetaTw,thetaFB,thetaLR,0.0)), rot))

        # Hints variation 1
        thetaTw, thetaFB, thetaLR = \
            Gf.Rotation.DecomposeRotation3(rot,
                                           twAxis = Gf.Vec3d(0, 0, 1),
                                           fbAxis = Gf.Vec3d(1, 0, 0),
                                           lrAxis = Gf.Vec3d(0, 1, 0),
                                           handedness = 1,
                                           thetaTwHint = -2.0,
                                           thetaFBHint = 2,
                                           thetaLRHint = 1.0,
                                           useHint = True)

        self.assertAlmostEqual(thetaTw, -1.641680, places=5)
        self.assertAlmostEqual(thetaFB, 1.998063, places=5)
        self.assertAlmostEqual(thetaLR, 0.943548, places=5)
        self.assertTrue(IsMatrixClose(ComposeRotation('XYZ', (thetaTw,thetaFB,thetaLR,0.0)), rot))


    def test_GimbalLockEdgeCases(self):
        """Test the gimbal lock edge where one or more angles is 90 degrees.  We
        catch them all by creating rotations using all permutations of the three
        rotation axes."""

        # Create all the permutations of axis switching rotation matrices.
        # XXX: Note that we only use right handed matrices as the tests for 
        # lefthanded matrices are inconsistent right now.
        mats = []
        for key in axes.keys():
            mat = Gf.Matrix3d(1.0)
            mat.SetRow(0, axes[key][0])
            mat.SetRow(1, axes[key][1])
            mat.SetRow(2, axes[key][2])
            if mat.IsLeftHanded():
                mat = mat * -1.0
            mats.append(mat)

            mat = Gf.Matrix3d(1.0)
            mat.SetRow(0, -axes[key][0])
            mat.SetRow(1, -axes[key][1])
            mat.SetRow(2, axes[key][2])
            if mat.IsLeftHanded():
                mat = mat * -1.0
            mats.append(mat)

            mat = Gf.Matrix3d(1.0)
            mat.SetRow(0, -axes[key][0])
            mat.SetRow(1, axes[key][1])
            mat.SetRow(2, -axes[key][2])
            if mat.IsLeftHanded():
                mat = mat * -1.0
            mats.append(mat)

            mat = Gf.Matrix3d(1.0)
            mat.SetRow(0, axes[key][0])
            mat.SetRow(1, -axes[key][1])
            mat.SetRow(2, -axes[key][2])
            if mat.IsLeftHanded():
                mat = mat * -1.0
            mats.append(mat)

        # For all our rotations, test decomposing the rotation with zeroing
        # each of the angles.
        for mat in mats:
            rot = Gf.Matrix4d(mat, Gf.Vec3d(0))
            self._TestDecomposeRotation(rot)
            self._TestDecomposeRotation(rot, thetaTwHint = None)
            self._TestDecomposeRotation(rot, thetaFBHint = None)
            self._TestDecomposeRotation(rot, thetaLRHint = None)
            self._TestDecomposeRotation(rot, thetaSwHint = None)

if __name__ == '__main__':
    unittest.main()
