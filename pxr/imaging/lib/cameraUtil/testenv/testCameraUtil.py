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

from pxr import CameraUtil, Gf
import unittest

class TestCameraUtil(unittest.TestCase):
    def _IsClose(self, a, b):
        eps = 1e-5
        if type(a) == Gf.Matrix4d and type(b) == Gf.Matrix4d:
            for i in [0,1,2,3]:
                self.assertTrue(Gf.IsClose(a.GetRow(i), b.GetRow(i), eps))
        elif type(a) == Gf.Range2d and type(b) == Gf.Range2d:
            self.assertTrue(Gf.IsClose(a.min, b.min, eps))
            self.assertTrue(Gf.IsClose(a.max, b.max, eps))
        else:
            self.assertTrue(Gf.IsClose(a,b,eps))

    def test_ScreenWindowParameters(self):
        cam = Gf.Camera()
        cam.projection = Gf.Camera.Perspective
        cam.horizontalAperture = 184.5
        cam.horizontalApertureOffset = 15.45
        cam.verticalAperture = 20.6
        cam.focalLength = 10.8
        cam.transform = Gf.Matrix4d(
             0.890425533492,  0.433328071165, -0.13917310100, 0.0,
            -0.373912364534,  0.870830610429,  0.31912942765, 0.0,
             0.259483935801, -0.232122447617,  0.93743653457, 0.0,
             6.533573569142,  9.880622442086,  1.89848943302, 1.0)

        self._IsClose(CameraUtil.ScreenWindowParameters(cam).screenWindow,
                      (-0.8325203582, 1.167479724, -0.1116531185, 0.1116531185))
        self._IsClose(CameraUtil.ScreenWindowParameters(cam).fieldOfView,
                      166.645202637)
        self._IsClose(CameraUtil.ScreenWindowParameters(cam).zFacingViewMatrix,
                    Gf.Matrix4d(
                 0.8904255335028, -0.3739123645233, -0.259483935838, 0,
                 0.4333280711640,  0.8708306104262,  0.232122447596, 0,
                -0.1391731009620,  0.3191294276581, -0.937436534593, 0,
                -9.8349931341753, -6.7672283767831,  1.181556474823, 1))
                

        cam.projection = Gf.Camera.Orthographic

        self._IsClose(CameraUtil.ScreenWindowParameters(cam).screenWindow,
                      (-7.6800003051, 10.770000457, -1.0300000190, 1.0300000190))

    def test_ConformedWindowGfVec2d(self):
        self._IsClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(1.0, 2.0),
                                       targetAspect = 3.0,
                                       policy = CameraUtil.Fit),
            Gf.Vec2d(6.0, 2.0))

        self._IsClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(9.0, 2.0),
                                       targetAspect = 3.0,
                                       policy = CameraUtil.Fit),
            Gf.Vec2d(9.0, 3.0))

        self._IsClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(3.3, 4.0),
                                       targetAspect = 1.5,
                                       policy = CameraUtil.Crop),
            Gf.Vec2d(3.3, 2.2))

        self._IsClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(10.0, 2.0),
                                       targetAspect = 4,
                                       policy = CameraUtil.Crop),
            Gf.Vec2d(8.0, 2.0))

        self._IsClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(0.1, 2.0),
                                       policy = CameraUtil.Crop,
                                       targetAspect = 0.1),
            Gf.Vec2d(0.1, 1.0))
        
        self._IsClose(
            CameraUtil.ConformedWindow(Gf.Vec2d(2.0, 1.9),
                                       CameraUtil.MatchVertically,
                                       2.0),
            Gf.Vec2d(3.8, 1.9))

        self._IsClose(
            CameraUtil.ConformedWindow(Gf.Vec2d(2.1, 1.9),
                                       CameraUtil.MatchHorizontally,
                                       1.0),
            Gf.Vec2d(2.1, 2.1))

    def test_ConformedWindowGfRange2d(self):
        self._IsClose(
            CameraUtil.ConformedWindow(
                window = Gf.Range2d(Gf.Vec2d(-8, -6), Gf.Vec2d(-4, -2)),
                targetAspect = 3.0,
                policy = CameraUtil.Fit),
            Gf.Range2d(Gf.Vec2d(-12,-6), Gf.Vec2d(0,-2)))
        
        self._IsClose(
            CameraUtil.ConformedWindow(
                Gf.Range2d(Gf.Vec2d(-10, -11), Gf.Vec2d(-1, -1)),
                CameraUtil.MatchHorizontally,
                1.5),
            Gf.Range2d(Gf.Vec2d(-10, -9), Gf.Vec2d(-1, -3)))

        self._IsClose(
            CameraUtil.ConformedWindow(
                Gf.Range2d(Gf.Vec2d(-10, -11), Gf.Vec2d(-1, -1)),
                CameraUtil.MatchVertically,
                1.5),
            Gf.Range2d(Gf.Vec2d(-13, -11), Gf.Vec2d(2, -1)))
                
    def test_ConformedWindowGfVec4d(self):

        self._IsClose(
            CameraUtil.ConformedWindow(
                Gf.Vec4d(-10, -1, -11, -1),
                CameraUtil.MatchHorizontally,
                1.5),
            Gf.Vec4d(-10, -1, -9, -3))

    def test_ConformProjectionMatrix(self):
        for projection in Gf.Camera.Projection.allValues:
            for policy in CameraUtil.ConformWindowPolicy.allValues:
                for targetAspect in [0.5, 1.0, 2.0]:
                    for xMirror in [-1, 1]:
                        for yMirror in [-1, 1]:
                            mirrorMatrix = Gf.Matrix4d(
                                xMirror, 0, 0, 0,
                                0, yMirror, 0, 0,
                                0, 0, 1, 0,
                                0, 0, 0, 1)

                            cam = Gf.Camera(
                                projection               = projection,
                                horizontalAperture       = 100.0,
                                verticalAperture         = 75.0,
                                horizontalApertureOffset = 11.0,
                                verticalApertureOffset   = 12.0)

                            originalMatrix = cam.frustum.ComputeProjectionMatrix()

                            CameraUtil.ConformWindow(cam, policy, targetAspect)

                            self._IsClose(
                                cam.frustum.ComputeProjectionMatrix() * mirrorMatrix,
                                CameraUtil.ConformedWindow(
                                    originalMatrix * mirrorMatrix, policy, targetAspect))

            
    def test_ConformWindow(self):
        cam = Gf.Camera()
        cam.horizontalAperture       = 100.0
        cam.verticalAperture         = 75.0
        cam.horizontalApertureOffset = 11.0
        cam.verticalApertureOffset   = 12.0
        
        CameraUtil.ConformWindow(
            camera = cam, policy = CameraUtil.Fit, targetAspect = 2.0)

        self._IsClose(cam.horizontalAperture,        150.0)
        self._IsClose(cam.verticalAperture,          75.0)
        self._IsClose(cam.horizontalApertureOffset,  11.0)
        self._IsClose(cam.verticalApertureOffset,    12.0)

        CameraUtil.ConformWindow(cam, CameraUtil.Fit, 1.5)

        self._IsClose(cam.horizontalAperture,       150.0)
        self._IsClose(cam.verticalAperture,         100.0)
        self._IsClose(cam.horizontalApertureOffset,  11.0)
        self._IsClose(cam.verticalApertureOffset,    12.0)

    def test_ConformFrustum(self):
        frustum = Gf.Frustum()
        frustum.window = Gf.Range2d(Gf.Vec2d(-1.2, -1.0), Gf.Vec2d(1.0, 1.5))

        CameraUtil.ConformWindow(frustum, CameraUtil.Crop, 1.3333)

        self._IsClose(frustum.window.min, Gf.Vec2d(-1.2, -0.575020625515638))
        self._IsClose(frustum.window.max, Gf.Vec2d(1.0, 1.075020625515638))

if __name__ == '__main__':
    unittest.main()
