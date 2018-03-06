#
# Copyright 2018 Pixar
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

from math import tan, atan, floor, ceil, radians as rad
from pxr import Gf, Tf

from qt import QtCore
from common import DEBUG_CLIPPING

# FreeCamera inherits from QObject only so that it can send signals...
# which is really a pretty nice, easy to use notification system.
class FreeCamera(QtCore.QObject):

    # Allows FreeCamera owner to act when the camera's relationship to
    # its viewed content changes.  For instance, to compute the value
    # to supply for setClosestVisibleDistFromPoint()
    signalFrustumChanged = QtCore.Signal()

    defaultNear = 1
    defaultFar = 1000000
    # Experimentally on Nvidia M6000, if Far/Near is greater than this,
    # then geometry in the back half of the volume will disappear
    maxSafeZResolution = 1e6
    # Experimentally on Nvidia M6000, if Far/Near is greater than this,
    # then we will often see Z-fighting artifacts even for geometry that
    # is close to camera
    maxGoodZResolution = 5e4

    def __init__(self, isZUp):
        """FreeCamera can be either a Z up or Y up camera, based on 'zUp'"""
        super(FreeCamera, self).__init__()

        self._camera = Gf.Camera()
        self._camera.SetPerspectiveFromAspectRatioAndFieldOfView(
            1.0, 60, Gf.Camera.FOVVertical)
        self._camera.clippingRange = Gf.Range1f(FreeCamera.defaultNear,
                                                FreeCamera.defaultFar)
        self._overrideNear = None
        self._overrideFar = None
        self._isZUp = isZUp

        self._cameraTransformDirty = True
        self._rotTheta = 0
        self._rotPhi = 0
        self._rotPsi = 0
        self._center = Gf.Vec3d(0,0,0)
        self._dist = 100
        self._camera.focusDistance = self._dist
        self._closestVisibleDist = None
        self._lastFramedDist = None
        self._lastFramedClosestDist = None
        self._selSize = 10

        if isZUp:
            # This is also Gf.Camera.Y_UP_TO_Z_UP_MATRIX
            self._YZUpMatrix = Gf.Matrix4d().SetRotate(
                Gf.Rotation(Gf.Vec3d.XAxis(), -90))
            self._YZUpInvMatrix = self._YZUpMatrix.GetInverse()
        else:
            self._YZUpMatrix = Gf.Matrix4d(1.0)
            self._YZUpInvMatrix = Gf.Matrix4d(1.0)

    # Why a clone() method vs copy.deepcopy()ing the FreeCamera ?
    # 1) Several of the Gf classes are not python-picklable (requirement of
    #    deepcopy), nor is GfCamera.  Adding that infrastructure for this
    #    single client seems weighty.
    # 2) We could make FreeCamera itself be picklable... that solution would
    #    require twice as much code as clone().  If we wind up extracting
    #    FreeCamera to be a more general building block, it may be worth it,
    #    and clone() would transition to __getstate__().
    def clone(self):
        clone = FreeCamera(self._isZUp)
        clone._camera = Gf.Camera(self._camera)
        # skipping stereo attrs for now

        clone._rotTheta = self._rotTheta
        clone._rotPhi = self._rotPhi
        clone._rotPsi = self._rotPsi
        clone._center = Gf.Vec3d(self._center)
        clone._dist = self._dist
        clone._closestVisibleDist = self._closestVisibleDist
        clone._lastFramedClosestDist = self._lastFramedClosestDist
        clone._lastFramedDist = self._lastFramedDist
        clone._selSize = self._selSize
        clone._overrideNear = self._overrideNear
        clone._overrideFar = self._overrideFar
        clone._YZUpMatrix = Gf.Matrix4d(self._YZUpMatrix)
        clone._YZUpInvMatrix = Gf.Matrix4d(self._YZUpInvMatrix)

        return clone


    def _updateCameraTransform(self):
        """
        Updates the camera's transform matrix, that is, the matrix that brings
        the camera to the origin, with the camera view pointing down:
           +Y if this is a Zup camera, or
           -Z if this is a Yup camera .
        """
        if not self._cameraTransformDirty:
            return

        def RotMatrix(vec, angle):
            return Gf.Matrix4d(1.0).SetRotate(Gf.Rotation(vec, angle))

        # self._YZUpInvMatrix influences the behavior about how the
        # FreeCamera will tumble. It is the identity or a rotation about the
        # x-Axis.
        self._camera.transform = (
            Gf.Matrix4d().SetTranslate(Gf.Vec3d.ZAxis() * self.dist) *
            RotMatrix(Gf.Vec3d.ZAxis(), -self._rotPsi) *
            RotMatrix(Gf.Vec3d.XAxis(), -self._rotPhi) *
            RotMatrix(Gf.Vec3d.YAxis(), -self._rotTheta) *
            self._YZUpInvMatrix *
            Gf.Matrix4d().SetTranslate(self.center))
        self._camera.focusDistance = self.dist

        self._cameraTransformDirty = False

    def _rangeOfBoxAlongRay(self, camRay, bbox, debugClipping=False):
        maxDist = -float('inf')
        minDist = float('inf')
        boxRange = bbox.GetRange()
        boxXform = bbox.GetMatrix()
        for i in range (8):
            # for each corner of the bounding box, transform to world
            # space and project
            point = boxXform.Transform(boxRange.GetCorner(i))
            pointDist = camRay.FindClosestPoint(point)[1]

            # find the projection of that point of the camera ray
            # and find the farthest and closest point.
            if pointDist > maxDist:
                maxDist = pointDist
            if pointDist < minDist:
                minDist = pointDist

        if debugClipping:
            print "Projected bounds near/far: %f, %f" % (minDist, maxDist)

        # if part of the bbox is behind the ray origin (i.e. camera),
        # we clamp minDist to be positive.  Otherwise, reduce minDist by a bit
        # so that geometry at exactly the edge of the bounds won't be clipped -
        # do the same for maxDist, also!
        if minDist < FreeCamera.defaultNear:
            minDist = FreeCamera.defaultNear
        else:
            minDist *= 0.99
        maxDist *= 1.01

        if debugClipping:
            print "Contracted bounds near/far: %f, %f" % (minDist, maxDist)

        return minDist, maxDist

    def setClippingPlanes(self, stageBBox):
        '''Computes and sets automatic clipping plane distances using the
           camera's position and orientation, the bouding box
           surrounding the stage, and the distance to the closest rendered
           object in the central view of the camera (closestVisibleDist).

           If either of the "override" clipping attributes are not None,
           we use those instead'''

        debugClipping = Tf.Debug.IsDebugSymbolNameEnabled(DEBUG_CLIPPING)

        # If the scene bounding box is empty, or we are fully on manual
        # override, then just initialize to defaults.
        if stageBBox.GetRange().IsEmpty() or \
               (self._overrideNear and self._overrideFar) :
            computedNear, computedFar = FreeCamera.defaultNear, FreeCamera.defaultFar
        else:
            # The problem: We want to include in the camera frustum all the
            # geometry the viewer should be able to see, i.e. everything within
            # the inifinite frustum starting at distance epsilon from the
            # camera itself.  However, the further the imageable geometry is
            # from the near-clipping plane, the less depth precision we will
            # have to resolve nearly colinear/incident polygons (which we get
            # especially with any doubleSided geometry).  We can run into such
            # situations astonishingly easily with large sets when we are
            # focussing in on just a part of a set that spans 10^5 units or
            # more.
            #
            # Our solution: Begin by projecting the endpoints of the imageable
            # world's bounds onto the ray piercing the center of the camera
            # frustum, and take the near/far clipping distances from its
            # extent, clamping at a positive value for near.  To address the
            # z-buffer precision issue, we rely on someone having told us how
            # close the closest imageable geometry actually is to the camera,
            # by having called setClosestVisibleDistFromPoint(). This gives us
            # the most liberal near distance we can use and not clip the
            # geometry we are looking at.  We actually choose some fraction of
            # that distance instead, because we do not expect the someone to
            # recompute the closest point with every camera manipulation, as
            # it can be expensive (we do emit signalFrustumChanged to notify
            # them, however).  We only use this if the current range of the
            # bbox-based frustum will have precision issues.
            frustum = self._camera.frustum
            camPos = frustum.position

            camRay = Gf.Ray(camPos, frustum.ComputeViewDirection())
            computedNear, computedFar = self._rangeOfBoxAlongRay(camRay,
                                                                 stageBBox,
                                                                 debugClipping)

            precisionNear = computedFar / FreeCamera.maxGoodZResolution

            if debugClipping:
                print "Proposed near for precision: {}, closestDist: {}"\
                    .format(precisionNear, self._closestVisibleDist)
            if self._closestVisibleDist:
                # Because of our concern about orbit/truck causing
                # clipping, make sure we don't go closer than half the
                # distance to the closest visible point
                halfClose = self._closestVisibleDist / 2.0

                if self._closestVisibleDist < self._lastFramedClosestDist:
                    # This can happen if we have zoomed in closer since
                    # the last time setClosestVisibleDistFromPoint() was called.
                    # Clamp to precisionNear, which gives a balance between
                    # clipping as we zoom in, vs bad z-fighting as we zoom in.
                    # See adjustDist() for comment about better solution.
                    halfClose = max(precisionNear, halfClose, computedNear)
                    if debugClipping:
                        print "ADJUSTING: Accounting for zoom-in"

                if halfClose < computedNear:
                    # If there's stuff very very close to the camera, it
                    # may have been clipped by computedNear.  Get it back!
                    computedNear = halfClose
                    if debugClipping:
                        print "ADJUSTING: closestDist was closer than bboxNear"
                elif precisionNear > computedNear:
                    computedNear = min((precisionNear + halfClose) / 2.0,
                                       halfClose)
                    if debugClipping:
                        print "ADJUSTING: gaining precision by pushing out"

        near = self._overrideNear or computedNear
        far  = self._overrideFar  or computedFar
        # Make sure far is greater than near
        far = max(near+1, far)

        if debugClipping:
            print "***Final Near/Far: {}, {}".format(near, far)

        self._camera.clippingRange = Gf.Range1f(near, far)

    def computeGfCamera(self, stageBBox):
        """Makes sure the FreeCamera's computed parameters are up-to-date, and
        returns the GfCamera object."""
        self._updateCameraTransform()
        self.setClippingPlanes(stageBBox)
        return self._camera

    def frameSelection(self, selBBox, frameFit):
        # needs to be recomputed
        self._closestVisibleDist = None

        self.center = selBBox.ComputeCentroid()
        selRange = selBBox.ComputeAlignedRange()
        self._selSize = max(*selRange.GetSize())
        if self.orthographic:
            self.fov = self._selSize * Gf.Camera.APERTURE_UNIT * frameFit
            self.dist = self._selSize
        else:
            halfFov = self.fov*0.5 or 0.5 # don't divide by zero
            self.dist = ((self._selSize * frameFit * 0.5)
                         / atan(rad(halfFov)))

    def setClosestVisibleDistFromPoint(self, point):
        frustum = self._camera.frustum
        camPos = frustum.position
        camRay = Gf.Ray(camPos, frustum.ComputeViewDirection())
        self._closestVisibleDist = camRay.FindClosestPoint(point)[1]
        self._lastFramedDist = self.dist
        self._lastFramedClosestDist = self._closestVisibleDist

        if Tf.Debug.IsDebugSymbolNameEnabled(DEBUG_CLIPPING):
            print "Resetting closest distance to {}; CameraPos: {}, closestPoint: {}".format(self._closestVisibleDist, camPos, point)

    def adjustDist(self, scaleFactor):
        # When dist gets very small, you can get stuck and not be able to
        # zoom back out, if you just keep multiplying.  Switch to addition
        # in that case, choosing an incr that works for the scale of the
        # framed geometry.
        if scaleFactor > 1 and self.dist < 2:
            selBasedIncr = self._selSize / 25.0
            scaleFactor -= 1.0
            self.dist += min(selBasedIncr, scaleFactor)
        else:
            self.dist *= scaleFactor

        # Make use of our knowledge that we are changing distance to camera
        # to also adjust _closestVisibleDist to keep it useful.  Make sure
        # not to recede farther than the last *computed* closeDist, since that
        # will generally cause unwanted clipping of close objects.
        # XXX:  This heuristic does a good job of preventing undesirable
        # clipping as we zoom in and out, but sacrifices the z-buffer
        # precision we worked hard to get.  If Hd/UsdImaging could cheaply
        # provide us with the closest-point from the last-rendered image,
        # we could use it safely here to update _closestVisibleDist much
        # more accurately than this calculation.
        if self._closestVisibleDist:
            if self.dist > self._lastFramedDist:
                self._closestVisibleDist = self._lastFramedClosestDist
            else:
                self._closestVisibleDist = \
                    self._lastFramedClosestDist - \
                    self._lastFramedDist + \
                    self.dist

    def Truck(self, offX, offY, height):
        self._updateCameraTransform()
        frustum = self._camera.frustum
        cam_up = frustum.ComputeUpVector()
        cam_right = Gf.Cross(frustum.ComputeViewDirection(), cam_up)

        # Figure out distance in world space of a point 'dist' into the
        # screen from center to top of frame
        offRatio = frustum.window.GetSize()[1] * self._dist / height

        self.center += - offRatio * offX * cam_right
        self.center +=   offRatio * offY * cam_up

        self._cameraTransformDirty = True
        self.signalFrustumChanged.emit()

    @staticmethod
    def FromGfCamera(cam, isZUp):
        # Get the data from the camera and its frustum
        cam_transform = cam.transform
        dist = cam.focusDistance
        frustum = cam.frustum
        cam_pos = frustum.position
        cam_axis = frustum.ComputeViewDirection()

        # Create a new FreeCamera setting the camera to be the given camera
        self = FreeCamera(isZUp)
        self._camera = cam

        # Compute translational parts
        self._dist = dist
        self._selSize = dist / 10.0
        self._center = cam_pos + dist * cam_axis

        # self._YZUpMatrix influences the behavior about how the
        # FreeCamera will tumble. It is the identity or a rotation about the
        # x-Axis.

        # Compute rotational part
        transform = cam_transform * self._YZUpMatrix
        transform.Orthonormalize()
        rotation = transform.ExtractRotation()

        # Decompose and set angles
        self._rotTheta, self._rotPhi, self._rotPsi =-rotation.Decompose(
            Gf.Vec3d.YAxis(), Gf.Vec3d.XAxis(), Gf.Vec3d.ZAxis())

        self._cameraTransformDirty = True

        return self

    @property
    def rotTheta(self):
        return self._rotTheta

    @rotTheta.setter
    def rotTheta(self, value):
        self._rotTheta = value
        self._cameraTransformDirty = True
        self.signalFrustumChanged.emit()

    @property
    def rotPhi(self):
        return self._rotPhi

    @rotPhi.setter
    def rotPhi(self, value):
        self._rotPhi = value
        self._cameraTransformDirty = True
        self.signalFrustumChanged.emit()

    @property
    def center(self):
        return self._center

    @center.setter
    def center(self, value):
        self._center = value
        self._cameraTransformDirty = True
        self.signalFrustumChanged.emit()

    @property
    def dist(self):
        return self._dist

    @dist.setter
    def dist(self, value):
        self._dist = value
        self._cameraTransformDirty = True
        self.signalFrustumChanged.emit()

    @property
    def orthographic(self):
        return self._camera.projection == Gf.Camera.Orthographic

    @orthographic.setter
    def orthographic(self, orthographic):
        if orthographic:
            self._camera.projection = Gf.Camera.Orthographic
        else:
            self._camera.projection = Gf.Camera.Perspective
        self.signalFrustumChanged.emit()

    @property
    def fov(self):
        if self._camera.projection == Gf.Camera.Perspective:
            return self._camera.GetFieldOfView(Gf.Camera.FOVVertical)
        else:
            return (self._camera.verticalAperture * Gf.Camera.APERTURE_UNIT)

    @fov.setter
    def fov(self, value):
        if self._camera.projection == Gf.Camera.Perspective:
            self._camera.SetPerspectiveFromAspectRatioAndFieldOfView(
                self._camera.aspectRatio, value, Gf.Camera.FOVVertical)
        else:
            self._camera.SetOrthographicFromAspectRatioAndSize(
                self._camera.aspectRatio, value, Gf.Camera.FOVVertical)
        self.signalFrustumChanged.emit()

    @property
    def near(self):
        return self._camera.clippingRange.min


    @property
    def far(self):
        return self._camera.clippingRange.max

    # no setters for near and far - one must set overrideNear/Far instead
    @property
    def overrideNear(self):
        return self._overrideNear

    @overrideNear.setter
    def overrideNear(self, value):
        """To remove the override, set to None"""
        self._overrideNear = value

    @property
    def overrideFar(self):
        return self._overrideFar

    @overrideFar.setter
    def overrideFar(self, value):
        """To remove the override, set to None"""
        self._overrideFar = value