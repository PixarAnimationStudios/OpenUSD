#!/pxrpythonsubst

from pxr import CameraUtil, Gf

from Mentor.Runtime import (Runner,
                            Fixture,
                            AssertClose,
                            SetIsCloseSignatureCallback)

SetIsCloseSignatureCallback(Gf.Range2d,
                            lambda gfRange2d: (gfRange2d.min, gfRange2d.max))

class TestCameraUtil(Fixture):

    def TestScreenWindowParameters(self):
        
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

        AssertClose(CameraUtil.ScreenWindowParameters(cam).screenWindow,
                    (-0.8325203582, 1.167479724, -0.1116531185, 0.1116531185))
        AssertClose(CameraUtil.ScreenWindowParameters(cam).fieldOfView,
                    166.645202637)
        AssertClose(CameraUtil.ScreenWindowParameters(cam).zFacingViewMatrix,
                    Gf.Matrix4d(
                 0.8904255335028, -0.3739123645233, -0.259483935838, 0,
                 0.4333280711640,  0.8708306104262,  0.232122447596, 0,
                -0.1391731009620,  0.3191294276581, -0.937436534593, 0,
                -9.8349931341753, -6.7672283767831,  1.181556474823, 1))
                

        cam.projection = Gf.Camera.Orthographic

        AssertClose(CameraUtil.ScreenWindowParameters(cam).screenWindow,
                    (-7.6800003051, 10.770000457, -1.0300000190, 1.0300000190))

    def TestConformedWindowGfVec2d(self):
        
        AssertClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(1.0, 2.0),
                                       targetAspect = 3.0,
                                       policy = CameraUtil.Fit),
            Gf.Vec2d(6.0, 2.0))

        AssertClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(9.0, 2.0),
                                       targetAspect = 3.0,
                                       policy = CameraUtil.Fit),
            Gf.Vec2d(9.0, 3.0))

        AssertClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(3.3, 4.0),
                                       targetAspect = 1.5,
                                       policy = CameraUtil.Crop),
            Gf.Vec2d(3.3, 2.2))

        AssertClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(10.0, 2.0),
                                       targetAspect = 4,
                                       policy = CameraUtil.Crop),
            Gf.Vec2d(8.0, 2.0))

        AssertClose(
            CameraUtil.ConformedWindow(window = Gf.Vec2d(0.1, 2.0),
                                       policy = CameraUtil.Crop,
                                       targetAspect = 0.1),
            Gf.Vec2d(0.1, 1.0))
        
        AssertClose(
            CameraUtil.ConformedWindow(Gf.Vec2d(2.0, 1.9),
                                       CameraUtil.MatchVertically,
                                       2.0),
            Gf.Vec2d(3.8, 1.9))

        AssertClose(
            CameraUtil.ConformedWindow(Gf.Vec2d(2.1, 1.9),
                                       CameraUtil.MatchHorizontally,
                                       1.0),
            Gf.Vec2d(2.1, 2.1))

    def TestConformedWindowGfRange2d(self):

        AssertClose(
            CameraUtil.ConformedWindow(
                window = Gf.Range2d(Gf.Vec2d(-8, -6), Gf.Vec2d(-4, -2)),
                targetAspect = 3.0,
                policy = CameraUtil.Fit),
            Gf.Range2d(Gf.Vec2d(-12,-6), Gf.Vec2d(0,-2)))
        
        AssertClose(
            CameraUtil.ConformedWindow(
                Gf.Range2d(Gf.Vec2d(-10, -11), Gf.Vec2d(-1, -1)),
                CameraUtil.MatchHorizontally,
                1.5),
            Gf.Range2d(Gf.Vec2d(-10, -9), Gf.Vec2d(-1, -3)))

        AssertClose(
            CameraUtil.ConformedWindow(
                Gf.Range2d(Gf.Vec2d(-10, -11), Gf.Vec2d(-1, -1)),
                CameraUtil.MatchVertically,
                1.5),
            Gf.Range2d(Gf.Vec2d(-13, -11), Gf.Vec2d(2, -1)))
                
    def TestConformedWindowGfVec4d(self):

        AssertClose(
            CameraUtil.ConformedWindow(
                Gf.Vec4d(-10, -1, -11, -1),
                CameraUtil.MatchHorizontally,
                1.5),
            Gf.Vec4d(-10, -1, -9, -3))
            
    def TestConformWindow(self):

        cam = Gf.Camera()
        cam.horizontalAperture       = 100.0
        cam.verticalAperture         = 75.0
        cam.horizontalApertureOffset = 11.0
        cam.verticalApertureOffset   = 12.0
        
        CameraUtil.ConformWindow(
            camera = cam, policy = CameraUtil.Fit, targetAspect = 2.0)

        AssertClose(cam.horizontalAperture,       150.0)
        AssertClose(cam.verticalAperture,          75.0)
        AssertClose(cam.horizontalApertureOffset,  11.0)
        AssertClose(cam.verticalApertureOffset,    12.0)

        CameraUtil.ConformWindow(
            cam, CameraUtil.Fit, 1.5)

        AssertClose(cam.horizontalAperture,       150.0)
        AssertClose(cam.verticalAperture,         100.0)
        AssertClose(cam.horizontalApertureOffset,  11.0)
        AssertClose(cam.verticalApertureOffset,    12.0)


if __name__ == '__main__':
    Runner().Main()
