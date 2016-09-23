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
'''
Module that provides the StageView class.
'''

from math import tan, atan, radians as rad
import os
from time import time

from PySide import QtGui, QtCore, QtOpenGL

from pxr import Tf
from pxr import Gf
from pxr import Glf
from pxr import Glfq
from pxr import Sdf, Usd, UsdGeom, UsdUtils
from pxr import UsdImaging
from pxr import CameraUtil

# default includedPurposes for the bbox cache
BBOXPURPOSES = [UsdGeom.Tokens.default_, UsdGeom.Tokens.proxy]

DEBUG_CLIPPING = "USDVIEWQ_DEBUG_CLIPPING"

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
            # situations astonishngly easily with large sets when we are 
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


class StageView(QtOpenGL.QGLWidget):
    '''
    QGLWidget that displays a USD Stage.
    '''

    signalBboxUpdateTimeChanged = QtCore.Signal(int)

    # First arg is primPath, (which could be empty Path)
    # Second arg is instanceIndex (or UsdImaging.GL.ALL_INSTANCES for all instances)
    # Third and Fourth args represent state at time of the pick
    signalPrimSelected = QtCore.Signal(Sdf.Path, int, QtCore.Qt.MouseButton,
                                       QtCore.Qt.KeyboardModifiers)

    # Only raised when StageView has been told to do so, setting
    # rolloverPicking to True
    signalPrimRollover = QtCore.Signal(Sdf.Path, int, QtCore.Qt.KeyboardModifiers)
    signalCurrentFrameChanged = QtCore.Signal(int)
    signalMouseDrag = QtCore.Signal()
    signalErrorMessage = QtCore.Signal(str)
    
    signalSwitchedToFreeCam = QtCore.Signal()

    @property
    def bboxCache(self):
        return self._bboxCache
        
    @bboxCache.setter
    def bboxCache(self, cache):
        self._bboxCache = cache
        
    @property
    def complexity(self):
        return self._complexity
        
    @complexity.setter
    def complexity(self, value):
        self._complexity = value
        
    @property
    def currentFrame(self):
        return self._currentFrame
        
    @property
    def freeCamera(self):
        return self._freeCamera
    
    @property
    def renderParams(self):
        return self._renderParams
        
    @renderParams.setter
    def renderParams(self, params):
        self._renderParams = params
        
    @property
    def timeSamples(self):
        return self._timeSamples
        
    @timeSamples.setter
    def timeSamples(self, samples):
        self._timeSamples = samples
        
    @property
    def xformCache(self):
        return self._xformCache
        
    @xformCache.setter
    def xformCache(self, cache):
        self._xformCache = cache
        
    @property
    def showBBoxes(self):
        return self._showBBoxes
        
    @showBBoxes.setter
    def showBBoxes(self, show):
        self._showBBoxes = show
        
    @property
    def showHUD(self):
        return self._showHUD
        
    @showHUD.setter
    def showHUD(self, show):
        self._showHUD = show
        
    @property
    def ambientLightOnly(self):
        return self._ambientLightOnly
        
    @ambientLightOnly.setter
    def ambientLightOnly(self, enabled):
        self._ambientLightOnly = enabled
    
    @property
    def keyLightEnabled(self):
        return self._keyLightEnabled
        
    @keyLightEnabled.setter
    def keyLightEnabled(self, enabled):
        self._keyLightEnabled = enabled
        
    @property
    def fillLightEnabled(self):
        return self._fillLightEnabled
        
    @fillLightEnabled.setter
    def fillLightEnabled(self, enabled):
        self._fillLightEnabled = enabled
        
    @property
    def backLightEnabled(self):
        return self._backLightEnabled
        
    @backLightEnabled.setter
    def backLightEnabled(self, enabled):
        self._backLightEnabled = enabled
        
    @property
    def clearColor(self):
        return self._clearColor
        
    @clearColor.setter
    def clearColor(self, clearColor):
        self._clearColor = clearColor
        
    @property
    def renderMode(self):
        return self._renderMode
        
    @renderMode.setter
    def renderMode(self, mode):
        self._renderMode = mode
        
    @property
    def cameraPrim(self):
        return self._cameraPrim
        
    @cameraPrim.setter
    def cameraPrim(self, prim):
        self._cameraPrim = prim
        
    @property
    def showAABox(self):
        return self._showAABBox
        
    @showAABox.setter
    def showAABox(self, state):
        self._showAABox = state
        
    @property
    def showOBBox(self):
        return self._showOBBox
        
    @showOBBox.setter
    def showOBBox(self, state):
        self._showOBBox = state
        
    @property
    def playing(self):
        return self._playing
        
    @playing.setter
    def playing(self, playing):
        self._playing = playing
        
    @property
    def rolloverPicking(self):
        return self._rolloverPicking
        
    @rolloverPicking.setter
    def rolloverPicking(self, enabled):
        self._rolloverPicking = enabled
        self.setMouseTracking(enabled)
        
    @property
    def fpsHUDInfo(self):
        return self._fpsHUDInfo
        
    @fpsHUDInfo.setter
    def fpsHUDInfo(self, info):
        self._fpsHUDInfo = info
        
    @property
    def fpsHUDKeys(self):
        return self._fpsHUDKeys
        
    @fpsHUDKeys.setter
    def fpsHUDKeys(self, keys):
        self._fpsHUDKeys = keys
        
    @property
    def upperHUDInfo(self):
        return self._upperHUDInfo
        
    @upperHUDInfo.setter
    def upperHUDInfo(self, info):
        self._upperHUDInfo = info
        
    @property
    def HUDStatKeys(self):
        return self._HUDStatKeys
        
    @HUDStatKeys.setter
    def HUDStatKeys(self, keys):
        self._HUDStatKeys = keys

    @property
    def highlightColor(self):
        return self._highlightColor
        
    @highlightColor.setter
    def highlightColor(self, keys):
        # We want just a wee bit of the underlying color to show through,
        # so set alpha to 0.5
        self._highlightColor = (keys[0], keys[1], keys[2], 0.5)

    @property
    def drawSelHighlights(self):
        return self._drawSelHighlights
        
    @drawSelHighlights.setter
    def drawSelHighlights(self, keys):
        self._drawSelHighlights = keys

    @property
    def noRender(self):
        return self._noRender
        
    @noRender.setter
    def noRender(self, value):
        if value and self._renderer:
            raise Exception('Renderer has already been initialized - too late to prevent rendering')
        self._noRender = value

    @property
    def overrideNear(self):
        return self._overrideNear

    @overrideNear.setter
    def overrideNear(self, value):
        """To remove the override, set to None.  Causes FreeCamera to become
        active."""
        self._overrideNear = value
        self.switchToFreeCamera()
        self._freeCamera.overrideNear = value
        self.updateGL()

    @property
    def overrideFar(self):
        return self._overrideFar
        
    @overrideFar.setter
    def overrideFar(self, value):
        """To remove the override, set to None.  Causes FreeCamera to become
        active."""
        self._overrideFar = value
        self.switchToFreeCamera()
        self._freeCamera.overrideFar = value
        self.updateGL()
       
    @property
    def allSceneCameras(self):
        return self._allSceneCameras
        
    @allSceneCameras.setter
    def allSceneCameras(self, value):
        self._allSceneCameras = value
    
    def __init__(self, parent=None, bboxCache=None, xformCache=None):
        glFormat = QtOpenGL.QGLFormat()
        msaa = os.getenv("USDVIEW_ENABLE_MSAA", "1")
        if msaa == "1":
            glFormat.setSampleBuffers(True)
            glFormat.setSamples(4)
        super(StageView, self).__init__(
            Glfq.CreateGLDebugContext(glFormat), parent)
                                    
        self._freeCamera = FreeCamera(True)
        self._lastComputedGfCamera = None

        self._complexity = 1.0
        
        self._renderStats = {}
                                    
        self._stage = None
        self._stageIsZup = True
        self._cameraIsZup = True
        self._currentFrame = 0
        self._bboxCache = bboxCache or UsdGeom.BBoxCache(self._currentFrame, 
                                                         BBOXPURPOSES, 
                                                         useExtentsHint=True)
        self._xformCache = xformCache or UsdGeom.XformCache(self._currentFrame)
        self._cameraMode = "none"
        self._rolloverPicking = False
        self._dragActive = False
        self._lastX = 0
        self._lastY = 0

        self._renderer = None
        self._renderModeDict={"Wireframe":UsdImaging.GL.DrawMode.DRAW_WIREFRAME,
                              "WireframeOnSurface":UsdImaging.GL.DrawMode.DRAW_WIREFRAME_ON_SURFACE,
                              "Smooth Shaded":UsdImaging.GL.DrawMode.DRAW_SHADED_SMOOTH,
                              "Points":UsdImaging.GL.DrawMode.DRAW_POINTS,
                              "Flat Shaded":UsdImaging.GL.DrawMode.DRAW_SHADED_FLAT,
                              "Geom Only":UsdImaging.GL.DrawMode.DRAW_GEOM_ONLY,
                              "Geom Smooth":UsdImaging.GL.DrawMode.DRAW_GEOM_SMOOTH,
                              "Geom Flat":UsdImaging.GL.DrawMode.DRAW_GEOM_FLAT,
                              "Hidden Surface Wireframe":UsdImaging.GL.DrawMode.DRAW_WIREFRAME}

        self._renderMode = "Smooth Shaded"
        self._renderParams = UsdImaging.GL.RenderParams()
        self._defaultFov = 60
        self._dist = 50
        self._oldDist = self._dist
        self._bbox = Gf.BBox3d()
        self._brange = self._bbox.ComputeAlignedRange()
        self._selectionBBox = Gf.BBox3d()
        self._selectionBrange = Gf.Range3d()
        self._selectionOrientedRange = Gf.Range3d()
        self._bbcenterForBoxDraw = (0, 0, 0)
        self._bbcenter = (0,0,0)
        self._rotTheta = 0
        self._rotPhi = 0
        self._oldRotTheta = self._rotTheta
        self._oldRotPhi = self._rotPhi
        self._oldBbCenter = self._bbcenter

        self._overrideNear = None
        self._overrideFar = None
        
        self._cameraPrim = None
        self._nodes = []

        # blind state of instance selection (key:path, value:indices)
        self._selectedInstances = dict()
        
        self._forceRefresh = False
        self._playing = False
        self._renderTime = 0
        self._noRender = False
        self._showBBoxes = True # Variable to show/hide all BBoxes
        self._showAABBox = True	# Show axis-aligned BBox
        self._showOBBox = False	# Show oriented BBox
        
        self._timeSamples = []

        self._clearColor = (0.0, 0.0, 0.0, 0.0)
        self._highlightColor = (1.0, 1.0, 0.0, 0.8)
        self._drawSelHighlights = True
        self._allSceneCameras = None
        
        # HUD properties
        self._showHUD = False
        self.showHUD_Info = False
        self.showHUD_VBO = False
        self.showHUD_Complexity = False
        self.showHUD_Performance = False
        self.showHUD_GPUstats = False
        self._HUDFont = QtGui.QFont("Menv Mono Numeric", 9)
        self._HUDLineSpacing = 15
        self._fpsHUDInfo = dict()
        self._fpsHUDKeys = []
        self._upperHUDInfo = dict()
        self._HUDStatKeys = list()

        self._displayGuides = False
        self._displayRenderingGuides = False
        self._displayCameraOracles = False
        self._displayPrimId = False
        self._cullBackfaces = True
        self._enableHardwareShading = True
        
        # Lighting properties
        self._ambientLightOnly = False
        self._keyLightEnabled = True
        self._fillLightEnabled = True
        self._backLightEnabled = True

        self._glPrimitiveGeneratedQuery = None
        self._glTimeElapsedQuery = None

    def InitRenderer(self):
        '''Create (or re-create) the imager.   If you intend to
        disable rendering for this widget, you MUST have already set
        self.noRender to True prior to calling this function'''
        if not self._noRender:
            self._renderer = UsdImaging.GL()

    def GetRenderGraphPlugins(self):
        if self._renderer:
            return self._renderer.GetRenderGraphPlugins()
        else:
            return []

    def SetRenderGraphPlugin(self, name):
        self._renderer.SetRenderGraphPlugin(name)

    def GetStage(self):
        return self._stage
        
    def SetStage(self, stage):
        '''Set the USD Stage this widget will be displaying. To decommission
        (even temporarily) this widget, supply None as 'stage' '''
        if stage is None:
            self._renderer = None
            self._stage = None
            self.allSceneCameras = None
        else:
            self.ReloadStage(stage)
            self._freeCamera = FreeCamera(self._stageIsZup)

    def ReloadStage(self, stage):
        self._stage = stage
        # Since this function gets call on startup as well we need to make it 
        # does not try to create a renderer because there is no OGL context yet
        if self._renderer != None:
            self.InitRenderer()
        self._stageIsZup = UsdGeom.GetStageUpAxis(stage) == UsdGeom.Tokens.z
        self._cameraIsZup = UsdUtils.GetCamerasAreZup(stage)
        self.allSceneCameras = None

    # XXX:
    # First pass at visualizing cameras in usdview-- just oracles for
    # now. Eventually the logic should live in usdImaging, where the delegate
    # would add the camera guide geometry to the GL buffers over the course over
    # its stage traversal, and get time samples accordingly.
    def DrawCameraGuides(self):
        from OpenGL import GL
        for camera in self._allSceneCameras:
            # Don't draw guides for the active camera.
            if camera == self._cameraPrim or not (camera and camera.IsActive()):
                continue

            gfCamera = UsdGeom.Camera(camera).GetCamera(self._currentFrame, 
                                                        self._cameraIsZup)
            frustum = gfCamera.frustum

            # (Gf documentation seems to be wrong)-- Ordered as
            # 0: left bottom near
            # 1: right bottom near
            # 2: left top near
            # 3: right top near
            # 4: left bottom far
            # 5: right bottom far
            # 6: left top far
            # 7: right top far
            oraclePoints = frustum.ComputeCorners()

            # XXX:
            # Grabbed fallback oracleColor from CamCamera.
            GL.glColor3f(0.82745, 0.39608, 0.1647)

            # Near plane
            GL.glBegin(GL.GL_LINE_LOOP)
            GL.glVertex3f(*oraclePoints[0])
            GL.glVertex3f(*oraclePoints[1])
            GL.glVertex3f(*oraclePoints[3])
            GL.glVertex3f(*oraclePoints[2])
            GL.glEnd()

            # Far plane
            GL.glBegin(GL.GL_LINE_LOOP)
            GL.glVertex3f(*oraclePoints[4])
            GL.glVertex3f(*oraclePoints[5])
            GL.glVertex3f(*oraclePoints[7])
            GL.glVertex3f(*oraclePoints[6])
            GL.glEnd()

            # Lines between near and far planes.
            GL.glBegin(GL.GL_LINES)
            GL.glVertex3f(*oraclePoints[3])
            GL.glVertex3f(*oraclePoints[7])
            GL.glEnd()
            GL.glBegin(GL.GL_LINES)
            GL.glVertex3f(*oraclePoints[0])
            GL.glVertex3f(*oraclePoints[4])
            GL.glEnd()
            GL.glBegin(GL.GL_LINES)
            GL.glVertex3f(*oraclePoints[1])
            GL.glVertex3f(*oraclePoints[5])
            GL.glEnd()
            GL.glBegin(GL.GL_LINES)
            GL.glVertex3f(*oraclePoints[2])
            GL.glVertex3f(*oraclePoints[6])
            GL.glEnd()

    def _updateBboxGuides(self):
        includedPurposes =  set(self._bboxCache.GetIncludedPurposes())

        if self._displayGuides:
            includedPurposes.add(UsdGeom.Tokens.guide)
        elif UsdGeom.Tokens.guide in includedPurposes:
            includedPurposes.remove(UsdGeom.Tokens.guide)

        if self._displayRenderingGuides:
            includedPurposes.add(UsdGeom.Tokens.render)
        elif UsdGeom.Tokens.render in includedPurposes:
            includedPurposes.remove(UsdGeom.Tokens.render)

        self._bboxCache.SetIncludedPurposes(includedPurposes)
        # force the bbox to refresh
        self._bbox = Gf.BBox3d()

    # XXX Why aren't these @properties?
    def setDisplayGuides(self, enabled):
        self._displayGuides = enabled
        self._updateBboxGuides()

    def setDisplayRenderingGuides(self, enabled):
        self._displayRenderingGuides = enabled
        self._updateBboxGuides()

    def setDisplayCameraOracles(self, enabled):
        self._displayCameraOracles = enabled

    def setDisplayPrimId(self, enabled):
        self._displayPrimId = enabled

    def setEnableHardwareShading(self, enabled):
        self._enableHardwareShading = enabled

    def setCullBackfaces(self, enabled):
        self._cullBackfaces = enabled
        
    def setNodes(self, nodes, frame, resetCam=False, forceComputeBBox=False, 
                 frameFit=1.1):
        '''Set the current nodes. resetCam = True causes the camera to reframe
        the specified nodes. frameFit sets the ratio of the camera's frustum's 
        relevant dimension to the object's bounding box. 1.1, the default, 
        fits the node's bounding box in the frame with a roughly 10% margin.
        '''
        self._nodes = nodes
        self._currentFrame = frame
        
        if self._noRender:
            return
        
        # set highlighted paths to renderer
        self._updateSelection()

        # Only compute BBox if forced, if needed for drawing,
        # or if this is the first time running.
        computeBBox = forceComputeBBox or \
                     (self._showBBoxes and 
                      (self._showAABBox or self._showOBBox))\
                     or self._bbox.GetRange().IsEmpty()
        if computeBBox:
            try:
                startTime = time()
                self._bbox = self.getStageBBox()
                if len(nodes) == 1 and nodes[0].GetPath() == '/':
                    if self._bbox.GetRange().IsEmpty():
                        self._selectionBBox = self._getDefaultBBox()
                    else:
                        self._selectionBBox = self._bbox
                else:
                    self._selectionBBox = self.getSelectionBBox()

                # BBox computation time for HUD
                endTime = time()
                ms = (endTime - startTime) * 1000.
                self.signalBboxUpdateTimeChanged.emit(ms)

            except RuntimeError:
                # This may fail, but we want to keep the UI available, 
                # so print the error and attempt to continue loading
                self.signalErrorMessage.emit("unable to get bounding box on "
                   "stage at frame {0}".format(self._currentFrame))
                import traceback
                traceback.print_exc()
                self._bbox = self._getEmptyBBox() 
                self._selectionBBox = self._getDefaultBBox()

        self._brange = self._bbox.ComputeAlignedRange()
        self._selectionBrange = self._selectionBBox.ComputeAlignedRange()
        self._selectionOrientedRange = self._selectionBBox.box
        self._bbcenterForBoxDraw = self._selectionBBox.ComputeCentroid()

        validFrameRange = (not self._selectionBrange.IsEmpty() and 
            self._selectionBrange.GetMax() != self._selectionBrange.GetMin())
        if resetCam and validFrameRange:
            self.switchToFreeCamera()
            self._freeCamera.frameSelection(self._selectionBBox, frameFit)
            self.computeAndSetClosestDistance()
            
        self.updateGL()

    def _updateSelection(self):
        psuRoot = self._stage.GetPseudoRoot()
        self._renderer.ClearSelected()

        for p in self._nodes:
            if p == psuRoot:
                continue
            if self._selectedInstances.has_key(p.GetPath()):
                for instanceIndex in self._selectedInstances[p.GetPath()]:
                    self._renderer.AddSelected(p.GetPath(), instanceIndex)
            else:
                self._renderer.AddSelected(p.GetPath(), UsdImaging.GL.ALL_INSTANCES)

    def _getEmptyBBox(self):
        return Gf.BBox3d()

    def _getDefaultBBox(self):
        return Gf.BBox3d(Gf.Range3d((-10,-10,-10), (10,10,10)))

    def getStageBBox(self):
        bbox = self._bboxCache.ComputeWorldBound(self._stage.GetPseudoRoot())
        if bbox.GetRange().IsEmpty():
            bbox = self._getEmptyBBox()
        return bbox

    def getSelectionBBox(self):
        bbox = Gf.BBox3d()
        for n in self._nodes:
            if n.IsActive() and not n.IsInMaster():
                primBBox = self._bboxCache.ComputeWorldBound(n)
                bbox = Gf.BBox3d.Combine(bbox, primBBox)
        return bbox

    def getCameraPrim(self):
        return self._cameraPrim

    def setCameraPrim(self, cameraPrim):
        if not cameraPrim:
            self.switchToFreeCamera()
            return

        if cameraPrim.IsA(UsdGeom.Camera):
            self._freeCamera = None
            self._cameraPrim = cameraPrim
        else:
            from common import PrintWarning
            PrintWarning("Incorrect Prim Type",
                         "Attempted to view the scene using the prim '%s', but "
                         "the prim is not a UsdGeom.Camera." %(cameraPrim.GetName()))
            
    def renderSinglePass(self, renderMode, renderSelHighlights):
        if not self._stage or not self._renderer:
            return

        # delete old render stats
        self._renderStats = {}

        # update rendering parameters
        self._renderParams.frame = self._currentFrame
        self._renderParams.complexity = self.complexity
        self._renderParams.drawMode = renderMode
        self._renderParams.showGuides = self._displayGuides
        self._renderParams.showRenderGuides = self._displayRenderingGuides
        self._renderParams.forceRefresh = self._forceRefresh
        self._renderParams.cullStyle =  (UsdImaging.GL.CullStyle.CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED
                                               if self._cullBackfaces
                                               else UsdImaging.GL.CullStyle.CULL_STYLE_NOTHING)
        self._renderParams.gammaCorrectColors = False
        self._renderParams.enableIdRender = self._displayPrimId
        self._renderParams.enableSampleAlphaToCoverage = not self._displayPrimId
        self._renderParams.highlight = renderSelHighlights
        self._renderParams.enableHardwareShading = self._enableHardwareShading

        pseudoRoot = self._stage.GetPseudoRoot()
        
        self._renderer.SetSelectionColor(self.highlightColor)
        self._renderer.Render(pseudoRoot, self._renderParams)
        self._forceRefresh = False


    def initializeGL(self):
        from pxr import Glf
        Glf.GlewInit()
        Glf.RegisterDefaultDebugOutputMessageCallback()
        # Initialize the renderer now since the context is available
        self.InitRenderer()

    def updateGL(self):
        """We override this virtual so that we can make it a no-op during 
        playback.  The client driving playback at a particular rate should
        instead call updateForPlayback() to image the next frame."""
        if not self.playing:
            super(StageView, self).updateGL()

    def updateForPlayback(self, currentTime, showHighlights):
        """If self.playing, update the GL canvas.  Otherwise a no-op"""
        if self.playing:
            self._currentFrame = currentTime
            drawHighlights = self.drawSelHighlights
            self.drawSelHighlights = showHighlights
            super(StageView, self).updateGL()
            self.drawSelHighlights = drawHighlights
        
    def computeGfCameraForCurrentPrim(self):
        if self._cameraPrim and self._cameraPrim.IsActive():
            gfCamera = UsdGeom.Camera(self._cameraPrim).GetCamera(
                self._currentFrame, self._cameraIsZup)
            return gfCamera
        else:
            return None

    def computeGfCamera(self):
        camera = self.computeGfCameraForCurrentPrim()
        if not camera:
            # If 'camera' is None, make sure we have a valid freeCamera
            self.switchToFreeCamera()
            camera = self._freeCamera.computeGfCamera(self._bbox)

        targetAspect = (
            float(self.size().width()) / max(1.0, self.size().height()))
        CameraUtil.ConformWindow(
            camera, CameraUtil.MatchVertically, targetAspect)

        self._lastComputedGfCamera = camera
        return camera

    def copyViewState(self):
        """Returns a copy of this StageView's view-affecting state,
        which can be used later to restore the view via restoreViewState().
        Take note that we do NOT include the StageView's notion of the
        current time (used by prim-based cameras to extract their data),
        since we do not want a restore operation to put us out of sync
        with respect to our owner's time.
        """
        viewState = {}
        for attr in ["_cameraPrim", "_stageIsZup", 
                     "_overrideNear", "_overrideFar" ]:
            viewState[attr] = getattr(self, attr)
        # Since FreeCamera is a compound/class object, we must copy
        # it more deeply
        viewState["_freeCamera"] = self._freeCamera.clone() if self._freeCamera else None
        return viewState

    def restoreViewState(self, viewState):
        """Restore view parameters from 'viewState', and redraw"""
        for key,val in viewState.iteritems():
            setattr(self, key, val)
        # Detach our freeCamera from the given viewState, to
        # insulate against changes to viewState by caller
        if viewState.has_key("_freeCamera") and self._freeCamera:
            self._freeCamera = self._freeCamera.clone()
        self.update()


    def setupOpenGLViewMatricesForFrustum(self, frustum):
        from OpenGL import GL
        import ctypes
        GLMtx = ctypes.c_double * 16
        MakeGLMtx = lambda m: GLMtx.from_buffer_copy(m)

        GL.glMatrixMode(GL.GL_PROJECTION)
        GL.glLoadIdentity()
        GL.glMultMatrixd(MakeGLMtx(frustum.ComputeProjectionMatrix()))

        GL.glMatrixMode(GL.GL_MODELVIEW)
        GL.glLoadIdentity()
        GL.glMultMatrixd(MakeGLMtx(frustum.ComputeViewMatrix()))

        GL.glViewport(0, 0, self.size().width(), self.size().height())

        GL.glClear(GL.GL_COLOR_BUFFER_BIT|GL.GL_DEPTH_BUFFER_BIT)

        self._renderer.SetCameraStateFromOpenGL()

    def drawWireframeCube(self, size):
        from OpenGL import GL

        s = 0.5 * size

        GL.glBegin(GL.GL_LINES)
        GL.glVertex3f( s, s, s); GL.glVertex3f(-s, s, s)
        GL.glVertex3f(-s, s, s); GL.glVertex3f(-s,-s, s)
        GL.glVertex3f(-s,-s, s); GL.glVertex3f( s,-s, s)
        GL.glVertex3f( s,-s, s); GL.glVertex3f( s, s, s)

        GL.glVertex3f(-s,-s,-s); GL.glVertex3f(-s, s,-s)
        GL.glVertex3f(-s, s,-s); GL.glVertex3f( s, s,-s)
        GL.glVertex3f( s, s,-s); GL.glVertex3f( s,-s,-s)
        GL.glVertex3f( s,-s,-s); GL.glVertex3f(-s,-s,-s)

        GL.glVertex3f( s, s, s); GL.glVertex3f( s, s,-s)
        GL.glVertex3f(-s, s, s); GL.glVertex3f(-s, s,-s)
        GL.glVertex3f(-s,-s, s); GL.glVertex3f(-s,-s,-s)
        GL.glVertex3f( s,-s, s); GL.glVertex3f( s,-s,-s)
        GL.glEnd()

    def paintGL(self):
        if not self._stage or not self._renderer:
            return
        
        from OpenGL import GL
        from OpenGL import GLU

        if self.showHUD_GPUstats:
            if self._glPrimitiveGeneratedQuery is None:
                self._glPrimitiveGeneratedQuery = Glf.GLQueryObject()
            if self._glTimeElapsedQuery is None:
                self._glTimeElapsedQuery = Glf.GLQueryObject()
            self._glPrimitiveGeneratedQuery.BeginPrimitivesGenerated()
            self._glTimeElapsedQuery.BeginTimeElapsed()

        # Enable sRGB in order to apply a final gamma to this window, just like
        # in Presto.
        from OpenGL.GL.EXT.framebuffer_sRGB import GL_FRAMEBUFFER_SRGB_EXT
        GL.glEnable(GL_FRAMEBUFFER_SRGB_EXT)

        GL.glClearColor(*(Gf.ConvertDisplayToLinear(Gf.Vec4f(self._clearColor))))
        GL.glShadeModel(GL.GL_SMOOTH)

        GL.glEnable(GL.GL_DEPTH_TEST)
        GL.glDepthFunc(GL.GL_LESS)

        GL.glBlendFunc(GL.GL_SRC_ALPHA, GL.GL_ONE_MINUS_SRC_ALPHA)
        GL.glEnable(GL.GL_BLEND)

        gfCamera = self.computeGfCamera()
        frustum = gfCamera.frustum

        cam_pos = frustum.position
        cam_up = frustum.ComputeUpVector()
        cam_right = Gf.Cross(frustum.ComputeViewDirection(), cam_up)

        self.setupOpenGLViewMatricesForFrustum(frustum)

        # Set the clipping planes.
        self._renderParams.clipPlanes = [Gf.Vec4d(i) for i in
                                         gfCamera.clippingPlanes]

        if self._nodes:
            
            GL.glColor3f(1.0,1.0,1.0)

            # for renderModes that need lights 
            if self.renderMode in ("Flat Shaded",
                                   "Smooth Shaded",
                                   "WireframeOnSurface",
                                   "Geom Smooth",
                                   "Geom Flat"):
                GL.glEnable(GL.GL_LIGHTING)
                stagePos = Gf.Vec3d(self._bbcenter[0], self._bbcenter[1],
                                    self._bbcenter[2])
                stageDir = (stagePos - cam_pos).GetNormalized()

                # ambient light located at the camera
                if self.ambientLightOnly:
                    GL.glEnable(GL.GL_LIGHT0)
                    GL.glLightfv(GL.GL_LIGHT0, GL.GL_POSITION,
                                 (cam_pos[0], cam_pos[1], cam_pos[2], 1))
                # three-point lighting
                else:
                    GL.glDisable(GL.GL_LIGHT0)

                    if self.keyLightEnabled:
                        # 45 degree horizontal viewing angle, 20 degree vertical
                        keyHorz = -1 / tan(rad(45)) * cam_right
                        keyVert = 1 / tan(rad(70)) * cam_up
                        keyPos = cam_pos + (keyVert + keyHorz) * self._dist
                        keyColor = (.8, .8, .8, 1.0)
                        
                        GL.glLightfv(GL.GL_LIGHT1, GL.GL_AMBIENT,(0,0,0,0))
                        GL.glLightfv(GL.GL_LIGHT1, GL.GL_DIFFUSE, keyColor)
                        GL.glLightfv(GL.GL_LIGHT1, GL.GL_SPECULAR, keyColor)
                        GL.glLightfv(GL.GL_LIGHT1, GL.GL_POSITION, 
                                    (keyPos[0], keyPos[1], keyPos[2], 1))
                        GL.glEnable(GL.GL_LIGHT1)
                
                    if self.fillLightEnabled:
                        # 60 degree horizontal viewing angle, 45 degree vertical
                        fillHorz = 1 / tan(rad(30)) * cam_right
                        fillVert = 1 / tan(rad(45)) * cam_up
                        fillPos = cam_pos + (fillVert + fillHorz) * self._dist
                        fillColor = (.6, .6, .6, 1.0)
                        
                        GL.glLightfv(GL.GL_LIGHT2, GL.GL_AMBIENT, (0,0,0,0))
                        GL.glLightfv(GL.GL_LIGHT2, GL.GL_DIFFUSE, fillColor)
                        GL.glLightfv(GL.GL_LIGHT2, GL.GL_SPECULAR, fillColor) 
                        GL.glLightfv(GL.GL_LIGHT2, GL.GL_POSITION,
                                     (fillPos[0], fillPos[1], fillPos[2], 1))
                        GL.glEnable(GL.GL_LIGHT2)
                        
                    if self.backLightEnabled:
                        # back light base is camera position refelcted over origin
                        # 30 degree horizontal viewing angle, 30 degree vertical
                        backPos = cam_pos + (stagePos - cam_pos) * 2
                        backHorz = 1 / tan(rad(60)) * cam_right
                        backVert = -1 / tan(rad(60)) * cam_up
                        backPos += (backHorz + backVert) * self._dist
                        backColor = (.6, .6, .6, 1.0)
                        
                        GL.glLightfv(GL.GL_LIGHT2, GL.GL_AMBIENT, (0,0,0,0))
                        GL.glLightfv(GL.GL_LIGHT3, GL.GL_DIFFUSE, backColor)
                        GL.glLightfv(GL.GL_LIGHT3, GL.GL_SPECULAR, backColor)
                        GL.glLightfv(GL.GL_LIGHT3, GL.GL_POSITION, 
                                     (backPos[0], backPos[1], backPos[2], 1))
                        GL.glEnable(GL.GL_LIGHT3)
                        
                GL.glMaterialfv(GL.GL_FRONT_AND_BACK, GL.GL_AMBIENT,
                                (0.2, 0.2, 0.2, 1.0))
                GL.glMaterialfv(GL.GL_FRONT_AND_BACK, GL.GL_SPECULAR,
                                (0.5, 0.5, 0.5, 1.0))
                GL.glMaterialfv(GL.GL_FRONT_AND_BACK, GL.GL_SHININESS, 32.0)

                # XXX continue to set lighting state via OpenGL
                # for compatibility w/ and w/o hydra enabled
                self._renderer.SetLightingStateFromOpenGL()

            if self.renderMode == "Hidden Surface Wireframe":
                GL.glEnable( GL.GL_POLYGON_OFFSET_FILL )
                GL.glPolygonOffset( 1.0, 1.0 )
                GL.glPolygonMode( GL.GL_FRONT_AND_BACK, GL.GL_FILL )

                self.renderSinglePass( self._renderer.DrawMode.DRAW_GEOM_ONLY,
                                       False)

                GL.glDisable( GL.GL_POLYGON_OFFSET_FILL )
                GL.glDepthFunc(GL.GL_LEQUAL)
                GL.glClear(GL.GL_COLOR_BUFFER_BIT)

            self.renderSinglePass(self._renderModeDict[self.renderMode],
                                  self.drawSelHighlights)

            # lights interfere with the correct coloring of bbox and axes
            GL.glDisable(GL.GL_LIGHTING)
            GL.glDisable(GL.GL_LIGHT0)
            GL.glDisable(GL.GL_LIGHT1)
            GL.glDisable(GL.GL_LIGHT2)
            GL.glDisable(GL.GL_LIGHT3)

            GL.glBegin(GL.GL_LINES)

            GL.glColor3f(1.0,0.0,0.0)
            GL.glVertex3f(0.0,0.0,0.0)
            GL.glVertex3f(self._dist/20.0,0.0,0.0)

            GL.glColor3f(0.0,1.0,0.0)
            GL.glVertex3f(0.0,0.0,0.0)
            GL.glVertex3f(0.0,self._dist/20.0,0.0)

            GL.glColor3f(0.0,0.0,1.0)
            GL.glVertex3f(0.0,0.0,0.0)
            GL.glVertex3f(0.0,0.0,self._dist/20.0)

            GL.glEnd()

            # XXX:
            # Draw camera guides-- no support for toggling guide visibility on
            # individual cameras until we move this logic directly into
            # usdImaging.
            if self._displayCameraOracles:
                self.DrawCameraGuides()

            if self._showBBoxes:
                col = self._clearColor

                # Draw axis-aligned bounding box
                if self._showAABBox:
                    bsize = self._selectionBrange.max - self._selectionBrange.min

                    GL.glPushAttrib(GL.GL_LINE_BIT)
                    GL.glEnable(GL.GL_LINE_STIPPLE)
                    GL.glLineStipple(2,0xAAAA)
                    GL.glColor3f(col[0]-.5 if col[0]>0.5 else col[0]+.5,
                                 col[1]-.5 if col[1]>0.5 else col[1]+.5,
                                 col[2]-.5 if col[2]>0.5 else col[2]+.5)

                    GL.glPushMatrix()

                    GL.glTranslatef(self._bbcenterForBoxDraw[0],
                                    self._bbcenterForBoxDraw[1],
                                    self._bbcenterForBoxDraw[2])
                    GL.glScalef( bsize[0], bsize[1], bsize[2] )

                    self.drawWireframeCube(1.0)

                    GL.glPopMatrix()
                    GL.glPopAttrib()

                # Draw oriented bounding box
                if self._showOBBox:
                    import ctypes
                    GLMtx = ctypes.c_double * 16
                    MakeGLMtx = lambda m: GLMtx.from_buffer_copy(m)

                    bsize = self._selectionOrientedRange.max - self._selectionOrientedRange.min
                    center = bsize / 2. + self._selectionOrientedRange.min

                    GL.glPushAttrib(GL.GL_LINE_BIT)
                    GL.glEnable(GL.GL_LINE_STIPPLE)
                    GL.glLineStipple(2,0xAAAA)
                    GL.glColor3f(col[0]-.2 if col[0]>0.5 else col[0]+.2,
                                 col[1]-.2 if col[1]>0.5 else col[1]+.2,
                                 col[2]-.2 if col[2]>0.5 else col[2]+.2)

                    GL.glPushMatrix()
                    GL.glMultMatrixd(MakeGLMtx(self._selectionBBox.matrix))

                    GL.glTranslatef(center[0], center[1], center[2])
                    GL.glScalef( bsize[0], bsize[1], bsize[2] )

                    self.drawWireframeCube(1.0)

                    GL.glPopMatrix()
                    GL.glPopAttrib()

        else:
            GL.glClear(GL.GL_COLOR_BUFFER_BIT)

        if self.showHUD_GPUstats:
            self._glPrimitiveGeneratedQuery.End()
            self._glTimeElapsedQuery.End()

        # ### DRAW HUD ### #
        if self.showHUD:
            # compute the time it took to render this frame,
            # so we can display it in the HUD
            ms = self._renderTime * 1000.
            fps = float("inf")
            if not self._renderTime == 0:
                fps = 1./self._renderTime
            # put the result in the HUD string
            self.fpsHUDInfo['Render'] = "%.2f ms (%.2f FPS)" % (ms, fps)
            
            GL.glPushMatrix()
            GL.glLoadIdentity()
            
            yPos = 14
            col = Gf.ConvertDisplayToLinear(Gf.Vec3f(.733,.604,.333))

            # the subtree info does not update while animating, grey it out
            if not self.playing:
                subtreeCol = col
            else:
                subtreeCol = Gf.ConvertDisplayToLinear(Gf.Vec3f(.6,.6,.6))

            # Subtree Info
            if self.showHUD_Info:
                yPos = self.printDict(-10, yPos, subtreeCol,
                                      self.upperHUDInfo,
                                      self.HUDStatKeys)

            # VBO Info
            if self.showHUD_VBO:
                self.printDict(-10, yPos, col, self._renderStats)

            # Complexity
            if self.showHUD_Complexity:
                # Camera name
                camName = "Free"
                if self._cameraPrim:
                    camName = self._cameraPrim.GetName()

                toPrint = {"Complexity" : self.complexity,
                           "Camera" : camName}
                self.printDict(self.width()-200, self.height()-21, col, toPrint)

            # Hydra Enabled
            toPrint = {"Hydra":
                         "Enabled" if UsdImaging.GL.IsEnabledHydra() 
                         else "Disabled"}
            self.printDict(self.width()-140, 14, col, toPrint)

            # Playback Rate
            if self.showHUD_Performance:
                self.printDict(-10, self.height()-24, col,
                               self.fpsHUDInfo,
                               self.fpsHUDKeys)
            # GPU stats (TimeElapsed is in nano seconds)
            if self.showHUD_GPUstats:
                allocInfo = self._renderer.GetResourceAllocation()
                gpuMemTotal = 0
                texMem = 0
                if "gpuMemoryUsed" in allocInfo:
                    gpuMemTotal = allocInfo["gpuMemoryUsed"]
                if "textureMemoryUsed" in allocInfo:
                    texMem = allocInfo["textureMemoryUsed"]
                    gpuMemTotal += texMem

                from collections import OrderedDict
                toPrint = OrderedDict()
                toPrint["GL prims "] = self._glPrimitiveGeneratedQuery.GetResult()
                toPrint["GPU time "] = "%.2f ms " % (self._glTimeElapsedQuery.GetResult() / 1000000.0)
                toPrint["GPU mem  "] = gpuMemTotal
                toPrint[" primvar "] = allocInfo["primVar"] if "primVar" in allocInfo else "N/A"
                toPrint[" topology"] = allocInfo["topology"] if "topology" in allocInfo else "N/A"
                toPrint[" shader  "] = allocInfo["drawingShader"] if "drawingShader" in allocInfo else "N/A"
                toPrint[" texture "] = texMem

                self.printDict(-10, self.height()-30-(15*len(toPrint)), col, toPrint, toPrint.keys())

            GL.glPopMatrix()

        GL.glDisable(GL_FRAMEBUFFER_SRGB_EXT)

        if (not self.playing) & (not self._renderer.IsConverged()):
            QtCore.QTimer.singleShot(5, self.update)
      
    def printDict(self, x, y, col, dic, keys = None):
        from prettyPrint import prettyPrint
        from OpenGL import GL
        if keys is None:
            keys = sorted(dic.keys())

        # find the longest key so we know how far from the edge to print
        # add [0] at the end so that max() never gets an empty sequence
        longestKeyLen = max([len(k) for k in dic.iterkeys()]+[0])
        margin = int(longestKeyLen*1.4)

        for key in keys:
            if not dic.has_key(key):
                continue
            line = key.rjust(margin) + ": " + str(prettyPrint(dic[key]))
            # Shadow of text
            GL.glColor3f(*(Gf.ConvertDisplayToLinear(Gf.Vec3f(.2, .2, .2))))
            self.renderText(x+1, y+1, line, self._HUDFont)
            # Colored text
            GL.glColor3f(*col)
            self.renderText(x, y, line, self._HUDFont)

            y += self._HUDLineSpacing
        return y + self._HUDLineSpacing

    def sizeHint(self):
        return QtCore.QSize(460, 460)

    def switchToFreeCamera(self, computeAndSetClosestDistance=True):
        """
        If our current camera corresponds to a prim, create a FreeCamera
        that has the same view and use it.
        """
        if self._cameraPrim != None:
            # _cameraPrim may no longer be valid, so use the last-computed
            # gf camera
            if self._lastComputedGfCamera:
                self._freeCamera = FreeCamera.FromGfCamera(self._lastComputedGfCamera, self._stageIsZup)
            else:
                self._freeCamera = FreeCamera(self._stageIsZup)
            # override clipping plane state is managed by StageView,
            # so that it can be persistent.  Therefore we must restore it
            # now
            self._freeCamera.overrideNear = self._overrideNear
            self._freeCamera.overrideFar = self._overrideFar
            self._cameraPrim = None
            if computeAndSetClosestDistance:
                self.computeAndSetClosestDistance()
            # let the controller know we've done this!
            self.signalSwitchedToFreeCam.emit()

    # It WBN to support marquee selection in the viewer also, at some point...
    def mousePressEvent(self, event):
        """This widget claims the Alt modifier key as the enabler for camera
        manipulation, and will consume mousePressEvents when Alt is present.
        In any other modifier state, a mousePressEvent will result in a
        pick operation, and the pressed button and active modifiers will be
        made available to clients via a signalPrimSelected()."""

        # It's important to set this first, since pickObject(), called below
        # may produce the mouse-up event that will terminate the drag
        # initiated by this mouse-press
        self._dragActive = True

        if (event.modifiers() & QtCore.Qt.AltModifier):
            if event.button() == QtCore.Qt.LeftButton:
                self.switchToFreeCamera()
                self._cameraMode = "tumble"
            if event.button() == QtCore.Qt.MidButton:
                self.switchToFreeCamera()
                self._cameraMode = "truck"
            if event.button() == QtCore.Qt.RightButton:
                self.switchToFreeCamera()
                self._cameraMode = "zoom"
        else:
            self._cameraMode = "pick"
            self.pickObject(event.x(), event.y(), 
                            event.button(), event.modifiers())

        self._lastX = event.x()
        self._lastY = event.y()

    def mouseReleaseEvent(self, event):
        self._cameraMode = "none"
        self._dragActive = False

    def mouseMoveEvent(self, event ):

        if self._dragActive:
            dx = event.x() - self._lastX
            dy = event.y() - self._lastY
            if dx == 0 and dy == 0:
                return
            if self._cameraMode == "tumble":
                self._freeCamera.rotTheta += 0.25 * dx
                self._freeCamera.rotPhi += 0.25 * dy

            elif self._cameraMode == "zoom":
                zoomDelta = -.002 * (dx + dy)
                self._freeCamera.adjustDist(1 + zoomDelta)

            elif self._cameraMode == "truck":
                height = float(self.size().height())
                self._freeCamera.Truck(dx, dy, height)

            self._lastX = event.x()
            self._lastY = event.y()
            self.updateGL()

            self.signalMouseDrag.emit()
        elif self._cameraMode == "none":
            # Mouse tracking is only enabled when rolloverPicking is enabled,
            # and this function only gets called elsewise when mouse-tracking
            # is enabled
            self.pickObject(event.x(), event.y(), None, event.modifiers())
        else:
            event.ignore()

    def wheelEvent(self, event):
        distBefore = self._dist
        self.switchToFreeCamera()
        self._freeCamera.adjustDist(1-max(-0.5,min(0.5,(event.delta()/1000.))))
        self.updateGL()

    def detachAndReClipFromCurrentCamera(self):
        """If we are currently rendering from a prim camera, switch to the
        FreeCamera.  Then reset the near/far clipping planes based on
        distance to closest geometry."""
        if not self._freeCamera:
            self.switchToFreeCamera()
        else:
            self.computeAndSetClosestDistance()
        
    def computeAndSetClosestDistance(self):
        '''Using the current FreeCamera's frustum, determine the world-space
        closest rendered point to the camera.  Use that point
        to set our FreeCamera's closest visible distance.'''
        # pick() operates at very low screen resolution, but that's OK for
        # our purposes.  Ironically, the same limited Z-buffer resolution for
        # which we are trying to compensate may cause us to completely lose
        # ALL of our geometry if we set the near-clip really small (which we 
        # want to do so we don't miss anything) when geometry is clustered 
        # closer to far-clip.  So in the worst case, we may need to perform
        # two picks, with the first pick() using a small near and far, and the
        # second pick() using a near that keeps far within the safe precision
        # range.  We don't expect the worst-case to happen often.
        if not self._freeCamera:
            return
        cameraFrustum = self.computeGfCamera().frustum
        trueFar = cameraFrustum.nearFar.max
        smallNear = min(FreeCamera.defaultNear, 
                        self._freeCamera._selSize / 10.0)
        cameraFrustum.nearFar = \
            Gf.Range1d(smallNear, smallNear*FreeCamera.maxSafeZResolution)
        scrSz = self.size()
        pickResults = self.pick(cameraFrustum)
        if pickResults[0] is None or pickResults[1] == Sdf.Path.emptyPath:
            cameraFrustum.nearFar = \
                Gf.Range1d(trueFar/FreeCamera.maxSafeZResolution, trueFar)
            pickResults = self.pick(cameraFrustum)
            if Tf.Debug.IsDebugSymbolNameEnabled(DEBUG_CLIPPING):
                print "computeAndSetClosestDistance: Needed to call pick() a second time"

        if pickResults[0] is not None and pickResults[1] != Sdf.Path.emptyPath:
            self._freeCamera.setClosestVisibleDistFromPoint(pickResults[0])
            self.updateGL()

    def pick(self, pickFrustum):
        '''
        Find closest point in scene rendered through 'pickFrustum'.  
        Returns a triple:
          selectedPoint, selectedPrimPath, selectedInstanceIndex
        '''
        if not self._stage or not self._renderer:
            return None, None, None, None
        
        from OpenGL import GL

        # Need a correct OpenGL Rendering context for FBOs
        self.makeCurrent()

        # delete old render stats
        self._renderStats = {}
        
        # update rendering parameters
        self._renderParams.frame = self._currentFrame
        self._renderParams.complexity = self.complexity
        self._renderParams.drawMode = self._renderModeDict[self.renderMode]
        self._renderParams.showGuides = self._displayGuides
        self._renderParams.showRenderGuides = self._displayRenderingGuides
        self._renderParams.forceRefresh = self._forceRefresh
        self._renderParams.cullStyle =  (UsdImaging.GL.CullStyle.CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED
                                               if self._cullBackfaces
                                               else UsdImaging.GL.CullStyle.CULL_STYLE_NOTHING)
        self._renderParams.gammaCorrectColors = False
        self._renderParams.enableIdRender = True
        self._renderParams.enableSampleAlphaToCoverage = False
        self._renderParams.enableHardwareShading = self._enableHardwareShading

        results = self._renderer.TestIntersection(
                pickFrustum.ComputeViewMatrix(),
                pickFrustum.ComputeProjectionMatrix(),
                Gf.Matrix4d(1.0),
                self._stage.GetPseudoRoot(), self._renderParams)
        if Tf.Debug.IsDebugSymbolNameEnabled(DEBUG_CLIPPING):
            print "Pick results = {}".format(results)
        return results

    def computePickFrustum(self, x, y):
        from OpenGL import GL

        # Need a correct OpenGL Rendering context to get viewport
        self.makeCurrent()

        posX, posY, width, height = GL.glGetIntegerv( GL.GL_VIEWPORT )
        size = Gf.Vec2d(1.0 / width, 1.0 / height)
        
        # compute pick frustum
        cameraFrustum = self.computeGfCamera().frustum
        
        return cameraFrustum.ComputeNarrowedFrustum(
            Gf.Vec2d((2.0 * x) / width - 1.0,
                     (2.0 * (height-y)) / height - 1.0),
            size)

    def pickObject(self, x, y, button, modifiers):
        '''
        Render stage into fbo with each piece as a different color.
        Emits a signalPrimSelected or signalRollover depending on
        whether 'button' is None.
        '''

        selectedPoint, selectedPrimPath, selectedInstancerPath, selectedInstanceIndex = \
            self.pick(self.computePickFrustum(x, y))

        # The call to TestIntersection will return the path to a master prim
        # (selectedPrimPath) and its instancer (selectedInstancerPath) if the prim is
        # instanced.
        # Figure out which instance was actually picked and use that as our selection
        # in this case.
        if selectedInstancerPath:
            instancePrimPath, absInstanceIndex = self._renderer.GetPrimPathFromInstanceIndex(
                selectedPrimPath, selectedInstanceIndex)
            if instancePrimPath:
                selectedPrimPath = instancePrimPath
                selectedInstanceIndex = absInstanceIndex
        else:
            selectedInstanceIndex = UsdImaging.GL.ALL_INSTANCES
 
        selectedPrim = self._stage.GetPrimAtPath(selectedPrimPath)

        if button:
            self.signalPrimSelected.emit(selectedPrimPath, selectedInstanceIndex, button, modifiers)
        else:
            self.signalPrimRollover.emit(selectedPrimPath, selectedInstanceIndex, modifiers)

    def clearInstanceSelection(self):
        self._selectedInstances.clear()

    def setInstanceSelection(self, path, instanceIndex, selected):
        if selected:
            if not self._selectedInstances.has_key(path):
                self._selectedInstances[path] = set()
            if instanceIndex == UsdImaging.GL.ALL_INSTANCES:
                del self._selectedInstances[path]
            else:
                self._selectedInstances[path].add(instanceIndex)
        else:
            if self._selectedInstances.has_key(path):
                self._selectedInstances[path].remove(instanceIndex)
                if len(self._selectedInstances[path]) == 0:
                    del self._selectedInstances[path]

    def getSelectedInstanceIndices(self, path):
        if self._selectedInstances.has_key(path):
            return self._selectedInstances[path]
        return set()

    def getInstanceSelection(self, path, instanceIndex):
        if instanceIndex in self.getSelectedInstanceIndices(path):
            return True
        return False

    def glDraw(self):
        # override glDraw so we can time it.
        startTime = time()
        QtOpenGL.QGLWidget.glDraw(self)
        self._renderTime = time() - startTime

    def SetForceRefresh(self, val):
        self._forceRefresh = val or self._forceRefresh
        
    def ExportFreeCameraToStage(self, stage, defcamName='usdviewCam', 
                                imgWidth=None, imgHeight=None):
        '''
        Export the free camera to the specified USD stage.
        '''
        imgWidth = imgWidth if imgWidth is not None else self.width()
        imgHeight = imgHeight if imgHeight is not None else self.height()
        
        defcam = UsdGeom.Camera.Define(stage, '/'+defcamName)

        # Map free camera params to usd camera.
        gfCamera = self.freeCamera.computeGfCamera(self._bbox)

        targetAspect = float(imgWidth) / max(1.0, imgHeight)
        CameraUtil.ConformWindow(
            gfCamera, CameraUtil.MatchVertically, targetAspect)
        
        when = self._currentFrame if stage.HasAuthoredTimeCodeRange() \
            else Usd.TimeCode.Default()
            
        defcam.SetFromCamera(gfCamera, when)

        # Pixar only: The sublayered file might have zUp=1 opinions, so
        # explicitly write out zUp=0 for the camera.
        defcam.GetPrim().SetCustomDataByKey('zUp', False)
        
    def ExportSession(self, stagePath, defcamName='usdviewCam',
                      imgWidth=None, imgHeight=None):
        '''
        Export the free camera (if currently active) and session layer to a  
        USD file at the specified stagePath that references the current-viewed 
        stage.
        '''
        
        tmpStage = Usd.Stage.CreateNew(stagePath)
        if self._stage:
            tmpStage.GetRootLayer().TransferContent(
                self._stage.GetSessionLayer())
                
        if not self.cameraPrim:
            # Export the free camera if it's the currently-visible camera
            self.ExportFreeCameraToStage(tmpStage, defcamName, imgWidth, 
                imgHeight)

        tmpStage.GetRootLayer().Save()
        del tmpStage

        # Reopen just the tmp layer, to sublayer in the pose cache without
        # incurring Usd composition cost.
        if self._stage:
            from pxr import Sdf
            sdfLayer = Sdf.Layer.FindOrOpen(stagePath)
            sdfLayer.subLayerPaths.append(
                os.path.abspath(self._stage.GetRootLayer().realPath))
            sdfLayer.Save()
