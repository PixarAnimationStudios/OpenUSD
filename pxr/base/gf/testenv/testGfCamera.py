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
from pxr import Gf

import math
import unittest

class TestGfCamera(unittest.TestCase):

    def AssertEqualCams(self, cam1, cam2):
        # Check fields
        self.assertEqual(cam1.transform, cam2.transform)
        self.assertEqual(cam1.projection, cam2.projection)
        self.assertEqual(cam1.horizontalAperture, cam2.horizontalAperture)
        self.assertEqual(cam1.verticalAperture, cam2.verticalAperture)
        self.assertEqual(cam1.horizontalApertureOffset, cam2.horizontalApertureOffset)
        self.assertEqual(cam1.verticalApertureOffset, cam2.verticalApertureOffset)
        self.assertEqual(cam1.focalLength, cam2.focalLength)
        self.assertEqual(cam1.clippingRange, cam2.clippingRange)
        self.assertEqual(cam1.clippingPlanes, cam2.clippingPlanes)
        self.assertEqual(cam1.fStop, cam2.fStop)
        self.assertEqual(cam1.focusDistance, cam2.focusDistance)

        self.assertTrue(cam1 == cam2)

        self.assertFalse(cam1 != cam2)

        # Check computation of frustum
        self.assertEqual(cam1.frustum.ComputeCorners(),
                    cam2.frustum.ComputeCorners())

    def AssertCamSelfEvaluating(self, cam):

        self.AssertEqualCams(cam, eval(repr(cam)))


    def test_CameraEqualOperator(self):

        cam1 = Gf.Camera()
        cam2 = Gf.Camera()

        self.assertTrue(cam1 == cam2)

        cam1.transform = Gf.Matrix4d(1.0).SetTranslate(Gf.Vec3d(10,20,30))
        self.assertFalse(cam1 == cam2)
        self.assertTrue(cam1 != cam2)
        
        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)

        cam1.horizontalAperture = 100
        self.assertFalse(cam1 == cam2)

        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)

        cam1.verticalAperture = 200
        self.assertFalse(cam1 == cam2)

        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)

        cam1.horizontalApertureOffset = 10
        self.assertFalse(cam1 == cam2)

        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)

        cam1.verticalApertureOffset = 20
        self.assertFalse(cam1 == cam2)

        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)

        cam1.focalLength = 45
        self.assertFalse(cam1 == cam2)

        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)

        cam1.clippingRange = Gf.Range1f(3,30)
        self.assertFalse(cam1 == cam2)

        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)

        cam1.clippingPlanes = [
            Gf.Vec4f(0.32929277420043, 0.5488212704658, 0.768349826335, 10.0),
            Gf.Vec4f(0.68100523948669, 0.4256282746791, 0.595879554748, 20.0)
            ]

        self.assertFalse(cam1 == cam2)

        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)

        cam1.fStop = 11
        self.assertFalse(cam1 == cam2)

        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)
        
        cam1.focusDistance = 102
        self.assertFalse(cam1 == cam2)

        cam2 = Gf.Camera(cam1)
        self.assertTrue(cam1 == cam2)
        
        self.AssertCamSelfEvaluating(cam1)

    def TestCameraNonOrthonormal(self):
        
        cam = Gf.Camera()

        transform = (
            Gf.Matrix4d().SetRotate(
                Gf.Rotation(Gf.Vec3d(1,2,3), 20.3)) *
            Gf.Matrix4d().SetTranslate(
                Gf.Vec3d(100,123,153)) *
            Gf.Matrix4d().SetScale(
                Gf.Vec3d(2.0,1.0,1.3)))

        cam.transform = transform
        self.assertAlmostEqual(cam.transform, transform)
        
        cam.horizontalAperture = 34 / 2
        self.assertAlmostEqual(cam.horizontalAperture, 34 / 2)
        
        cam.verticalAperture = 20
        self.assertAlmostEqual(cam.verticalAperture, 20)

        cam.horizontalAperture *= 2
        
        cam.focalLength = 50
        self.assertAlmostEqual(cam.focalLength, 50)

        self.assertAlmostEqual(cam.GetFieldOfView(Gf.Camera.FOVHorizontal),
                    37.556064606)

        self.assertAlmostEqual(cam.horizontalFieldOfView, 37.556064606)

        self.assertAlmostEqual(cam.GetFieldOfView(Gf.Camera.FOVVertical),
                    22.619864948)
        self.assertAlmostEqual(cam.verticalFieldOfView, 22.619864948)

        cam.projection = Gf.Camera.Orthographic

        cam.SetPerspectiveFromAspectRatioAndFieldOfView(
            aspectRatio = 1.7,
            fieldOfView = 65.0,
            direction = Gf.Camera.FOVHorizontal,
            horizontalAperture = 34.0)

        self.assertEqual(cam.projection, Gf.Camera.Perspective)
            
        # Field of view is achieved by setting new focal
        # length
        self.assertAlmostEqual(cam.focalLength, 26.6846561432)
        self.assertAlmostEqual(cam.horizontalAperture, 34)

        cam.clippingRange = Gf.Range1f(10, 100)
        self.assertAlmostEqual(cam.clippingRange, Gf.Range1f(10, 100))

        self.assertAlmostEqual(cam.aspectRatio, 34.0/20.0)
        
        cam.SetPerspectiveFromAspectRatioAndFieldOfView(
            aspectRatio = 2.4,
            fieldOfView = 29.7321955,
            direction = Gf.Camera.FOVVertical,
            horizontalAperture = 34.0)

        self.assertAlmostEqual(cam.aspectRatio, 2.4)

        self.assertAlmostEqual(cam.verticalAperture, 34.0/2.4)

        self.assertAlmostEqual(cam.focalLength, 26.6846561432)

        # We have setup focal length, horizontal and vertical
        # aperture. We are now in a position to test the frustum

        # We test this with perspective first.

        cam.projection = Gf.Camera.Perspective
        self.assertEqual(cam.projection, Gf.Camera.Perspective)

        self.assertAlmostEqual(
            cam.frustum.ComputeCorners(),
            (Gf.Vec3d(192.846230529544, 118.9700660570964, 189.943705234630),
             Gf.Vec3d(204.624699588231, 123.2629367321672, 187.667234572988),
             Gf.Vec3d(191.159053763979, 123.9580649710064, 190.620399628732),
             Gf.Vec3d(202.937522822666, 128.2509356460773, 188.343928967090),
             Gf.Vec3d(128.462305295447, 82.70066057096416, 109.337052346305),
             Gf.Vec3d(246.246995882312, 125.6293673216727, 86.5723457298875),
             Gf.Vec3d(111.590537639799, 132.5806497100646, 116.103996287320),
             Gf.Vec3d(229.375228226663, 175.5093564607731, 93.3392896709023)))

        cam.horizontalAperture = 340
        self.assertAlmostEqual(cam.horizontalAperture, 340)

        cam.verticalAperture = 340 / 2.4
        self.assertAlmostEqual(cam.aspectRatio, 2.4)
        
        cam.projection = Gf.Camera.Orthographic
        self.assertEqual(cam.projection, Gf.Camera.Orthographic)
        
        self.assertAlmostEqual(
            cam.frustum.ComputeCorners(),
            (Gf.Vec3d(184.427743371372, 111.227660346205, 191.278291105439),
             Gf.Vec3d(215.858183043795, 122.683038129349, 185.203607422842),
             Gf.Vec3d(179.925570308415, 124.537963573823, 193.084026778878),
             Gf.Vec3d(211.356009980839, 135.993341356968, 187.009343096281),
             Gf.Vec3d(165.454633456322, 116.722168010487, 103.472645013183),
             Gf.Vec3d(196.885073128745, 128.177545793631, 97.3979613305862),
             Gf.Vec3d(160.952460393365, 130.032471238105, 105.278380686621),
             Gf.Vec3d(192.382900065789, 141.487849021250, 99.2036970040248)))

        cam = Gf.Camera()
        cam.SetOrthographicFromAspectRatioAndSize(
            2.4, 34.0,
            direction = Gf.Camera.FOVHorizontal)
        cam.transform = transform
        cam.clippingRange = Gf.Range1f(10, 100)
        
        self.assertAlmostEqual(
            cam.frustum.ComputeCorners(),
            (Gf.Vec3d(184.427743371372, 111.227660346205, 191.278291105439),
             Gf.Vec3d(215.858183043795, 122.683038129349, 185.203607422842),
             Gf.Vec3d(179.925570308415, 124.537963573823, 193.084026778878),
             Gf.Vec3d(211.356009980839, 135.993341356968, 187.009343096281),
             Gf.Vec3d(165.454633456322, 116.722168010487, 103.472645013183),
             Gf.Vec3d(196.885073128745, 128.177545793631, 97.3979613305862),
             Gf.Vec3d(160.952460393365, 130.032471238105, 105.278380686621),
             Gf.Vec3d(192.382900065789, 141.487849021250, 99.2036970040248)))

        cam = Gf.Camera()
        cam.SetOrthographicFromAspectRatioAndSize(
            2.4, 34.0 / 2.4,
            direction = Gf.Camera.FOVVertical)
        cam.transform = transform
        cam.clippingRange = Gf.Range1f(10, 100)
        
        self.assertAlmostEqual(
            cam.frustum.ComputeCorners(),
            (Gf.Vec3d(184.427743371372, 111.227660346205, 191.278291105439),
             Gf.Vec3d(215.858183043795, 122.683038129349, 185.203607422842),
             Gf.Vec3d(179.925570308415, 124.537963573823, 193.084026778878),
             Gf.Vec3d(211.356009980839, 135.993341356968, 187.009343096281),
             Gf.Vec3d(165.454633456322, 116.722168010487, 103.472645013183),
             Gf.Vec3d(196.885073128745, 128.177545793631, 97.3979613305862),
             Gf.Vec3d(160.952460393365, 130.032471238105, 105.278380686621),
             Gf.Vec3d(192.382900065789, 141.487849021250, 99.2036970040248)),
            epsilon = 1e-4)

        transform = (
            Gf.Matrix4d().SetRotate(
                Gf.Rotation(Gf.Vec3d(2,3,4), 14.2)) *
            Gf.Matrix4d().SetTranslate(
                Gf.Vec3d(99.0, 111.0, 134.0)))

        cam = Gf.Camera()
        cam.SetPerspectiveFromAspectRatioAndFieldOfView(
            aspectRatio = 2.4,
            fieldOfView = 29.7321955,
            direction = Gf.Camera.FOVVertical,
            horizontalAperture = 34.0)
        cam.clippingRange = Gf.Range1f(10, 100)

        clippingPlanes = [
            Gf.Vec4f(0.32929277420043, 0.5488212704658, 0.768349826335, 10.0),
            Gf.Vec4f(0.68100523948669, 0.4256282746791, 0.595879554748, 20.0)
            ]

        cam.clippingPlanes = clippingPlanes
        self.assertAlmostEqual(cam.clippingPlanes, clippingPlanes)

        self.AssertCamSelfEvaluating(cam)
        
    def TestCameraOrthonormal(self):
        
        cam = Gf.Camera()

        self.AssertCamSelfEvaluating(cam)

        transform = (
            Gf.Matrix4d().SetRotate(
                Gf.Rotation(Gf.Vec3d(1,-2,3), 30.3)) *
            Gf.Matrix4d().SetTranslate(
                Gf.Vec3d(-100,123,153)))

        cam.transform = transform
        
        self.AssertCamSelfEvaluating(cam)

        cam.horizontalAperture = 34 / 2
        self.assertAlmostEqual(cam.horizontalAperture, 34 / 2)
        
        cam.verticalAperture = 20
        self.assertAlmostEqual(cam.verticalAperture, 20)

        self.AssertCamSelfEvaluating(cam)

        cam.horizontalAperture *= 2

        self.AssertCamSelfEvaluating(cam)
        
        cam.focalLength = 50
        self.assertAlmostEqual(cam.focalLength, 50)

        self.AssertCamSelfEvaluating(cam)

        self.assertAlmostEqual(cam.GetFieldOfView(Gf.Camera.FOVHorizontal),
                    37.556064606)

        self.assertAlmostEqual(cam.horizontalFieldOfView, 37.556064606)

        self.assertAlmostEqual(cam.GetFieldOfView(Gf.Camera.FOVVertical),
                    22.619864948)
        self.assertAlmostEqual(cam.verticalFieldOfView, 22.619864948)

        cam.projection = Gf.Camera.Orthographic

        self.AssertCamSelfEvaluating(cam)

        cam.SetPerspectiveFromAspectRatioAndFieldOfView(
            aspectRatio = 1.7,
            fieldOfView = 65.0,
            direction = Gf.Camera.FOVHorizontal,
            horizontalAperture = 34.0)

        self.AssertCamSelfEvaluating(cam)

        otherCam = Gf.Camera(
            transform = Gf.Matrix4d(
                0.8731530112777166, 0.3850071166178898, 0.2989537406526877, 0,
                -0.4240369593016693, 0.9024253932905512, 0.07629591529425725, 0,
                -0.2404089766270184, -0.1933854433455958, 0.9512126966452756, 0,
                -100.0, 123.0, 153.0, 1.0),
            projection = Gf.Camera.Perspective,
            horizontalAperture = 34.0,
            verticalAperture = 20.0,
            focalLength = 26.684656143188477)

        self.AssertEqualCams(cam, otherCam)

        self.assertEqual(cam.projection, Gf.Camera.Perspective)
            
        # Field of view is achieved by setting new focal
        # length
        self.assertAlmostEqual(cam.focalLength, 26.6846561432)
        self.assertAlmostEqual(cam.horizontalAperture, 34)

        cam.clippingRange = Gf.Range1f(10, 100)
        self.assertAlmostEqual(cam.clippingRange, Gf.Range1f(10, 100))

        self.assertAlmostEqual(cam.aspectRatio, 34.0/20.0)
        
        cam.SetPerspectiveFromAspectRatioAndFieldOfView(
            aspectRatio = 2.4,
            fieldOfView = 29.7321955,
            direction = Gf.Camera.FOVVertical,
            horizontalAperture = 34.0)

        self.assertAlmostEqual(cam.aspectRatio, 2.4)

        self.assertAlmostEqual(cam.verticalAperture, 34.0/2.4)

        self.assertAlmostEqual(cam.focalLength, 26.6846561432)

        # We have setup focal length, horizontal and vertical
        # aperture. We are now in a position to test the frustum

        # We test this with perspective first.

        cam.projection = Gf.Camera.Perspective
        self.assertEqual(cam.projection, Gf.Camera.Perspective)

        self.assertAlmostEqual(
            cam.frustum.ComputeCorners(),
            (Gf.Vec3d(-102.032919327869, 120.085637354940, 141.38080336149),
             Gf.Vec3d(-90.9077235508873, 124.991168793969, 145.18989392187),
             Gf.Vec3d(-104.284096916572, 124.876540072942, 141.78585214522),
             Gf.Vec3d(-93.1589011395900, 129.782071511971, 145.59494270559),
             Gf.Vec3d(-120.329193278696, 93.8563735494037, 36.808033614959),
             Gf.Vec3d(-9.07723550887376, 142.911687939691, 74.898939218725),
             Gf.Vec3d(-142.840969165722, 141.765400729428, 40.858521452219),
             Gf.Vec3d(-31.5890113959001, 190.820715119715, 78.949427055984)))

        cam.horizontalAperture = 340
        self.assertAlmostEqual(cam.horizontalAperture, 340)

        cam.verticalAperture = 340 / 2.4
        self.assertAlmostEqual(cam.aspectRatio, 2.4)
        
        cam.projection = Gf.Camera.Orthographic
        self.assertEqual(cam.projection, Gf.Camera.Orthographic)
        
        self.assertAlmostEqual(
            cam.frustum.ComputeCorners(),
            (Gf.Vec3d(-109.435916189226, 111.996553352311, 137.8652300230474),
             Gf.Vec3d( -79.748713805783, 125.086795317320, 148.0296572052388),
             Gf.Vec3d(-115.443106661676, 124.780913549591, 138.9460888618555),
             Gf.Vec3d( -85.755904278233, 137.871155514600, 149.1105160440469),
             Gf.Vec3d( -87.799108292794, 129.401243253415,  52.2560873249726),
             Gf.Vec3d( -58.111905909352, 142.491485218423,  62.4205145071640),
             Gf.Vec3d( -93.806298765244, 142.185603450695,  53.3369461637807),
             Gf.Vec3d( -64.119096381802, 155.275845415703,  63.5013733459721)))

        cam = Gf.Camera()
        cam.SetOrthographicFromAspectRatioAndSize(
            2.4, 34.0,
            direction = Gf.Camera.FOVHorizontal)
        cam.transform = transform
        cam.clippingRange = Gf.Range1f(10, 100)
        
        self.assertAlmostEqual(
            cam.frustum.ComputeCorners(),
            (Gf.Vec3d(-109.43591651274, 111.9965540408077, 137.8652300812566),
             Gf.Vec3d( -79.74871412929, 125.0867960058159, 148.0296572634480),
             Gf.Vec3d(-115.44310633816, 124.7809128610959, 138.9460888036464),
             Gf.Vec3d( -85.75590395471, 137.8711548261041, 149.1105159858378),
             Gf.Vec3d( -87.79910861630, 129.4012439419113,  52.2560873831818),
             Gf.Vec3d( -58.11190623286, 142.4914859069195,  62.4205145653732),
             Gf.Vec3d( -93.80629844172, 142.1856027621995,  53.3369461055715),
             Gf.Vec3d( -64.11909605828, 155.2758447272077,  63.5013732877629)))

        cam = Gf.Camera()
        cam.SetOrthographicFromAspectRatioAndSize(
            2.4, 34.0 / 2.4,
            direction = Gf.Camera.FOVVertical)
        cam.transform = transform
        cam.clippingRange = Gf.Range1f(10, 100)
        
        self.assertAlmostEqual(
            cam.frustum.ComputeCorners(),
            (Gf.Vec3d(-109.435917521551, 111.996552764837, 137.8652295668803),
             Gf.Vec3d( -79.748712473457, 125.086795904794, 148.0296576614060),
             Gf.Vec3d(-115.443107994001, 124.780912962117, 138.9460884056883),
             Gf.Vec3d( -85.755902945907, 137.871156102074, 149.1105165002141),
             Gf.Vec3d( -87.799109625120, 129.401242665941,  52.2560868688054),
             Gf.Vec3d( -58.111904577026, 142.491485805897,  62.4205149633312),
             Gf.Vec3d( -93.806300097570, 142.185602863221,  53.3369457076135),
             Gf.Vec3d( -64.119095049476, 155.275846003177,  63.5013738021393)),
            epsilon = 1e-4)

        self.assertAlmostEqual(
            Gf.Camera(cam).frustum.ComputeCorners(),
            (Gf.Vec3d(-109.435917521551, 111.996552764837, 137.8652295668803),
             Gf.Vec3d( -79.748712473457, 125.086795904794, 148.0296576614060),
             Gf.Vec3d(-115.443107994001, 124.780912962117, 138.9460884056883),
             Gf.Vec3d( -85.755902945907, 137.871156102074, 149.1105165002141),
             Gf.Vec3d( -87.799109625120, 129.401242665941,  52.2560868688054),
             Gf.Vec3d( -58.111904577026, 142.491485805897,  62.4205149633312),
             Gf.Vec3d( -93.806300097570, 142.185602863221,  53.3369457076135),
             Gf.Vec3d( -64.119095049476, 155.275846003177,  63.5013738021393)),
            epsilon = 1e-4)

        transform = (
            Gf.Matrix4d().SetRotate(
                Gf.Rotation(Gf.Vec3d(2,3,4), 14.2)) *
            Gf.Matrix4d().SetTranslate(
                Gf.Vec3d(99.0, 111.0, 134.0)))

        cam = Gf.Camera()
        cam.SetPerspectiveFromAspectRatioAndFieldOfView(
            aspectRatio = 2.4,
            fieldOfView = 29.7321955,
            direction = Gf.Camera.FOVVertical,
            horizontalAperture = 34.0)
        cam.clippingRange = Gf.Range1f(10, 100)
        
        cam = Gf.Camera()
        cam.projection = Gf.Camera.Perspective
        cam.focalLength = 50
        cam.verticalApertureOffset = 1.2
        
        self.assertAlmostEqual(
            Gf.Camera(cam).frustum.ComputeCorners(),
            (Gf.Vec3d(-0.20954999923706, -0.128907999992370, -1.0),
             Gf.Vec3d(0.209549999237060, -0.128907999992370, -1.0),
             Gf.Vec3d(-0.20954999923706, 0.1769080018997192, -1.0),
             Gf.Vec3d(0.209549999237060, 0.1769080018997192, -1.0),
             Gf.Vec3d(-209549.999237060, -128907.9999923706, -1000000.0),
             Gf.Vec3d(209549.9992370605, -128907.9999923706, -1000000.0),
             Gf.Vec3d(-209549.999237060, 176908.00189971924, -1000000.0),
             Gf.Vec3d(209549.9992370605, 176908.00189971924, -1000000.0)),
            delta = 1e-4)

        cam.focusDistance = 200
        cam.fStop = 11

        self.assertAlmostEqual(cam.focusDistance, 200)
        self.assertAlmostEqual(cam.fStop, 11)

        self.AssertCamSelfEvaluating(cam)
        
    def TestConstants(self):
        
        self.assertAlmostEqual(Gf.Camera.DEFAULT_HORIZONTAL_APERTURE, 20.955)
        self.assertAlmostEqual(Gf.Camera.DEFAULT_VERTICAL_APERTURE, 15.2908)
        self.assertAlmostEqual(Gf.Camera.APERTURE_UNIT, 0.1)
        self.assertAlmostEqual(Gf.Camera.FOCAL_LENGTH_UNIT, 0.1)

if __name__ == '__main__':
    unittest.main()

